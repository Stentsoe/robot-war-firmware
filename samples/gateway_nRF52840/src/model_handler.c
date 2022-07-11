#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/mesh/msg.h>

#include "../../common/mesh_model_defines/robot_movement_cli.h"
#include "model_handler.h"
#include "robot_movement_cli.h"

/* SIG models */

static void attention_on(struct bt_mesh_model *model)
{
    printk("attention_on()\n");
}

static void attention_off(struct bt_mesh_model *model)
{
    printk("attention_off()\n");
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
    .attn_on = attention_on,
    .attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
    .cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_model sig_models[] = {
    BT_MESH_MODEL_CFG_SRV,
    BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
};

/* Vendor models */

struct bt_mesh_robot_config_cli robot_conf_cli;

static struct bt_mesh_model vendor_models[] = {
    BT_MESH_MODEL_VND_ROBOT_CONFIG_CLI(&robot_conf_cli),
};

/* Composition */
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, sig_models, vendor_models),
};

static struct bt_mesh_comp comp = {
    .cid = CONFIG_BT_COMPANY_ID,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(struct bt_mesh_robot_config_cli **config_client)
{
    *config_client = &robot_conf_cli;
    (*config_client)->model = &vendor_models[0];
    bt_mesh_msg_ack_ctx_init(&(*config_client)->ack_ctx);
    return &comp;
}
