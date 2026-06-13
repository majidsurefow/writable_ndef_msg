/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nfc_session_mock.h"
#include "nfc_virtual_rf.h"

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

static const nfc_session_mock_step_t *s_steps;
static size_t s_step_count;
static size_t s_step_idx;
static bool s_active = true;

static struct {
	uint8_t buf[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t len;
} s_tx_log[NFC_SESSION_MOCK_MAX_TX];
static size_t s_tx_count;
static uint32_t s_session_end_count;

void nfc_session_mock_reset(void)
{
	s_steps = NULL;
	s_step_count = 0U;
	s_step_idx = 0U;
	s_active = true;
	s_tx_count = 0U;
	s_session_end_count = 0U;
}

void nfc_session_mock_load(const nfc_session_mock_step_t *steps, size_t count)
{
	s_steps = steps;
	s_step_count = count;
	s_step_idx = 0U;
}

void nfc_session_mock_set_active(bool active)
{
	s_active = active;
}

size_t nfc_session_mock_tx_count(void)
{
	return s_tx_count;
}

uint32_t nfc_session_mock_session_end_count(void)
{
	return s_session_end_count;
}

void nfc_reader_session_end(void)
{
	s_session_end_count++;
}

int nfc_session_mock_get_tx(size_t index, const uint8_t **tx, size_t *tx_len)
{
	if ((tx == NULL) || (tx_len == NULL)) {
		return -EINVAL;
	}

	if (index >= s_tx_count) {
		return -ENOENT;
	}

	*tx = s_tx_log[index].buf;
	*tx_len = s_tx_log[index].len;
	return 0;
}

int nfc_reader_session_transceive(nfc_reader_session_t *session, const uint8_t *tx,
				  size_t tx_len, uint8_t *rx, size_t rx_max, size_t *rx_len,
				  k_timeout_t timeout)
{
	ARG_UNUSED(timeout);

	if ((session == NULL) || (rx == NULL) || (rx_len == NULL)) {
		return -EINVAL;
	}

	if (!s_active || !session->active) {
		return -EINVAL;
	}

	if ((tx != NULL) && (tx_len > 0U) && (s_tx_count < NFC_SESSION_MOCK_MAX_TX)) {
		size_t copy_len = tx_len;

		if (copy_len > sizeof(s_tx_log[0].buf)) {
			copy_len = sizeof(s_tx_log[0].buf);
		}

		(void)memcpy(s_tx_log[s_tx_count].buf, tx, copy_len);
		s_tx_log[s_tx_count].len = copy_len;
		s_tx_count++;
	}

	if (nfc_virtual_rf_is_enabled()) {
		return nfc_virtual_rf_transceive(session, tx, tx_len, rx, rx_max, rx_len, timeout);
	}

	if ((s_steps == NULL) || (s_step_idx >= s_step_count)) {
		return -EIO;
	}

	const nfc_session_mock_step_t *step = &s_steps[s_step_idx];

	s_step_idx++;

	if (step->err != 0) {
		return step->err;
	}

	if ((step->rx == NULL) || (step->rx_len == 0U)) {
		*rx_len = 0U;
		return 0;
	}

	if (step->rx_len > rx_max) {
		return -ENOSPC;
	}

	(void)memcpy(rx, step->rx, step->rx_len);
	*rx_len = step->rx_len;
	return 0;
}
