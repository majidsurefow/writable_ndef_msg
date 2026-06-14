/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC product applet shell — Layer-2 adapter ONLY (LOCKED 2026-06-14, Phase A).
 *
 * This is the only place a `struct shell *` is allowed for the applets. It
 * parses argv, calls the headless Layer-1 service (nfc_applet_service.h), and
 * renders the returned result structs / errno. It owns NO orchestration or
 * discovery logic.
 *
 * Locked command surface:
 *   nfc scan start|stop  → continuous discovery (prints each tag)
 *   nfc read <slot>      → one-shot scan + clone to slot
 *   nfc emulate <slot>   → load slot + start listen   (NFC_LISTEN_STACK)
 *   nfc emulate golden <name> → store import + emulate (shortcut)
 *   nfc loop <slot>      → read → emulate → check       (NFC_LISTEN_STACK)
 *   nfc check <slot>     → field diff vs stored slot     (DK)
 *
 * DEFERRED (no code): Flipper macro recorder — out of scope for this stack.
 */

#include "applets/nfc_applet_service.h"

#if IS_ENABLED(CONFIG_NFC_STORE)
#include "applets/nfc_applet_policy.h"
#include "store/nfc_store.h"
#if IS_ENABLED(CONFIG_NFC_STORE_RAM)
#include "store/nfc_store_golden.h"
#include "store/nfc_store_ram.h"
#endif
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

#if IS_ENABLED(CONFIG_NFC_STORE)
static void nfc_applet_shell_print_protocol(const struct shell *sh, const char *slot)
{
	nfc_applet_card_meta_t meta;

	if (nfc_applet_get_card_meta(slot, &meta) == 0) {
		shell_print(sh, "Protocol: %s (persist_id=0x%02x)", meta.protocol_name,
			    meta.persist_id);
	}
}
#endif

/* --- nfc scan start|stop : continuous discovery, prints each tag --- */

static void nfc_applet_scan_tag_cb(const nfc_transport_tag_info_t *info, void *ctx)
{
	const struct shell *sh = ctx;

	if ((sh == NULL) || (info == NULL)) {
		return;
	}

	if (info->valid && info->uid.len > 0U) {
		shell_print(sh, "UID (%u):", info->uid.len);
		for (uint8_t i = 0U; i < info->uid.len; i++) {
			shell_fprintf(sh, SHELL_NORMAL, " %02x", info->uid.bytes[i]);
		}
		shell_print(sh, "");
	} else {
		shell_print(sh, "Tag detected (UID not reported)");
	}

	shell_print(sh, "Protocol: 0x%02x  Interface: 0x%02x  Tech: 0x%02x", info->protocol,
		    info->interface, info->mode_tech);
}

static int cmd_nfc_scan_start(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = nfc_applet_discover_start(nfc_applet_scan_tag_cb, (void *)sh);
	if (ret == -EBUSY) {
		shell_warn(sh, "Scan already running");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Scan start failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Scan started — discovering tags; run \"nfc scan stop\" to end");
	return 0;
}

static int cmd_nfc_scan_stop(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = nfc_applet_discover_stop();
	if (ret == -EALREADY) {
		shell_warn(sh, "Scan not running");
		return ret;
	}

	shell_print(sh, "Scan stopped");
	return 0;
}

static const struct shell_static_entry nfc_applet_scan_entries[] = {
	SHELL_CMD(start, NULL, "Start continuous discovery; prints each tag",
		  cmd_nfc_scan_start),
	SHELL_CMD(stop, NULL, "Stop continuous discovery", cmd_nfc_scan_stop),
	SHELL_SUBCMD_SET_END
};

const union shell_cmd_entry nfc_applet_scan_cmds = {
	.entry = nfc_applet_scan_entries
};

/* --- nfc read <slot> : one-shot scan + clone --- */

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

int cmd_nfc_read(const struct shell *sh, size_t argc, char **argv)
{
	k_timeout_t timeout;
	k_timeout_t wait_deadline;
	const char *slot;
	int ret;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc read <slot> [timeout_ms]");
		return -EINVAL;
	}

	slot = argv[1];
	timeout = nfc_applet_parse_timeout((int)argc, argv, 2,
					   K_MSEC(CONFIG_NFC_READER_SCAN_TIMEOUT_MS));
	wait_deadline = K_MSEC((int32_t)k_ticks_to_ms_floor64(timeout.ticks) + 5000);

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
#if IS_ENABLED(CONFIG_NFC_STORE)
	nfc_applet_shell_print_protocol(sh, slot);
#endif
	return 0;
}

