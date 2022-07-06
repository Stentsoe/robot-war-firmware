#pragma once
#include <zephyr/device.h>
#include "../../common/nRF9160dk_uart_interface/messages.h"

int init_uart(const struct device *dev);

