# SATA Phantom — ESP32-S3 CMake Build Configuration
#
# Author: jayis1
# Copyright © 2025 jayis1. All rights reserved.

cmake_minimum_required(VERSION 3.16)

# Set the project name and default components
set(EXTRA_COMPONENT_DIRS drivers)

# Include the ESP-IDF project configuration
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(sata-phantom-esp32)
