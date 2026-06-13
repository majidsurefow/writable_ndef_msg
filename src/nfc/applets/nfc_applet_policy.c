/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "applets/nfc_applet_policy.h"

#include "hal/nfc_transport.h"
#include "protocols/ndef/ndef.h"
#include "store/nfc_store.h"

#include <errno.h>

int nfc_applet_caps_for_card(uint8_t persist_id, uint8_t store_flags, nfc_applet_caps_t *caps)
{
	const nfc_transport_caps_t *hal;

	if (caps == NULL) {
		return -EINVAL;
	}

	hal = nfc_transport_get_capabilities();
	*caps = NFC_APPLET_CAP_READ | NFC_APPLET_CAP_VERIFY;

	if ((store_flags & NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE) == 0U) {
		return 0;
	}

	if ((persist_id == NFC_PERSIST_ID_NDEF) && (hal != NULL) &&
	    ((hal->roles & NFC_ROLE_LISTEN) != 0U)) {
		*caps |= NFC_APPLET_CAP_EMULATE;
	}

	return 0;
}

int nfc_applet_check_emulate(uint8_t persist_id, uint8_t store_flags)
{
	nfc_applet_caps_t caps;
	int ret;

	ret = nfc_applet_caps_for_card(persist_id, store_flags, &caps);
	if (ret != 0) {
		return ret;
	}

	if ((caps & NFC_APPLET_CAP_EMULATE) == 0U) {
		return -ENOTSUP;
	}

	return 0;
}
