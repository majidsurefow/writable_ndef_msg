/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc scan — type-agnostic discover + print (Flipper: NFC → Detect Reader).
 */

#include "applets/nfc_applet.h"

#include "reader/nfc_reader_engine.h"

#include <errno.h>

#include <zephyr/shell/shell.h>

int nfc_applet_scan_start(k_timeout_t timeout)
{
	return nfc_reader_scan_start(timeout);
}

bool nfc_applet_scan_busy(void)
{
	return nfc_reader_scan_busy();
}

int nfc_applet_scan_wait(k_timeout_t deadline)
{
	int64_t end_ms = k_uptime_get() + (int64_t)k_ticks_to_ms_floor64(deadline.ticks);

	while (nfc_applet_scan_busy()) {
		if (k_uptime_get() >= end_ms) {
			return -ETIMEDOUT;
		}
		k_sleep(K_MSEC(50));
	}

	if (nfc_reader_session_get() == NULL) {
		return -ENOENT;
	}

	return 0;
}

void nfc_applet_scan_print(const struct shell *sh)
{
	const nfc_reader_session_t *session = nfc_reader_session_get();

	if ((sh == NULL) || (session == NULL)) {
		return;
	}

	if (session->tag.valid && session->tag.uid.len > 0U) {
		shell_print(sh, "UID (%u):", session->tag.uid.len);
		for (uint8_t i = 0U; i < session->tag.uid.len; i++) {
			shell_fprintf(sh, SHELL_NORMAL, " %02x", session->tag.uid.bytes[i]);
		}
		shell_print(sh, "");
	} else {
		shell_print(sh, "Tag detected (UID not reported)");
	}

	shell_print(sh, "Protocol: 0x%02x  Interface: 0x%02x  Tech: 0x%02x",
		    session->tag.protocol, session->tag.interface, session->tag.mode_tech);
}
