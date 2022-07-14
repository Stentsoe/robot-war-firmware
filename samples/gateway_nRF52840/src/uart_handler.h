#pragma once
#include <zephyr/device.h>
#include "../../common/nRF9160dk_uart_interface/messages.h"
#include "robot_movement_cli.h"

int init_uart(const struct device *dev, struct bt_mesh_robot_config_cli *config_client);

