# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

set(EXTRA_DTC_OVERLAY_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../boards/overlays/pn7160_unit_test.overlay
  CACHE STRING "" FORCE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pn7160_tml_unit)

target_sources(app PRIVATE src/main.c)
