/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef BT_MESH_ID_H__
#define BT_MESH_ID_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Id status message parameters.  */
struct bt_mesh_id_status {
	/** static identity of device */
	uint8_t *id;
	/** length of id in bytes */
	uint8_t len;
};

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_ID_H__ */
