/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#define MODULE mesh_module

#include "../../common/nRF9160dk_uart_interface/messages.h"
#include "mesh_module_event.h"
#include "robot_module_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_MODULE_LOG_LEVEL);

struct mesh_msg_data
{
	union
	{
		struct robot_module_event robot;
	} module;
};

#define MESH_QUEUE_ENTRY_COUNT 10
#define MESH_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_mesh, sizeof(struct mesh_msg_data),
			  MESH_QUEUE_ENTRY_COUNT, MESH_QUEUE_BYTE_ALIGNMENT);

/* mesh module super states. */
static enum state_type {
	STATE_MESH_NOT_READY,
	STATE_MESH_READY,
} state;

static enum uart_tx_state_type {
	STATE_IDLE,
	STATE_TRANSMITTING,
} uart_tx_state = STATE_IDLE;

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state)
	{
	case STATE_MESH_NOT_READY:
		return "STATE_MESH_NOT_READY";
	case STATE_MESH_READY:
		return "STATE_MESH_READY";
	default:
		return "Unknown state";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state)
	{
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
			state2str(state),
			state2str(new_state));

	state = new_state;
}

static void uart_tx_state_set(enum uart_tx_state_type new_state) {
	uart_tx_state = new_state;
}

/* Mesh module devices and static module data. */

// UART interface to bluetooth mesh MCU
static const struct device *mesh_uart = DEVICE_DT_GET(DT_ALIAS(uartmesh));
K_MEM_SLAB_DEFINE_STATIC(mesh_uart_rx_slab, CONFIG_MESH_UART_RX_BUF_SIZE, CONFIG_MESH_UART_RX_BUF_COUNT, 4);

/* Module state handlers. */

/* Message handler for all states. */
static void on_all_states(struct mesh_msg_data *msg)
{
}

/* Event handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct mesh_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_robot_module_event(aeh))
	{
		struct robot_module_event *evt = cast_robot_module_event(aeh);
		msg.module.robot = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg)
	{
		int err = k_msgq_put(&msgq_mesh, &msg, K_NO_WAIT);

		if (err)
		{
			LOG_ERR("Message could not be enqueued");
		}
	}

	return false;
}

/* UART related functions*/

static int mesh_uart_send(const void *data, size_t len)
{
	int err = uart_tx(mesh_uart, data, len, SYS_FOREVER_US);
	uart_tx_state_set(STATE_TRANSMITTING);

	if (err)
	{
		LOG_ERR("Failed to start UART tx: %d", err);
	}
	return err;
}

static int uart_send_hello(void)
{
	static struct mesh_uart_hello_msg msg = {
		.header = {
			.type = HELLO,
		},
		.echo = 0x1010
	};
	return mesh_uart_send(&msg, sizeof(msg));
}

static int uart_data_rx_rdy_handler(struct uart_event_rx event_data)
{
	static union mesh_uart_msg msg;

	uint8_t *msg_fill_start = ((uint8_t *)&msg) + event_data.offset;
	memcpy(msg_fill_start, event_data.buf, event_data.len);

	size_t msg_received_len = event_data.offset + event_data.len;
	if (msg_received_len < sizeof(msg.header))
	{
		return 0; // Header not yet received. Keep waiting for more data.
	}

	switch (msg.header.type)
	{
	case HELLO:
	{
		if (msg_received_len < sizeof(msg.hello))
		{
			return 0; // Message not complete. Keep waiting for more data.
		}
		LOG_DBG("UART \"HELLO\" received");
		state_set(STATE_MESH_READY);
		struct mesh_module_event *evt = new_mesh_module_event();
		evt->type = MESH_EVT_READY;
		APP_EVENT_SUBMIT(evt);
		break;
	}
	case ROBOT_ADDED:
	{
		if (msg_received_len < sizeof(msg.robot_added))
		{
			return 0; // Message not complete. Keep waiting for more data.
		}
		LOG_DBG("UART \"ROBOT_ADDED\" received");
		break;
	}
	case CONFIG_ACK:
	{
		if (msg_received_len < sizeof(msg.config_ack))
		{
			return 0; // Message not complete. Keep waiting for more data.
		}
		LOG_DBG("UART \"CONFIG_ACK\" received");
		break;
	}
	case MOVEMENT_REPORTED:
	{
		if (msg_received_len < sizeof(msg.movement_reported))
		{
			return 0; // Message not complete. Keep waiting for more data.
		}
		LOG_DBG("UART \"MOVEMENT_REPORTED\" received");
		break;
	}
	default:
	{
		LOG_ERR("Unknown message type %d", msg.header.type);
		return -EINVAL;
	}
	}

	return 0;
}

