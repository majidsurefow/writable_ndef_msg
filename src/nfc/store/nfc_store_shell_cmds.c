/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc store * shell — import, save, load (wave6 DECISION-ST-8 subset).
 */

#include "store/nfc_persist_name.h"
#include "store/nfc_store.h"
#include "store/nfc_store_golden.h"
#include "store/nfc_store_ram.h"

#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
#include "nfc_stack/nfc_stack.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/shell/shell.h>

#if IS_ENABLED(CONFIG_NFC_STORE_RAM_SHELL)

static int nfc_store_shell_parse_hex(const struct shell *sh, const char *hex, uint8_t *out,
				     size_t out_max, size_t *out_len)
{
	size_t hex_len;
	size_t byte_len;
	char byte_str[3];

	if ((hex == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	hex_len = strlen(hex);
	if ((hex_len == 0U) || ((hex_len % 2U) != 0U)) {
		shell_error(sh, "Hex length must be non-zero and even");
		return -EINVAL;
	}

	byte_len = hex_len / 2U;
	if (byte_len > out_max) {
		shell_error(sh, "Hex blob too large (max %zu bytes)", out_max);
		return -ENOSPC;
	}

	for (size_t i = 0U; i < byte_len; i++) {
		char *end;

		byte_str[0] = hex[i * 2U];
		byte_str[1] = hex[(i * 2U) + 1U];
		byte_str[2] = '\0';

		unsigned long val = strtoul(byte_str, &end, 16);

		if ((end == byte_str) || (*end != '\0') || (val > 0xFFUL)) {
			shell_error(sh, "Invalid hex at offset %zu", i * 2U);
			return -EINVAL;
		}

		out[i] = (uint8_t)val;
	}

	*out_len = byte_len;
	return 0;
}

static int nfc_store_shell_print_meta(const struct shell *sh, const char *slot)
{
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	ret = nfc_store_peek_entry_meta(slot, &persist_id, &flags);
	if (ret != 0) {
		return ret;
	}

	shell_print(sh, "Protocol: %s (persist_id=0x%02x flags=0x%02x)", nfc_persist_id_name(persist_id),
		    persist_id, flags);
	return 0;
}

static int cmd_nfc_store_import(const struct shell *sh, size_t argc, char **argv)
{
	const uint8_t *blob;
	size_t len;
	int ret;

#if !IS_ENABLED(CONFIG_NFC_STORE_RAM)
	shell_error(sh, "Import requires CONFIG_NFC_STORE_RAM");
	return -ENOTSUP;
#else
	if (argc < 3) {
		shell_error(sh, "Usage: nfc store import <slot> <hex>");
		shell_error(sh, "       nfc store import golden <name>");
		return -EINVAL;
	}

	if (strcmp(argv[1], "golden") == 0) {
		if (argc < 3) {
			shell_error(sh, "Usage: nfc store import golden <name>");
			return -EINVAL;
		}

#if IS_ENABLED(CONFIG_NFC_STORE_GOLDENS)
		ret = nfc_store_golden_lookup(argv[2], &blob, &len);
		if (ret != 0) {
			shell_error(sh, "Unknown golden \"%s\" (%d)", argv[2], ret);
			return ret;
		}

		ret = nfc_store_ram_import(argv[2], blob, len);
		if (ret != 0) {
			shell_error(sh, "Import failed: %d", ret);
			return ret;
		}

		shell_print(sh, "Imported golden \"%s\" (%zu bytes)", argv[2], len);
		(void)nfc_store_shell_print_meta(sh, argv[2]);
		return 0;
#else
		shell_error(sh, "Golden import requires CONFIG_NFC_STORE_GOLDENS");
		return -ENOTSUP;
#endif
	}

	{
		uint8_t staging[CONFIG_NFC_STORE_BLOB_SIZE];
		size_t staging_len;

		ret = nfc_store_shell_parse_hex(sh, argv[2], staging, sizeof(staging),
						&staging_len);
		if (ret != 0) {
			return ret;
		}

		ret = nfc_store_ram_import(argv[1], staging, staging_len);
		if (ret != 0) {
			shell_error(sh, "Import failed: %d", ret);
			return ret;
		}

		shell_print(sh, "Imported slot \"%s\" (%zu bytes)", argv[1], staging_len);
		(void)nfc_store_shell_print_meta(sh, argv[1]);
		return 0;
	}
#endif /* CONFIG_NFC_STORE_RAM */
}

static int cmd_nfc_store_save(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc store save <slot>");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
	ret = nfc_stack_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		shell_error(sh, "Stack init failed: %d", ret);
		return ret;
	}

	ret = nfc_stack_save(argv[1]);
	if (ret == -EBUSY) {
		shell_warn(sh, "Listen active — stop stack before save");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Save failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Saved slot \"%s\" from listen listeners", argv[1]);
	(void)nfc_store_shell_print_meta(sh, argv[1]);
	return 0;
#elif IS_ENABLED(CONFIG_NFC_STORE_RAM)
	ret = nfc_store_ram_init();
	if (ret != 0) {
		shell_error(sh, "RAM store init failed: %d", ret);
		return ret;
	}

	shell_warn(sh, "Reader-only: re-exporting RAM slot (no listener serialize)");
	return cmd_nfc_store_ram_dump(sh, argc, argv);
#else
	shell_error(sh, "Save not supported in this build");
	return -ENOTSUP;
#endif
}

static int cmd_nfc_store_load(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc store load <slot>");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
	ret = nfc_stack_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		shell_error(sh, "Stack init failed: %d", ret);
		return ret;
	}

	ret = nfc_stack_load(argv[1]);
	if (ret == -EBUSY) {
		shell_warn(sh, "Listen active — stop stack before load");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Load failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Loaded slot \"%s\" into listeners (not started)", argv[1]);
	(void)nfc_store_shell_print_meta(sh, argv[1]);
	return 0;
#else
	uint8_t persist_id;
	uint8_t flags;

	ret = nfc_store_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		shell_error(sh, "Store init failed: %d", ret);
		return ret;
	}

	ret = nfc_store_peek_entry_meta(argv[1], &persist_id, &flags);
	if (ret == 0) {
		shell_print(sh, "Slot \"%s\" present (reader-only peek; no listeners)", argv[1]);
		shell_print(sh, "Protocol: %s (persist_id=0x%02x flags=0x%02x)",
			    nfc_persist_id_name(persist_id), persist_id, flags);
		return 0;
	}

	shell_error(sh, "Load failed: %d", ret);
	return ret;
#endif
}

/* list/dump handlers live in nfc_store_ram.c */
#include "store/nfc_store_ram.h"

static const struct shell_static_entry nfc_store_shell_entries[] = {
	SHELL_CMD(list, NULL, "List occupied RAM store slots", cmd_nfc_store_ram_list),
	SHELL_CMD_ARG(dump, NULL, "Hex dump RAM slot blob: <slot>", cmd_nfc_store_ram_dump, 2, 0),
	SHELL_CMD_ARG(import, NULL, "Import hex or golden: <slot> <hex> | golden <name>",
		      cmd_nfc_store_import, 2, 1),
	SHELL_CMD_ARG(save, NULL, "Serialize listeners to slot: <slot>", cmd_nfc_store_save, 2, 0),
	SHELL_CMD_ARG(load, NULL, "Load slot into listeners: <slot>", cmd_nfc_store_load, 2, 0),
	SHELL_SUBCMD_SET_END
};

const union shell_cmd_entry nfc_store_cmds = {
	.entry = nfc_store_shell_entries
};

#endif /* CONFIG_NFC_STORE_RAM_SHELL */
