/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/slix/slix_poller_i.h"

#include "protocols/iso15693_3/iso15693_3_poller.h"
#include "protocols/iso15693_3/iso15693_3_poller_i.h"
#include "protocols/slix/slix.h"

size_t slix_poller_prepare_request(const uint8_t uid[ISO15693_UID_SIZE], uint8_t command,
				   bool skip_uid, uint8_t *tx, size_t tx_max)
{
	size_t offset = 0U;
	uint8_t flags = ISO15693_CMD_FLAGS;

	if ((uid == NULL) || (tx == NULL)) {
		return 0U;
	}

	if (!skip_uid) {
		flags = ISO15693_ADDR_FLAGS;
	}

	if (tx_max < 3U) {
		return 0U;
	}

	tx[offset++] = flags;
	tx[offset++] = command;
	tx[offset++] = SLIX_NXP_MFG_CODE;

	if (!skip_uid) {
		size_t uid_len = 0U;

		if ((offset + ISO15693_UID_SIZE) > tx_max) {
			return 0U;
		}

		iso15693_3_append_uid_reversed(uid, &tx[offset], &uid_len);
		offset += uid_len;
	}

	return offset;
}
