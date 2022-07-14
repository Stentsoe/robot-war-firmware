/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdbool.h>
#include <cJSON.h>
#include <sys/slist.h>
#include <stdio.h>

#define MODULE robot_module

#include "modules_common.h"
#include "app_module_event.h"
#include "robot_module_event.h"
#include "cloud_module_event.h"
#include "mesh_module_event.h"
#include "ui_module_event.h"

#include <zephyr/logging/log.h>
#define ROBOT_MODULE_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, ROBOT_MODULE_LOG_LEVEL);

/* Robot module super states. */
static enum state_type {
	STATE_CLOUD_DISCONNECTED,
	STATE_CLOUD_CONNECTED,
} state;

enum robot_state {
	ROBOT_STATE_READY,
	ROBOT_STATE_CONFIGURING,
	ROBOT_STATE_CONFIGURED,
};

struct robot {
	sys_snode_t node;
	uint64_t addr;
	enum robot_state state;
	struct robot_cfg cfg;
};

static sys_slist_t robot_list;

struct robot_msg_data {
	union {
		struct app_module_event app;
		struct ui_module_event ui;
		struct robot_module_event robot;
		struct cloud_module_event cloud;
		struct mesh_module_event mesh;
	} module;
};

/* Robot module message queue. */
#define ROBOT_QUEUE_ENTRY_COUNT		10
#define ROBOT_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_robot, sizeof(struct robot_msg_data),
	      ROBOT_QUEUE_ENTRY_COUNT, ROBOT_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "robot",
	.msg_q = &msgq_robot,
	.supports_shutdown = true,
};

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_CLOUD_DISCONNECTED:
		return "STATE_CLOUD_DISCONNECTED";
	case STATE_CLOUD_CONNECTED:
		return "STATE_CLOUD_CONNECTED";
	default:
		return "Unknown state";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

/* JSON functions */
static bool has_shadow_update_been_handled(cJSON *root_obj)
{
	cJSON *version_obj;
	static int version_prev;
	bool retval = false;

	version_obj = cJSON_GetObjectItem(root_obj, "version");
	if (version_obj == NULL) {
		/* No version number present in message. */
		return false;
	}

	/* If the incoming version number is lower than or equal to the previous,
	 * the incoming message is considered a retransmitted message and will be ignored.
	 */

	// LOG_INF("prev %d curr %d", version_prev, version_obj->valueint);
	if (version_prev >= version_obj->valueint) {
		retval = true;
	}
	
	version_prev = version_obj->valueint;
	
	return retval;
}

static cJSON *json_object_decode(cJSON *obj, const char *str)
{
	return obj ? cJSON_GetObjectItem(obj, str) : NULL;
}

static cJSON *json_create_reported_object(cJSON *obj, char* str) 
{
	cJSON *root_obj = cJSON_CreateObject();
	if (root_obj == NULL) {
		return NULL;
	}

	cJSON *state_obj = cJSON_CreateObject();
	if (state_obj == NULL) {
		cJSON_Delete(root_obj);
		return NULL;
	} 

	cJSON_AddItemToObject(root_obj, "state", state_obj);
	cJSON *reported_obj = cJSON_CreateObject();
	if (reported_obj == NULL) {
		cJSON_Delete(root_obj);
		return NULL;
	} 
	
	cJSON_AddItemToObject(state_obj, "reported", reported_obj);


	cJSON_AddItemToObject(reported_obj, str, obj);

	return root_obj;
}

static cJSON *json_parse_root_object(const char *input, size_t input_len)
{
	cJSON *obj = NULL;
	obj = cJSON_ParseWithLength(input, input_len);
	if (obj == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		LOG_ERR("could not parse json object %s", error_ptr);
		return NULL;
	}

	/* Verify that the incoming JSON string is an object. */
	if (!cJSON_IsObject(obj)) {
		LOG_ERR("incoming json string is not an object");
		return NULL;
	}

	if (has_shadow_update_been_handled(obj)) {
		LOG_ERR("shadow update has already been handled");
		cJSON_Delete(obj);
		return NULL;
	}

	return obj;
}

