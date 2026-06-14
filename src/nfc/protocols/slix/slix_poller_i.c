/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/slix/slix_poller_i.h"

#include "protocols/iso15693_3/iso15693_3_poller.h"
#include "protocols/iso15693_3/iso15693_3_poller_i.h"
#include "protocols/slix/slix.h"
#include "reader/nfc_reader_engine.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

static uint32_t slix_poller_read_be32(const uint8_t *bytes)
{
	return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) |
	       (uint32_t)bytes[3];
}

size_t slix_poller_prepare_request(const uint8_t uid[ISO15693_UID_SIZE], uint8_t command,
				   bool skip_uid, uint8_t *tx, size_t tx_max)
{
	size_t offset = 0U;
	uint8_t flags = ISO15693_CMD_FLAGS;

	if (tx == NULL) {
		return 0U;
	}

	if (!skip_uid && (uid == NULL)) {
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

static int slix_poller_transceive(nfc_reader_session_t *session, uint8_t *tx, size_t tx_payload_len,
				  uint8_t *rx, size_t *rx_len)
{
	size_t tx_len;
	size_t rx_raw_len = 0U;
	int ret;

	tx_len = iso15693_3_crc_append(tx, 32U, tx_payload_len);
	if (tx_len == 0U) {
		return -EINVAL;
	}

	ret = nfc_reader_session_transceive(session, tx, tx_len, rx, NFC_TRANSPORT_MAX_RESPONSE_LEN,
					  &rx_raw_len, K_MSEC(5000));
	if (ret != 0) {
		return ret;
	}

	if (!iso15693_3_crc_check(rx, rx_raw_len)) {
		return -EIO;
	}

	*rx_len = iso15693_3_crc_trim(rx, rx_raw_len);
	return 0;
}

int slix_poller_get_random_number(nfc_reader_session_t *session, uint16_t *random_out)
{
	uint8_t tx[32];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	size_t tx_len;
	int ret;

	if ((session == NULL) || (random_out == NULL)) {
		return -EINVAL;
	}

	tx_len = slix_poller_prepare_request(NULL, SLIX_CMD_GET_RANDOM, true, tx, sizeof(tx));
	if (tx_len == 0U) {
		return -EINVAL;
	}

	ret = slix_poller_transceive(session, tx, tx_len, rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < 3U) {
		return -EIO;
	}

	*random_out = (uint16_t)rx[1] | ((uint16_t)rx[2] << 8U);
	return 0;
}

int slix_poller_set_password(nfc_reader_session_t *session, const uint8_t uid[ISO15693_UID_SIZE],
			     slix_password_type_t type, const uint8_t password[SLIX_PASSWORD_SIZE],
			     uint16_t random)
{
	uint8_t tx[32];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	size_t tx_len;
	uint32_t password_word;
	uint32_t xored;
	uint8_t rn_l;
	uint8_t rn_h;
	bool skip_uid;

	if ((session == NULL) || (password == NULL)) {
		return -EINVAL;
	}

	skip_uid = (type == SLIX_PASSWORD_PRIVACY);
	tx_len = slix_poller_prepare_request(uid, SLIX_CMD_SET_PASSWORD, skip_uid, tx, sizeof(tx));
	if (tx_len == 0U) {
		return -EINVAL;
	}

	if ((tx_len + 1U + SLIX_PASSWORD_SIZE) > sizeof(tx)) {
		return -EINVAL;
	}

	tx[tx_len++] = (uint8_t)(1U << (uint8_t)type);
	password_word = slix_poller_read_be32(password);
	rn_l = (uint8_t)(random >> 8);
	rn_h = (uint8_t)(random & 0xFFU);
	xored = password_word ^ (((uint32_t)rn_h << 24) | ((uint32_t)rn_l << 16) |
				 ((uint32_t)rn_h << 8) | (uint32_t)rn_l);
	tx[tx_len++] = (uint8_t)((xored >> 24) & 0xFFU);
	tx[tx_len++] = (uint8_t)((xored >> 16) & 0xFFU);
	tx[tx_len++] = (uint8_t)((xored >> 8) & 0xFFU);
	tx[tx_len++] = (uint8_t)(xored & 0xFFU);

	return slix_poller_transceive(session, tx, tx_len, rx, &rx_len);
}
