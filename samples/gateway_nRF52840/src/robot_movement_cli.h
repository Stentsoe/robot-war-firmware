#pragma once

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>
#include "../../common/mesh_model_defines/robot_movement_srv.h"

// Defined in model_handler.c
extern const struct bt_mesh_model_op robot_config_cli_ops[];
extern const struct bt_mesh_model_cb robot_config_cli_cb;

struct bt_mesh_robot_config_cli_handlers {
};

struct bt_mesh_robot_config_cli
{
    struct bt_mesh_model *model;
    struct bt_mesh_model_pub pub;
    struct net_buf_simple pub_msg;
    uint8_t buf[BT_MESH_MODEL_BUF_LEN(OP_VND_ROBOT_CLEAR_TO_MOVE, 0)];
    struct bt_mesh_robot_config_cli_handlers handlers;
    struct bt_mesh_msg_ack_ctx ack_ctx;
};

#define BT_MESH_MODEL_VND_ROBOT_CONFIG_CLI(_robot_config_cli)                        \
    BT_MESH_MODEL_VND_CB(                                                            \
        CONFIG_BT_COMPANY_ID,                                                        \
        ROBOT_MOVEMENT_CLI_MODEL_ID,                                                 \
        robot_config_cli_ops,                                                        \
        &(_robot_config_cli)->pub,                                                   \
        BT_MESH_MODEL_USER_DATA(struct bt_mesh_robot_config_cli, _robot_config_cli), \
        &robot_config_cli_cb)

/**
 * @brief Configure the next movement for a robot.
 *
 * @param config_client The robot configuration client to send from
 * @param address Address of robot to configure.
 * @param msg Movement configuration
 * @return 0 on success, negative error code otherwise.
 */
int configure_robot_movement(struct bt_mesh_robot_config_cli *config_client, uint16_t address, struct robot_movement_set_msg msg);

/**
 * @brief Signal a robot that it should start moving.
 *
 * @param config_client The robot configuration client to send from
 * @param address Address of robot to configure.
 * @return 0 on success, negative error code otherwise.
 */
int send_clear_to_move(struct bt_mesh_robot_config_cli *config_client, uint16_t address);