static cJSON *json_get_object_in_state(cJSON *root_obj, char *str) 
{
	cJSON *state_obj = NULL;
	cJSON *desired_obj = NULL;
	state_obj = cJSON_GetObjectItem(root_obj, "state");
	if (state_obj == NULL) {
		return NULL;
	}

	desired_obj = cJSON_GetObjectItem(state_obj, str);
	if (desired_obj == NULL) {
		return NULL;
	}

	return desired_obj;
}

static char* json_encode_add_robot_report(uint64_t addr)
{
	char *msg;
	char robot_addr[13];
	cJSON *root_obj;

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	} 
	
	cJSON *robot_obj = cJSON_CreateObject();
	if (robot_obj == NULL) {
		cJSON_Delete(robots_obj);
		return NULL;
	} 

	sprintf(robot_addr, "%x", (uint32_t) ((addr >> 32) & 0xffffffff));
	sprintf(&robot_addr[4], "%x", (uint32_t) (addr & 0xffffffff));

	cJSON_AddItemToObject(robots_obj, robot_addr, robot_obj);
	
	root_obj = json_create_reported_object(robots_obj, "robots");

	msg = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	return msg;
}

static char* json_encode_remove_robot_report(uint64_t addr) 
{
	char *msg;
	char robot_addr[13];
	cJSON *root_obj;

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	} 

	cJSON *robot_obj = cJSON_CreateNull();
	if (robot_obj == NULL) {
		return NULL;
	}  
	
	sprintf(robot_addr, "%x", (uint32_t) ((addr >> 32) & 0xffffffff));
	sprintf(&robot_addr[4], "%x", (uint32_t) (addr & 0xffffffff));
	cJSON_AddItemToObject(robots_obj, robot_addr, robot_obj);
	
	
	root_obj = json_create_reported_object(robots_obj, "robots");

	msg = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	return msg;
}

static char* json_encode_robot_list_report(void)
{
	char *msg;
	char robot_addr[13];
	cJSON *root_obj;
	cJSON *robot_obj;

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	} 

	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {

		robot_obj = cJSON_CreateObject();
		if (robot_obj == NULL) {
			cJSON_Delete(robots_obj);
			return NULL;
		} 

		sprintf(robot_addr, "%x", (uint32_t) ((robot->addr >> 32) & 0xffffffff));
		sprintf(&robot_addr[4], "%x", (uint32_t) (robot->addr & 0xffffffff));
		cJSON_AddItemToObject(robots_obj, robot_addr, robot_obj);
	}

	root_obj = json_create_reported_object(robots_obj, "robots");

	msg = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	return msg;
}

static char* json_encode_robot_movement_config_report(uint64_t addr)
{
	char *msg;
	char robot_addr[13];
	cJSON *root_obj;

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	}

	cJSON *robot_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	} 

	sprintf(robot_addr, "%x", (uint32_t) ((addr >> 32) & 0xffffffff));
	sprintf(&robot_addr[4], "%x", (uint32_t) (addr & 0xffffffff));
	cJSON_AddItemToObject(robots_obj, robot_addr, robot_obj);

	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (robot->addr == addr) {
			break;
		}
	}

	if (!cJSON_AddNumberToObject(robot_obj, "driveTimeMs", robot->cfg.drive_time)) {
		LOG_ERR("unable to report drivetime config on robot addr %lld", addr);
		return NULL;
	}

	if (!cJSON_AddNumberToObject(robot_obj, "angleDeg", robot->cfg.rotation)) {
		LOG_ERR("unable to report rotation config on robot addr %lld", addr);
		return NULL;
	}

	if (!cJSON_AddNumberToObject(robot_obj, "speedPct", robot->cfg.speed)) {
		LOG_ERR("unable to report speed config on robot addr %lld", addr);
		return NULL;
	}

	root_obj = json_create_reported_object(robots_obj, "robots");

	msg = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	return msg;
}

