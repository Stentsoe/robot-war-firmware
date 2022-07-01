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

#ifdef __cplusplus
extern "C" {
#endif


enum mesh_module_event_type {
	MESH_EVT_ROBOT_ADDED,
    MESH_EVT_CONFIG_ACK,
    MESH_EVT_MOVEMENT_REPORTED,
};

struct mesh_config_ack_data {
    int32_t seq_num;
};

struct mesh_robot_added_data {
    uint64_t addr;
};

//TODO: Add data for movement reported
struct mesh_module_event {
    struct app_event_header header;
    enum mesh_module_event_type type;
    union {
        struct mesh_config_ack_data config_ack;
        struct mesh_robot_added_data new_robot;
    } data;
};

APP_EVENT_TYPE_DECLARE(mesh_module_event);

#ifdef __cplusplus
}
#endif