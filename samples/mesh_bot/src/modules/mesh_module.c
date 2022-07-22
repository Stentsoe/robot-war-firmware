
#include <zephyr.h>
#include <app_event_manager.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/dk_prov.h>
#include <zephyr/bluetooth/mesh/msg.h>
#include <bluetooth/mesh/models.h>

// #include "id_srv.h"
// #include "movement_srv.h"
#include "robot_srv.h"

#define MODULE mesh
#include "../events/module_state_event.h"
#include "../events/motor_module_event.h"
#include "../events/mesh_module_event.h"

// #include "model_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_MODULE_LOG_LEVEL);

/* State handling*/

// Top level module states
static enum state_type
{
    STATE_UNPROVISIONED,
    STATE_PROVISIONED,
    STATE_READY_TO_MOVE,
    STATE_MOVING,
} state;

/** Message item that holds any received events */
struct mesh_msg_data
{
    union
    {
        struct motor_module_event motor;
    } event;
};

/* Modem module message queue. */
#define MESH_QUEUE_ENTRY_COUNT		10
#define MESH_QUEUE_BYTE_ALIGNMENT	4

/** Module message queue */
K_MSGQ_DEFINE(mesh_module_msg_q, sizeof(struct mesh_msg_data),
        MESH_QUEUE_ENTRY_COUNT, MESH_QUEUE_BYTE_ALIGNMENT);

/** Globals */
bt_addr_le_t addr;

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_UNPROVISIONED:
		return "STATE_UNPROVISIONED";
	case STATE_PROVISIONED:
		return "STATE_PROVISIONED";
	case STATE_READY_TO_MOVE:
		return "STATE_READY_TO_MOVE";
	case STATE_MOVING:
		return "STATE_MOVING";
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

/* Mesh handlers */
static bool app_event_handler(const struct app_event_header *header)
{
    struct mesh_msg_data msg = {0};
    bool enqueue = false;

    if (is_motor_module_event(header))
    {
        LOG_DBG("Motor module event received");
        struct motor_module_event *evt = cast_motor_module_event(header);
        msg.event.motor = *evt;
        enqueue = true;
    }

    if (enqueue)
    {
        int err = k_msgq_put(&mesh_module_msg_q, &msg, K_FOREVER);
        if (err)
        {
            LOG_ERR("Message could not be enqueued");
        }
    }

    return false;
}

/* SIG models */
static void attention_on(struct bt_mesh_model *model)
{
    printk("attention_on()\n");
}

static void attention_off(struct bt_mesh_model *model)
{
    printk("attention_off()\n");
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
    .attn_on = attention_on,
    .attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
    .cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);



/* Vendor models */
// struct bt_mesh_id_status handle_id_start(struct bt_mesh_id_srv *id)
// {
// 	size_t count;
//     struct bt_mesh_id_status identity;

// 	bt_id_get(&addr, &count);
// 	if (count == 0) {
//         identity.id = NULL;
// 		return identity;
// 	}

//     identity.id = addr.a.val;
//     identity.len = BT_ADDR_SIZE;

//     return identity;
// }

// static void handle_movement_set_message(struct bt_mesh_movement_srv *movement,
// 				struct bt_mesh_movement_set msg)
// {
	
// }

// static const struct bt_mesh_id_srv_handlers id_cb = {
// 	.start = handle_id_start,
// };

// static const struct bt_mesh_movement_srv_handlers movement_cb = {
// 	.set = handle_movement_set_message,
// };

// static struct bt_mesh_id_srv id = {
//     .handlers = &id_cb,
// };

// static struct bt_mesh_movement_srv movement = {
//     .handlers = &movement_cb,
// };

static struct bt_mesh_id_status handle_robot_identify(struct bt_mesh_robot_srv *robot)
{
    size_t count;
    struct bt_mesh_id_status identity;
    // do {
    bt_id_get(&addr, &count);
    if (count == 0) {
        identity.id = NULL;
        LOG_ERR("No ids found");
        return identity;
    }
    // } while (count == 0);
	

    identity.id = addr.a.val;
    identity.len = BT_ADDR_SIZE;
    return identity;
}

static const struct bt_mesh_robot_srv_handlers robot_cb = {
	.identify = handle_robot_identify,
};

static struct bt_mesh_robot_srv robot = {
    .handlers = &robot_cb,
};

/* Composition */
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(
		0,
        BT_MESH_MODEL_LIST(
            BT_MESH_MODEL_CFG_SRV,
            BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)
        ),
        BT_MESH_MODEL_LIST( 
            BT_MESH_MODEL_ROBOT_SRV(&robot)
        )
    )
};

static struct bt_mesh_comp comp = {
    .cid = CONFIG_BT_COMPANY_ID,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

/* Setup */
static void register_id(void) 
{
    size_t count;
    struct bt_mesh_id_status identity;

    bt_id_get(&addr, &count);
    if (count == 0) {
        identity.id = NULL;
        LOG_ERR("No ids found");
        return;
    }

    identity.id = addr.a.val;
    identity.len = BT_ADDR_SIZE;
    bt_mesh_robot_register_id(identity);
}

static int setup_mesh(void)
{
    int err;
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Failed to initialize bluetooth: Error %d", err);
        return err;
    }
    LOG_DBG("Bluetooth initialized");

    err = bt_mesh_init(bt_mesh_dk_prov_init(), &comp);
    if (err) {
        LOG_ERR("Failed to initialize mesh: Error %d", err);
        return err;
    }
    
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        err = settings_load();
        if (err) {
            LOG_ERR("Failed to load settings: Error %d", err);
        }
    }
    

    err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
    if (err == -EALREADY) {
        LOG_DBG("Device already provisioned");
        state_set(STATE_PROVISIONED);
        struct mesh_module_event *evt = new_mesh_module_event();
        evt->type = MESH_EVT_PROVISIONED;
        APP_EVENT_SUBMIT(evt);
    }
    LOG_DBG("Mesh initialized");
    // register_id();
    return 0;
}

/* Module thread */
static void module_thread_fn(void)
{
    LOG_DBG("Mesh module thread started");
    struct mesh_msg_data msg;

    int err;

    err = setup_mesh();

    if (err) {
        LOG_ERR("Failed to set up mesh. Error %d", err);
        return;
    }

    while (true) {
        k_msgq_get(&mesh_module_msg_q, &msg, K_FOREVER);

        switch(state) {
            case STATE_UNPROVISIONED: {
                break;
            }
            case STATE_PROVISIONED: {
                break;
            }
            case STATE_READY_TO_MOVE: {
                break;
            }
            case STATE_MOVING: {
                break;
            }
            default: {
                LOG_ERR("Unknown mesh module state %d", state);
            }
        }
    }
}

K_THREAD_DEFINE(mesh_module_thread, CONFIG_MESH_THREAD_STACK_SIZE,module_thread_fn,
    NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

/* Event handling */

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, motor_module_event);