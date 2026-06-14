/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/iso15693_3/iso15693_3_poller.c
 */

#include "protocols/iso15693_3/iso15693_3_poller.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define ISO15693_POLLER_TIMEOUT K_MSEC(5000)
#define ISO15693_BLOCKS_PROBE   ISO15693_BLOCKS_MAX

static int iso15693_3_poller_session_valid(const nfc_reader_session_t *session)
{
	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	return 0;
}

static int iso15693_3_poller_transceive(nfc_reader_session_t *session, const uint8_t *tx,
					size_t tx_len, uint8_t *rx, size_t *rx_len)
{
	return nfc_reader_session_transceive(session, tx, tx_len, rx,
					     NFC_TRANSPORT_MAX_RESPONSE_LEN, rx_len,
					     ISO15693_POLLER_TIMEOUT);
}

static void iso15693_3_uid_from_inventory(const uint8_t *inv_uid, uint8_t out[ISO15693_UID_SIZE])
{
	for (size_t i = 0U; i < ISO15693_UID_SIZE; i++) {
		out[i] = inv_uid[ISO15693_UID_SIZE - 1U - i];
	}
}

int iso15693_3_poller_inventory(nfc_reader_session_t *session, iso15693_3_data_t *data)
{
	uint8_t tx[] = {ISO15693_INV_FLAGS, ISO15693_CMD_INVENTORY};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = iso15693_3_poller_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < (2U + ISO15693_UID_SIZE)) {
		return -ENOTSUP;
	}

	data->dsfid = rx[1];
	iso15693_3_uid_from_inventory(&rx[2], data->uid);
	return 0;
}

static int iso15693_3_poller_get_system_info(nfc_reader_session_t *session,
					     iso15693_3_data_t *data)
{
	uint8_t tx[] = {ISO15693_INV_FLAGS, ISO15693_CMD_GET_SYS_INFO};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = iso15693_3_poller_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len >= 4U) {
		data->block_count = (uint16_t)rx[1] | ((uint16_t)rx[2] << 8U);
		data->block_size = rx[3];
	}

	if ((data->block_count == 0U) || (data->block_count > ISO15693_BLOCKS_MAX) ||
	    (data->block_size == 0U) || (data->block_size > ISO15693_BLOCK_SIZE_MAX)) {
		return -EIO;
	}

	return 0;
}

static int iso15693_3_poller_read_block(nfc_reader_session_t *session, uint8_t block_num,
					uint8_t block_size, uint8_t *out)
{
	uint8_t tx[] = {ISO15693_INV_FLAGS, ISO15693_CMD_READ, block_num};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = iso15693_3_poller_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < (1U + block_size)) {
		return -EIO;
	}

	(void)memcpy(out, &rx[1], block_size);
	return 0;
}

int iso15693_3_poller_detect(const nfc_reader_session_t *session)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	iso15693_3_data_t scratch;
	int ret;

	ret = iso15693_3_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}

	iso15693_3_data_reset(&scratch);
	ret = iso15693_3_poller_inventory(mut, &scratch);
	if (ret != 0) {
		return -ENOTSUP;
	}

	if (scratch.uid[0] != 0xE0U) {
		return -ENOTSUP;
	}

	return 0;
}

int iso15693_3_poller_read(const nfc_reader_session_t *session, iso15693_3_data_t *out)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint16_t block;
	int ret;

	ret = iso15693_3_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}
	if (out == NULL) {
		return -EINVAL;
	}

	iso15693_3_data_reset(out);
	ret = iso15693_3_poller_inventory(mut, out);
	if (ret != 0) {
		return ret;
	}

	ret = iso15693_3_poller_get_system_info(mut, out);
	if (ret != 0) {
		return ret;
	}

	for (block = 0U; block < out->block_count; block++) {
		uint8_t *dst = &out->block_data[(size_t)block * (size_t)out->block_size];

		ret = iso15693_3_poller_read_block(mut, (uint8_t)block, out->block_size, dst);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}
