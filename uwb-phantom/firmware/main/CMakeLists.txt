# main/CMakeLists.txt — component manifest for UWB-PHANTOM firmware.
#
# Author: jayis1
# SPDX-License-Identifier: GPL-3.0-or-later
#
# ESP-IDF expects the application sources to live in a "main" component.
# In this reference layout the sources sit next to the Makefile (one
# directory up).  We register them explicitly so the project builds
# without restructuring.

idf_component_register(
    SRCS
        "main.c"
        "dw3110.c"
        "uwb_sniffer.c"
        "relay.c"
        "sts_audit.c"
        "display.c"
        "ble_comms.c"
        "fuel_gauge.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
        esp_timer
        esp_log
        nvs_flash
        bt
)