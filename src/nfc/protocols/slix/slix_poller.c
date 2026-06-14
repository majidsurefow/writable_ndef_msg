/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/slix/slix_poller.c
 */

#include "protocols/slix/slix_poller.h"

#include "protocols/iso15693_3/iso15693_3_poller.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define SLIX_POLLER_TIMEOUT K_MSEC(5000)
#define SLIX_CMD_GET_NXP_SYSINFO 0xABU
#define SLIX_CMD_READ_SIGNATURE  0xBDU
#define SLIX_CAP_MARKER_ACCEPT   0xACU

static int slix_poller_transceive(nfc_reader_session_t *session, const uint8_t *tx,
				  size_t tx_len, uint8_t *rx, size_t *rx_len)
{
	return nfc_reader_session_transceive(session, tx, tx_len, rx,
					     NFC_TRANSPORT_MAX_RESPONSE_LEN, rx_len,
					     SLIX_POLLER_TIMEOUT);
}

static slix_capabilities_t slix_poller_parse_capabilities(const uint8_t *rx, size_t rx_len)
{
	if (rx_len == 0U) {
		return SLIX_CAP_MISSED;
	}
	if ((rx_len >= 1U) && (rx[0] == SLIX_CAP_MARKER_ACCEPT)) {
		return SLIX_CAP_ACCEPT_ALL;
	}

	return SLIX_CAP_DEFAULT;
}

int slix_poller_detect(const nfc_reader_session_t *session)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	iso15693_3_data_t parent;
	slix_type_t type;
	int ret;

	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	ret = iso15693_3_poller_inventory(mut, &parent);
	if (ret != 0) {
		return -ENOTSUP;
	}

	if (parent.uid[0] != 0xE0U) {
		return -ENOTSUP;
	}

	type = slix_type_from_uid(parent.uid);
	if (type == SLIX_TYPE_UNKNOWN) {
		return -ENOTSUP;
	}

	return 0;
}

int slix_poller_read(const nfc_reader_session_t *session, slix_data_t *out)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t tx_sys[] = {ISO15693_INV_FLAGS, SLIX_CMD_GET_NXP_SYSINFO};
	uint8_t tx_sig[] = {ISO15693_INV_FLAGS, SLIX_CMD_READ_SIGNATURE};
	uint8_t tx_cap[] = {ISO15693_INV_FLAGS, 0xB0U};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	if ((session == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	slix_data_reset(out);
	ret = iso15693_3_poller_read(session, &out->parent);
	if (ret != 0) {
		return ret;
	}

	out->type = slix_type_from_uid(out->parent.uid);
	if (out->type == SLIX_TYPE_UNKNOWN) {
		return -ENOTSUP;
	}

	ret = slix_poller_transceive(mut, tx_sys, sizeof(tx_sys), rx, &rx_len);
	if ((ret == 0) && (rx_len >= 2U)) {
		out->protection_pointer = rx[1];
		if (rx_len >= 3U) {
			out->protection_condition = rx[2];
		}
	}

	ret = slix_poller_transceive(mut, tx_sig, sizeof(tx_sig), rx, &rx_len);
	if (ret == 0) {
		size_t copy = rx_len;

		if (copy > SLIX_SIGNATURE_SIZE) {
			copy = SLIX_SIGNATURE_SIZE;
		}
		(void)memcpy(out->signature, rx, copy);
	}

	ret = slix_poller_transceive(mut, tx_cap, sizeof(tx_cap), rx, &rx_len);
	if (ret == 0) {
		out->capabilities = slix_poller_parse_capabilities(rx, rx_len);
	}

	return 0;
}
