#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mesh_bot)

# Include application application event headers
zephyr_library_include_directories(src/events)

# NORDIC SDK APP START
target_sources(app PRIVATE 
    src/main.c
    src/model_handler.c
    src/uart_handler.c
    src/robot_movement_cli.c
)

include_directories(
    src
    ../common/mesh_model_defines
)
# NORDIC SDK APP END