static char* json_encode_robot_led_config_report(uint64_t addr)
{
	char *msg;
	char robot_addr[13];
	cJSON *root_obj;
	int led[4];

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	}

	cJSON *robot_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	} 

	sprintf(robot_addr, "%x", (uint32_t) ((addr >> 32) & 0xffffffff));
	sprintf(&robot_addr[4], "%x", (uint32_t) (addr & 0xffffffff));
	cJSON_AddItemToObject(robots_obj, robot_addr, robot_obj);

	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (robot->addr == addr) {
			break;
		}
	}

	led[0] = robot->cfg.led.r;
	led[1] = robot->cfg.led.g;
	led[2] = robot->cfg.led.b;
	led[3] = robot->cfg.led.time;

	cJSON *led_obj = cJSON_CreateIntArray(led, 4);
	if (!cJSON_AddItemToObject(robot_obj, "led", led_obj)) {
		LOG_ERR("unable to report led config on robot addr %lld", addr);
		return NULL;
	};

	root_obj = json_create_reported_object(robots_obj, "robots");

	msg = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	return msg;
}

static char* json_encode_robot_revolution_count_list(void)
{
	char *msg;
	char robot_addr[13];
	cJSON *root_obj;

	cJSON *robots_obj = cJSON_CreateObject();
	if (robots_obj == NULL) {
		return NULL;
	}

	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		cJSON *robot_obj = cJSON_CreateObject();
		if (robots_obj == NULL) {
			return NULL;
		} 

		sprintf(robot_addr, "%x", (uint32_t) ((robot->addr >> 32) & 0xffffffff));
		sprintf(&robot_addr[4], "%x", (uint32_t) (robot->addr & 0xffffffff));
		cJSON_AddItemToObject(robots_obj, robot_addr, robot_obj);

		if (!cJSON_AddNumberToObject(robot_obj, "revolutionCount", robot->cfg.revolutions)) {
			LOG_ERR("unable to report revolution count on robot addr %lld", robot->addr);
			return NULL;
		}
	}

	root_obj = json_create_reported_object(robots_obj, "robots");

	msg = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	return msg;
}

static int json_get_delta_robot_config(cJSON *root_obj)
{
	char robot_addr[13];
	bool movement_config = false;

	cJSON *robots_obj = NULL;
	cJSON *robot_obj = NULL;
	cJSON *value_obj = NULL;
	cJSON *led_value_obj;
	struct robot_module_event *event;

	robots_obj = json_get_object_in_state(root_obj, "robots");
	if (robots_obj == NULL) {
		LOG_ERR("could not get robots object");
		return -ENODATA;
	}

	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		sprintf(robot_addr, "%x", (uint32_t) ((robot->addr >> 32) & 0xffffffff));
		sprintf(&robot_addr[4], "%x", (uint32_t) (robot->addr & 0xffffffff));
		robot_obj = cJSON_GetObjectItem(robots_obj, robot_addr);
		if (robots_obj == NULL) {
			continue;
		}

		value_obj = json_object_decode(robot_obj, "driveTimeMs");
		if(value_obj != NULL) {
			robot->cfg.drive_time = value_obj->valueint;
			movement_config = true;
		}

		value_obj = json_object_decode(robot_obj, "angleDeg");
		if(value_obj != NULL) {
			robot->cfg.rotation = value_obj->valueint;
			movement_config = true;
		}

		value_obj = json_object_decode(robot_obj, "speedPct");
		if(value_obj != NULL) {
			robot->cfg.speed = value_obj->valueint;
			movement_config = true;
		}

		value_obj = json_object_decode(robot_obj, "led");
		if(value_obj != NULL) {
			led_value_obj = cJSON_GetArrayItem(value_obj, 0);
			if(led_value_obj != NULL) {
				robot->cfg.led.r = led_value_obj->valueint;
			}

			led_value_obj = cJSON_GetArrayItem(value_obj, 1);
			if(led_value_obj != NULL) {
				robot->cfg.led.g = led_value_obj->valueint;
			}

			led_value_obj = cJSON_GetArrayItem(value_obj, 2);
			if(led_value_obj != NULL) {
				robot->cfg.led.b = led_value_obj->valueint;
			}

			led_value_obj = cJSON_GetArrayItem(value_obj, 3);
			if(led_value_obj != NULL) {
				robot->cfg.led.time = led_value_obj->valueint;
			}
			event = new_robot_module_event();
			event->type = ROBOT_EVT_LED_CONFIGURE;
			event->data.robot.addr = robot->addr;
			event->data.robot.cfg = &robot->cfg;
			APP_EVENT_SUBMIT(event);
		}

		if (movement_config) {
			robot->state = ROBOT_STATE_CONFIGURING;

			event = new_robot_module_event();
			event->type = ROBOT_EVT_MOVEMENT_CONFIGURE;
			event->data.robot.addr = robot->addr;
			event->data.robot.cfg = &robot->cfg;
			APP_EVENT_SUBMIT(event);
		}
	}

	// TODO: Ensure that this is the correct place to submit this event
	struct robot_module_event *clear_to_move_event = new_robot_module_event();
	clear_to_move_event->type = ROBOT_EVT_CLEAR_TO_MOVE;
	APP_EVENT_SUBMIT(clear_to_move_event);
	return 0;
}

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct robot_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_app_module_event(aeh)) {
		struct app_module_event *evt = cast_app_module_event(aeh);
		
		msg.module.app = *evt;
		enqueue_msg = true;
	}

	if (is_robot_module_event(aeh)) {
		struct robot_module_event *evt = cast_robot_module_event(aeh);
		msg.module.robot = *evt;
		enqueue_msg = true;
	}

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(aeh);
		msg.module.cloud = *evt;
		enqueue_msg = true;
	}

	if (is_ui_module_event(aeh)) {
		struct ui_module_event *evt = cast_ui_module_event(aeh);

		msg.module.ui = *evt;
		enqueue_msg = true;
	}

	if (is_mesh_module_event(aeh)) {
		struct mesh_module_event *evt = cast_mesh_module_event(aeh);

		msg.module.mesh = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(robot, ROBOT_EVT_ERROR, err);
		}
	}

	return false;
}

