/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "id_srv.h"
#include "mesh/net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(id_srv);

static struct bt_mesh_id_status *identity;

static int bt_mesh_id_srv_update_handler(struct bt_mesh_model *model)
{
	if (identity == NULL) {
		return -ENODATA;
	}

	if (identity->id == NULL) {
		return -ENODATA;
	}

	bt_mesh_model_msg_init(model->pub->msg, BT_MESH_ID_SRV_OP_STATUS);
	net_buf_simple_add_mem(model->pub->msg, identity->id, identity->len);
	// LOG_HEXDUMP_DBG(model->pub->msg->data, model->pub->msg->len, "publishing:");

    return 0;
}

static int bt_mesh_id_srv_start(struct bt_mesh_model *model)
{
	struct bt_mesh_id_srv *id = model->user_data;
	
	if (id->handlers->start) {
		identity = id->handlers->start(id);
	}

	return 0;
}

static int bt_mesh_id_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_id_srv *id = model->user_data;

	id->model = model;

	net_buf_simple_init_with_data(&id->pub_msg, id->buf,
				      sizeof(id->buf));

	id->pub.msg = &id->pub_msg;
	id->pub.update = bt_mesh_id_srv_update_handler;
	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_id_srv_cb = {
	.init = bt_mesh_id_srv_init,
	.start = bt_mesh_id_srv_start,
};
