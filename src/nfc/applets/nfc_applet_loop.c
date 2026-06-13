/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc loop — read → emulate → verify HIL sign-off (Gate 4 shell orchestration).
 */

#include "applets/nfc_applet.h"

#include "store/nfc_store.h"

#include <errno.h>

#include <zephyr/shell/shell.h>

int nfc_applet_loop(const struct shell *sh, const char *slot, k_timeout_t timeout)
{
	k_timeout_t wait_deadline = K_MSEC((int32_t)k_ticks_to_ms_floor64(timeout.ticks) + 5000);
	int ret;
	int verify_ret;

	if ((sh == NULL) || (slot == NULL)) {
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_NFC_STORE)
	(void)nfc_store_init(NULL);
#endif

	shell_print(sh, "loop: read \"%s\"…", slot);
	ret = nfc_applet_read_start(slot, timeout);
	if (ret != 0) {
		shell_error(sh, "read failed: %d", ret);
		return ret;
	}

	ret = nfc_applet_read_wait(wait_deadline);
	if (ret != 0) {
		shell_error(sh, "read timed out: %d", ret);
		return ret;
	}

	shell_print(sh, "loop: emulate \"%s\"…", slot);
	ret = nfc_applet_emulate(slot, NFC_PROFILE_NDEF);
	if (ret != 0) {
		shell_error(sh, "emulate failed: %d", ret);
		return ret;
	}

	shell_print(sh, "loop: verify \"%s\" (present emulated tag)…", slot);
	ret = nfc_applet_verify_start(slot, timeout);
	if (ret != 0) {
		shell_error(sh, "verify start failed: %d", ret);
		return ret;
	}

	ret = nfc_applet_verify_wait(wait_deadline);
	if (ret != 0) {
		shell_error(sh, "verify timed out: %d", ret);
		return ret;
	}

	verify_ret = nfc_applet_verify_last_result();
	if (verify_ret == 0) {
		shell_print(sh, "PASS");
		return 0;
	}

	shell_error(sh, "FAIL (%d)", verify_ret);
	return verify_ret;
}
