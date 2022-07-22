/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOVEMENT_SRV_H__
#define MOVEMENT_SRV_H__


#include <movement.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOVEMENT_SRV_MODEL_ID 0x0003

struct bt_mesh_movement_srv;

#define BT_MESH_MOVEMENT_SRV_OP_MOVEMENT_SET BT_MESH_MODEL_OP_3(0x0A, \
				       CONFIG_BT_COMPANY_ID)

#define BT_MESH_MOVEMENT_SRV_OP_MOVEMENT_ACK BT_MESH_MODEL_OP_3(0x0B, \
				       CONFIG_BT_COMPANY_ID)

#define BT_MESH_MOVEMENT_SRV_OP_READY_SET BT_MESH_MODEL_OP_3(0x0C, \
				       CONFIG_BT_COMPANY_ID)

/** Bluetooth Mesh Movement Server model handlers. */
struct bt_mesh_movement_srv_handlers {
	/** @brief Handler for a set movement message. 
	 *
	 * @param[in] movement Movement Server that received the set message.
	 * @param[in] msg The message containing the movement data.
	 */
	void (*const set)(struct bt_mesh_movement_srv *movement, 
			struct bt_mesh_movement_set msg);
			
	/** @brief Handler for a ready to move message. 
	 *
	 * @param[in] movement Movement Server that received the ready message.
	 */
	void (*const ready)(struct bt_mesh_movement_srv *movement);
};

/** @def BT_MESH_MODEL_MOVEMENT_SRV
 *
 * @brief Bluetooth Mesh Movement Server model composition data entry.
 *
 * @param[in] _movement Pointer to a @ref bt_mesh_movement_srv instance.
 */
#define BT_MESH_MODEL_MOVEMENT_SRV(_movement)			\
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			MOVEMENT_SRV_MODEL_ID,                      \
			_bt_mesh_movement_srv_op, NULL,       		\
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_movement_srv, _movement), \
			&_bt_mesh_movement_srv_cb)
		
/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_SRV_INIT.
 */
struct bt_mesh_movement_srv {
    /** Application handler functions. */
	const struct bt_mesh_movement_srv_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_MOVEMENT_SRV_OP_MOVEMENT_ACK, 
		BT_MESH_LEN_EXACT(sizeof(0)))];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

extern const struct bt_mesh_model_op _bt_mesh_movement_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_movement_srv_cb;

#ifdef __cplusplus
}
#endif

#endif /* MOVEMENT_SRV_H__ */

