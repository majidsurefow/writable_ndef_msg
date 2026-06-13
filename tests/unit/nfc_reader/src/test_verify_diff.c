/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — nfc verify diff (cookbook §14.6a).
 */

#include "applets/nfc_applet.h"
#include "protocols/ndef/ndef.h"

#include <string.h>

#include <zephyr/ztest.h>

static ndef_data_t s_expected;
static ndef_data_t s_actual;

static void fill_model(ndef_data_t *data, uint8_t nlen)
{
	ndef_data_reset(data);
	for (uint16_t i = 0U; i < NFC_NDEF_CC_LEN; i++) {
		data->cc[i] = (uint8_t)i;
	}
	data->cc_len = NFC_NDEF_CC_LEN;
	data->ndef_file[0] = 0U;
	data->ndef_file[1] = nlen;
	for (uint8_t i = 0U; i < nlen; i++) {
		data->ndef_file[2U + i] = (uint8_t)(0xA0U + i);
	}
	data->ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + nlen);
}

ZTEST_SUITE(nfc_reader_verify, NULL, NULL, NULL, NULL, NULL);

ZTEST(nfc_reader_verify, test_verify_diff_pass)
{
	nfc_uid_t uid_a = { .bytes = {0x04, 0x11, 0x22, 0x33}, .len = 4U };
	nfc_uid_t uid_b = uid_a;
	int ret;

	fill_model(&s_expected, 5U);
	s_actual = s_expected;

	ret = nfc_applet_verify_compare(&s_expected, &s_actual, &uid_a, &uid_b);
	zassert_ok(ret);
}

ZTEST(nfc_reader_verify, test_verify_diff_fail)
{
	int ret;

	fill_model(&s_expected, 5U);
	fill_model(&s_actual, 5U);
	s_actual.ndef_file[4] ^= 0xFFU;

	ret = nfc_applet_verify_compare(&s_expected, &s_actual, NULL, NULL);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(nfc_reader_verify, test_verify_diff_uid_fail)
{
	nfc_uid_t uid_a = { .bytes = {0x04, 0x11, 0x22, 0x33}, .len = 4U };
	nfc_uid_t uid_b = { .bytes = {0x04, 0x11, 0x22, 0x34}, .len = 4U };
	int ret;

	fill_model(&s_expected, 3U);
	s_actual = s_expected;

	ret = nfc_applet_verify_compare(&s_expected, &s_actual, &uid_a, &uid_b);
	zassert_equal(ret, -EBADMSG);
}
