/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — nfc_store_ram import hex + golden name.
 */

#include "store/nfc_store.h"
#include "store_fixture.h"

#if IS_ENABLED(CONFIG_NFC_STORE_RAM)
#include "store/nfc_store_ram.h"

#if IS_ENABLED(CONFIG_NFC_STORE_GOLDENS)
#include "store/nfc_store_golden.h"
#endif

#include <string.h>

#include <zephyr/ztest.h>

static void *suite_setup(void)
{
	nfc_store_ram_reset();
	zassert_ok(nfc_store_ram_init());
	return NULL;
}

ZTEST_SUITE(nfc_reader_store_import, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(nfc_reader_store_import, test_store_ram_import_hex_blob)
{
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	ret = nfc_store_ram_import("hex1", store_fixture_ndef_uri_5byte_card,
				   STORE_FIXTURE_NDEF_URI_5BYTE_CARD_LEN);
	zassert_ok(ret);

	ret = nfc_store_peek_entry_meta("hex1", &persist_id, &flags);
	zassert_ok(ret);
	zassert_equal(persist_id, NFC_PERSIST_ID_NDEF);
}

#if IS_ENABLED(CONFIG_NFC_STORE_GOLDENS)

ZTEST(nfc_reader_store_import, test_store_ram_import_golden_name)
{
	const uint8_t *blob;
	size_t len;
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	ret = nfc_store_golden_lookup("ndef_empty", &blob, &len);
	zassert_ok(ret);
	zassert_equal(len, STORE_FIXTURE_NDEF_EMPTY_CARD_LEN);

	ret = nfc_store_ram_import("ndef_empty", blob, len);
	zassert_ok(ret);

	ret = nfc_store_peek_entry_meta("ndef_empty", &persist_id, &flags);
	zassert_ok(ret);
	zassert_equal(persist_id, NFC_PERSIST_ID_NDEF);
}

#endif /* CONFIG_NFC_STORE_GOLDENS */

#endif /* CONFIG_NFC_STORE_RAM */
