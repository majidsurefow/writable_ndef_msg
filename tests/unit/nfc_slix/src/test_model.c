/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * SLIX Tier A — model serialize/deserialize (cookbook §14.2 / §5.9).
 */

#include "protocols/slix/slix.h"

#include "Slix_cap_default_mock.h"
#include "Slix_cap_missed_mock.h"
#include "Slix_cap_accept_all_pass_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static slix_data_t s_data;
static slix_data_t s_copy;
static uint8_t s_out[4096];

ZTEST(slix_model, test_model_reset)
{
	slix_data_reset(&s_data);
	s_data.parent.block_count = 4U;
	slix_data_reset(&s_data);
	zassert_equal(s_data.parent.block_count, 0U);
}

ZTEST(slix_model, test_persist_id)
{
	zassert_equal(slix_persist_id(), NFC_PERSIST_ID_SLIX);
	zassert_equal(slix_service_get()->persist_id, NFC_PERSIST_ID_SLIX);
}

ZTEST(slix_model, test_type_from_uid)
{
	static const uint8_t uid[ISO15693_UID_SIZE] = {
		0xE0U, 0x04U, 0x01U, 0x08U, 0x49U, 0xD0U, 0xDCU, 0x81U,
	};

	zassert_equal(slix_type_from_uid(uid), SLIX_TYPE_SLIX);
}

ZTEST(slix_model, test_serialize_roundtrip_default)
{
	size_t out_len = 0U;
	int ret;

	ret = slix_deserialize(&s_data, slix_Slix_cap_default_model, SLIX_SLIX_CAP_DEFAULT_MODEL_LEN);
	zassert_ok(ret);
	zassert_equal(s_data.capabilities, SLIX_CAP_DEFAULT);

	ret = slix_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_equal(out_len, SLIX_SLIX_CAP_DEFAULT_MODEL_LEN);
	zassert_mem_equal(s_out, slix_Slix_cap_default_model, out_len);

	ret = slix_deserialize(&s_copy, s_out, out_len);
	zassert_ok(ret);
	zassert_equal(s_copy.parent.block_count, s_data.parent.block_count);
}

ZTEST(slix_model, test_serialize_roundtrip_missed)
{
	size_t out_len = 0U;
	int ret;

	ret = slix_deserialize(&s_data, slix_Slix_cap_missed_model, SLIX_SLIX_CAP_MISSED_MODEL_LEN);
	zassert_ok(ret);
	zassert_equal(s_data.capabilities, SLIX_CAP_MISSED);

	ret = slix_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_mem_equal(s_out, slix_Slix_cap_missed_model, out_len);
}

ZTEST(slix_model, test_serialize_roundtrip_accept_all)
{
	size_t out_len = 0U;
	int ret;

	ret = slix_deserialize(&s_data, slix_Slix_cap_accept_all_pass_model,
			       SLIX_SLIX_CAP_ACCEPT_ALL_PASS_MODEL_LEN);
	zassert_ok(ret);
	zassert_equal(s_data.capabilities, SLIX_CAP_ACCEPT_ALL);

	ret = slix_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_mem_equal(s_out, slix_Slix_cap_accept_all_pass_model, out_len);
}

ZTEST(slix_model, test_deserialize_bad_magic)
{
	uint8_t bad[32] = {ISO15693_FORMAT_VERSION};

	zassert_equal(slix_deserialize(&s_data, bad, sizeof(bad)), -EBADMSG);
}

ZTEST_SUITE(slix_model, NULL, NULL, NULL, NULL, NULL);
