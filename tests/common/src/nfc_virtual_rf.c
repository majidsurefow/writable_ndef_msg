/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nfc_virtual_rf.h"

#include "framing/apdu_types.h"
#include "nfc_response_spy.h"
#include "nfc_test_apdu.h"

#include <errno.h>
#include <string.h>

static const nfc_service_t *s_listener_svc;
static bool s_enabled;

static bool apdu_is_select_by_aid(const nfc_apdu_t *apdu)
{
	return (apdu->cla == NFC_CLA_ISO7816) && (apdu->ins == NFC_INS_SELECT) &&
	       (apdu->p1 == NFC_SELECT_P1_BY_AID);
}

bool nfc_virtual_rf_is_enabled(void)
{
	return s_enabled;
}

int nfc_virtual_rf_enable(const nfc_service_t *listener_svc)
{
	if (listener_svc == NULL) {
		return -EINVAL;
	}

	s_listener_svc = listener_svc;
	s_enabled = true;
	return 0;
}

void nfc_virtual_rf_disable(void)
{
	s_enabled = false;
	s_listener_svc = NULL;
}

int nfc_virtual_rf_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
			      uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout)
{
	uint8_t apdu_storage[128];
	nfc_apdu_t apdu;
	const uint8_t *resp = NULL;
	size_t resp_len = 0U;
	int ret;

	ARG_UNUSED(timeout);

	if ((session == NULL) || (rx == NULL) || (rx_len == NULL) || (s_listener_svc == NULL)) {
		return -EINVAL;
	}

	if (!s_enabled || !session->active) {
		return -EINVAL;
	}

	if ((tx == NULL) || (tx_len == 0U)) {
		return -EINVAL;
	}

	nfc_response_spy_reset();

	if (nfc_test_apdu_from_bytes(tx, tx_len, apdu_storage, sizeof(apdu_storage), &apdu) == 0) {
		if (apdu_is_select_by_aid(&apdu)) {
			if (s_listener_svc->on_select != NULL) {
				s_listener_svc->on_select(apdu.data, apdu.lc, s_listener_svc->user_ctx);
			}
		} else if (s_listener_svc->on_apdu != NULL) {
			s_listener_svc->on_apdu(&apdu, s_listener_svc->user_ctx);
		}
	} else if (s_listener_svc->on_tag_cmd != NULL) {
		s_listener_svc->on_tag_cmd(tx, tx_len, s_listener_svc->user_ctx);
	} else {
		return -ENOTSUP;
	}

	ret = nfc_response_spy_last(&resp, &resp_len);
	if (ret != 0) {
		return ret;
	}

	if (resp_len == 0U) {
		return -EIO;
	}

	if (resp_len > rx_max) {
		return -ENOSPC;
	}

	(void)memcpy(rx, resp, resp_len);
	*rx_len = resp_len;
	return 0;
}
