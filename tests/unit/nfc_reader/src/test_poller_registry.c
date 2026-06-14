/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Poller registry table — clone_fn wiring (cookbook §14.13 Step 2).
 */

#include "reader/nfc_reader_poller_registry.h"
#include "nfc_reader_session_mock.h"

#include <string.h>

#include <zephyr/ztest.h>

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	nfc_reader_session_mock_clear();
}

ZTEST_SUITE(nfc_reader_registry, NULL, NULL, before_each, NULL, NULL);

ZTEST(nfc_reader_registry, test_pollers_table_clone_hooks)
{
	const nfc_reader_poller_entry_t *pollers = nfc_reader_pollers_get();
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
	bool found_ndef = false;
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ULTRALIGHT)
	bool found_ultralight = false;
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_CLASSIC)
	bool found_classic = false;
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_FELICA)
	bool found_felica = false;
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_SLIX)
	bool found_slix = false;
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ISO15693_3)
	bool found_iso15693_3 = false;
#endif

	for (const nfc_reader_poller_entry_t *e = pollers; e->detect != NULL; e++) {
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
		if (strcmp(e->name, "ndef") == 0) {
			found_ndef = true;
			zassert_not_null(e->clone_fn, "NDEF poller needs clone_fn");
		}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ULTRALIGHT)
		if (strcmp(e->name, "ultralight") == 0) {
			found_ultralight = true;
			zassert_not_null(e->clone_fn, "Ultralight poller needs clone_fn");
		}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_CLASSIC)
		if (strcmp(e->name, "classic") == 0) {
			found_classic = true;
			zassert_not_null(e->clone_fn, "Classic poller needs clone_fn");
		}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_FELICA)
		if (strcmp(e->name, "felica") == 0) {
			found_felica = true;
			zassert_not_null(e->clone_fn, "FeliCa poller needs clone_fn");
		}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_SLIX)
		if (strcmp(e->name, "slix") == 0) {
			found_slix = true;
			zassert_not_null(e->clone_fn, "SLIX poller needs clone_fn");
		}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ISO15693_3)
		if (strcmp(e->name, "iso15693_3") == 0) {
			found_iso15693_3 = true;
			zassert_not_null(e->clone_fn, "ISO15693-3 poller needs clone_fn");
		}
#endif
	}

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
	zassert_true(found_ndef, "NDEF poller registered");
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ULTRALIGHT)
	zassert_true(found_ultralight, "Ultralight poller registered");
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_CLASSIC)
	zassert_true(found_classic, "Classic poller registered");
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_FELICA)
	zassert_true(found_felica, "FeliCa poller registered");
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_SLIX)
	zassert_true(found_slix, "SLIX poller registered");
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ISO15693_3)
	zassert_true(found_iso15693_3, "ISO15693-3 poller registered");
#endif
}

ZTEST(nfc_reader_registry, test_pollers_run_no_session)
{
	zassert_equal(nfc_reader_pollers_run("slot"), -ENODEV);
}
