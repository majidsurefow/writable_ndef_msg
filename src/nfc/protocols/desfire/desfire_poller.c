/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "desfire_poller.h"

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(desfire_poller, CONFIG_LOG_DEFAULT_LEVEL);

#define DESFIRE_POLLER_TIMEOUT K_MSEC(5000)

static const uint8_t SELECT_AID[] = {
	0x00U, 0xA4U, 0x04U, 0x00U, DESFIRE_AID_LEN,
	0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x00U,
};

static const uint8_t GET_KEY_VERSION[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_KEY_VERSION,
					  0x00U, 0x00U, 0x01U, 0x00U};
static const uint8_t GET_VERSION[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_VERSION,
				      0x00U, 0x00U, 0x00U};
static const uint8_t GET_FREE_MEMORY[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_FREE_MEMORY,
					  0x00U, 0x00U, 0x00U};
static const uint8_t GET_KEY_SETTINGS[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_KEY_SETTINGS,
					   0x00U, 0x00U, 0x00U};
static const uint8_t AF_CONT[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_ADDITIONAL_FRAME, 0x00U,
				  0x00U, 0x00U};

static int desfire_poller_transceive(nfc_reader_session_t *session, const uint8_t *tx,
				     size_t tx_len, uint8_t *rx, size_t *rx_len)
{
	return nfc_reader_session_transceive(session, tx, tx_len, rx,
					     NFC_TRANSPORT_MAX_RESPONSE_LEN, rx_len,
					     DESFIRE_POLLER_TIMEOUT);
}

static bool desfire_poller_status_ok(const uint8_t *rx, size_t rx_len, uint8_t expect_sw2)
{
	if (rx_len < 2U) {
		return false;
	}

	return (rx[rx_len - 2U] == DESFIRE_SW1) && (rx[rx_len - 1U] == expect_sw2);
}

static int desfire_poller_select(nfc_reader_session_t *session)
{
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = desfire_poller_transceive(session, SELECT_AID, sizeof(SELECT_AID), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if ((rx_len < 2U) || (rx[rx_len - 2U] != 0x90U) || (rx[rx_len - 1U] != 0x00U)) {
		return -EIO;
	}

	return 0;
}

static int desfire_poller_send_chunks(nfc_reader_session_t *session, const uint8_t *first_tx,
				      size_t first_tx_len, uint8_t *out, size_t out_max,
				      size_t *out_len)
{
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	size_t total = 0U;
	int ret;

	ret = desfire_poller_transceive(session, first_tx, first_tx_len, rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	while (true) {
		size_t payload_len;

		if (rx_len < 2U) {
			return -EIO;
		}

		payload_len = rx_len - 2U;
		if ((total + payload_len) > out_max) {
			return -ENOSPC;
		}

		if (payload_len > 0U) {
			(void)memcpy(&out[total], rx, payload_len);
			total += payload_len;
		}

		if (desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK)) {
			break;
		}

		if (!desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_ADDITIONAL_FRAME)) {
			return -EIO;
		}

		ret = desfire_poller_transceive(session, AF_CONT, sizeof(AF_CONT), rx, &rx_len);
		if (ret != 0) {
			return ret;
		}
	}

	*out_len = total;
	return 0;
}

static int desfire_poller_parse_version(const uint8_t *buf, size_t len, desfire_data_t *out)
{
	if (len < DESFIRE_VERSION_TOTAL_BYTES) {
		return -EBADMSG;
	}

	(void)memcpy(out->hw_version, buf, DESFIRE_VERSION_SIZE);
	(void)memcpy(out->sw_version, &buf[DESFIRE_VERSION_SIZE], DESFIRE_VERSION_SIZE);
	(void)memcpy(out->uid, &buf[DESFIRE_VERSION_SIZE * 2U], DESFIRE_UID_SIZE);
	(void)memcpy(out->batch, &buf[DESFIRE_VERSION_SIZE * 2U + DESFIRE_UID_SIZE],
		     DESFIRE_BATCH_SIZE);
	out->prod_week = buf[DESFIRE_VERSION_SIZE * 2U + DESFIRE_UID_SIZE + DESFIRE_BATCH_SIZE];
	out->prod_year = buf[DESFIRE_VERSION_SIZE * 2U + DESFIRE_UID_SIZE + DESFIRE_BATCH_SIZE + 1U];
	return 0;
}

int desfire_poller_detect(const nfc_reader_session_t *session)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	ret = desfire_poller_select(mut);
	if (ret != 0) {
		return -ENOTSUP;
	}

	ret = desfire_poller_transceive(mut, GET_KEY_VERSION, sizeof(GET_KEY_VERSION), rx, &rx_len);
	if ((ret != 0) || !desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK)) {
		return -ENOTSUP;
	}

	ret = desfire_poller_transceive(mut, GET_VERSION, sizeof(GET_VERSION), rx, &rx_len);
	if (ret != 0) {
		return -ENOTSUP;
	}

	if (!desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK) &&
	    !desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_ADDITIONAL_FRAME)) {
		return -ENOTSUP;
	}

	return 0;
}

int desfire_poller_read(const nfc_reader_session_t *session, desfire_data_t *out)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t version_buf[DESFIRE_VERSION_TOTAL_BYTES];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t version_len = 0U;
	size_t rx_len = 0U;
	int ret;

	if ((session == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	desfire_data_reset(out);

	ret = desfire_poller_select(mut);
	if (ret != 0) {
		return ret;
	}

	ret = desfire_poller_transceive(mut, GET_KEY_VERSION, sizeof(GET_KEY_VERSION), rx, &rx_len);
	if ((ret != 0) || !desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK)) {
		return -EIO;
	}

	ret = desfire_poller_send_chunks(mut, GET_VERSION, sizeof(GET_VERSION), version_buf,
					 sizeof(version_buf), &version_len);
	if (ret != 0) {
		return ret;
	}

	ret = desfire_poller_parse_version(version_buf, version_len, out);
	if (ret != 0) {
		return ret;
	}

	ret = desfire_poller_transceive(mut, GET_FREE_MEMORY, sizeof(GET_FREE_MEMORY), rx, &rx_len);
	if ((ret == 0) && desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK) && (rx_len >= 6U)) {
		out->free_memory = (uint32_t)rx[0] | ((uint32_t)rx[1] << 8U) |
				   ((uint32_t)rx[2] << 16U) | ((uint32_t)rx[3] << 24U);
	}

	ret = desfire_poller_transceive(mut, GET_KEY_SETTINGS, sizeof(GET_KEY_SETTINGS), rx, &rx_len);
	if ((ret == 0) && desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK) && (rx_len >= 3U)) {
		out->master_key_settings = rx[0];
	}

	return 0;
}
