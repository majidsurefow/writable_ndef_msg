/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nfc_aliro_crypto_shim.h"

#include "aliro_vectors.h"

#include <string.h>

void nfc_aliro_crypto_shim_build_auth0_rsp(uint8_t *out, size_t *out_len)
{
	aliro_vectors_build_auth0_rsp(out, out_len);
}

void nfc_aliro_crypto_shim_build_auth1_rsp(uint8_t *out, size_t *out_len)
{
	aliro_vectors_build_auth1_rsp(out, out_len);
}

bool nfc_aliro_crypto_shim_verify_reader_sig(const uint8_t *sig, size_t sig_len)
{
	uint8_t expected[ALIRO_AUTH1_DATA_SIZE];

	if ((sig == NULL) || (sig_len < ALIRO_AUTH1_DATA_SIZE)) {
		return false;
	}

	aliro_vectors_fill(expected, ALIRO_AUTH1_DATA_SIZE, ALIRO_TEST_READER_SIG_FILL);
	return memcmp(sig, expected, ALIRO_AUTH1_DATA_SIZE) == 0;
}
