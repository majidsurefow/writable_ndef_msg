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
#define EMV_TAG_AID              0x4FU
#define EMV_TAG_PAN                0x5AU
#define EMV_TAG_TRACK2             0x57U

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

static int emv_poller_parse_ppse_aid(const uint8_t *payload, size_t payload_len, uint8_t *aid,
				     size_t aid_max, size_t *aid_len)
{
	size_t i;

	if ((payload == NULL) || (aid == NULL) || (aid_len == NULL)) {
		return -EINVAL;
	}

	for (i = 0U; i + 2U < payload_len; i++) {
		uint8_t alen;

		if (payload[i] != EMV_TAG_AID) {
			continue;
		}

		alen = payload[i + 1U];
		if ((alen == 0U) || (alen > aid_max) || ((i + 2U + (size_t)alen) > payload_len)) {
			continue;
		}

		(void)memcpy(aid, &payload[i + 2U], alen);
		*aid_len = alen;
		return 0;
	}

	return -EBADMSG;
}

static void emv_poller_parse_record_tlv(emv_card_image_t *out, const uint8_t *data, size_t len)
{
	size_t pos = 0U;
	size_t end = len;

	if ((data == NULL) || (out == NULL) || (len < 2U)) {
		return;
	}

	if (data[0] == EMV_TAG_RECORD_TMPL) {
		pos = 2U;
		if (data[1] < (len - 2U)) {
			end = 2U + (size_t)data[1];
		}
	}

	while ((pos + 2U) <= end) {
		uint8_t tag = data[pos++];
		uint8_t tlen = data[pos++];

		if ((pos + (size_t)tlen) > end) {
			break;
		}

		if (tag == EMV_TAG_TRACK2) {
			out->track2_len = tlen;
			if (out->track2_len > EMV_TRACK2_MAX_BYTES) {
				out->track2_len = EMV_TRACK2_MAX_BYTES;
			}
			(void)memcpy(out->track2, &data[pos], out->track2_len);
		} else if (tag == EMV_TAG_PAN) {
			out->pan_len = (uint8_t)(tlen * 2U);
			if (tlen > EMV_PAN_BYTES) {
				tlen = EMV_PAN_BYTES;
			}
			(void)memcpy(out->pan, &data[pos], tlen);
		}

		pos += (size_t)tlen;
	}
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
	uint8_t tx_app[5U + EMV_APP_AID_MAX_BYTES];
	uint8_t tx_gpo[] = {0x80U, 0xA8U, 0x00U, 0x00U, 0x02U, 0x83U, 0x00U, 0x00U};
	uint8_t tx_read[5U];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	uint8_t app_aid[EMV_APP_AID_MAX_BYTES];
	size_t app_aid_len = 0U;
	size_t tx_app_len;
	size_t rx_len = 0U;
	size_t payload_len;
	uint8_t sfi;
	uint8_t first_rec;
	uint8_t last_rec;
	uint8_t rec;
	uint8_t record_idx = 0U;
	int ret;

	if ((session == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	emv_card_image_reset(out);

	ret = emv_poller_transceive(mut, tx_ppse, sizeof(tx_ppse), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}
	ret = emv_poller_sw_ok(rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < 2U) {
		return -EBADMSG;
	}
	ret = emv_poller_parse_ppse_aid(rx, rx_len - 2U, app_aid, sizeof(app_aid), &app_aid_len);
	if (ret != 0) {
		return ret;
	}

	out->app_aid_len = (uint8_t)app_aid_len;
	(void)memcpy(out->app_aid, app_aid, app_aid_len);

	tx_app[0] = 0x00U;
	tx_app[1] = 0xA4U;
	tx_app[2] = 0x04U;
	tx_app[3] = 0x00U;
	tx_app[4] = (uint8_t)app_aid_len;
	(void)memcpy(&tx_app[5], app_aid, app_aid_len);
	tx_app_len = 5U + app_aid_len;

	ret = emv_poller_transceive(mut, tx_app, tx_app_len, rx, &rx_len);
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

	sfi = (uint8_t)(out->afl[0] >> 3U);
	first_rec = out->afl[1];
	last_rec = out->afl[2];

	for (rec = first_rec; (rec <= last_rec) && (record_idx < CONFIG_NFC_EMV_MAX_RECORDS);
	     rec++) {
		tx_read[0] = 0x00U;
		tx_read[1] = 0xB2U;
		tx_read[2] = rec;
		tx_read[3] = (uint8_t)((sfi << 3U) | 0x04U);
		tx_read[4] = 0x00U;

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
			out->record_len[record_idx] = (uint8_t)payload_len;
			(void)memcpy(out->record_data[record_idx], rx, payload_len);
			emv_poller_parse_record_tlv(out, rx, payload_len);
			record_idx++;
		}
	}

	out->record_count = record_idx;
	return 0;
}
