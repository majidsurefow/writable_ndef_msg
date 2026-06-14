/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared nfc_reader_session mock for nfc_reader unit tests.
 * Configurable via set/clear functions so multiple test suites can coexist.
 * Also provides stubs for reader engine functions needed by nfc_applet_scan.c.
 */

#include "reader/nfc_reader_engine.h"
#include "run/nfc_stack_workq.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

static nfc_reader_session_t s_mock_session;
static bool s_session_active;

void nfc_reader_session_mock_set(const nfc_reader_session_t *session)
{
	if (session != NULL) {
		s_mock_session = *session;
		s_session_active = true;
	}
}

void nfc_reader_session_mock_clear(void)
{
	memset(&s_mock_session, 0, sizeof(s_mock_session));
	s_session_active = false;
}

const nfc_reader_session_t *nfc_reader_session_get(void)
{
	return s_session_active ? &s_mock_session : NULL;
}

/* Stubs for nfc_applet_scan.c dependencies */

bool nfc_reader_scan_busy(void)
{
	return false;
}

int nfc_reader_discover_once(k_timeout_t timeout, nfc_transport_tag_info_t *info)
{
	ARG_UNUSED(timeout);
	ARG_UNUSED(info);
	return -ENOTSUP;
}

/* Work queue stub - returns NULL (tests don't submit work) */
struct k_work_q *nfc_stack_wq_get(void)
{
	return NULL;
}
