#include <zephyr.h>
#include <zephyr/drivers/uart.h>

#include "uart_handler.h"

#define MODULE uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

K_MEM_SLAB_DEFINE_STATIC(mesh_uart_rx_slab, CONFIG_MESH_UART_RX_BUF_SIZE, CONFIG_MESH_UART_RX_BUF_COUNT, 4);

static int uart_data_rx_rdy_handler(const struct device* dev, struct uart_event_rx event_data)
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
		uart_tx(dev, (uint8_t*)&msg, sizeof(msg.hello), SYS_FOREVER_US);
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
		uart_data_rx_rdy_handler(dev, event->data.rx);
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


int init_uart(const struct device *uart_dev)
{
	int err = 0;

	if (!device_is_ready(uart_dev))
	{
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}
	LOG_DBG("UART device ready");

	err = uart_callback_set(uart_dev, uart_callback, NULL);
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
