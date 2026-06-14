# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0
#
# Shared protocol unit-test mocks (cookbook §14.5).
# Unit apps add these sources via NFC_TEST_COMMON_DIR.

set(NFC_TEST_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "NFC test common mocks")

set(NFC_TEST_COMMON_SOURCES
  ${NFC_TEST_COMMON_DIR}/src/nfc_session_mock.c
  ${NFC_TEST_COMMON_DIR}/src/nfc_response_spy.c
  ${NFC_TEST_COMMON_DIR}/src/nfc_test_apdu.c
  ${NFC_TEST_COMMON_DIR}/src/nfc_virtual_rf.c
  ${NFC_TEST_COMMON_DIR}/src/nfc_work_spy.c
  ${NFC_TEST_COMMON_DIR}/src/nfc_aliro_crypto_shim.c
)

set(NFC_TEST_LOOPBACK_SOURCES
  ${NFC_TEST_COMMON_DIR}/src/nfc_virtual_loopback.c
)

set(NFC_TEST_COMMON_INCLUDES
  ${NFC_TEST_COMMON_DIR}/include
)