int cmd_nfc_emulate(const struct shell *sh, size_t argc, char **argv)
{
	nfc_profile_t profile = NFC_PROFILE_NDEF;
	const char *slot;
	int ret = 0;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc emulate <slot> [profile]");
		shell_error(sh, "       nfc emulate golden <name>");
		return -EINVAL;
	}

	if (strcmp(argv[1], "golden") == 0) {
#if IS_ENABLED(CONFIG_NFC_STORE_RAM) && IS_ENABLED(CONFIG_NFC_STORE_GOLDENS)
		const uint8_t *blob;
		size_t len;

		if (argc < 3) {
			shell_error(sh, "Usage: nfc emulate golden <name>");
			return -EINVAL;
		}

		ret = nfc_store_golden_lookup(argv[2], &blob, &len);
		if (ret != 0) {
			shell_error(sh, "Unknown golden \"%s\" (%d)", argv[2], ret);
			return ret;
		}

		ret = nfc_store_ram_import(argv[2], blob, len);
		if (ret != 0) {
			shell_error(sh, "Golden import failed: %d", ret);
			return ret;
		}

		slot = argv[2];
#else
		shell_error(sh, "Golden emulate requires CONFIG_NFC_STORE_RAM and CONFIG_NFC_STORE_GOLDENS");
		return -ENOTSUP;
#endif
	} else {
		slot = argv[1];

		if (argc >= 3) {
			if (strcmp(argv[2], "ndef") == 0) {
				profile = NFC_PROFILE_NDEF;
			} else {
				shell_error(sh, "Unknown profile \"%s\" (supported: ndef)", argv[2]);
				return -EINVAL;
			}
		}
	}

#if IS_ENABLED(CONFIG_NFC_STORE)
	{
		nfc_applet_card_meta_t meta;

		if (nfc_applet_get_card_meta(slot, &meta) == 0) {
			ret = nfc_applet_check_emulate(meta.persist_id, meta.flags);
			if (ret == -ENOTSUP) {
				shell_warn(sh, "Slot \"%s\" is clone-only (%s)", slot,
					   meta.protocol_name);
				return ret;
			}
		}
	}
#endif

	ret = nfc_applet_emulate(slot, profile);
	if (ret == -ENOTSUP) {
		shell_error(sh, "Emulate not supported for this card type");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Emulate failed: %d", ret);
		return ret;
	}

	shell_print(sh, "Emulating slot \"%s\"", slot);
#if IS_ENABLED(CONFIG_NFC_STORE)
	nfc_applet_shell_print_protocol(sh, slot);
#endif
	return 0;
}

/* --- nfc check <slot> : field diff vs stored slot (DK; was nfc verify) --- */

int cmd_nfc_check(const struct shell *sh, size_t argc, char **argv)
{
	k_timeout_t timeout = nfc_applet_parse_timeout((int)argc, argv, 2,
						       K_MSEC(CONFIG_NFC_READER_SCAN_TIMEOUT_MS));
	k_timeout_t wait_deadline = K_MSEC((int32_t)k_ticks_to_ms_floor64(timeout.ticks) + 5000);
	int ret;
	int check_ret;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc check <slot> [timeout_ms]");
		return -EINVAL;
	}

	ret = nfc_applet_verify_start(argv[1], timeout);
	if (ret == -EBUSY) {
		shell_warn(sh, "Check already in progress");
		return ret;
	}
	if (ret != 0) {
		shell_error(sh, "Check start failed: %d", ret);
		return ret;
	}

	ret = nfc_applet_verify_wait(wait_deadline);
	if (ret != 0) {
		shell_error(sh, "Check timed out: %d", ret);
		return ret;
	}

	check_ret = nfc_applet_verify_last_result();
	if (check_ret == 0) {
		shell_print(sh, "PASS");
		return 0;
	}

	shell_error(sh, "FAIL (%d)", check_ret);
	return check_ret;
}

/* --- nfc loop <slot> : thin adapter over nfc_applet_loop_run --- */

struct nfc_applet_loop_log_ctx {
	const struct shell *sh;
	const char *slot;
};

static void nfc_applet_loop_shell_log(void *ctx, const char *stage, int code)
{
	const struct nfc_applet_loop_log_ctx *c = ctx;

	if (c == NULL) {
		return;
	}

	if (code == 0) {
		shell_print(c->sh, "loop: %s \"%s\"…", stage, c->slot);
	} else {
		shell_error(c->sh, "loop: %s failed: %d", stage, code);
	}
}

int cmd_nfc_loop(const struct shell *sh, size_t argc, char **argv)
{
	k_timeout_t timeout = nfc_applet_parse_timeout((int)argc, argv, 2,
						       K_MSEC(CONFIG_NFC_READER_SCAN_TIMEOUT_MS));
	struct nfc_applet_loop_log_ctx log_ctx;
	nfc_applet_loop_result_t result;
	int ret;

	if (argc < 2) {
		shell_error(sh, "Usage: nfc loop <slot> [timeout_ms]");
		return -EINVAL;
	}

	log_ctx.sh = sh;
	log_ctx.slot = argv[1];

	ret = nfc_applet_loop_run(argv[1], timeout, nfc_applet_loop_shell_log, &log_ctx, &result);
	if (ret == 0) {
		shell_print(sh, "PASS");
		return 0;
	}

	shell_error(sh, "FAIL (%d)", ret);
	return ret;
}
