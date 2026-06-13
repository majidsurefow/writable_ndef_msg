/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Captures nfc_transport_send_response() for listener unit tests (cookbook §14.5).
 */

#ifndef TESTS_COMMON_NFC_RESPONSE_SPY_H_
#define TESTS_COMMON_NFC_RESPONSE_SPY_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void nfc_response_spy_reset(void);
int nfc_response_spy_last(const uint8_t **buf, size_t *len);
size_t nfc_response_spy_call_count(void);

#ifdef __cplusplus
}
#endif

#endif /* TESTS_COMMON_NFC_RESPONSE_SPY_H_ */
