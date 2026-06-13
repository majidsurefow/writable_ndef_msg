/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc emulate — load slot + stack start (Flipper: Emulate → Load from file).
 */

#include "applets/nfc_applet.h"

#include "applets/nfc_applet_policy.h"
#include "nfc_stack/nfc_stack.h"
#include "reader/nfc_reader_engine.h"
#include "store/nfc_store.h"

#include <errno.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(nfc_reader, CONFIG_LOG_DEFAULT_LEVEL);

int nfc_applet_emulate(const char *slot, nfc_profile_t profile)
{
#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
	uint8_t persist_id;
	uint8_t store_flags;
	int ret;

	ARG_UNUSED(profile);

	if (slot == NULL) {
		return -EINVAL;
	}

	nfc_reader_session_end();

	ret = nfc_stack_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		return ret;
	}

	ret = nfc_store_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		return ret;
	}

	ret = nfc_store_peek_entry_meta(slot, &persist_id, &store_flags);
	if (ret != 0) {
		LOG_WRN("Emulate slot \"%s\" metadata unavailable: %d", slot, ret);
		return ret;
	}

	ret = nfc_applet_check_emulate(persist_id, store_flags);
	if (ret != 0) {
		LOG_WRN("Emulate not supported for slot \"%s\": %d", slot, ret);
		return ret;
	}

	ret = nfc_stack_load(slot);
	if (ret != 0) {
		return ret;
	}

	ret = nfc_stack_start(NULL);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("Emulating slot \"%s\"", slot);
	return 0;
#else
	ARG_UNUSED(slot);
	ARG_UNUSED(profile);
	return -ENOTSUP;
#endif
}
