#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
	ui_module_event.c
	modem_module_event.c
	cloud_module_event.c
	robot_module_event.c
	mesh_module_event.c
)
