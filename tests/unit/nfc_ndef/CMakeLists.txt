# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common/CMakeLists.txt)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nfc_ndef_unit)

set(REPO_NFC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/nfc)

target_sources(app PRIVATE
  src/test_model.c
  ${REPO_NFC}/protocols/ndef/ndef.c
  ${NFC_TEST_COMMON_SOURCES}
)

if(CONFIG_NFC_NDEF_TEST_TIER_POLLER)
  target_sources(app PRIVATE
    src/test_poller.c
    ${REPO_NFC}/protocols/ndef/ndef_poller.c
  )
endif()

if(CONFIG_NFC_NDEF_TEST_TIER_LISTENER)
  target_sources(app PRIVATE
    src/test_listener_select.c
    src/test_listener_read.c
    src/test_listener_update.c
    src/test_listener_dispatch.c
    ${REPO_NFC}/protocols/ndef/ndef_listener.c
  )
endif()

target_include_directories(app PRIVATE
  ${REPO_NFC}
  ${REPO_NFC}/protocols/ndef
  ${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures/ndef
  ${NFC_TEST_COMMON_INCLUDES}
)
