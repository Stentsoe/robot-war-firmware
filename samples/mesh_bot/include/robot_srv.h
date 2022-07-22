/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ROBOT_SRV_H__
#define ROBOT_SRV_H__


#include <robot.h>
#include <id_srv.h>
#include <movement_srv.h>
#include <zephyr/bluetooth/mesh/access.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ROBOT_SRV_MODEL_ID 0x0001


struct bt_mesh_robot_srv;

#define OP_VND_ROBOT_SET BT_MESH_MODEL_OP_3(0x0A, \
				       CONFIG_BT_COMPANY_ID)

/** @def BT_MESH_MODEL_ROBOT_SRV
 *
 * @brief Bluetooth Mesh Robot Server model composition data entry.
 *
 * @param[in] _robot Pointer to a @ref bt_mesh_robot_srv instance.
 */
#define BT_MESH_MODEL_ROBOT_SRV(_robot)					\
		BT_MESH_MODEL_ID_SRV(&(_robot)->id),  			\
		BT_MESH_MODEL_MOVEMENT_SRV(&(_robot)->movement),  			\
		BT_MESH_MODEL_VND_CB(CONFIG_BT_COMPANY_ID,      \
			ROBOT_SRV_MODEL_ID,                      	\
			BT_MESH_MODEL_NO_OPS, &(_robot)->pub,       			\
			BT_MESH_MODEL_USER_DATA(					\
				struct bt_mesh_robot_srv, _robot), 		\
			&_bt_mesh_robot_srv_cb), 

/** Bluetooth Mesh robot server model handlers. */
struct bt_mesh_robot_srv_handlers {
	/** @brief Handler for registering identity of robot.
	 * 
	 * Called once and should return the identity of the robot.
	 *
	 * @param[in] robot Robot Server
	 * 
	 * @retval Robot id
	 */
	struct bt_mesh_id_status 
		(*const identify)(struct bt_mesh_robot_srv *robot);
	/** @brief Handler for incoming movement configurations.
	 *
	 * @param[in] robot Robot Server
	 * 
	 * @retval Robot id
	 */
	struct bt_mesh_id_status 
		(*const move)(struct bt_mesh_robot_srv *robot,
					  struct bt_mesh_movement_set *movement);
};

/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_SRV_INIT.
 */
struct bt_mesh_robot_srv {
	/** Light Level Server instance. */
	struct bt_mesh_id_srv id;
	/** Light Level Server instance. */
	struct bt_mesh_movement_srv movement;
    /** Application handler functions. */
	const struct bt_mesh_robot_srv_handlers * handlers;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_msg;
	/* Publication data */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(
		OP_VND_ROBOT_SET, CONFIG_ID_SRV_MAX_LEN)];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

void bt_mesh_robot_register_id(struct bt_mesh_id_status id);

extern const struct bt_mesh_model_cb _bt_mesh_robot_srv_cb;

#ifdef __cplusplus
}
#endif

#endif /* ROBOT_SRV_H__ */

