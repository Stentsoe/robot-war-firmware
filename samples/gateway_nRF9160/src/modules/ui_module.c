/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/device.h>

#define MODULE ui_module

#include "modules_common.h"
#include "ui_module_event.h"

#include <zephyr/logging/log.h>
#define UI_MODULE_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, UI_MODULE_LOG_LEVEL);

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states) 
	{
		struct ui_module_event *event = new_ui_module_event();
		event->type = UI_EVT_BUTTON;
		event->data.button.action = BUTTON_PRESS;

		if (has_changed & DK_BTN1_MSK) 
		{
			event->data.button.num = DK_BTN1;
		} 
		else if (has_changed & DK_BTN2_MSK) 
		{
			event->data.button.num = DK_BTN2;
		}
		APP_EVENT_SUBMIT(event);
	}
}

/* Static module functions. */
static int setup(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
		return err;
	}

	return 0;
}

SYS_INIT(setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

