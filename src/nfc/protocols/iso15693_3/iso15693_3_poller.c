/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/iso15693_3/iso15693_3_poller.c
 */

#include "protocols/iso15693_3/iso15693_3_poller.h"
#include "protocols/iso15693_3/iso15693_3_poller_i.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define ISO15693_POLLER_TIMEOUT K_MSEC(5000)
#define ISO15693_TX_MAX           32U

static int iso15693_3_poller_session_valid(const nfc_reader_session_t *session)
{
	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	return 0;
}

static int iso15693_3_poller_send_frame(nfc_reader_session_t *session, uint8_t *tx,
				      size_t tx_payload_len, uint8_t *rx, size_t *rx_len)
{
	size_t tx_len;
	size_t rx_raw_len = 0U;
	int ret;

	tx_len = iso15693_3_crc_append(tx, ISO15693_TX_MAX, tx_payload_len);
	if (tx_len == 0U) {
		return -EINVAL;
	}

	ret = nfc_reader_session_transceive(session, tx, tx_len, rx, NFC_TRANSPORT_MAX_RESPONSE_LEN,
					  &rx_raw_len, ISO15693_POLLER_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (!iso15693_3_crc_check(rx, rx_raw_len)) {
		return -EIO;
	}

	*rx_len = iso15693_3_crc_trim(rx, rx_raw_len);
	return 0;
}

int iso15693_3_poller_inventory(nfc_reader_session_t *session, iso15693_3_data_t *data)
{
	uint8_t tx[ISO15693_TX_MAX];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	if (data == NULL) {
		return -EINVAL;
	}

	tx[0] = ISO15693_INV_FLAGS;
	tx[1] = ISO15693_CMD_INVENTORY;

	ret = iso15693_3_poller_send_frame(session, tx, 2U, rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	return iso15693_3_inventory_response_parse(rx, rx_len, data->uid, &data->dsfid);
}

static int iso15693_3_poller_get_system_info(nfc_reader_session_t *session,
					     iso15693_3_data_t *data)
{
	uint8_t tx[ISO15693_TX_MAX];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	tx[0] = ISO15693_CMD_FLAGS;
	tx[1] = ISO15693_CMD_GET_SYS_INFO;

	ret = iso15693_3_poller_send_frame(session, tx, 2U, rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	return iso15693_3_system_info_response_parse(rx, rx_len, data);
}

static int iso15693_3_poller_read_block(nfc_reader_session_t *session, uint8_t block_num,
					uint8_t block_size, uint8_t *out)
{
	uint8_t tx[ISO15693_TX_MAX];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	tx[0] = ISO15693_CMD_FLAGS;
	tx[1] = ISO15693_CMD_READ;
	tx[2] = block_num;

	ret = iso15693_3_poller_send_frame(session, tx, 3U, rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	return iso15693_3_read_block_response_parse(rx, rx_len, block_size, out);
}

static int iso15693_3_poller_get_blocks_security(nfc_reader_session_t *session,
						 iso15693_3_data_t *data)
{
	uint16_t start;
	uint16_t remaining;
	uint8_t batch;
	uint8_t tx[ISO15693_TX_MAX];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	if ((data->block_count == 0U) || (data->block_count > ISO15693_BLOCKS_MAX)) {
		return -EINVAL;
	}

	for (start = 0U; start < data->block_count; start += ISO15693_BLOCKS_PER_QUERY) {
		remaining = data->block_count - start;
		batch = (remaining > ISO15693_BLOCKS_PER_QUERY) ? ISO15693_BLOCKS_PER_QUERY
								: (uint8_t)remaining;

		tx[0] = ISO15693_CMD_FLAGS;
		tx[1] = ISO15693_CMD_GET_BLOCKS_SEC;
		tx[2] = (uint8_t)start;
		tx[3] = (uint8_t)(batch - 1U);

		ret = iso15693_3_poller_send_frame(session, tx, 4U, rx, &rx_len);
		if (ret != 0) {
			return ret;
		}

		ret = iso15693_3_get_block_security_response_parse(rx, rx_len,
								   &data->block_security[start],
								   batch);
		if (ret != 0) {
			return ret;
		}
	}

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
	if (!iso15693_3_optional_step_ok(ret)) {
		return ret;
	}

	if ((out->block_count == 0U) || (out->block_size == 0U)) {
		return 0;
	}

	for (block = 0U; block < out->block_count; block++) {
		uint8_t *dst = &out->block_data[(size_t)block * (size_t)out->block_size];

		ret = iso15693_3_poller_read_block(mut, (uint8_t)block, out->block_size, dst);
		if (!iso15693_3_optional_step_ok(ret)) {
			return ret;
		}
		if (ret != 0) {
			break;
		}
	}

	ret = iso15693_3_poller_get_blocks_security(mut, out);
	if (!iso15693_3_optional_step_ok(ret)) {
		return ret;
	}

	return 0;
}
