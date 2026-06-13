/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nfc_test_apdu.h"

#include <errno.h>
#include <string.h>

static void apdu_zero(nfc_apdu_t *out)
{
	(void)memset(out, 0, sizeof(*out));
}

int nfc_test_apdu_from_bytes(const uint8_t *apdu, size_t apdu_len, uint8_t *storage,
			     size_t storage_cap, nfc_apdu_t *out)
{
	if ((apdu == NULL) || (storage == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	apdu_zero(out);

	if (apdu_len < 4U) {
		return -EINVAL;
	}

	out->cla = apdu[0];
	out->ins = apdu[1];
	out->p1 = apdu[2];
	out->p2 = apdu[3];

	size_t idx = 4U;

	if (idx >= apdu_len) {
		return 0;
	}

	/* ISO 7816-4 case 2: CLA INS P1 P2 Le (5 bytes, no Lc). */
	if (apdu_len == 5U) {
		out->has_le = true;
		out->le = apdu[idx];
		return 0;
	}

	{
		uint8_t lc = apdu[idx];

		idx++;

		if ((size_t)lc > (apdu_len - idx)) {
			return -EINVAL;
		}

		if (lc > 0U) {
			if ((size_t)lc > storage_cap) {
				return -EINVAL;
			}

			(void)memcpy(storage, &apdu[idx], lc);
			out->data = storage;
			out->lc = lc;
			idx += lc;
		}
	}

	if (idx < apdu_len) {
		out->has_le = true;
		out->le = apdu[idx];
	}

	return 0;
}
