/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Single-DES ECB (mbed TLS subset) for Ultralight C 3DES-2KEY auth.
 */

#ifndef NFC_PROTOCOLS_ULTRALIGHT_ULTRALIGHT_DES_H_
#define NFC_PROTOCOLS_ULTRALIGHT_ULTRALIGHT_DES_H_

#include <stdint.h>

#define ULTRALIGHT_DES_KEY_SIZE 8U
#define ULTRALIGHT_DES_BLOCK_SIZE 8U

typedef struct {
	uint32_t sk[32];
} ultralight_des_ctx_t;

void ultralight_des_setkey_enc(ultralight_des_ctx_t *ctx, const uint8_t key[ULTRALIGHT_DES_KEY_SIZE]);
void ultralight_des_setkey_dec(ultralight_des_ctx_t *ctx, const uint8_t key[ULTRALIGHT_DES_KEY_SIZE]);
void ultralight_des_crypt_ecb(ultralight_des_ctx_t *ctx, const uint8_t input[ULTRALIGHT_DES_BLOCK_SIZE],
			      uint8_t output[ULTRALIGHT_DES_BLOCK_SIZE]);

#endif /* NFC_PROTOCOLS_ULTRALIGHT_ULTRALIGHT_DES_H_ */
