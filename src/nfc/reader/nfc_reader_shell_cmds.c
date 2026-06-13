/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC reader shell commands — Gate 0 scan + Gate 4 applet registration.
 */

#include "reader/nfc_reader_engine.h"

#if IS_ENABLED(CONFIG_NFC_APPLETS_SHELL)
#include "applets/nfc_applet.h"
#endif

#if IS_ENABLED(CONFIG_NFC_STORE)
#include "store/nfc_store.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK_SHELL)
extern const union shell_cmd_entry nfc_stack_cmds;
#endif

#if IS_ENABLED(CONFIG_NFC_APPLETS_SHELL)
int cmd_nfc_scan(const struct shell *sh, size_t argc, char **argv);
int cmd_nfc_read(const struct shell *sh, size_t argc, char **argv);
int cmd_nfc_emulate(const struct shell *sh, size_t argc, char **argv);
int cmd_nfc_verify(const struct shell *sh, size_t argc, char **argv);
int cmd_nfc_loop(const struct shell *sh, size_t argc, char **argv);
int nfc_applet_shell_read_alias(const struct shell *sh, size_t argc, char **argv);
#endif

static int cmd_nfc_reader_scan(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	unsigned long timeout_ms = CONFIG_NFC_READER_SCAN_TIMEOUT_MS;
	k_timeout_t timeout;

	ARG_UNUSED(argv);

	if (argc >= 2) {
		char *end;

		timeout_ms = strtoul(argv[1], &end, 10);
		if (end == argv[1] || *end != '\0' || timeout_ms == 0UL) {
			shell_error(sh, "Usage: nfc reader scan [timeout_ms]");
			return -EINVAL;
		}
	}

	timeout = K_MSEC((int32_t)timeout_ms);
	ret = nfc_reader_scan_start(timeout);
	if (ret == -EBUSY) {
		shell_warn(sh, "Scan already in progress");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Scan start failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Scan started (%lu ms timeout); watch logs for UID/protocol",
		    timeout_ms);
	return 0;
}

#if IS_ENABLED(CONFIG_NFC_STORE)
static int nfc_reader_shell_save_cb(const char *tag, const uint8_t *blob, size_t len,
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

static int cmd_nfc_reader_clone(const struct shell *sh, size_t argc, char **argv)
{
#if IS_ENABLED(CONFIG_NFC_APPLETS_SHELL)
	return nfc_applet_shell_read_alias(sh, argc, argv);
#else
	int ret;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc reader clone <name>");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_NFC_STORE)
	(void)nfc_store_init(NULL);
	(void)nfc_store_register_save_cb(nfc_reader_shell_save_cb, (void *)sh);
#endif

	ret = nfc_reader_clone_start(argv[1]);
	if (ret == -EBUSY) {
		shell_warn(sh, "Clone already in progress");
		return ret;
	}
	if (ret == -ENODEV) {
		shell_error(sh, "No active session — run \"nfc reader scan\" first");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Clone start failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Clone started for tag \"%s\"; watch logs for @@NFCDUMP@@", argv[1]);
	return 0;
#endif
}

SHELL_STATIC_SUBCMD_SET_CREATE(nfc_reader_cmds,
			       SHELL_CMD(scan, NULL,
					 "Async tag scan (discovery start + wait; non-blocking)",
					 cmd_nfc_reader_scan),
			       SHELL_CMD(clone, NULL,
					 "Clone active tag to .card blob (alias: nfc read)",
					 cmd_nfc_reader_clone),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(nfc_cmds,
#if IS_ENABLED(CONFIG_NFC_APPLETS_SHELL)
			       SHELL_CMD(scan, NULL, "Discover tag (UID, protocol, tech)", cmd_nfc_scan),
			       SHELL_CMD(read, NULL, "One-shot scan + save to slot", cmd_nfc_read),
#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
			       SHELL_CMD(emulate, NULL, "Load slot + start listen", cmd_nfc_emulate),
			       SHELL_CMD(loop, NULL, "read → emulate → verify", cmd_nfc_loop),
#endif
			       SHELL_CMD(verify, NULL, "Poll tag + diff vs stored slot", cmd_nfc_verify),
#endif
#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK_SHELL)
			       SHELL_CMD(stack, &nfc_stack_cmds, "NFC listen stack commands", NULL),
#endif
			       SHELL_CMD(reader, &nfc_reader_cmds, "NFC reader commands", NULL),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nfc, &nfc_cmds, "NFC stack commands", NULL);
