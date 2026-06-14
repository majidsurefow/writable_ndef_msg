/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_classic/mf_classic_poller.c
 */

#include "protocols/classic/classic_poller.h"

#include <errno.h>

int classic_poller_detect(const nfc_reader_session_t *session)
{
	ARG_UNUSED(session);

	return -ENOTSUP;
}

int classic_poller_read(const nfc_reader_session_t *session, classic_data_t *out)
{
	ARG_UNUSED(session);
	ARG_UNUSED(out);

	return -ENOTSUP;
}
