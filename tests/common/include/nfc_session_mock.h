/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Scripted nfc_reader_session_transceive() for protocol unit tests (cookbook §14.5).
 */

#ifndef TESTS_COMMON_NFC_SESSION_MOCK_H_
#define TESTS_COMMON_NFC_SESSION_MOCK_H_

#include <stddef.h>
#include <stdint.h>

#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NFC_SESSION_MOCK_MAX_TX
#define NFC_SESSION_MOCK_MAX_TX 32U
#endif

typedef struct {
	const uint8_t *rx;
	size_t rx_len;
	int err;
} nfc_session_mock_step_t;

void nfc_session_mock_reset(void);
void nfc_session_mock_load(const nfc_session_mock_step_t *steps, size_t count);
void nfc_session_mock_set_active(bool active);

size_t nfc_session_mock_tx_count(void);
int nfc_session_mock_get_tx(size_t index, const uint8_t **tx, size_t *tx_len);

/** @brief Times the test stub for nfc_reader_session_end() was invoked. */
uint32_t nfc_session_mock_session_end_count(void);

#ifdef __cplusplus
}
#endif

#endif /* TESTS_COMMON_NFC_SESSION_MOCK_H_ */
