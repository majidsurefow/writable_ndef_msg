/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC reader shell commands — DK primitives + top-level applet registration.
 *
 * DK primitives here: `nfc reader scan` (async discovery start) and
 * `nfc reader detect` (poller match on the active session). The product
 * `nfc reader clone` command was DROPPED (LOCKED 2026-06-14): use `nfc read`.
 */

#include "reader/nfc_reader_engine.h"
#include "reader/nfc_reader_poller_registry.h"

#include <stdlib.h>
#include <zephyr/shell/shell.h>

#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK_SHELL)
extern const union shell_cmd_entry nfc_stack_cmds;
#endif

#if IS_ENABLED(CONFIG_NFC_STORE_RAM) && IS_ENABLED(CONFIG_NFC_STORE_RAM_SHELL)
extern const union shell_cmd_entry nfc_store_cmds;
#endif

#if IS_ENABLED(CONFIG_NFC_APPLETS_SHELL)
extern const union shell_cmd_entry nfc_applet_scan_cmds;
int cmd_nfc_read(const struct shell *sh, size_t argc, char **argv);
int cmd_nfc_emulate(const struct shell *sh, size_t argc, char **argv);
int cmd_nfc_check(const struct shell *sh, size_t argc, char **argv);
int cmd_nfc_loop(const struct shell *sh, size_t argc, char **argv);
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

static int cmd_nfc_reader_detect(const struct shell *sh, size_t argc, char **argv)
{
	const nfc_reader_session_t *session;
	const char *poller_name;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	session = nfc_reader_session_get();
	if (session == NULL) {
		shell_error(sh, "No active session — run \"nfc reader scan\" first");
		return -ENODEV;
	}

	ret = nfc_reader_pollers_detect(session, &poller_name);
	if (ret != 0) {
		shell_warn(sh, "No poller matched active tag (%d)", ret);
		return ret;
	}

	shell_print(sh, "Poller: %s", poller_name);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(nfc_reader_cmds,
			       SHELL_CMD(scan, NULL,
					 "Async tag scan (discovery start + wait; non-blocking)",
					 cmd_nfc_reader_scan),
			       SHELL_CMD(detect, NULL,
					 "Detect poller on active session (no save)",
					 cmd_nfc_reader_detect),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(nfc_cmds,
#if IS_ENABLED(CONFIG_NFC_APPLETS_SHELL)
			       SHELL_CMD(scan, &nfc_applet_scan_cmds,
					 "Continuous discovery (start/stop)", NULL),
			       SHELL_CMD(read, NULL, "One-shot scan + save to slot", cmd_nfc_read),
#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
			       SHELL_CMD(emulate, NULL, "Load slot + start listen", cmd_nfc_emulate),
			       SHELL_CMD(loop, NULL, "read → emulate → check", cmd_nfc_loop),
#endif
			       SHELL_CMD(check, NULL, "Poll tag + diff vs stored slot (DK)",
					 cmd_nfc_check),
#endif
#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK_SHELL)
			       SHELL_CMD(stack, &nfc_stack_cmds, "NFC listen stack commands", NULL),
#endif
#if IS_ENABLED(CONFIG_NFC_STORE_RAM) && IS_ENABLED(CONFIG_NFC_STORE_RAM_SHELL)
			       SHELL_CMD(store, &nfc_store_cmds, "Store slots (list, import, save, load)", NULL),
#endif
			       SHELL_CMD(reader, &nfc_reader_cmds, "NFC reader commands", NULL),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nfc, &nfc_cmds, "NFC stack commands", NULL);