/* Functions to report updates */
static void report_robot_list(void) 
{	
	struct robot_module_event *event = new_robot_module_event();
	event->type = ROBOT_EVT_REPORT;
	event->data.str = json_encode_robot_list_report();
	APP_EVENT_SUBMIT(event);
}

static void report_clear_robot_list(void) 
{
	struct robot_module_event *event = new_robot_module_event();
	event->type = ROBOT_EVT_CLEAR_ALL;
	APP_EVENT_SUBMIT(event);
}

static void report_add_robot(uint64_t addr) 
{	
	struct robot_module_event *event = new_robot_module_event();
	event->type = ROBOT_EVT_REPORT;
	event->data.str = json_encode_add_robot_report(addr);
	APP_EVENT_SUBMIT(event);
}

static void report_remove_robot(uint64_t addr) 
{	
	struct robot_module_event *event = new_robot_module_event();
	event->type = ROBOT_EVT_REPORT;
	event->data.str = json_encode_remove_robot_report(addr);
	APP_EVENT_SUBMIT(event);
}

static void report_robot_movement_config(uint64_t addr) 
{	
	struct robot_module_event *event = new_robot_module_event();
	event->type = ROBOT_EVT_REPORT;
	event->data.str = json_encode_robot_movement_config_report(addr);
	APP_EVENT_SUBMIT(event);
}

static void report_robot_led_config(uint64_t addr) 
{	
	struct robot_module_event *event = new_robot_module_event();
	event->type = ROBOT_EVT_REPORT;
	event->data.str = json_encode_robot_led_config_report(addr);
	APP_EVENT_SUBMIT(event);
}

static void report_revolution_count_list(void) {
	struct robot_module_event *event = new_robot_module_event();
	event->type = ROBOT_EVT_REPORT;
	event->data.str = json_encode_robot_revolution_count_list();
	APP_EVENT_SUBMIT(event);
}

static void set_revolution_count(uint64_t addr, int revolutions) 
{
	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (robot->addr == addr) {
			robot->state = ROBOT_STATE_READY;
			robot->cfg.revolutions = revolutions;
			break;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (robot->state != ROBOT_STATE_READY) {
			return;
		}
	}

	report_revolution_count_list();
}

