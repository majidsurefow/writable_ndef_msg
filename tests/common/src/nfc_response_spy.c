/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nfc_response_spy.h"

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

static uint8_t s_last[NFC_TRANSPORT_MAX_RESPONSE_LEN];
static size_t s_last_len;
static size_t s_call_count;

void nfc_response_spy_reset(void)
{
	s_last_len = 0U;
	s_call_count = 0U;
	(void)memset(s_last, 0, sizeof(s_last));
}

int nfc_response_spy_last(const uint8_t **buf, size_t *len)
{
	if ((buf == NULL) || (len == NULL)) {
		return -EINVAL;
	}

	*buf = s_last;
	*len = s_last_len;
	return 0;
}

size_t nfc_response_spy_call_count(void)
{
	return s_call_count;
}

int nfc_transport_send_response(const uint8_t *buf, size_t len)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	if (len > NFC_TRANSPORT_MAX_RESPONSE_LEN) {
		return -EINVAL;
	}

	(void)memcpy(s_last, buf, len);
	s_last_len = len;
	s_call_count++;
	return 0;
}
