/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * DESFire Tier A — model helpers and serialize (cookbook §14.2 / §5.4).
 */

#include "desfire.h"

#include "Desfire_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static desfire_data_t s_data;
static uint8_t s_out[512];

ZTEST(desfire_model, test_model_reset)
{
	desfire_data_reset(&s_data);
	s_data.app_count = 1U;
	desfire_data_reset(&s_data);
	zassert_equal(s_data.app_count, 0U);
}

ZTEST(desfire_model, test_persist_id)
{
	zassert_equal(desfire_persist_id(), NFC_PERSIST_ID_DESFIRE);
}

ZTEST(desfire_model, test_rot_left_16)
{
	zassert_equal(desfire_rot_left_16(0x1234U, 1U), 0x2468U);
	zassert_equal(desfire_rot_left_16(0x8001U, 1U), 0x0003U);
}

ZTEST(desfire_model, test_access_free_read)
{
	zassert_true(desfire_access_free_read(0x0E0EU));
	zassert_false(desfire_access_free_read(0x0101U));
}

ZTEST(desfire_model, test_serialize_roundtrip_golden)
{
	size_t out_len = 0U;
	desfire_data_t copy;
	int ret;

	ret = desfire_deserialize(&s_data, desfire_Desfire_model, DESFIRE_DESFIRE_MODEL_LEN);
	zassert_ok(ret);
	zassert_equal(s_data.app_count, 1U);
	zassert_mem_equal(s_data.uid, &desfire_Desfire_model[15], DESFIRE_UID_SIZE);

	ret = desfire_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_equal(out_len, DESFIRE_DESFIRE_MODEL_LEN);
	zassert_mem_equal(s_out, desfire_Desfire_model, out_len);

	ret = desfire_deserialize(&copy, s_out, out_len);
	zassert_ok(ret);
	zassert_mem_equal(copy.apps[0].file_data[0], s_data.apps[0].file_data[0], 15U);
}

ZTEST(desfire_model, test_deserialize_bad_version)
{
	uint8_t bad[] = {0xFFU};

	desfire_data_reset(&s_data);
	zassert_equal(desfire_deserialize(&s_data, bad, sizeof(bad)), -EBADMSG);
}

ZTEST(desfire_model, test_build_version_bytes)
{
	uint8_t ver[DESFIRE_VERSION_TOTAL_BYTES];

	zassert_ok(desfire_deserialize(&s_data, desfire_Desfire_model, DESFIRE_DESFIRE_MODEL_LEN));
	desfire_build_version_bytes(&s_data, ver);
	zassert_equal(ver[0], 0x04U);
	zassert_equal(ver[DESFIRE_VERSION_SIZE], 0x04U);
	zassert_equal(ver[DESFIRE_VERSION_SIZE * 2U], 0x04U);
}

ZTEST_SUITE(desfire_model, NULL, NULL, NULL, NULL, NULL);
