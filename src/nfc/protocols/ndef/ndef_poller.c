/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Type-4 poller — NXP RW_NDEF_T4T port (cookbook §5.1).
 */

#include "ndef_poller.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define NDEF_SW1_OK           0x90U
#define NDEF_SW2_OK           0x00U
#define NDEF_SW1_NOT_FOUND    0x6AU
#define NDEF_SW2_NOT_FOUND    0x82U

#define NDEF_CC_OFF_MLE       3U
#define NDEF_CC_OFF_FILE_ID   9U
#define NDEF_CC_TLV_T_NDEF    0x04U

#define NDEF_POLLER_TIMEOUT   K_MSEC(5000)

static const uint8_t SELECT_APP_V2[] = {
	0x00U, 0xA4U, 0x04U, 0x00U, 0x07U, 0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x01U, 0x00U,
};

static const uint8_t SELECT_APP_V1[] = {
	0x00U, 0xA4U, 0x04U, 0x00U, 0x07U, 0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x00U,
};

static const uint8_t SELECT_CC[] = {0x00U, 0xA4U, 0x00U, 0x0CU, 0x02U, 0xE1U, 0x03U};

static const uint8_t READ_CC[] = {0x00U, 0xB0U, 0x00U, 0x00U, 0x0FU};

static int ndef_poller_session_valid(const nfc_reader_session_t *session)
{
	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	return 0;
}

static bool ndef_poller_sw_ok(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 2U) {
		return false;
	}

	return (rx[rx_len - 2U] == NDEF_SW1_OK) && (rx[rx_len - 1U] == NDEF_SW2_OK);
}

static bool ndef_poller_sw_not_found(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 2U) {
		return false;
	}

	return (rx[rx_len - 2U] == NDEF_SW1_NOT_FOUND) &&
	       (rx[rx_len - 1U] == NDEF_SW2_NOT_FOUND);
}

