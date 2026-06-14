/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC product applet shell — Gate 4 top-level nfc scan/read/emulate/verify/loop.
 */

#include "applets/nfc_applet.h"

#if IS_ENABLED(CONFIG_NFC_STORE)
#include "store/nfc_store.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <zephyr/shell/shell.h>

static k_timeout_t nfc_applet_parse_timeout(int argc, char **argv, int idx,
					    k_timeout_t default_timeout)
{
	if (argc <= idx) {
		return default_timeout;
	}

	char *end;
	unsigned long timeout_ms = strtoul(argv[idx], &end, 10);

	if ((end == argv[idx]) || (*end != '\0') || (timeout_ms == 0UL)) {
		return default_timeout;
	}

	return K_MSEC((int32_t)timeout_ms);
}

int cmd_nfc_scan(const struct shell *sh, size_t argc, char **argv)
{
	k_timeout_t timeout = nfc_applet_parse_timeout((int)argc, argv, 1,
						       K_MSEC(CONFIG_NFC_READER_SCAN_TIMEOUT_MS));
	k_timeout_t wait_deadline = K_MSEC((int32_t)k_ticks_to_ms_floor64(timeout.ticks) + 2000);
	int ret;

	ret = nfc_applet_scan_start(timeout);
	if (ret == -EBUSY) {
		shell_warn(sh, "Scan already in progress");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Scan start failed: %d", ret);
		return ret;
	}

	ret = nfc_applet_scan_wait(wait_deadline);
	if (ret == -ETIMEDOUT) {
		shell_error(sh, "Scan timed out");
		return ret;
	}
	if (ret == -ENOENT) {
		shell_warn(sh, "No tag detected");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Scan failed: %d", ret);
		return ret;
	}

	nfc_applet_scan_print(sh);
	shell_print(sh, "Scan complete — session active for nfc reader clone");
	return 0;
}

#if IS_ENABLED(CONFIG_NFC_STORE)
static int nfc_applet_shell_save_cb(const char *tag, const uint8_t *blob, size_t len,
				    void *user_ctx)
{
	const struct shell *sh = user_ctx;
	size_t i;

	if (sh == NULL) {
		return -ENODEV;
	}

	shell_print(sh, "@@NFCDUMP@@ %s", tag);
	for (i = 0U; i < len; i++) {
		shell_fprintf(sh, SHELL_NORMAL, "%02x", blob[i]);
	}
	shell_print(sh, "");
	return 0;
}
#endif

static int nfc_applet_shell_read(const struct shell *sh, const char *slot, int argc, char **argv)
{
	k_timeout_t timeout = nfc_applet_parse_timeout(argc, argv, 2,
						       K_MSEC(CONFIG_NFC_READER_SCAN_TIMEOUT_MS));
	k_timeout_t wait_deadline = K_MSEC((int32_t)k_ticks_to_ms_floor64(timeout.ticks) + 5000);
	int ret;

#if IS_ENABLED(CONFIG_NFC_STORE) && !IS_ENABLED(CONFIG_NFC_STORE_RAM)
	(void)nfc_store_init(NULL);
	(void)nfc_store_register_save_cb(nfc_applet_shell_save_cb, (void *)sh);
#endif

	ret = nfc_applet_read_start(slot, timeout);
	if (ret == -EBUSY) {
		shell_warn(sh, "Read already in progress");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Read start failed: %d", ret);
		return ret;
	}

	ret = nfc_applet_read_wait(wait_deadline);
	if (ret != 0) {
		shell_error(sh, "Read timed out or failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Read complete for slot \"%s\"", slot);
	return 0;
}

int cmd_nfc_read(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Usage: nfc read <slot> [timeout_ms]");
		return -EINVAL;
	}

	return nfc_applet_shell_read(sh, argv[1], (int)argc, argv);
}

int cmd_nfc_emulate(const struct shell *sh, size_t argc, char **argv)
{
	nfc_profile_t profile = NFC_PROFILE_NDEF;
	int ret;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc emulate <slot> [profile]");
		return -EINVAL;
	}

	if (argc >= 3) {
		if (strcmp(argv[2], "ndef") == 0) {
			profile = NFC_PROFILE_NDEF;
		} else {
			shell_error(sh, "Unknown profile \"%s\" (supported: ndef)", argv[2]);
			return -EINVAL;
		}
	}

	ret = nfc_applet_emulate(argv[1], profile);
	if (ret == -ENOTSUP) {
		shell_error(sh, "Emulate not supported for this card type");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Emulate failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Emulating slot \"%s\"", argv[1]);
	return 0;
}

int cmd_nfc_verify(const struct shell *sh, size_t argc, char **argv)
{
	k_timeout_t timeout = nfc_applet_parse_timeout((int)argc, argv, 2,
						       K_MSEC(CONFIG_NFC_READER_SCAN_TIMEOUT_MS));
	k_timeout_t wait_deadline = K_MSEC((int32_t)k_ticks_to_ms_floor64(timeout.ticks) + 5000);
	int ret;
	int verify_ret;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc verify <slot> [timeout_ms]");
		return -EINVAL;
	}

	ret = nfc_applet_verify_start(argv[1], timeout);
	if (ret == -EBUSY) {
		shell_warn(sh, "Verify already in progress");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Verify start failed: %d", ret);
		return ret;
	}

	ret = nfc_applet_verify_wait(wait_deadline);
	if (ret != 0) {
		shell_error(sh, "Verify timed out: %d", ret);
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

int cmd_nfc_loop(const struct shell *sh, size_t argc, char **argv)
{
	k_timeout_t timeout = nfc_applet_parse_timeout((int)argc, argv, 2,
						       K_MSEC(CONFIG_NFC_READER_SCAN_TIMEOUT_MS));

	if (argc < 2) {
		shell_error(sh, "Usage: nfc loop <slot> [timeout_ms]");
		return -EINVAL;
	}

	return nfc_applet_loop(sh, argv[1], timeout);
}

int nfc_applet_shell_read_alias(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Usage: nfc reader clone <slot> [timeout_ms]");
		return -EINVAL;
	}

	return nfc_applet_shell_read(sh, argv[1], (int)argc, argv);
}