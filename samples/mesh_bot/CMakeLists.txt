#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dev_options.conf")
    message(WARNING "Using dev_options.conf. Ensure any critical settings are moved to prj.conf before committing to source control.")
    list(APPEND CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dev_options.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mesh_bot)

# Include application application event headers
zephyr_library_include_directories(src/events)

# NORDIC SDK APP START
target_sources(app PRIVATE 
    src/main.c
    src/model_handler.c
)

include_directories(
    src
    ../common/mesh_model_defines
)

add_subdirectory(src/modules)
add_subdirectory(src/events)
add_subdirectory(drivers)
# NORDIC SDK APP END
