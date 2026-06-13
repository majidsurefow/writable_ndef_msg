/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 TML framing validation — port of NXP tml_Rx bounds check.
 */

#include <nfc/pn7160_tml.h>

#include <errno.h>

int pn7160_tml_frame_len_validate(const uint8_t hdr[PN7160_TML_NCI_HDR_LEN], size_t max_len)
{
	size_t frame_len;

	if (max_len == 0U) {
		return -EINVAL;
	}

	frame_len = pn7160_tml_frame_len_get(hdr);
	if (frame_len > max_len) {
		return -EINVAL;
	}

	return 0;
}
