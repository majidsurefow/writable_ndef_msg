/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * MIFARE Ultralight C 3DES-2KEY CBC — Flipper mf_ultralight.c parity.
 */

#ifndef NFC_PROTOCOLS_ULTRALIGHT_3DES_H_
#define NFC_PROTOCOLS_ULTRALIGHT_3DES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ultralight.h"

#ifdef __cplusplus
extern "C" {
#endif

void ultralight_3des_shift_data(uint8_t data[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE]);

bool ultralight_3des_key_valid(const ultralight_data_t *data);

const uint8_t *ultralight_3des_get_key(const ultralight_data_t *data);

void ultralight_3des_encrypt(const uint8_t key[ULTRALIGHT_C_AUTH_DES_KEY_SIZE], const uint8_t *iv,
			     const uint8_t *input, size_t length, uint8_t *out);

void ultralight_3des_decrypt(const uint8_t key[ULTRALIGHT_C_AUTH_DES_KEY_SIZE], const uint8_t *iv,
			     const uint8_t *input, size_t length, uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ULTRALIGHT_3DES_H_ */
