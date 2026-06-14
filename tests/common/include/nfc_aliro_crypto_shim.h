/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Deterministic Aliro crypto shim for unit tests when protocol is unverified.
 */

#ifndef TESTS_COMMON_NFC_ALIRO_CRYPTO_SHIM_H_
#define TESTS_COMMON_NFC_ALIRO_CRYPTO_SHIM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void nfc_aliro_crypto_shim_build_auth0_rsp(uint8_t *out, size_t *out_len);
void nfc_aliro_crypto_shim_build_auth1_rsp(uint8_t *out, size_t *out_len);
bool nfc_aliro_crypto_shim_verify_reader_sig(const uint8_t *sig, size_t sig_len);

#ifdef __cplusplus
}
#endif

#endif /* TESTS_COMMON_NFC_ALIRO_CRYPTO_SHIM_H_ */
