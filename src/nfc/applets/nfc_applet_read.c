/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc read — one-shot scan + poller walk + save (Flipper: Read → Save).
 */

#include "applets/nfc_applet.h"

#include "reader/nfc_reader_engine.h"

#include <errno.h>

int nfc_applet_read_start(const char *slot, k_timeout_t timeout)
{
	return nfc_reader_read_start(slot, timeout);
}

bool nfc_applet_read_busy(void)
{
	return nfc_reader_read_busy();
}

int nfc_applet_read_wait(k_timeout_t deadline)
{
	int64_t end_ms = k_uptime_get() + (int64_t)k_ticks_to_ms_floor64(deadline.ticks);

	while (nfc_applet_read_busy()) {
		if (k_uptime_get() >= end_ms) {
			return -ETIMEDOUT;
		}
		k_sleep(K_MSEC(50));
	}

	return 0;
}
