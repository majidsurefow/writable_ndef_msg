# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common/CMakeLists.txt)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nfc_reader_unit)

set(REPO_NFC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/nfc)
set(REPO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src)

target_sources(app PRIVATE
  src/test_store_roundtrip.c
  src/test_verify_diff.c
  src/test_virtual_loopback.c
  src/test_poller_registry.c
  ${REPO_NFC}/reader/nfc_reader_poller_registry.c
  ${REPO_NFC}/protocols/ndef/ndef.c
  ${REPO_NFC}/protocols/ndef/ndef_listener.c
  ${REPO_NFC}/protocols/ndef/ndef_poller.c
  ${REPO_NFC}/protocols/ultralight/ultralight.c
  ${REPO_NFC}/protocols/ultralight/ultralight_listener.c
  ${REPO_NFC}/protocols/ultralight/ultralight_poller.c
  ${REPO_NFC}/store/nfc_store.c
  ${REPO_NFC}/applets/nfc_applet_policy.c
  ${REPO_NFC}/applets/nfc_applet_verify_compare.c
  ${NFC_TEST_COMMON_SOURCES}
  ${NFC_TEST_LOOPBACK_SOURCES}
)

target_include_directories(app PRIVATE
  ${REPO_NFC}
  ${REPO_NFC}/protocols/ndef
  ${REPO_NFC}/protocols/ultralight
  ${REPO_NFC}/applets
  ${REPO_SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures/store
  ${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures/ultralight
  ${NFC_TEST_COMMON_INCLUDES}
)
