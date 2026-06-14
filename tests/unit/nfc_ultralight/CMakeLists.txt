# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common/CMakeLists.txt)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nfc_ultralight_unit)

set(REPO_NFC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/nfc)

target_sources(app PRIVATE
  src/test_model.c
  ${REPO_NFC}/protocols/ultralight/ultralight.c
  ${NFC_TEST_COMMON_SOURCES}
)

if(CONFIG_NFC_ULTRALIGHT_TEST_TIER_POLLER)
  target_sources(app PRIVATE
    src/test_poller.c
    ${REPO_NFC}/protocols/ultralight/ultralight_poller.c
    ${REPO_NFC}/protocols/ultralight/ultralight_poller_i.c
    ${REPO_NFC}/protocols/ultralight/ultralight_des.c
    ${REPO_NFC}/protocols/ultralight/ultralight_3des.c
  )
  target_compile_definitions(app PRIVATE NFC_SESSION_MOCK_MAX_TX=128)
endif()

target_include_directories(app PRIVATE
  ${REPO_NFC}
  ${REPO_NFC}/protocols/ultralight
  ${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures/ultralight
  ${NFC_TEST_COMMON_INCLUDES}
)
