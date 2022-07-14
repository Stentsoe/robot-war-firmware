/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

/**
 * @brief MESH Event
 * @defgroup mesh_module_event MESH Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#include "../../common/nRF9160dk_uart_interface/messages.h"

#ifdef __cplusplus
extern "C" {
#endif


enum mesh_module_event_type {
    MESH_EVT_READY, // Mesh module is ready to use.
	MESH_EVT_ROBOT_ADDED, // Robot added to network.
    MESH_EVT_OP_STATUS, // Status of previous operation.
    MESH_EVT_MOVEMENT_REPORTED, // Movement reported by robot.
};

struct mesh_module_event {
    struct app_event_header header;
    enum mesh_module_event_type type;
    union {
        int status;
        struct mesh_uart_robot_added_data new_robot;
        struct mesh_uart_movement_reported_data movement_reported;
    } data;
};

APP_EVENT_TYPE_DECLARE(mesh_module_event);

#ifdef __cplusplus
}
#endif