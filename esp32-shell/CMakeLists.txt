# ESP32 Shell - Main CMakeLists.txt
# ============================================================================
# This is the top-level CMakeLists.txt for the ESP32 shell project.
# It uses the ESP-IDF build system which PlatformIO invokes.
# ============================================================================

cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name and version
project(esp32_shell VERSION 1.0.0)
