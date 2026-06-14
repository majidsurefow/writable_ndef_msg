/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Poller registry table — clone_fn wiring (cookbook §14.13 Step 2).
 */

#include "reader/nfc_reader_poller_registry.h"

#include <string.h>

#include <zephyr/ztest.h>

const nfc_reader_session_t *nfc_reader_session_get(void)
{
	return NULL;
}

ZTEST_SUITE(nfc_reader_registry, NULL, NULL, NULL, NULL, NULL);

ZTEST(nfc_reader_registry, test_pollers_table_clone_hooks)
{
	const nfc_reader_poller_entry_t *pollers = nfc_reader_pollers_get();
	bool found_ndef = false;
	bool found_ultralight = false;

	for (const nfc_reader_poller_entry_t *e = pollers; e->detect != NULL; e++) {
		if (strcmp(e->name, "ndef") == 0) {
			found_ndef = true;
			zassert_not_null(e->clone_fn, "NDEF poller needs clone_fn");
		}
		if (strcmp(e->name, "ultralight") == 0) {
			found_ultralight = true;
			zassert_not_null(e->clone_fn, "Ultralight poller needs clone_fn");
		}
	}

	zassert_true(found_ndef, "NDEF poller registered");
	zassert_true(found_ultralight, "Ultralight poller registered");
}

ZTEST(nfc_reader_registry, test_pollers_run_no_session)
{
	zassert_equal(nfc_reader_pollers_run("slot"), -ENODEV);
}
