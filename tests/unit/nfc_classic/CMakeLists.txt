# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common/CMakeLists.txt)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nfc_classic_unit)

set(REPO_NFC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/nfc)

target_sources(app PRIVATE
  src/test_model.c
  ${REPO_NFC}/protocols/classic/classic.c
  ${REPO_NFC}/protocols/classic/classic_i.c
  ${REPO_NFC}/protocols/classic/crypto1.c
  ${REPO_NFC}/protocols/classic/iso14443_crc.c
  ${NFC_TEST_COMMON_SOURCES}
)

if(CONFIG_NFC_CLASSIC_TEST_TIER_POLLER)
  target_sources(app PRIVATE
    src/test_poller.c
    ${REPO_NFC}/protocols/classic/classic_poller.c
  )
  target_compile_definitions(app PRIVATE NFC_SESSION_MOCK_MAX_TX=128)
endif()

if(CONFIG_NFC_CLASSIC_TEST_VARIANT_1K_7B OR CONFIG_NFC_CLASSIC_TEST_VARIANT_MINI_4B)
  target_sources(app PRIVATE
    src/test_poller_variants_small.c
    ${REPO_NFC}/protocols/classic/classic_poller.c
  )
  target_compile_definitions(app PRIVATE NFC_SESSION_MOCK_MAX_TX=128)
endif()

if(CONFIG_NFC_CLASSIC_TEST_VARIANT_4K_4B)
  target_sources(app PRIVATE
    src/test_poller_4k.c
    ${REPO_NFC}/protocols/classic/classic_poller.c
  )
  target_compile_definitions(app PRIVATE NFC_SESSION_MOCK_MAX_TX=384)
endif()

if(CONFIG_NFC_CLASSIC_TEST_TIER_LISTENER)
  target_sources(app PRIVATE
    src/test_listener.c
    ${REPO_NFC}/protocols/classic/classic_poller.c
    ${REPO_NFC}/protocols/classic/classic_listener.c
  )
endif()

target_include_directories(app PRIVATE
  ${REPO_NFC}
  ${REPO_NFC}/protocols/classic
  ${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures/classic
  ${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures/store
  ${NFC_TEST_COMMON_INCLUDES}
)
