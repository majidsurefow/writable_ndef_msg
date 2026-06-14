/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Crypto1 — port from flipperzero/lib/nfc/helpers/crypto1.c
 */

#include "protocols/classic/crypto1.h"

#include <string.h>

#define LF_POLY_ODD  0x29CE5CU
#define LF_POLY_EVEN 0x870804U

#define SWAPENDIAN(x) \
	((x) = (((x) >> 8) & 0x00FF00FFU) | (((x) & 0x00FF00FFU) << 8), \
	 (x) = ((x) >> 16) | ((x) << 16))

#define BEBIT(x, n) (((x) >> ((n) ^ 24)) & 1U)

static uint8_t nfc_util_even_parity32(uint32_t data)
{
	data ^= data >> 16;
	data ^= data >> 8;

	return (uint8_t)((0x6996U >> (data & 0x0FU)) & 1U);
}

void crypto1_reset(struct crypto1 *crypto1)
{
	if (crypto1 == NULL) {
		return;
	}

	crypto1->odd = 0U;
	crypto1->even = 0U;
}

void crypto1_init(struct crypto1 *crypto1, uint64_t key)
{
	if (crypto1 == NULL) {
		return;
	}

	crypto1->even = 0U;
	crypto1->odd = 0U;

	for (int8_t i = 47; i > 0; i -= 2) {
		crypto1->odd = (crypto1->odd << 1) |
			       (uint32_t)((key >> ((uint32_t)(i - 1) ^ 7U)) & 1ULL);
		crypto1->even = (crypto1->even << 1) |
				(uint32_t)((key >> ((uint32_t)i ^ 7U)) & 1ULL);
	}
}

static uint32_t crypto1_filter(uint32_t in)
{
	uint32_t out = 0U;

	out = (0xF22C0U >> (in & 0xFU)) & 16U;
	out |= (0x6C9C0U >> ((in >> 4) & 0xFU)) & 8U;
	out |= (0x3C8B0U >> ((in >> 8) & 0xFU)) & 4U;
	out |= (0x1E458U >> ((in >> 12) & 0xFU)) & 2U;
	out |= (0x0D938U >> ((in >> 16) & 0xFU)) & 1U;

	return (0xEC57E80AU >> out) & 1U;
}

uint8_t crypto1_bit(struct crypto1 *crypto1, uint8_t in, int is_encrypted)
{
	uint8_t out = (uint8_t)crypto1_filter(crypto1->odd);
	uint32_t feed = (uint32_t)(out & (!!is_encrypted));

	feed ^= (uint32_t)(!!in);
	feed ^= LF_POLY_ODD & crypto1->odd;
	feed ^= LF_POLY_EVEN & crypto1->even;
	crypto1->even = (crypto1->even << 1) | (uint32_t)nfc_util_even_parity32(feed);

	{
		uint32_t t = crypto1->odd;

		crypto1->odd = crypto1->even;
		crypto1->even = t;
	}

	return out;
}

uint8_t crypto1_byte(struct crypto1 *crypto1, uint8_t in, int is_encrypted)
{
	uint8_t out = 0U;

	for (uint8_t i = 0U; i < 8U; i++) {
		out |= (uint8_t)(crypto1_bit(crypto1, (uint8_t)((in >> i) & 1U), is_encrypted) << i);
	}

	return out;
}

uint32_t crypto1_word(struct crypto1 *crypto1, uint32_t in, int is_encrypted)
{
	uint32_t out = 0U;

	for (uint8_t i = 0U; i < 32U; i++) {
		out |= (uint32_t)crypto1_bit(crypto1, (uint8_t)BEBIT(in, i), is_encrypted)
		       << (24U ^ i);
	}

	return out;
}

uint32_t crypto1_prng_successor(uint32_t x, uint32_t n)
{
	SWAPENDIAN(x);
	while (n-- > 0U) {
		x = (x >> 1) |
		    (((x >> 16) ^ (x >> 18) ^ (x >> 19) ^ (x >> 21)) << 31);
	}

	return x;
}

void crypto1_decrypt(struct crypto1 *crypto, const uint8_t *in, size_t in_len, uint8_t *out)
{
	if ((crypto == NULL) || (in == NULL) || (out == NULL)) {
		return;
	}

	for (size_t i = 0U; i < in_len; i++) {
		out[i] = (uint8_t)(crypto1_byte(crypto, 0U, 0) ^ in[i]);
	}
}

void crypto1_encrypt(struct crypto1 *crypto, const uint8_t *keystream, const uint8_t *in,
		     size_t in_len, uint8_t *out)
{
	if ((crypto == NULL) || (in == NULL) || (out == NULL)) {
		return;
	}

	for (size_t i = 0U; i < in_len; i++) {
		uint8_t ks = (keystream != NULL) ? keystream[i] : 0U;

		out[i] = (uint8_t)(crypto1_byte(crypto, ks, 0) ^ in[i]);
	}
}

void crypto1_encrypt_reader_nonce(struct crypto1 *crypto, uint64_t key, uint32_t cuid,
				  uint8_t *nt, const uint8_t *nr, uint8_t *out)
{
	uint32_t nt_num;

	if ((crypto == NULL) || (nt == NULL) || (nr == NULL) || (out == NULL)) {
		return;
	}

	nt_num = ((uint32_t)nt[0] << 24) | ((uint32_t)nt[1] << 16) | ((uint32_t)nt[2] << 8) |
		 (uint32_t)nt[3];

	crypto1_init(crypto, key);
	crypto1_word(crypto, nt_num ^ cuid, 0);

	for (size_t i = 0U; i < 4U; i++) {
		out[i] = (uint8_t)(crypto1_byte(crypto, nr[i], 0) ^ nr[i]);
	}

	nt_num = crypto1_prng_successor(nt_num, 32U);
	for (size_t i = 4U; i < 8U; i++) {
		nt_num = crypto1_prng_successor(nt_num, 8U);
		out[i] = (uint8_t)(crypto1_byte(crypto, 0U, 0) ^ (uint8_t)nt_num);
	}
}

uint32_t classic_cuid_from_uid(const uint8_t *uid, uint8_t uid_len)
{
	const uint8_t *cuid_start = uid;

	if (uid_len == 7U) {
		cuid_start = &uid[3];
	}

	return ((uint32_t)cuid_start[0] << 24) | ((uint32_t)cuid_start[1] << 16) |
	       ((uint32_t)cuid_start[2] << 8) | (uint32_t)cuid_start[3];
}