static int ndef_poller_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				  uint8_t *rx, size_t *rx_len)
{
	int ret;

	ret = nfc_reader_session_transceive(session, tx, tx_len, rx, NFC_TRANSPORT_MAX_RESPONSE_LEN,
					    rx_len, NDEF_POLLER_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (!ndef_poller_sw_ok(rx, *rx_len)) {
		return -EIO;
	}

	return 0;
}

static int ndef_poller_select_app(nfc_reader_session_t *session)
{
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = nfc_reader_session_transceive(session, SELECT_APP_V2, sizeof(SELECT_APP_V2), rx,
					    NFC_TRANSPORT_MAX_RESPONSE_LEN, &rx_len,
					    NDEF_POLLER_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (ndef_poller_sw_ok(rx, rx_len)) {
		return 0;
	}

	ret = nfc_reader_session_transceive(session, SELECT_APP_V1, sizeof(SELECT_APP_V1), rx,
					    NFC_TRANSPORT_MAX_RESPONSE_LEN, &rx_len,
					    NDEF_POLLER_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (ndef_poller_sw_ok(rx, rx_len)) {
		return 0;
	}

	return -EIO;
}

int ndef_poller_detect(const nfc_reader_session_t *session)
{
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = ndef_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}

	ret = nfc_reader_session_transceive((nfc_reader_session_t *)session, SELECT_APP_V2,
					    sizeof(SELECT_APP_V2), rx, NFC_TRANSPORT_MAX_RESPONSE_LEN,
					    &rx_len, NDEF_POLLER_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (ndef_poller_sw_ok(rx, rx_len)) {
		return 0;
	}

	ret = nfc_reader_session_transceive((nfc_reader_session_t *)session, SELECT_APP_V1,
					    sizeof(SELECT_APP_V1), rx, NFC_TRANSPORT_MAX_RESPONSE_LEN,
					    &rx_len, NDEF_POLLER_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (ndef_poller_sw_ok(rx, rx_len)) {
		return 0;
	}

	if (ndef_poller_sw_not_found(rx, rx_len)) {
		return -ENOTSUP;
	}

	return -EIO;
}

static uint16_t ndef_poller_cc_mle(const uint8_t *cc)
{
	uint16_t mle = (uint16_t)(((uint16_t)cc[NDEF_CC_OFF_MLE] << 8) |
				  (uint16_t)cc[NDEF_CC_OFF_MLE + 1U]);

	if (mle < 2U) {
		mle = 2U;
	}

	return mle;
}

static int ndef_poller_select_ndef_file(nfc_reader_session_t *session, const uint8_t *cc)
{
	uint8_t cmd[] = {0x00U, 0xA4U, 0x00U, 0x0CU, 0x02U, cc[NDEF_CC_OFF_FILE_ID],
			 cc[NDEF_CC_OFF_FILE_ID + 1U]};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = nfc_reader_session_transceive(session, cmd, sizeof(cmd), rx,
					    NFC_TRANSPORT_MAX_RESPONSE_LEN, &rx_len,
					    NDEF_POLLER_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (!ndef_poller_sw_ok(rx, rx_len)) {
		return -EIO;
	}

	return 0;
}

static size_t ndef_poller_read_chunk_le(uint16_t remaining, uint16_t tag_mle)
{
	size_t chunk;
	uint16_t mle_cap;
	size_t transport_cap;

	mle_cap = (uint16_t)(tag_mle - 1U);
	chunk = (size_t)remaining;

	if (chunk > (size_t)mle_cap) {
		chunk = (size_t)mle_cap;
	}

	/* R-APDU data + SW1/SW2 must fit in NFC_TRANSPORT_MAX_RESPONSE_LEN (PN7160 TML). */
	transport_cap = (size_t)NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U;
	if (chunk > transport_cap) {
		chunk = transport_cap;
	}

	return chunk;
}

int ndef_poller_read(const nfc_reader_session_t *session, ndef_data_t *out)
{
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	uint8_t read_cmd[5];
	size_t rx_len = 0U;
	uint16_t nlen = 0U;
	uint16_t tag_mle;
	uint16_t bytes_read = 0U;
	size_t data_len;
	int ret;

	ret = ndef_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}

	if (out == NULL) {
		return -EINVAL;
	}

	ndef_data_reset(out);

	ret = ndef_poller_select_app((nfc_reader_session_t *)session);
	if (ret != 0) {
		return ret;
	}

	ret = ndef_poller_transceive((nfc_reader_session_t *)session, SELECT_CC, sizeof(SELECT_CC),
				     rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = ndef_poller_transceive((nfc_reader_session_t *)session, READ_CC, sizeof(READ_CC), rx,
				     &rx_len);
	if (ret != 0) {
		return ret;
	}

	data_len = rx_len - 2U;
	if (data_len != NFC_NDEF_CC_LEN) {
		return -EIO;
	}

	out->cc_len = NFC_NDEF_CC_LEN;
	(void)memcpy(out->cc, rx, NFC_NDEF_CC_LEN);

	if (out->cc[7] != NDEF_CC_TLV_T_NDEF) {
		return -EIO;
	}

	tag_mle = ndef_poller_cc_mle(out->cc);

	ret = ndef_poller_select_ndef_file((nfc_reader_session_t *)session, out->cc);
	if (ret != 0) {
		return ret;
	}

	read_cmd[0] = 0x00U;
	read_cmd[1] = 0xB0U;
	read_cmd[2] = 0x00U;
	read_cmd[3] = 0x00U;
	read_cmd[4] = NFC_NDEF_NLEN_FIELD_LEN;

	ret = ndef_poller_transceive((nfc_reader_session_t *)session, read_cmd, sizeof(read_cmd),
				     rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	data_len = rx_len - 2U;
	if (data_len != NFC_NDEF_NLEN_FIELD_LEN) {
		return -EIO;
	}

	nlen = (uint16_t)(((uint16_t)rx[0] << 8) | (uint16_t)rx[1]);
	if (nlen > CONFIG_NFC_NDEF_MAX_SIZE) {
		return -ENOSPC;
	}

	out->ndef_file[0] = rx[0];
	out->ndef_file[1] = rx[1];

	while (bytes_read < nlen) {
		uint16_t offset = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + bytes_read);
		size_t chunk = ndef_poller_read_chunk_le((uint16_t)(nlen - bytes_read), tag_mle);

		read_cmd[0] = 0x00U;
		read_cmd[1] = 0xB0U;
		read_cmd[2] = (uint8_t)(offset >> 8);
		read_cmd[3] = (uint8_t)(offset & 0xFFU);
		read_cmd[4] = (uint8_t)chunk;

		ret = nfc_reader_session_transceive((nfc_reader_session_t *)session, read_cmd,
						    sizeof(read_cmd), rx,
						    NFC_TRANSPORT_MAX_RESPONSE_LEN, &rx_len,
						    NDEF_POLLER_TIMEOUT);
		if (ret != 0) {
			return ret;
		}

		if (!ndef_poller_sw_ok(rx, rx_len)) {
			return -EIO;
		}

		data_len = rx_len - 2U;
		if (data_len < chunk) {
			return -EIO;
		}

		(void)memcpy(&out->ndef_file[NFC_NDEF_NLEN_FIELD_LEN + bytes_read], rx, chunk);
		bytes_read = (uint16_t)(bytes_read + (uint16_t)chunk);
	}

	if (bytes_read != nlen) {
		return -EIO;
	}

	out->ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + nlen);
	return 0;
}
