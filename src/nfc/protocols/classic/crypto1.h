/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Crypto1 — port from flipperzero/lib/nfc/helpers/crypto1.h
 */

#ifndef NFC_PROTOCOLS_CLASSIC_CRYPTO1_H_
#define NFC_PROTOCOLS_CLASSIC_CRYPTO1_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct crypto1 {
	uint32_t odd;
	uint32_t even;
};

void crypto1_reset(struct crypto1 *crypto1);
void crypto1_init(struct crypto1 *crypto1, uint64_t key);
uint8_t crypto1_bit(struct crypto1 *crypto1, uint8_t in, int is_encrypted);
uint8_t crypto1_byte(struct crypto1 *crypto1, uint8_t in, int is_encrypted);
uint32_t crypto1_word(struct crypto1 *crypto1, uint32_t in, int is_encrypted);
uint32_t crypto1_prng_successor(uint32_t x, uint32_t n);
void crypto1_decrypt(struct crypto1 *crypto, const uint8_t *in, size_t in_len, uint8_t *out);
void crypto1_encrypt(struct crypto1 *crypto, const uint8_t *keystream, const uint8_t *in,
		     size_t in_len, uint8_t *out);
void crypto1_encrypt_reader_nonce(struct crypto1 *crypto, uint64_t key, uint32_t cuid,
				  uint8_t *nt, const uint8_t *nr, uint8_t *out);
uint32_t classic_cuid_from_uid(const uint8_t *uid, uint8_t uid_len);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_CLASSIC_CRYPTO1_H_ */
