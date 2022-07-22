/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "movement_srv.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(movement_srv);

struct bt_mesh_movement_set extract_msg(struct net_buf_simple *buf)
{
	struct bt_mesh_movement_set mov_conf;
    mov_conf.time = net_buf_simple_pull_be32(buf);
    mov_conf.angle = net_buf_simple_pull_be32(buf);
    mov_conf.speed = net_buf_simple_pull_u8(buf);
	return mov_conf;
}

static int handle_message_movement_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_movement_srv *movement = model->user_data;
	struct bt_mesh_movement_set msg;
	int err;

	msg = extract_msg(buf);

	if (movement->handlers->set) {
		movement->handlers->set(movement, msg);
	}

	BT_MESH_MODEL_BUF_DEFINE(ack, BT_MESH_MOVEMENT_SRV_OP_MOVEMENT_ACK, sizeof(0));
    bt_mesh_model_msg_init(&ack, BT_MESH_MOVEMENT_SRV_OP_MOVEMENT_ACK);
    net_buf_simple_add_u8(&ack, 0);
    err = bt_mesh_model_send(model, ctx, &ack, NULL, NULL);
    if (err)
    {
        printk("Failed to send message ack (err %d)", err);
    }

	return 0;
}

static int handle_message_ready_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_movement_srv *movement = model->user_data;

	if (movement->handlers->ready) {
		movement->handlers->ready(movement);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_movement_srv_op[] = {
	{
		BT_MESH_MOVEMENT_SRV_OP_MOVEMENT_SET, BT_MESH_LEN_EXACT(sizeof(struct bt_mesh_movement_set)),
		handle_message_movement_set
	},
	{
		BT_MESH_MOVEMENT_SRV_OP_READY_SET, BT_MESH_LEN_EXACT(sizeof(0)), handle_message_ready_set
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_movement_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_movement_srv *movement = model->user_data;

	movement->model = model;

	net_buf_simple_init_with_data(&movement->pub_msg, movement->buf,
				      sizeof(movement->buf));

	movement->pub.msg = &movement->pub_msg;

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_movement_srv_cb = {
	.init = bt_mesh_movement_srv_init,
};