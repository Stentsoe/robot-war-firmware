/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ID_SRV_H__
#define ID_SRV_H__


#include <zephyr/bluetooth/bluetooth.h>
#include <id.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ID_SRV_MODEL_ID 0x0002


struct bt_mesh_id_srv;

#define BT_MESH_ID_SRV_OP_STATUS BT_MESH_MODEL_OP_3(0x0A, \
				       CONFIG_BT_COMPANY_ID)

/** @def BT_MESH_MODEL_ID_SRV
 *
 * @brief Bluetooth Mesh Id Server model composition data entry.
 *
 * @param[in] _id Pointer to a @ref bt_mesh_id_srv instance.
 */
#define BT_MESH_MODEL_ID_SRV(_id)                                          \
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			ID_SRV_MODEL_ID,                      	\
			BT_MESH_MODEL_NO_OPS, &(_id)->pub,       \
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_id_srv, _id), 		\
			&_bt_mesh_id_srv_cb)

/** Bluetooth Mesh id server model handlers. */
struct bt_mesh_id_srv_handlers {
	// /** @brief Handler for sending id status message. 
	//  * Should return data that will be transmitted signalling
	//  * the identity of the device.
	//  *
	//  * @param[in] id Id Server that is sending the status message.
	//  * 
	//  * @retval The message containing the id status data.
	//  */
	// struct bt_mesh_id_status 
	// 	(*const status)(struct bt_mesh_id_srv *id);

	/** @brief Handler for server starting. Register id to be published. 
	 *
	 * @param[in] id Id Server.
	 * 
	 * @retval Id status data.
	 */
	struct bt_mesh_id_status *
		(*const start)(struct bt_mesh_id_srv *id);
};

/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_SRV_INIT.
 */
struct bt_mesh_id_srv {
    /** Application handler functions. */
	const struct bt_mesh_id_srv_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_ID_SRV_OP_STATUS, CONFIG_ID_SRV_MAX_LEN)];
};

// int bt_mesh_id_srv_pub(struct bt_mesh_id_srv *id,
// 			const struct bt_mesh_id_status *status);


extern const struct bt_mesh_model_cb _bt_mesh_id_srv_cb;

#ifdef __cplusplus
}
#endif

#endif /* ID_SRV_H__ */

