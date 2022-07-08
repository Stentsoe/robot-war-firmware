/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>
#include <dk_buttons_and_leds.h>
#include <bluetooth/mesh/dk_prov.h>

#define MODULE main

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#include "uart_handler.h"
#include "model_handler.h"
#include "robot_movement_cli.h"

static const struct device *mesh_uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

static struct bt_mesh_robot_config_cli *config_client;

static int setup_mesh()
{
	int err;
	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Failed to initialize bluetooth: Error %d", err);
		return err;
	}
	LOG_DBG("Bluetooth initialized");
	err = bt_mesh_init(bt_mesh_dk_prov_init(), model_handler_init(&config_client));
	if (err)
	{
		LOG_ERR("Failed to initialize mesh: Error %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		err = settings_load();
		if (err)
		{
			LOG_ERR("Failed to load settings: Error %d", err);
		}
	}

	err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
	if (err == -EALREADY)
	{
		LOG_DBG("Device already provisioned");
		LOG_DBG("Mesh initialized");
	}

	return 0;
}

void main(void)
{
	int err = 0;
	err = setup_mesh();
	if (err)
	{
		LOG_ERR("Failed to setup mesh: Error %d", err);
		return;
	}

	err = init_uart(mesh_uart);
	if (err)
	{
		LOG_ERR("Could not initialize UART: Error %d", err);
		return;
	}
}
