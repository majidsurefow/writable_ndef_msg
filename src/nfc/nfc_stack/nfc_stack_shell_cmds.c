/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC listen stack shell commands — Gate 3.
 */

#include "nfc_stack/nfc_stack.h"

#include <stdlib.h>
#include <string.h>

#include <zephyr/shell/shell.h>

static int cmd_nfc_stack_init(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = nfc_stack_init(NULL);
	if (ret == -EALREADY) {
		shell_print(sh, "Already initialized");
		return 0;
	}
	if (ret != 0) {
		shell_error(sh, "Init failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Stack initialized");
	return 0;
}

static int cmd_nfc_stack_start(const struct shell *sh, size_t argc, char **argv)
{
	nfc_uid_t uid = { 0 };
	int ret;

	if (argc >= 2) {
		size_t hex_len = strlen(argv[1]);
		size_t uid_len = hex_len / 2U;

		if ((hex_len == 0U) || ((hex_len % 2U) != 0U) ||
		    (uid_len > sizeof(uid.bytes))) {
			shell_error(sh, "Usage: nfc stack start [uid_hex]");
			return -EINVAL;
		}

		for (size_t i = 0U; i < uid_len; i++) {
			char byte_str[3] = { argv[1][i * 2U], argv[1][(i * 2U) + 1U], '\0' };
			char *end;
			unsigned long val = strtoul(byte_str, &end, 16);

			if ((end == byte_str) || (*end != '\0') || (val > 0xFFUL)) {
				shell_error(sh, "Invalid UID hex");
				return -EINVAL;
			}
			uid.bytes[i] = (uint8_t)val;
		}
		uid.len = (uint8_t)uid_len;
		ret = nfc_stack_start(&uid);
	} else {
		ret = nfc_stack_start(NULL);
	}

	if (ret != 0) {
		shell_error(sh, "Start failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Listen started");
	return 0;
}

static int cmd_nfc_stack_stop(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = nfc_stack_stop();
	if (ret != 0) {
		shell_error(sh, "Stop failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Listen stopped");
	return 0;
}

static int cmd_nfc_stack_load(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc stack load <tag>");
		return -EINVAL;
	}

	ret = nfc_stack_load(argv[1]);
	if (ret != 0) {
		shell_error(sh, "Load failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Loaded tag \"%s\"", argv[1]);
	return 0;
}

static const struct shell_static_entry shell_nfc_stack_cmds_entries[] = {
	SHELL_CMD(init, NULL, "Initialize listen stack", cmd_nfc_stack_init),
	SHELL_CMD(start, NULL, "Start listen (optional uid_hex)", cmd_nfc_stack_start),
	SHELL_CMD(stop, NULL, "Stop listen", cmd_nfc_stack_stop),
	SHELL_CMD(load, NULL, "Load .card tag before start", cmd_nfc_stack_load),
	SHELL_SUBCMD_SET_END
};

const union shell_cmd_entry nfc_stack_cmds = {
	.entry = shell_nfc_stack_cmds_entries
};

#if !IS_ENABLED(CONFIG_NFC_READER_SHELL)
SHELL_STATIC_SUBCMD_SET_CREATE(nfc_cmds,
			       SHELL_CMD(stack, &nfc_stack_cmds, "NFC listen stack commands", NULL),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nfc, &nfc_cmds, "NFC stack commands", NULL);
#endif
