#pragma once

#include <zephyr/bluetooth/mesh.h>
#include "../../common/mesh_model_defines/robot_movement_srv.h"
#include "./robot_movement_cli.h"

const struct bt_mesh_comp *model_handler_init(struct bt_mesh_robot_config_cli **config_client);
