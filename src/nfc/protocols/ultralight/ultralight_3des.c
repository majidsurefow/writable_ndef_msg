/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight.c
 */

#include "ultralight_3des.h"

#include "ultralight_des.h"

#include <string.h>

void ultralight_3des_shift_data(uint8_t data[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE])
{
	uint8_t buf = data[0];

	for (size_t i = 1U; i < ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE; i++) {
		data[i - 1U] = data[i];
	}

	data[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE - 1U] = buf;
}

bool ultralight_3des_key_valid(const ultralight_data_t *data)
{
	if ((data == NULL) || (data->type != UL_TYPE_MFUL_C)) {
		return false;
	}

	return data->pages_read != (uint16_t)(data->pages_total - 4U);
}

const uint8_t *ultralight_3des_get_key(const ultralight_data_t *data)
{
	return data->pages[ULTRALIGHT_C_KEY_PAGE];
}

static void tdes2_block(const uint8_t key[ULTRALIGHT_C_AUTH_DES_KEY_SIZE], const uint8_t *in,
			uint8_t *out, bool decrypt)
{
	ultralight_des_ctx_t ctx;
	uint8_t tmp[ULTRALIGHT_DES_BLOCK_SIZE];

	if (!decrypt) {
		ultralight_des_setkey_enc(&ctx, key);
		ultralight_des_crypt_ecb(&ctx, in, tmp);
		ultralight_des_setkey_dec(&ctx, key + ULTRALIGHT_DES_KEY_SIZE);
		ultralight_des_crypt_ecb(&ctx, tmp, tmp);
		ultralight_des_setkey_enc(&ctx, key);
		ultralight_des_crypt_ecb(&ctx, tmp, out);
		return;
	}

	ultralight_des_setkey_dec(&ctx, key);
	ultralight_des_crypt_ecb(&ctx, in, tmp);
	ultralight_des_setkey_enc(&ctx, key + ULTRALIGHT_DES_KEY_SIZE);
	ultralight_des_crypt_ecb(&ctx, tmp, tmp);
	ultralight_des_setkey_dec(&ctx, key);
	ultralight_des_crypt_ecb(&ctx, tmp, out);
}

void ultralight_3des_encrypt(const uint8_t key[ULTRALIGHT_C_AUTH_DES_KEY_SIZE], const uint8_t *iv,
			     const uint8_t *input, size_t length, uint8_t *out)
{
	uint8_t iv_mut[ULTRALIGHT_C_AUTH_IV_BLOCK_SIZE];
	size_t off;

	(void)memcpy(iv_mut, iv, sizeof(iv_mut));

	for (off = 0U; off < length; off += ULTRALIGHT_DES_BLOCK_SIZE) {
		uint8_t block[ULTRALIGHT_DES_BLOCK_SIZE];
		size_t i;

		for (i = 0U; i < ULTRALIGHT_DES_BLOCK_SIZE; i++) {
			block[i] = (uint8_t)(input[off + i] ^ iv_mut[i]);
		}

		tdes2_block(key, block, block, false);
		(void)memcpy(out + off, block, sizeof(block));
		(void)memcpy(iv_mut, block, sizeof(block));
	}
}

void ultralight_3des_decrypt(const uint8_t key[ULTRALIGHT_C_AUTH_DES_KEY_SIZE], const uint8_t *iv,
			     const uint8_t *input, size_t length, uint8_t *out)
{
	uint8_t iv_mut[ULTRALIGHT_C_AUTH_IV_BLOCK_SIZE];
	size_t off;

	(void)memcpy(iv_mut, iv, sizeof(iv_mut));

	for (off = 0U; off < length; off += ULTRALIGHT_DES_BLOCK_SIZE) {
		uint8_t block[ULTRALIGHT_DES_BLOCK_SIZE];
		uint8_t plain[ULTRALIGHT_DES_BLOCK_SIZE];
		size_t i;

		(void)memcpy(block, input + off, sizeof(block));
		tdes2_block(key, block, plain, true);

		for (i = 0U; i < ULTRALIGHT_DES_BLOCK_SIZE; i++) {
			out[off + i] = (uint8_t)(plain[i] ^ iv_mut[i]);
		}

		(void)memcpy(iv_mut, block, sizeof(block));
	}
}
