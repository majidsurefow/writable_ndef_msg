# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common/CMakeLists.txt)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nfc_iso15693_3_unit)

set(REPO_NFC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/nfc)

target_sources(app PRIVATE
  src/test_model.c
  ${REPO_NFC}/protocols/iso15693_3/iso15693_3.c
  ${NFC_TEST_COMMON_SOURCES}
)

if(CONFIG_NFC_ISO15693_3_TEST_TIER_POLLER)
  target_sources(app PRIVATE
    src/test_poller.c
    ${REPO_NFC}/protocols/iso15693_3/iso15693_3_poller.c
    ${REPO_NFC}/protocols/iso15693_3/iso15693_3_poller_i.c
  )
endif()

target_include_directories(app PRIVATE
  ${REPO_NFC}
  ${REPO_NFC}/protocols/iso15693_3
  ${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures/iso15693_3
  ${NFC_TEST_COMMON_INCLUDES}
)