static void set_state_configured(uint64_t addr) 
{
	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (robot->addr == addr) {
			robot->state = ROBOT_STATE_CONFIGURED;
			break;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (robot->state != ROBOT_STATE_CONFIGURED) {
			return;
		}
	}

	struct robot_module_event *clear_to_move_event = new_robot_module_event();
	clear_to_move_event->type = ROBOT_EVT_CLEAR_TO_MOVE;
	APP_EVENT_SUBMIT(clear_to_move_event);
}

/* Internal robot list functions */
static void add_robot(uint64_t addr) 
{
	struct robot *robot = k_malloc(sizeof(struct robot));
	robot->addr = addr;
	sys_slist_append(&robot_list, &robot->node);
}

static void remove_robot(uint64_t addr) 
{
	struct robot *robot;
	SYS_SLIST_FOR_EACH_CONTAINER(&robot_list, robot, node) {
		if (robot->addr == addr) {
			break;
		}
	}
	sys_slist_find_and_remove(&robot_list, &robot->node);

	k_free(robot);
}

static void clear_robot_list(void) {
	sys_snode_t *node;

	node = sys_slist_get(&robot_list);
	while (node != NULL) {
		k_free(CONTAINER_OF(node, struct robot, node));
		node = sys_slist_get(&robot_list);
	}
}

/* Message handler for STATE_CONFIGURING. */
static void on_state_cloud_disconnected(struct robot_msg_data *msg)
{

	if (IS_EVENT(msg, mesh, MESH_EVT_ROBOT_ADDED)) {
		add_robot(msg->module.mesh.data.new_robot.addr);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) {
			report_clear_robot_list();
			report_robot_list();

			state_set(STATE_CLOUD_CONNECTED);
	}
}

/* Message handler for STATE_EXECUTING. */
static void on_state_cloud_connected(struct robot_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_UPDATE_DELTA)) {
		int err;
		cJSON *root_obj = json_parse_root_object(msg->module.cloud.data.pub_msg.ptr, 
											msg->module.cloud.data.pub_msg.len);
		if (root_obj == NULL) {
			LOG_ERR("Root object could not be obtained");
		}

		err = json_get_delta_robot_config(root_obj);
		if (err) {
			// LOG_ERR("could not get robot config %d", err);
		} 

		cJSON_Delete(root_obj);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_DISCONNECTED)) {
		state_set(STATE_CLOUD_DISCONNECTED);
	}

	if (IS_EVENT(msg, mesh, MESH_EVT_ROBOT_ADDED)) {
		add_robot(msg->module.mesh.data.new_robot.addr);
		report_add_robot(msg->module.mesh.data.new_robot.addr);
	}

	if (IS_EVENT(msg, mesh, MESH_EVT_MOVEMENT_CONFIG_ACCEPTED)) {
		report_robot_movement_config(msg->module.mesh.data.movement_config.addr); 
		set_state_configured(msg->module.mesh.data.movement_config.addr); 
	}

	if (IS_EVENT(msg, mesh, MESH_EVT_MOVEMENT_REPORTED)) {
		set_revolution_count(msg->module.mesh.data.movement_reported.addr, msg->module.mesh.data.movement_reported.yaw); 
	}
}

/* Message handler for all states. */
static void on_all_states(struct robot_msg_data *msg)
{ 

}

static void module_thread_fn(void)
{
	int err;
	struct robot_msg_data msg;

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(robot, ROBOT_EVT_ERROR, err);
	}

	state_set(STATE_CLOUD_DISCONNECTED);
	sys_slist_init(&robot_list);

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_CLOUD_DISCONNECTED:
			on_state_cloud_disconnected(&msg);
			break;
		case STATE_CLOUD_CONNECTED:
			on_state_cloud_connected(&msg);
			break;
		default:
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(robot_module_thread, CONFIG_ROBOT_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, robot_module_event);
APP_EVENT_SUBSCRIBE(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE(MODULE, ui_module_event);
APP_EVENT_SUBSCRIBE(MODULE, mesh_module_event);
