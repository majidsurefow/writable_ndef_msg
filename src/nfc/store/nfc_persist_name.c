/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "store/nfc_persist_name.h"

#include "router/service.h"

const char *nfc_persist_id_name(uint8_t persist_id)
{
	switch (persist_id) {
	case NFC_PERSIST_ID_NDEF:
		return "ndef";
	case NFC_PERSIST_ID_DESFIRE:
		return "desfire";
	case NFC_PERSIST_ID_ULTRALIGHT:
		return "ultralight";
	case NFC_PERSIST_ID_EMV:
		return "emv";
	case NFC_PERSIST_ID_ALIRO:
		return "aliro";
	case NFC_PERSIST_ID_CLASSIC:
		return "classic";
	case NFC_PERSIST_ID_FELICA:
		return "felica";
	case NFC_PERSIST_ID_ISO15693:
		return "iso15693";
	case NFC_PERSIST_ID_SLIX:
		return "slix";
	case NFC_PERSIST_ID_ST25TB:
		return "st25tb";
	default:
		return "unknown";
	}
}
