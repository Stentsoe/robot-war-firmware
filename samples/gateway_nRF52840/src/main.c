/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/device.h>

#define MODULE main

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#include "uart_handler.h"
#include "model_handler.h"

static const struct device *mesh_uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

void main(void)
{
	int err = init_uart(mesh_uart);
	if (err)
	{
		LOG_ERR("Could not initialize UART: Error %d", err);
		return;
	}
}
