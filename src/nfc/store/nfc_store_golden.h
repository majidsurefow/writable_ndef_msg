/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Compiled-in Tier E .card golden lookup (tests/fixtures/store).
 */

#ifndef NFC_STORE_NFC_STORE_GOLDEN_H_
#define NFC_STORE_NFC_STORE_GOLDEN_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int nfc_store_golden_lookup(const char *name, const uint8_t **blob_out, size_t *len_out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_STORE_NFC_STORE_GOLDEN_H_ */
