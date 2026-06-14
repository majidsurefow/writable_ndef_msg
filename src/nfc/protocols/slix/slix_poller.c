/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/slix/slix_poller.c
 */

#include "protocols/slix/slix_poller.h"

#include "protocols/iso15693_3/iso15693_3_poller.h"
#include "protocols/iso15693_3/iso15693_3_poller_i.h"
#include "protocols/slix/slix_poller_i.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define SLIX_POLLER_TIMEOUT K_MSEC(5000)
#define SLIX_TX_MAX           32U

static int slix_poller_transceive_crc(nfc_reader_session_t *session, uint8_t *tx,
				      size_t tx_payload_len, uint8_t *rx, size_t *rx_len)
{
	size_t tx_len;
	size_t rx_raw_len = 0U;
	int ret;

	tx_len = iso15693_3_crc_append(tx, SLIX_TX_MAX, tx_payload_len);
	if (tx_len == 0U) {
		return -EINVAL;
	}

	ret = nfc_reader_session_transceive(session, tx, tx_len, rx, NFC_TRANSPORT_MAX_RESPONSE_LEN,
					  &rx_raw_len, SLIX_POLLER_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (!iso15693_3_crc_check(rx, rx_raw_len)) {
		return -EIO;
	}

	*rx_len = iso15693_3_crc_trim(rx, rx_raw_len);
	return 0;
}

static bool slix_type_has_nxp_sysinfo(slix_type_t type)
{
	return type == SLIX_TYPE_SLIX2;
}

static bool slix_type_has_signature(slix_type_t type)
{
	return type == SLIX_TYPE_SLIX2;
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
	uint8_t tx[SLIX_TX_MAX];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	size_t tx_len;
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

	if (slix_type_has_nxp_sysinfo(out->type)) {
		tx_len = slix_poller_prepare_request(out->parent.uid, SLIX_CMD_GET_NXP_SYSINFO, false,
						     tx, sizeof(tx));
		if (tx_len == 0U) {
			return -EINVAL;
		}

		ret = slix_poller_transceive_crc(mut, tx, tx_len, rx, &rx_len);
		if ((ret == 0) && (rx_len >= 2U)) {
			out->protection_pointer = rx[1];
			if (rx_len >= 3U) {
				out->protection_condition = rx[2];
			}
		}
	}

	if (slix_type_has_signature(out->type)) {
		tx_len = slix_poller_prepare_request(out->parent.uid, SLIX_CMD_READ_SIGNATURE, false,
						     tx, sizeof(tx));
		if (tx_len == 0U) {
			return -EINVAL;
		}

		ret = slix_poller_transceive_crc(mut, tx, tx_len, rx, &rx_len);
		if (ret == 0) {
			size_t copy = rx_len;

			if (copy > SLIX_SIGNATURE_SIZE) {
				copy = SLIX_SIGNATURE_SIZE;
			}
			(void)memcpy(out->signature, rx, copy);
		}
	}

	return 0;
}
