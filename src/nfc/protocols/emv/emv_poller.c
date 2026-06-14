/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "emv_poller.h"

#include "framing/apdu_types.h"
#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define EMV_POLLER_TIMEOUT K_MSEC(5000)

static int emv_poller_sw_ok(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 2U) {
		return -EIO;
	}

	if ((rx[rx_len - 2U] == NFC_SW1_OK) && (rx[rx_len - 1U] == NFC_SW2_OK)) {
		return 0;
	}

	return -EIO;
}

static int emv_poller_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				 uint8_t *rx, size_t *rx_len)
{
	return nfc_reader_session_transceive(session, tx, tx_len, rx, NFC_TRANSPORT_MAX_RESPONSE_LEN,
					     rx_len, EMV_POLLER_TIMEOUT);
}

int emv_poller_detect(const nfc_reader_session_t *session)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t tx[] = {0x00U, 0xA4U, 0x04U, 0x00U, EMV_PPSE_AID_LEN,
			0x32U, 0x50U, 0x41U, 0x59U, 0x2EU, 0x53U, 0x59U, 0x53U,
			0x2EU, 0x44U, 0x44U, 0x46U, 0x30U, 0x31U};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	ret = emv_poller_transceive(mut, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return -ENOTSUP;
	}

	return emv_poller_sw_ok(rx, rx_len);
}

int emv_poller_read(const nfc_reader_session_t *session, emv_card_image_t *out)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t tx_ppse[] = {0x00U, 0xA4U, 0x04U, 0x00U, EMV_PPSE_AID_LEN,
			     0x32U, 0x50U, 0x41U, 0x59U, 0x2EU, 0x53U, 0x59U, 0x53U,
			     0x2EU, 0x44U, 0x44U, 0x46U, 0x30U, 0x31U};
	uint8_t tx_app[] = {0x00U, 0xA4U, 0x04U, 0x00U, EMV_SERVICE_APP_AID_LEN,
			    0xA0U, 0x00U, 0x00U, 0x00U, 0x03U, 0x10U, 0x10U};
	uint8_t tx_gpo[] = {0x80U, 0xA8U, 0x00U, 0x00U, 0x02U, 0x83U, 0x00U, 0x00U};
	uint8_t tx_read[] = {0x00U, 0xB2U, 0x01U, 0x0CU, 0x00U};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	size_t payload_len;
	int ret;

	if ((session == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	emv_card_image_reset(out);
	(void)memcpy(out->app_aid, emv_service_app_aid, EMV_SERVICE_APP_AID_LEN);
	out->app_aid_len = EMV_SERVICE_APP_AID_LEN;

	ret = emv_poller_transceive(mut, tx_ppse, sizeof(tx_ppse), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}
	ret = emv_poller_sw_ok(rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = emv_poller_transceive(mut, tx_app, sizeof(tx_app), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}
	ret = emv_poller_sw_ok(rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = emv_poller_transceive(mut, tx_gpo, sizeof(tx_gpo), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}
	ret = emv_poller_sw_ok(rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len >= 8U) {
		(void)memcpy(out->aip, &rx[2], EMV_AIP_BYTES);
		(void)memcpy(out->afl, &rx[4], EMV_AFL_BYTES);
	}

	ret = emv_poller_transceive(mut, tx_read, sizeof(tx_read), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}
	ret = emv_poller_sw_ok(rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	payload_len = rx_len - 2U;
	if ((payload_len > 0U) && (payload_len <= CONFIG_NFC_EMV_RECORD_SIZE)) {
		out->record_len[0] = (uint8_t)payload_len;
		(void)memcpy(out->record_data[0], rx, payload_len);
		out->record_count = 1U;
	}

	out->aip[0] = 0x00U;
	out->aip[1] = 0x00U;
	return 0;
}
