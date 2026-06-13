/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC reader shell commands — Gate 0 scan.
 */

#include "reader/nfc_reader_engine.h"

#include <stdlib.h>
#include <zephyr/shell/shell.h>

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

SHELL_STATIC_SUBCMD_SET_CREATE(nfc_reader_cmds,
			       SHELL_CMD(scan, NULL,
					 "Async tag scan (discovery start + wait; non-blocking)",
					 cmd_nfc_reader_scan),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(nfc_cmds,
			       SHELL_CMD(reader, &nfc_reader_cmds, "NFC reader commands", NULL),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nfc, &nfc_cmds, "NFC stack commands", NULL);
