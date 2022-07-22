/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef BT_MESH_MOVEMENT_H__
#define BT_MESH_MOVEMENT_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Movement set message parameters.  */
struct bt_mesh_movement_set {
	/** Amount of time the device should be moving */
	uint32_t time;
	/** Angle device should turn before moving */
	int32_t angle;
	/** Speed the device should move at */
	uint8_t speed;
};

// /** Movement set status message parameters.  */
// struct bt_mesh_movement_set_status {
// 	/** status of received */
// 	uint8_t status;
// };

#ifdef __cplusplus
}
#endif

#define BT_MESH_MOVEMENT_MSG_LEN_SET 10

#endif /* BT_MESH_MOVEMENT_H__ */

