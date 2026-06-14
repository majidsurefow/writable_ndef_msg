/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "desfire_poller_i.h"

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define DESFIRE_POLLER_I_TIMEOUT K_MSEC(5000)

static const uint8_t AF_CONT[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_ADDITIONAL_FRAME, 0x00U,
				  0x00U, 0x00U};

static bool desfire_poller_i_status_ok(const uint8_t *rx, size_t rx_len, uint8_t expect_sw2)
{
	if (rx_len < 2U) {
		return false;
	}

	return (rx[rx_len - 2U] == DESFIRE_SW1) && (rx[rx_len - 1U] == expect_sw2);
}

static int desfire_poller_i_status_to_errno(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 2U) {
		return -EIO;
	}

	if (rx[rx_len - 1U] == DESFIRE_STATUS_AUTH_ERROR) {
		return DESFIRE_POLLER_I_AUTH;
	}

	return -EIO;
}

int desfire_poller_i_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				uint8_t *rx, size_t *rx_len)
{
	return nfc_reader_session_transceive(session, tx, tx_len, rx,
					     NFC_TRANSPORT_MAX_RESPONSE_LEN, rx_len,
					     DESFIRE_POLLER_I_TIMEOUT);
}

int desfire_poller_i_send_chunks(nfc_reader_session_t *session, const uint8_t *first_tx,
				 size_t first_tx_len, uint8_t *out, size_t out_max,
				 size_t *out_len)
{
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	size_t total = 0U;
	int ret;

	ret = desfire_poller_i_transceive(session, first_tx, first_tx_len, rx, &rx_len);
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

		if (desfire_poller_i_status_ok(rx, rx_len, DESFIRE_STATUS_OK)) {
			break;
		}

		if (!desfire_poller_i_status_ok(rx, rx_len, DESFIRE_STATUS_ADDITIONAL_FRAME)) {
			return desfire_poller_i_status_to_errno(rx, rx_len);
		}

		ret = desfire_poller_i_transceive(session, AF_CONT, sizeof(AF_CONT), rx, &rx_len);
		if (ret != 0) {
			return ret;
		}
	}

	*out_len = total;
	return 0;
}

