/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 TML framing validation — port of NXP tml_Rx bounds check.
 */

#include <nfc/pn7160_tml.h>

#include <errno.h>

int pn7160_tml_frame_len_validate_mode(const uint8_t *hdr, size_t max_len, bool dwl_mode)
{
	size_t frame_len;
	size_t hdr_read_len = pn7160_tml_hdr_read_len_get(dwl_mode);

	if (hdr == NULL || max_len < hdr_read_len) {
		return -EINVAL;
	}

	frame_len = pn7160_tml_frame_len_get_mode(hdr, dwl_mode);
	if (frame_len > max_len) {
		return -EINVAL;
	}

	return 0;
}

int pn7160_tml_frame_len_validate(const uint8_t hdr[PN7160_TML_NCI_HDR_LEN], size_t max_len)
{
	return pn7160_tml_frame_len_validate_mode(hdr, max_len, false);
}
