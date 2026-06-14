/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ultralight Tier A — model serialize/deserialize (cookbook §14.2 / §5.2).
 */

#include "protocols/ultralight/ultralight.h"

#include "Ultralight_11_mock.h"
#include "Ultralight_21_mock.h"
#include "Ntag216_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static ultralight_data_t s_data;
static ultralight_data_t s_copy; /* Static to avoid ~1KB stack allocation */
static uint8_t s_out[ULTRALIGHT_MAX_SERIALIZED];
static uint8_t s_round[ULTRALIGHT_MAX_SERIALIZED];

ZTEST(ultralight_model, test_model_reset)
{
	ultralight_data_reset(&s_data);
	s_data.pages_total = 20U;
	ultralight_data_reset(&s_data);
	zassert_equal(s_data.pages_total, 0U);
	zassert_equal(s_data.type, UL_TYPE_UNKNOWN);
}

ZTEST(ultralight_model, test_serialize_roundtrip_ul11_golden)
{
	size_t out_len = 0U;
	int ret;

	(void)memset(&s_copy, 0, sizeof(s_copy));

	ret = ultralight_deserialize(&s_data, ultralight_Ultralight_11_model,
				     ULTRALIGHT_ULTRALIGHT_11_MODEL_LEN);
	zassert_ok(ret);

	ret = ultralight_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_equal(out_len, ULTRALIGHT_ULTRALIGHT_11_MODEL_LEN);
	zassert_mem_equal(s_out, ultralight_Ultralight_11_model, out_len);

	ret = ultralight_deserialize(&s_copy, s_out, out_len);
	zassert_ok(ret);
	zassert_equal(s_copy.pages_total, s_data.pages_total);
	zassert_mem_equal(s_copy.pages, s_data.pages,
			  (size_t)s_data.pages_total * ULTRALIGHT_PAGE_SIZE);
}

ZTEST(ultralight_model, test_deserialize_bad_version)
{
	uint8_t bad[] = {0x02U, 0x01U, 0x14U, 0x00U};

	ultralight_data_reset(&s_data);
	zassert_equal(ultralight_deserialize(&s_data, bad, sizeof(bad)), -EBADMSG);
}

ZTEST(ultralight_model, test_deserialize_truncated)
{
	ultralight_data_reset(&s_data);
	zassert_equal(ultralight_deserialize(&s_data, ultralight_Ultralight_11_model,
					     ULTRALIGHT_ULTRALIGHT_11_MODEL_LEN - 1U),
		      -EBADMSG);
}

ZTEST(ultralight_model, test_serialize_enospc)
{
	size_t out_len = 0U;

	zassert_ok(ultralight_deserialize(&s_data, ultralight_Ultralight_11_model,
					  ULTRALIGHT_ULTRALIGHT_11_MODEL_LEN));
	zassert_equal(ultralight_serialize(&s_data, s_out, 4U, &out_len), -ENOSPC);
}

ZTEST(ultralight_model, test_serialize_null_out)
{
	size_t out_len = 0U;

	zassert_ok(ultralight_deserialize(&s_data, ultralight_Ultralight_11_model,
					  ULTRALIGHT_ULTRALIGHT_11_MODEL_LEN));
	zassert_equal(ultralight_serialize(&s_data, NULL, sizeof(s_out), &out_len), -EINVAL);
}

ZTEST(ultralight_model, test_deserialize_null_in)
{
	ultralight_data_reset(&s_data);
	zassert_equal(ultralight_deserialize(&s_data, NULL, 0U), -EINVAL);
}

ZTEST(ultralight_model, test_persist_id_stable)
{
	zassert_equal(ultralight_persist_id(), NFC_PERSIST_ID_ULTRALIGHT);
}

ZTEST(ultralight_model, test_tlv_scan_uri_ntag216)
{
	const uint8_t *msg = NULL;
	size_t msg_len = 0U;
	int ret;

	zassert_ok(ultralight_deserialize(&s_data, ultralight_Ntag216_model,
					  ULTRALIGHT_NTAG216_MODEL_LEN));
	ret = ultralight_extract_ndef(&s_data, &msg, &msg_len);
	zassert_ok(ret);
	zassert_not_null(msg);
	zassert_true(msg_len > 0U);
	zassert_equal(msg[0], 0xD1U);
}

ZTEST(ultralight_model, test_pages_total_for_type)
{
	zassert_equal(ultralight_pages_total_for_type(UL_TYPE_UL11), 20U);
	zassert_equal(ultralight_pages_total_for_type(UL_TYPE_UL21), 41U);
	zassert_equal(ultralight_pages_total_for_type(UL_TYPE_NTAG216), 231U);
	zassert_equal(ultralight_pages_total_for_type(UL_TYPE_MFUL_C), 48U);
}

ZTEST(ultralight_model, test_fixture_ul21_bin)
{
	size_t out_len = 0U;
	int ret;

	ret = ultralight_deserialize(&s_data, ultralight_Ultralight_21_model,
				     ULTRALIGHT_ULTRALIGHT_21_MODEL_LEN);
	zassert_ok(ret);
	zassert_equal(s_data.pages_total, 41U);

	ret = ultralight_serialize(&s_data, s_round, sizeof(s_round), &out_len);
	zassert_ok(ret);
	zassert_mem_equal(s_round, ultralight_Ultralight_21_model, out_len);
}

ZTEST_SUITE(ultralight_model, NULL, NULL, NULL, NULL, NULL);
