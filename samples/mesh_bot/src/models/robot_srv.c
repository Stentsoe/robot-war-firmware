/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "robot_srv.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(robot_srv);

static struct bt_mesh_id_status identity;

static struct bt_mesh_movement_set movement_config;

void bt_mesh_robot_register_id(struct bt_mesh_id_status id) {
	identity = id;
	LOG_HEXDUMP_DBG(identity.id, identity.len, "registered in robot model:");
}

struct bt_mesh_id_status* handle_id_start(struct bt_mesh_id_srv *id)
{
    return &identity;
}

static const struct bt_mesh_id_srv_handlers id_cb = {
	.start = handle_id_start,
};


static void handle_movement_set(struct bt_mesh_movement_srv *movement, 
				struct bt_mesh_movement_set msg)
{
	movement_config = msg;
}

static void handle_movement_ready(struct bt_mesh_movement_srv *movement)
{	
	struct bt_mesh_robot_srv *robot = 
		CONTAINER_OF(movement, struct bt_mesh_robot_srv, movement);

	if (robot->handlers->move) {
		identity = robot->handlers->move(robot, &movement_config);
	}
}

static const struct bt_mesh_movement_srv_handlers movement_cb = {
	.set = handle_movement_set,
	.ready = handle_movement_ready,
};

// static int bt_mesh_robot_srv_update_handler(struct bt_mesh_model *model)
// {
// 	return 1;
// }

static int bt_mesh_robot_srv_start(struct bt_mesh_model *model)
{
	struct bt_mesh_robot_srv *robot = model->user_data;

	if (robot->handlers->identify) {
		identity = robot->handlers->identify(robot);
	}

	return 0;
}

static int bt_mesh_robot_srv_init(struct bt_mesh_model *model)
{
	int err;
	struct bt_mesh_robot_srv *robot = model->user_data;

	robot->model = model;

	net_buf_simple_init_with_data(&robot->pub_msg, robot->buf,
				      sizeof(robot->buf));

	robot->pub.msg = &robot->pub_msg;
	// robot->pub.update = bt_mesh_robot_srv_update_handler;

	robot->id.handlers = &id_cb;
	robot->movement.handlers = &movement_cb;

	err = bt_mesh_model_extend(model, robot->movement.model);
	if (err) {
		return err;
	}

	return bt_mesh_model_extend(model, robot->id.model);
}

const struct bt_mesh_model_cb _bt_mesh_robot_srv_cb = {
	.init = bt_mesh_robot_srv_init,
	.start = bt_mesh_robot_srv_start,

};
