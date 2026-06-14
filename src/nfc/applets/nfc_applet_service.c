/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc applet service — shared headless helpers (Layer-1, no struct shell).
 */

#include "applets/nfc_applet_service.h"

#include "store/nfc_persist_name.h"
#include "store/nfc_store.h"

#include <errno.h>

int nfc_applet_get_card_meta(const char *slot, nfc_applet_card_meta_t *out)
{
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	if ((slot == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	out->persist_id = 0U;
	out->flags = 0U;
	out->protocol_name = NULL;
	out->present = false;

	ret = nfc_store_peek_entry_meta(slot, &persist_id, &flags);
	if (ret != 0) {
		return ret;
	}

	out->persist_id = persist_id;
	out->flags = flags;
	out->protocol_name = nfc_persist_id_name(persist_id);
	out->present = true;
	return 0;
}
