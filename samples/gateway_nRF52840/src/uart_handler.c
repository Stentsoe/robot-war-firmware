#include <zephyr.h>
#include <zephyr/drivers/uart.h>

#include "./robot_movement_cli.h"
#include "uart_handler.h"

#define MODULE uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_UART_MODULE_LOG_LEVEL);

struct uart_thread_msg
{
	struct device *uart_dev;
	struct bt_mesh_robot_config_cli *config_client;
	union mesh_uart_msg msg;
};
K_MSGQ_DEFINE(uart_msg_queue, sizeof(struct uart_thread_msg), 4, 4);

K_MEM_SLAB_DEFINE_STATIC(mesh_uart_rx_slab, CONFIG_MESH_UART_RX_BUF_SIZE, CONFIG_MESH_UART_RX_BUF_COUNT, 4);

static int uart_data_rx_rdy_handler(const struct device *dev, struct uart_event_rx event_data, struct bt_mesh_robot_config_cli *config_client)
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

	struct uart_thread_msg thread_msg; // Prepare message for message queue
	thread_msg.uart_dev = dev;
	thread_msg.config_client = config_client;
	thread_msg.msg = msg;

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
		int err = k_msgq_put(&uart_msg_queue, &thread_msg, K_NO_WAIT);
		if (err)
		{
			LOG_ERR("Failed to enqueue message: Error %d", err);
		}
		break;
	}
	case CLEAR_TO_MOVE:
	{
		if (current_msg_len < sizeof(msg.clear_to_move))
		{
			return 0; // Message not complete. Keep waiting for more data.
		}
		LOG_DBG("UART \"CLEAR_TO_MOVE\" received");
		int err = k_msgq_put(&uart_msg_queue, &thread_msg, K_NO_WAIT);
		if (err)
		{
			LOG_ERR("Failed to enqueue message: Error %d", err);
		}
		break;
	}
	case SET_MOVEMENT_CONFIG:
	{
		if (current_msg_len < sizeof(msg.set_movement_config))
		{
			return 0; // Message not complete. Keep waiting for more data.
		}
		LOG_DBG("UART \"SET_MOVEMENT_CONFIG\" received");
		int err = k_msgq_put(&uart_msg_queue, &thread_msg, K_NO_WAIT);
		if (err)
		{
			LOG_ERR("Failed to enqueue message: Error %d", err);
		}
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

static K_SEM_DEFINE(uart_tx_sem, 1, 1);
static int mesh_uart_send(const struct device *dev, void *data, size_t len, k_timeout_t timeout)
{
	int err = k_sem_take(&uart_tx_sem, timeout);
	if (err)
	{
		LOG_ERR("Failed to take semaphore: Error %d", err);
		return err;
	}
	err = uart_tx(dev, data, len, SYS_FOREVER_US);
	if (err)
	{
		LOG_ERR("Failed to send data: Error %d", err);
		return err;
	}
	return 0;
}

static int mesh_uart_send_status(const struct device *dev, int status, k_timeout_t timeout)
{
	static struct mesh_uart_status_msg msg;
	msg.header.type = STATUS;
	msg.data.status = status;
	return mesh_uart_send(dev, &msg, sizeof(msg), timeout);
}

static int mesh_uart_send_hello(const struct device *dev, uint16_t echo, k_timeout_t timeout)
{
	static struct mesh_uart_hello_msg msg;
	msg.header.type = HELLO;
	msg.echo = echo;
	return mesh_uart_send(dev, &msg, sizeof(msg), timeout);
}

static void uart_callback(const struct device *dev, struct uart_event *event, void *user_data)
{
	struct bt_mesh_robot_config_cli *config_client = (struct bt_mesh_robot_config_cli *)user_data;
	switch (event->type)
	{
	case UART_TX_DONE:
	{
		LOG_DBG("UART_TX_DONE: Sent %d bytes", event->data.tx.len);
		k_sem_give(&uart_tx_sem);
		break;
	}
	case UART_TX_ABORTED:
	{
		LOG_ERR("UART_TX_ABORTED");
		k_sem_give(&uart_tx_sem);
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

	err = uart_callback_set(uart_dev, uart_callback, (void *)config_client);
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

static int uart_send_generic_ack(const struct device *uart_dev, int status, int32_t timeout)
{
	static struct mesh_uart_status_data msg;
	return uart_tx(uart_dev, &msg, sizeof(msg), timeout);
}

static void uart_thread_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("Starting UART thread");
	struct uart_thread_msg thread_msg;
	while (true)
	{
		int err = k_msgq_get(&uart_msg_queue, &thread_msg, K_FOREVER);
		if (err)
		{
			LOG_ERR("Failed to dequeue message: Error %d", err);
			continue;
		}
		switch (thread_msg.msg.header.type)
		{
		case HELLO:
		{
			mesh_uart_send_hello(thread_msg.uart_dev, thread_msg.msg.hello.echo, K_FOREVER);
			break;
		}
		case CLEAR_TO_MOVE:
		{
			err = send_clear_to_move(thread_msg.config_client, BT_MESH_ADDR_ALL_NODES);
			if (err)
			{
				LOG_ERR("Failed to send CLEAR_TO_MOVE: Error %d", err);
			}
			break;
		}
		case SET_MOVEMENT_CONFIG:
		{
			struct robot_movement_set_msg movement_config;
			movement_config.time = thread_msg.msg.set_movement_config.data.time;
			movement_config.angle = thread_msg.msg.set_movement_config.data.angle;
			err = configure_robot_movement(thread_msg.config_client, thread_msg.msg.set_movement_config.data.addr, movement_config);
			if (err)
			{
				LOG_ERR("Failed to set robot movement configuration: Error %d", err);
			}
			mesh_uart_send_status(thread_msg.uart_dev, err, K_FOREVER);
			break;
		}
		default:
		{
			LOG_ERR("Unknown message type %d", thread_msg.msg.header.type);
			break;
		}
		}
	}
}

K_THREAD_DEFINE(uart_thread, CONFIG_UART_THREAD_STACK_SIZE, uart_thread_fn, NULL, NULL, NULL, CONFIG_UART_THREAD_PRIORITY, 0, 0);