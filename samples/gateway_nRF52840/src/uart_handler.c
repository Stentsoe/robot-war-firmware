#include <zephyr.h>
#include <zephyr/drivers/uart.h>

#include "./robot_movement_cli.h"
#include "uart_handler.h"

#define MODULE uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_UART_MODULE_LOG_LEVEL);

K_MEM_SLAB_DEFINE_STATIC(mesh_uart_rx_slab, CONFIG_MESH_UART_RX_BUF_SIZE, CONFIG_MESH_UART_RX_BUF_COUNT, 4);

static int uart_data_rx_rdy_handler(const struct device* dev, struct uart_event_rx event_data, struct bt_mesh_robot_config_cli *config_client)
{
	static union mesh_uart_msg msg;
	static size_t current_msg_len = 0; // Length of currently received fragment

	uint8_t *msg_fill_start = ((uint8_t *)&msg) + current_msg_len;
	memcpy(msg_fill_start, (event_data.buf + event_data.offset), event_data.len);
	current_msg_len += event_data.len;

	LOG_DBG("Message fragment received: length: %d offset: %d", event_data.len, event_data.offset);
	if (current_msg_len < sizeof(msg.header))
	{
		return 0; // Header not yet received. Keep waiting for more data.
	}

	LOG_DBG("Got message with type %d", msg.header.type);
	switch (msg.header.type)
	{
	case HELLO:
	{
		if (current_msg_len < sizeof(msg.hello))
		{
			return 0; // Message not complete. Keep waiting for more data.
		}
		LOG_DBG("UART \"HELLO\" received");
		uart_tx(dev, (uint8_t*)&msg, sizeof(msg.hello), SYS_FOREVER_US);
		break;
	}
	case CLEAR_TO_MOVE: 
	{
		if (current_msg_len < sizeof(msg.clear_to_move))
		{
			return 0; // Message not complete. Keep waiting for more data.
		}
		LOG_DBG("UART \"CLEAR_TO_MOVE\" received");
		send_clear_to_move(config_client, BT_MESH_ADDR_ALL_NODES);
		break;
	}
	default:
	{
		LOG_ERR("Unknown message type %d", msg.header.type);
		return -EINVAL;
	}
	}
	current_msg_len = 0;

	return 0;
}

static void uart_callback(const struct device *dev, struct uart_event *event, void *user_data)
{
	struct bt_mesh_robot_config_cli *config_client = (struct bt_mesh_robot_config_cli *)user_data;
	switch (event->type)
	{
	case UART_TX_DONE:
	{
		LOG_DBG("UART_TX_DONE: Sent %d bytes", event->data.tx.len);
		break;
	}
	case UART_TX_ABORTED:
	{
		LOG_ERR("UART_TX_ABORTED");
		break;
	}
	case UART_RX_RDY:
	{
		LOG_DBG("UART_RX_RDY");
		uart_data_rx_rdy_handler(dev, event->data.rx, config_client);
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


int init_uart(const struct device *uart_dev, struct bt_mesh_robot_config_cli *config_client)
{
	int err = 0;

	if (!device_is_ready(uart_dev))
	{
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}
	LOG_DBG("UART device ready");

	err = uart_callback_set(uart_dev, uart_callback, (void*)config_client);
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
	uart_rx_enable(uart_dev, initial_rx_buf, CONFIG_MESH_UART_RX_BUF_SIZE, 1000);
	return 0;
}
