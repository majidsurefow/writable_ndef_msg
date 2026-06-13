/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC transport HAL shell — config, stats, state.
 */

#include "hal/nfc_transport.h"

#include <zephyr/shell/shell.h>

static int cmd_nfc_transport_config(const struct shell *sh, size_t argc, char **argv)
{
	const nfc_transport_config_t *cfg;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	cfg = nfc_transport_get_config();
	shell_print(sh, "fwi: %u", cfg->fwi);
	return 0;
}

static int cmd_nfc_transport_stats(const struct shell *sh, size_t argc, char **argv)
{
	nfc_transport_stats_t stats;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = nfc_transport_get_stats(&stats);
	if (ret != 0) {
		shell_error(sh, "get_stats failed: %d", ret);
		return ret;
	}

	shell_print(sh, "discover_start: %u", stats.discover_start_count);
	shell_print(sh, "tag_detect:     %u", stats.tag_detect_count);
	shell_print(sh, "transceive:     %u", stats.transceive_count);
	shell_print(sh, "field_on:       %u", stats.field_on_count);
	shell_print(sh, "field_off:      %u", stats.field_off_count);
	shell_print(sh, "fragment_rx:    %u", stats.fragment_rx_count);
	shell_print(sh, "apdu_assembled: %u", stats.apdu_assembled_count);
	shell_print(sh, "frag_drop_buf:  %u", stats.frag_dropped_no_buffer);
	shell_print(sh, "apdu_oversized: %u", stats.apdu_oversized_count);
	shell_print(sh, "apdu_drop_cons: %u", stats.apdu_dropped_no_consumer);
	shell_print(sh, "response_tx:    %u", stats.response_tx_count);
	shell_print(sh, "errors:         %u (last %d)", stats.error_count, stats.last_error_code);
	return 0;
}

static int cmd_nfc_transport_state(const struct shell *sh, size_t argc, char **argv)
{
	nfc_transport_state_t st;
	const nfc_transport_caps_t *caps;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	st = nfc_transport_get_state();
	caps = nfc_transport_get_capabilities();

	shell_print(sh, "state: %d", (int)st);
	shell_print(sh, "roles: 0x%02x  technologies: 0x%08x  tier: %d", caps->roles,
		    caps->technologies, (int)caps->tier);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(nfc_transport_cmds,
			       SHELL_CMD(config, NULL, "Print HAL config", cmd_nfc_transport_config),
			       SHELL_CMD(stats, NULL, "Print HAL stats", cmd_nfc_transport_stats),
			       SHELL_CMD(state, NULL, "Print HAL state", cmd_nfc_transport_state),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nfc_transport, &nfc_transport_cmds, "NFC HAL control", NULL);
