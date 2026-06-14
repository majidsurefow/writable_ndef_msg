/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Deterministic Aliro wire vectors for unverified builds and unit tests.
 * Sources: wave5-aliro.md synthetic fixtures (not Flipper — no Aliro dumps).
 */

#ifndef NFC_PROTOCOLS_ALIRO_VECTORS_H_
#define NFC_PROTOCOLS_ALIRO_VECTORS_H_

#include "aliro.h"

#define ALIRO_AUTH0_DATA_SIZE          81U
#define ALIRO_P256_SIGNATURE_SIZE      64U
#define ALIRO_AUTH1_DATA_SIZE          ALIRO_P256_SIGNATURE_SIZE
#define ALIRO_AUTH0_RSP_SIZE           (2U + ALIRO_NONCE_SIZE + ALIRO_P256_PUBLIC_KEY_SIZE)
#define ALIRO_AUTH1_RSP_SIZE           (2U + ALIRO_P256_SIGNATURE_SIZE)
#define ALIRO_EXCHANGE_DATA_SIZE       29U /* GCM nonce(12) + plaintext(1) + tag(16) */
#define ALIRO_EXCHANGE_RSP_SIZE        (2U + ALIRO_EXCHANGE_DATA_SIZE)

#define ALIRO_TEST_SELECT_NONCE_FILL   0x11U
#define ALIRO_TEST_READER_NONCE_FILL   0x22U
#define ALIRO_TEST_READER_PUBKEY_FILL  0x33U
#define ALIRO_TEST_CARD_NONCE_FILL     0xCCU
#define ALIRO_TEST_CARD_PUBKEY_FILL    0xDDU
#define ALIRO_TEST_READER_SIG_FILL     0x44U
#define ALIRO_TEST_CARD_SIG_FILL       0x55U
#define ALIRO_TEST_EXCHANGE_FILL       0x66U
#define ALIRO_TEST_EXCHANGE_RSP_FILL   0x77U

static inline void aliro_vectors_fill(uint8_t *dst, size_t len, uint8_t val)
{
	for (size_t i = 0U; i < len; i++) {
		dst[i] = val;
	}
}

static inline void aliro_vectors_build_select_rsp(uint8_t *out, size_t *out_len)
{
	out[0] = 0x00U;
	out[1] = 0x00U;
	aliro_vectors_fill(&out[2], ALIRO_NONCE_SIZE, ALIRO_TEST_SELECT_NONCE_FILL);
	*out_len = 2U + ALIRO_NONCE_SIZE;
}

static inline void aliro_vectors_build_auth0_tx(uint8_t *out, size_t *out_len)
{
	out[0] = ALIRO_CLA_PROP;
	out[1] = ALIRO_INS_AUTH0;
	out[2] = 0x00U;
	out[3] = 0x00U;
	out[4] = (uint8_t)ALIRO_AUTH0_DATA_SIZE;
	aliro_vectors_fill(&out[5], ALIRO_NONCE_SIZE, ALIRO_TEST_READER_NONCE_FILL);
	out[5U + ALIRO_NONCE_SIZE] = 0x04U;
	aliro_vectors_fill(&out[6U + ALIRO_NONCE_SIZE],
			   ALIRO_P256_PUBLIC_KEY_SIZE - 1U, ALIRO_TEST_READER_PUBKEY_FILL);
	*out_len = 5U + ALIRO_AUTH0_DATA_SIZE;
}

static inline void aliro_vectors_build_auth0_rsp(uint8_t *out, size_t *out_len)
{
	out[0] = 0x00U;
	out[1] = 0x00U;
	aliro_vectors_fill(&out[2], ALIRO_NONCE_SIZE, ALIRO_TEST_CARD_NONCE_FILL);
	out[2U + ALIRO_NONCE_SIZE] = 0x04U;
	aliro_vectors_fill(&out[3U + ALIRO_NONCE_SIZE],
			   ALIRO_P256_PUBLIC_KEY_SIZE - 1U, ALIRO_TEST_CARD_PUBKEY_FILL);
	*out_len = ALIRO_AUTH0_RSP_SIZE;
}

static inline void aliro_vectors_build_auth1_tx(uint8_t *out, size_t *out_len)
{
	out[0] = ALIRO_CLA_PROP;
	out[1] = ALIRO_INS_AUTH1;
	out[2] = 0x00U;
	out[3] = 0x00U;
	out[4] = (uint8_t)ALIRO_AUTH1_DATA_SIZE;
	aliro_vectors_fill(&out[5], ALIRO_AUTH1_DATA_SIZE, ALIRO_TEST_READER_SIG_FILL);
	*out_len = 5U + ALIRO_AUTH1_DATA_SIZE;
}

static inline void aliro_vectors_build_auth1_rsp(uint8_t *out, size_t *out_len)
{
	out[0] = 0x00U;
	out[1] = 0x00U;
	aliro_vectors_fill(&out[2], ALIRO_P256_SIGNATURE_SIZE, ALIRO_TEST_CARD_SIG_FILL);
	*out_len = ALIRO_AUTH1_RSP_SIZE;
}

static inline void aliro_vectors_build_exchange_tx(uint8_t *out, size_t *out_len)
{
	out[0] = ALIRO_CLA_PROP;
	out[1] = ALIRO_INS_EXCHANGE;
	out[2] = 0x00U;
	out[3] = 0x00U;
	out[4] = (uint8_t)ALIRO_EXCHANGE_DATA_SIZE;
	aliro_vectors_fill(&out[5], ALIRO_EXCHANGE_DATA_SIZE, ALIRO_TEST_EXCHANGE_FILL);
	*out_len = 5U + ALIRO_EXCHANGE_DATA_SIZE;
}

static inline void aliro_vectors_build_exchange_rsp(uint8_t *out, size_t *out_len)
{
	out[0] = 0x00U;
	out[1] = 0x00U;
	aliro_vectors_fill(&out[2], ALIRO_EXCHANGE_DATA_SIZE, ALIRO_TEST_EXCHANGE_RSP_FILL);
	*out_len = ALIRO_EXCHANGE_RSP_SIZE;
}

#endif /* NFC_PROTOCOLS_ALIRO_VECTORS_H_ */