int desfire_poller_i_read_application_ids(nfc_reader_session_t *session, uint8_t *ids_out,
					    size_t ids_max, uint8_t *count_out)
{
	static const uint8_t cmd[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_APPLICATION_IDS, 0x00U,
				      0x00U, 0x00U};
	uint8_t buf[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t len = 0U;
	uint8_t app_count;
	int ret;

	if ((ids_out == NULL) || (count_out == NULL)) {
		return -EINVAL;
	}

	ret = desfire_poller_i_send_chunks(session, cmd, sizeof(cmd), buf, sizeof(buf), &len);
	if (ret != 0) {
		return ret;
	}

	if ((len == 0U) || ((len % DESFIRE_APP_ID_SIZE) != 0U)) {
		return -EBADMSG;
	}

	app_count = (uint8_t)(len / DESFIRE_APP_ID_SIZE);
	if (app_count > CONFIG_NFC_DESFIRE_MAX_APPS) {
		app_count = CONFIG_NFC_DESFIRE_MAX_APPS;
	}

	if (len > ids_max) {
		return -ENOSPC;
	}

	(void)memcpy(ids_out, buf, len);
	*count_out = app_count;
	return 0;
}

int desfire_poller_i_select_application(nfc_reader_session_t *session,
				      const uint8_t app_id[DESFIRE_APP_ID_SIZE])
{
	uint8_t tx[5U + DESFIRE_APP_ID_SIZE];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	if (app_id == NULL) {
		return -EINVAL;
	}

	tx[0] = DESFIRE_CLA_NATIVE;
	tx[1] = DESFIRE_CMD_SELECT_APPLICATION;
	tx[2] = 0x00U;
	tx[3] = 0x00U;
	tx[4] = DESFIRE_APP_ID_SIZE;
	(void)memcpy(&tx[5], app_id, DESFIRE_APP_ID_SIZE);

	ret = desfire_poller_i_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (!desfire_poller_i_status_ok(rx, rx_len, DESFIRE_STATUS_OK)) {
		return desfire_poller_i_status_to_errno(rx, rx_len);
	}

	return 0;
}

int desfire_poller_i_read_key_settings(nfc_reader_session_t *session, uint8_t *settings_out,
				       uint8_t *max_keys_out)
{
	static const uint8_t cmd[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_KEY_SETTINGS, 0x00U,
				      0x00U, 0x00U};
	uint8_t buf[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t len = 0U;
	int ret;

	if ((settings_out == NULL) || (max_keys_out == NULL)) {
		return -EINVAL;
	}

	ret = desfire_poller_i_send_chunks(session, cmd, sizeof(cmd), buf, sizeof(buf), &len);
	if (ret != 0) {
		return ret;
	}

	if (len < 2U) {
		return -EBADMSG;
	}

	*settings_out = buf[0];
	*max_keys_out = buf[1];
	return 0;
}

int desfire_poller_i_read_file_ids(nfc_reader_session_t *session, uint8_t *ids_out,
				   size_t ids_max, uint8_t *count_out)
{
	static const uint8_t cmd[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_FILE_IDS, 0x00U, 0x00U,
				      0x00U};
	uint8_t buf[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t len = 0U;
	int ret;

	if ((ids_out == NULL) || (count_out == NULL)) {
		return -EINVAL;
	}

	ret = desfire_poller_i_send_chunks(session, cmd, sizeof(cmd), buf, sizeof(buf), &len);
	if (ret != 0) {
		return ret;
	}

	if (len > CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP) {
		return -ENOSPC;
	}
	if (len > ids_max) {
		return -ENOSPC;
	}

	(void)memcpy(ids_out, buf, len);
	*count_out = (uint8_t)len;
	return 0;
}

static int desfire_poller_i_parse_file_settings(const uint8_t *in, size_t in_len,
						desfire_file_settings_t *fs, uint32_t *size_out)
{
	if ((in_len < 7U) || (fs == NULL) || (size_out == NULL)) {
		return -EBADMSG;
	}

	fs->type = (desfire_file_type_t)in[0];
	fs->comm = (desfire_comm_t)in[1];
	fs->access_rights = (uint16_t)in[2] | ((uint16_t)in[3] << 8U);

	switch (fs->type) {
	case DESFIRE_FILE_TYPE_STANDARD:
	case DESFIRE_FILE_TYPE_BACKUP:
		*size_out = (uint32_t)in[4] | ((uint32_t)in[5] << 8U) | ((uint32_t)in[6] << 16U);
		fs->settings.data.size = *size_out;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

int desfire_poller_i_read_file_settings(nfc_reader_session_t *session, uint8_t file_id,
					desfire_file_settings_t *out, uint32_t *size_out)
{
	uint8_t tx[6];
	uint8_t buf[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t len = 0U;
	int ret;

	if ((out == NULL) || (size_out == NULL)) {
		return -EINVAL;
	}

	tx[0] = DESFIRE_CLA_NATIVE;
	tx[1] = DESFIRE_CMD_GET_FILE_SETTINGS;
	tx[2] = 0x00U;
	tx[3] = 0x00U;
	tx[4] = 0x01U;
	tx[5] = file_id;

	ret = desfire_poller_i_send_chunks(session, tx, sizeof(tx), buf, sizeof(buf), &len);
	if (ret != 0) {
		return ret;
	}

	return desfire_poller_i_parse_file_settings(buf, len, out, size_out);
}

int desfire_poller_i_read_file_data(nfc_reader_session_t *session, uint8_t file_id, uint32_t offset,
				    uint32_t length, uint8_t *out, size_t out_max,
				    size_t *out_len)
{
	uint8_t tx[12];
	uint8_t chunk_buf[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t chunk_len = 0U;
	size_t total = 0U;
	uint32_t current_offset = offset;
	uint32_t remaining = length;
	int ret;

	if ((out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	while (remaining > 0U) {
		uint32_t chunk_size = remaining;

		if (chunk_size > (NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U)) {
			chunk_size = NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U;
		}
		if ((total + chunk_size) > out_max) {
			return -ENOSPC;
		}

		tx[0] = DESFIRE_CLA_NATIVE;
		tx[1] = DESFIRE_CMD_READ_DATA;
		tx[2] = 0x00U;
		tx[3] = 0x00U;
		tx[4] = 0x07U;
		tx[5] = file_id;
		tx[6] = (uint8_t)(current_offset & 0xFFU);
		tx[7] = (uint8_t)((current_offset >> 8U) & 0xFFU);
		tx[8] = (uint8_t)((current_offset >> 16U) & 0xFFU);
		tx[9] = (uint8_t)(chunk_size & 0xFFU);
		tx[10] = (uint8_t)((chunk_size >> 8U) & 0xFFU);
		tx[11] = (uint8_t)((chunk_size >> 16U) & 0xFFU);

		ret = desfire_poller_i_send_chunks(session, tx, sizeof(tx), chunk_buf,
						   sizeof(chunk_buf), &chunk_len);
		if (ret != 0) {
			return ret;
		}

		if (chunk_len > remaining) {
			return -EBADMSG;
		}

		(void)memcpy(&out[total], chunk_buf, chunk_len);
		total += chunk_len;
		current_offset += (uint32_t)chunk_len;
		remaining -= (uint32_t)chunk_len;

		if (chunk_len < chunk_size) {
			break;
		}
	}

	*out_len = total;
	return 0;
}
