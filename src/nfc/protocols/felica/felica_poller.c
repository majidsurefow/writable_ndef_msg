/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/felica/felica_poller.c
 */

#include "protocols/felica/felica_poller.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define FELICA_POLLER_TIMEOUT K_MSEC(5000)
#define FELICA_CMD_POLL       0x00U
#define FELICA_CMD_READ       0x06U
#define FELICA_BLOCKS_PROBE   FELICA_BLOCKS_MAX

static int felica_poller_session_valid(const nfc_reader_session_t *session)
{
	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	return 0;
}

static int felica_poller_transceive(nfc_reader_session_t *session, const uint8_t *tx,
				    size_t tx_len, uint8_t *rx, size_t *rx_len)
{
	return nfc_reader_session_transceive(session, tx, tx_len, rx,
					     NFC_TRANSPORT_MAX_RESPONSE_LEN, rx_len,
					     FELICA_POLLER_TIMEOUT);
}

static int felica_poller_poll(nfc_reader_session_t *session, felica_data_t *data)
{
	uint8_t tx[] = {FELICA_CMD_POLL, 0xFFU, 0xFFU, 0x00U, 0x00U};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = felica_poller_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < FELICA_IDM_SIZE) {
		return -ENOTSUP;
	}

	(void)memcpy(data->idm, rx, FELICA_IDM_SIZE);
	if (rx_len >= (FELICA_IDM_SIZE + FELICA_PMM_SIZE)) {
		(void)memcpy(data->pmm, &rx[FELICA_IDM_SIZE], FELICA_PMM_SIZE);
	}

	return 0;
}

static int felica_poller_read_block(nfc_reader_session_t *session, uint8_t block_num,
				    uint8_t out[FELICA_BLOCK_DATA_SIZE])
{
	uint8_t tx[] = {FELICA_CMD_READ, block_num};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = felica_poller_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < FELICA_BLOCK_DATA_SIZE) {
		return -EIO;
	}

	(void)memcpy(out, rx, FELICA_BLOCK_DATA_SIZE);
	return 0;
}

int felica_poller_detect(const nfc_reader_session_t *session)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t tx[] = {FELICA_CMD_POLL, 0xFFU, 0xFFU, 0x00U, 0x00U};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = felica_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}

	ret = felica_poller_transceive(mut, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return -ENOTSUP;
	}

	if (rx_len < FELICA_IDM_SIZE) {
		return -ENOTSUP;
	}

	return 0;
}

int felica_poller_read(const nfc_reader_session_t *session, felica_data_t *out)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint16_t block;
	int ret;

	ret = felica_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}
	if (out == NULL) {
		return -EINVAL;
	}

	felica_data_reset(out);
	ret = felica_poller_poll(mut, out);
	if (ret != 0) {
		return ret;
	}

	for (block = 0U; block < FELICA_BLOCKS_PROBE; block++) {
		ret = felica_poller_read_block(mut, (uint8_t)block, out->blocks[block].data);
		if (ret != 0) {
			break;
		}

		out->blocks[block].sf1 = 0U;
		out->blocks[block].sf2 = 0U;
		out->blocks_total = (uint16_t)(block + 1U);
		out->blocks_read = out->blocks_total;
	}

	if (out->blocks_total == 0U) {
		return -EIO;
	}

	return 0;
}
