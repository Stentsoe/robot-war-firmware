
#include <zephyr.h>
#include <zephyr/bluetooth/mesh.h>
#include "./robot_movement_cli.h"
#include "../../common/mesh_model_defines/robot_movement_cli.h"

#define MODULE robot_config_client

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ROBOT_CONFIG_CLIENT_LOG_LEVEL);

/* Sent commands */

int configure_robot_movement(struct bt_mesh_robot_config_cli *config_client, uint16_t address, struct robot_movement_set_msg msg)
{
    if (!bt_mesh_is_provisioned())
    {
        LOG_ERR("Device not provisioned");
        return -EAGAIN;
    }
    struct bt_mesh_msg_ctx ctx =
        {
            .addr = address,
            .app_idx = config_client->model->keys[0],
            .send_ttl = BT_MESH_TTL_DEFAULT,
            .send_rel = false,
        };

    if (bt_mesh_msg_ack_ctx_prepare(&config_client->ack_ctx, OP_VND_ROBOT_MOVEMENT_SET_STATUS, address, NULL)){
        LOG_ERR("Failed to prepare ack ctx");
        return -EALREADY;
    }

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_VND_ROBOT_MOVEMENT_SET, sizeof(struct robot_movement_set_msg));
    bt_mesh_model_msg_init(&buf, OP_VND_ROBOT_MOVEMENT_SET);
    net_buf_simple_add_be32(&buf, msg.time);
    net_buf_simple_add_be32(&buf, msg.angle);
    int err = bt_mesh_model_send(config_client->model, &ctx, &buf, NULL, NULL);
    if (err)
    {
        LOG_ERR("Failed to send message (err %d)", err);
        bt_mesh_msg_ack_ctx_clear(&config_client->ack_ctx);
        return err;
    }

    return bt_mesh_msg_ack_ctx_wait(&config_client->ack_ctx, K_MSEC((CONFIG_BT_MESH_MOD_ACKD_TIMEOUT_BASE + 256 * CONFIG_BT_MESH_MOD_ACKD_TIMEOUT_PER_HOP)));
}

int send_clear_to_move(struct bt_mesh_robot_config_cli *config_client, uint16_t address)
{
    if (!bt_mesh_is_provisioned())
    {
        LOG_ERR("Device not provisioned");
        return -EAGAIN;
    }
    struct bt_mesh_msg_ctx ctx =
        {
            .addr = address,
            .app_idx = config_client->model->keys[0],
            .send_ttl = BT_MESH_TTL_DEFAULT,
            .send_rel = false,
        };

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_VND_ROBOT_CLEAR_TO_MOVE, 0);
    bt_mesh_model_msg_init(&buf, OP_VND_ROBOT_CLEAR_TO_MOVE);

    return bt_mesh_model_send(config_client->model, &ctx, &buf, NULL, NULL);
}

/* Received commands */
static int handle_robot_movement_done_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    LOG_DBG("Movement done status received");
    return 0;
}

static int handle_robot_movement_set_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    LOG_DBG("Movement set status received");
    struct bt_mesh_robot_config_cli *config_client = model->user_data;

    if (bt_mesh_msg_ack_ctx_match(&config_client->ack_ctx, OP_VND_ROBOT_MOVEMENT_SET_STATUS, ctx->addr, NULL))
    {
        bt_mesh_msg_ack_ctx_rx(&config_client->ack_ctx);
        LOG_DBG("ACK received for movement set message");
    }

    // TODO: Inform rest of system
    return 0;
}

/* Model callbacks */
const struct bt_mesh_model_cb robot_config_cli_cb;

/* Model operations */
const struct bt_mesh_model_op robot_config_cli_ops[] = {
    {
        OP_VND_ROBOT_MOVEMENT_DONE_STATUS,
        sizeof(struct robot_movement_done_status_msg),
        handle_robot_movement_done_status,
    },
    {
        OP_VND_ROBOT_MOVEMENT_SET_STATUS,
        sizeof(struct robot_movement_set_status_msg),
        handle_robot_movement_set_status,
    },
    BT_MESH_MODEL_OP_END,
};
