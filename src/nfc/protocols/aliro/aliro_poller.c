/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aliro_poller.h"

#include "aliro_vectors.h"
#include "framing/apdu_types.h"
#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define ALIRO_POLLER_TIMEOUT K_MSEC(5000)

static bool aliro_poller_select_ok(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 2U) {
		return false;
	}

	/* Expedited listener SELECT: 0000 || nonce (virtual loopback / PICC). */
	if ((rx_len >= (2U + ALIRO_NONCE_SIZE)) && (rx[0] == 0x00U) && (rx[1] == 0x00U)) {
		return true;
	}

	return (rx[rx_len - 2U] == 0x90U) && (rx[rx_len - 1U] == 0x00U);
}

static bool aliro_poller_auth0_ok(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < ALIRO_AUTH0_RSP_SIZE) {
		return false;
	}

	return (rx[0] == 0x00U) && (rx[1] == 0x00U);
}

static bool aliro_poller_auth1_ok(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < ALIRO_AUTH1_RSP_SIZE) {
		return false;
	}

	return (rx[0] == 0x00U) && (rx[1] == 0x00U);
}

static int aliro_poller_append_transcript(aliro_data_t *out, const uint8_t *rx, size_t rx_len)
{
	if ((out == NULL) || (rx_len == 0U)) {
		return 0;
	}

	if ((out->transcript_len + rx_len) > CONFIG_NFC_ALIRO_MAX_TRANSCRIPT) {
		return -ENOSPC;
	}

	(void)memcpy(&out->transcript[out->transcript_len], rx, rx_len);
	out->transcript_len = (uint16_t)(out->transcript_len + rx_len);
	return 0;
}

static int aliro_poller_tx(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
			   uint8_t *rx, size_t *rx_len)
{
	return nfc_reader_session_transceive(session, tx, tx_len, rx, NFC_TRANSPORT_MAX_RESPONSE_LEN,
					     rx_len, ALIRO_POLLER_TIMEOUT);
}

int aliro_poller_detect(const nfc_reader_session_t *session)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t tx[] = {0x00U, 0xA4U, 0x04U, 0x00U, ALIRO_AID_LEN,
			0xA0U, 0x00U, 0x00U, 0x08U, 0x58U, 0x01U, 0x01U, 0x01U, 0x00U};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	ret = aliro_poller_tx(mut, tx, sizeof(tx), rx, &rx_len);
	if ((ret != 0) || !aliro_poller_select_ok(rx, rx_len)) {
		return -ENOTSUP;
	}

	return 0;
}

int aliro_poller_read(const nfc_reader_session_t *session, aliro_data_t *out)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t tx_select[] = {0x00U, 0xA4U, 0x04U, 0x00U, ALIRO_AID_LEN,
			       0xA0U, 0x00U, 0x00U, 0x08U, 0x58U, 0x01U, 0x01U, 0x01U, 0x00U};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	if ((session == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	aliro_data_reset(out);
	ret = aliro_poller_tx(mut, tx_select, sizeof(tx_select), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (!aliro_poller_select_ok(rx, rx_len)) {
		return -EIO;
	}

	out->protocol_major = 0x00U;
	out->protocol_minor = 0x09U;
	ret = aliro_poller_append_transcript(out, rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	{
		uint8_t tx_auth0[5U + ALIRO_AUTH0_DATA_SIZE];
		size_t tx_auth0_len = 0U;

		aliro_vectors_build_auth0_tx(tx_auth0, &tx_auth0_len);
		rx_len = 0U;
		ret = aliro_poller_tx(mut, tx_auth0, tx_auth0_len, rx, &rx_len);
		if (ret != 0) {
			return ret;
		}
		if (!aliro_poller_auth0_ok(rx, rx_len)) {
			return -EIO;
		}
		(void)memcpy(out->credential_pubkey, &rx[2U + ALIRO_NONCE_SIZE],
			      ALIRO_P256_PUBLIC_KEY_SIZE);
		ret = aliro_poller_append_transcript(out, rx, rx_len);
		if (ret != 0) {
			return ret;
		}
	}

	{
		uint8_t tx_auth1[5U + ALIRO_AUTH1_DATA_SIZE];
		size_t tx_auth1_len = 0U;

		aliro_vectors_build_auth1_tx(tx_auth1, &tx_auth1_len);
		rx_len = 0U;
		ret = aliro_poller_tx(mut, tx_auth1, tx_auth1_len, rx, &rx_len);
		if (ret == 0) {
			if (aliro_poller_auth1_ok(rx, rx_len)) {
				ret = aliro_poller_append_transcript(out, rx, rx_len);
				if (ret != 0) {
					return ret;
				}
			}
		}
	}

	return 0;
}
