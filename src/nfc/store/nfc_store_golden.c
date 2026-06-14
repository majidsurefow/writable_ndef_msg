/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "store/nfc_store_golden.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>

#if IS_ENABLED(CONFIG_NFC_STORE_GOLDENS)
#include "aliro_store_fixture.h"
#include "classic_store_fixture.h"
#include "desfire_store_fixture.h"
#include "emv_store_fixture.h"
#include "felica_store_fixture.h"
#include "slix_store_fixture.h"
#include "store_fixture.h"
#include "ultralight_store_fixture.h"

typedef struct {
	const char *name;
	const uint8_t *blob;
	size_t len;
} nfc_store_golden_entry_t;

static const nfc_store_golden_entry_t s_goldens[] = {
	{ "ndef_empty", store_fixture_ndef_empty_card, STORE_FIXTURE_NDEF_EMPTY_CARD_LEN },
	{ "ndef_uri_5byte", store_fixture_ndef_uri_5byte_card,
	  STORE_FIXTURE_NDEF_URI_5BYTE_CARD_LEN },
	{ "Ultralight_11", store_fixture_ultralight_11_card,
	  STORE_FIXTURE_ULTRALIGHT_11_CARD_LEN },
	{ "ultralight_11", store_fixture_ultralight_11_card,
	  STORE_FIXTURE_ULTRALIGHT_11_CARD_LEN },
	{ "MfClassic_1K_4b", store_fixture_mfclassic_1k_4b_card,
	  STORE_FIXTURE_MFCLASSIC_1K_4B_CARD_LEN },
	{ "mfclassic_1k_4b", store_fixture_mfclassic_1k_4b_card,
	  STORE_FIXTURE_MFCLASSIC_1K_4B_CARD_LEN },
	{ "Felica", store_fixture_felica_card, STORE_FIXTURE_FELICA_CARD_LEN },
	{ "felica", store_fixture_felica_card, STORE_FIXTURE_FELICA_CARD_LEN },
	{ "Slix_cap_default", store_fixture_slix_cap_default_card,
	  STORE_FIXTURE_SLIX_CAP_DEFAULT_CARD_LEN },
	{ "slix_cap_default", store_fixture_slix_cap_default_card,
	  STORE_FIXTURE_SLIX_CAP_DEFAULT_CARD_LEN },
	{ "desfire", store_fixture_desfire_card, STORE_FIXTURE_DESFIRE_CARD_LEN },
	{ "Desfire", store_fixture_desfire_card, STORE_FIXTURE_DESFIRE_CARD_LEN },
	{ "emv", store_fixture_emv_card, STORE_FIXTURE_EMV_CARD_LEN },
	{ "Emv", store_fixture_emv_card, STORE_FIXTURE_EMV_CARD_LEN },
	{ "aliro", store_fixture_aliro_card, STORE_FIXTURE_ALIRO_CARD_LEN },
	{ "Aliro", store_fixture_aliro_card, STORE_FIXTURE_ALIRO_CARD_LEN },
	{ NULL, NULL, 0U },
};

int nfc_store_golden_lookup(const char *name, const uint8_t **blob_out, size_t *len_out)
{
	if ((name == NULL) || (blob_out == NULL) || (len_out == NULL)) {
		return -EINVAL;
	}

	for (size_t i = 0U; s_goldens[i].name != NULL; i++) {
		if (strcmp(s_goldens[i].name, name) == 0) {
			*blob_out = s_goldens[i].blob;
			*len_out = s_goldens[i].len;
			return 0;
		}
	}

	return -ENOENT;
}

#else /* !CONFIG_NFC_STORE_GOLDENS */

int nfc_store_golden_lookup(const char *name, const uint8_t **blob_out, size_t *len_out)
{
	ARG_UNUSED(name);
	ARG_UNUSED(blob_out);
	ARG_UNUSED(len_out);

	return -ENOTSUP;
}

#endif /* CONFIG_NFC_STORE_GOLDENS */
