/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ultralight Tier B fixture umbrella — Flipper-derived mock scripts.
 */

#ifndef TESTS_FIXTURES_ULTRALIGHT_ULTRALIGHT_FIXTURE_H_
#define TESTS_FIXTURES_ULTRALIGHT_ULTRALIGHT_FIXTURE_H_

#include "Ultralight_11_mock.h"
#include "Ultralight_21_mock.h"
#include "Ultralight_C_mock.h"
#include "Ntag213_locked_mock.h"
#include "Ntag215_mock.h"
#include "Ntag216_mock.h"

#include "nfc_session_mock.h"

#include <zephyr/sys/util.h>

/* Detect probe: READ page 0 — first page-read step after type/extras prefix. */
#define ULTRALIGHT_DETECT_STEP_INDEX_UL11  1U
#define ULTRALIGHT_DETECT_STEP_INDEX_UL21  1U
#define ULTRALIGHT_DETECT_STEP_INDEX_UL_C  4U
#define ULTRALIGHT_DETECT_STEP_INDEX_NTAG213 1U
#define ULTRALIGHT_DETECT_STEP_INDEX_NTAG215 1U
#define ULTRALIGHT_DETECT_STEP_INDEX_NTAG216 1U

static inline void ultralight_fixture_load_detect(const nfc_session_mock_step_t *steps,
						  size_t detect_idx)
{
	nfc_session_mock_step_t script[1];

	script[0] = steps[detect_idx];
	nfc_session_mock_load(script, ARRAY_SIZE(script));
}

#endif /* TESTS_FIXTURES_ULTRALIGHT_ULTRALIGHT_FIXTURE_H_ */
