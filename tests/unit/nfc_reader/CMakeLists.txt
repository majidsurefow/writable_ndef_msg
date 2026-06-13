# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0
#
# Tier E applet tests (store / clone / verify) — scaffold at Gate 2.
# Normative: docs/nfc/NFC_PROTOCOLS_COOKBOOK.md §14.6a, §14.11

cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common/CMakeLists.txt)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nfc_reader_unit)

# Placeholder until store + reader orchestration tests land (Gate 2+).
target_sources(app PRIVATE
  src/test_placeholder.c
  ${NFC_TEST_COMMON_SOURCES}
)

target_include_directories(app PRIVATE
  ${NFC_TEST_COMMON_INCLUDES}
)
