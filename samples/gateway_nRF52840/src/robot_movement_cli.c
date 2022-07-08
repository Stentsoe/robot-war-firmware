
#include <zephyr.h>
#include <zephyr/bluetooth/mesh.h>
#include "./robot_movement_cli.h"
#include "../../common/mesh_model_defines/robot_movement_cli.h"

/* Sent commands */

int configure_robot_movement(struct bt_mesh_robot_config_cli *config_client, uint16_t address, struct robot_movement_set_msg msg)
{
    struct bt_mesh_msg_ctx ctx;
}

int send_clear_to_move(struct bt_mesh_robot_config_cli *config_client, uint16_t address)
{
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
    BT_MESH_MODEL_OP_END,
};