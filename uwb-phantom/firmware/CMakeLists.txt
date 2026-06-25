# CMakeLists.txt — ESP-IDF project manifest for UWB-PHANTOM firmware.
#
# Author: jayis1
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This is the minimal CMake file ESP-IDF v5.x expects in the project
# root.  The companion Makefile wraps `idf.py` for convenience.
#
# Build:
#   idf.py set-target esp32s3
#   idf.py build flash monitor

cmake_minimum_required(VERSION 3.16)

# Include the ESP-IDF build system.  This pulls in the component
# definitions and the toolchain.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(uwb-phantom
    VERSION 1.0.0
    DESCRIPTION "UWB-PHANTOM — Ultra-Wideband Ranging & Digital-Key Manipulation Tool"
    LANGUAGES C)

# Component definitions are in main/CMakeLists.txt (created by
# `idf.py create-project`).  When this file is in the project root,
# the `main` component is auto-discovered.