static void uart_callback(const struct device *dev, struct uart_event *event, void *user_data)
{
	switch (event->type)
	{
	case UART_TX_DONE:
	{
		LOG_DBG("UART_TX_DONE: Sent %d bytes", event->data.tx.len);
		uart_tx_state_set(STATE_IDLE);
		break;
	}
	case UART_TX_ABORTED:
	{
		LOG_ERR("UART_TX_ABORTED");
		uart_tx_state_set(STATE_IDLE);
		break;
	}
	case UART_RX_RDY:
	{
		LOG_DBG("UART_RX_RDY");
		uart_data_rx_rdy_handler(event->data.rx);
		break;
	}
	case UART_RX_BUF_REQUEST:
	{
		LOG_DBG("UART_RX_BUF_REQUEST");
		void *new_buf;
		int err = k_mem_slab_alloc(&mesh_uart_rx_slab, &new_buf, K_NO_WAIT);
		if (err)
		{
			LOG_ERR("Could not allocate new buffer: Error %d", err);
		}
		else
		{
			err = uart_rx_buf_rsp(dev, new_buf, CONFIG_MESH_UART_RX_BUF_SIZE);
			if (err)
			{
				LOG_ERR("Could not set new buffer: Error %d", err);
			}
		}
		break;
	}
	case UART_RX_BUF_RELEASED:
	{
		LOG_DBG("UART_RX_BUF_RELEASED");
		k_mem_slab_free(&mesh_uart_rx_slab, (void **)&event->data.rx_buf.buf);
		break;
	}
	case UART_RX_DISABLED:
	{
		LOG_DBG("UART_RX_DISABLED");
		break;
	}
	case UART_RX_STOPPED:
	{
		LOG_DBG("UART_RX_STOPPED: Reason: %d", event->data.rx_stop.reason);
		break;
	}
	default:
		LOG_ERR("Unknown UART event type %d", event->type);
		break;
	}
}

static int init_uart()
{
	int err = 0;
	const struct gpio_dt_spec reset_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(nrf52840_reset), gpios, 0);
	if (!device_is_ready(reset_gpio.port)){
		LOG_ERR("nRF52840 Reset GPIO not ready");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&reset_gpio, GPIO_OUTPUT_INACTIVE);
	gpio_pin_set_dt(&reset_gpio, 1);
	k_sleep(K_MSEC(10)); 
	gpio_pin_set_dt(&reset_gpio, 0);
	k_sleep(K_MSEC(1000)); // Wait for UART on nRF52840 to be ready.

	if (!device_is_ready(mesh_uart))
	{
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}
	LOG_DBG("UART device ready");

	err = uart_callback_set(mesh_uart, uart_callback, NULL);
	if (err)
	{
		LOG_ERR("Failed to set UART callback: Error %d", err);
		return err;
	}
	void *initial_rx_buf;
	err = k_mem_slab_alloc(&mesh_uart_rx_slab, &initial_rx_buf, K_NO_WAIT);
	if (err)
	{
		LOG_ERR("Could not allocate initial RX buffer");
		return err;
	}
	uart_rx_enable(mesh_uart, initial_rx_buf, CONFIG_MESH_UART_RX_BUF_SIZE, 1000);

	

	return 0;
}

static void module_thread_fn(void)
{
	LOG_INF("Mesh module thread started");
	state_set(STATE_MESH_NOT_READY);

	int err = init_uart();
	if (err)
	{
		LOG_ERR("Could not initialize UART: Error %d", err);
		return;
	}
	LOG_DBG("UART initialized");

	err = uart_send_hello();

	struct mesh_msg_data msg = {0};
	while (true)
	{
		k_msgq_get(&msgq_mesh, &msg, K_FOREVER);

		switch (state)
		{

		default:
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(mesh_module_thread, CONFIG_MESH_THREAD_STACK_SIZE,
				module_thread_fn, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, robot_module_event);