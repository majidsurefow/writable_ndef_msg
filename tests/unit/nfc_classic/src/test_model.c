/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Classic Tier A — model serialize/deserialize (cookbook §14.2 / §5.3).
 */

#include "protocols/classic/classic.h"

#include "MfClassic_1K_4b_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static classic_data_t s_data;
static uint8_t s_out[4096];

ZTEST(classic_model, test_model_reset)
{
	classic_data_reset(&s_data);
	s_data.type = CLASSIC_TYPE_1K;
	classic_data_reset(&s_data);
	zassert_equal(s_data.type, CLASSIC_TYPE_UNKNOWN);
}

ZTEST(classic_model, test_persist_id)
{
	zassert_equal(classic_persist_id(), NFC_PERSIST_ID_CLASSIC);
	zassert_equal(classic_service_get()->persist_id, NFC_PERSIST_ID_CLASSIC);
}

ZTEST(classic_model, test_serialize_roundtrip_1k_golden)
{
	size_t out_len = 0U;
	classic_data_t copy;
	int ret;

	ret = classic_deserialize(&s_data, classic_MfClassic_1K_4b_model,
				  CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN);
	zassert_ok(ret);
	zassert_equal(s_data.type, CLASSIC_TYPE_1K);
	zassert_equal(s_data.uid_len, 4U);

	ret = classic_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_equal(out_len, CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN);
	zassert_mem_equal(s_out, classic_MfClassic_1K_4b_model, out_len);

	ret = classic_deserialize(&copy, s_out, out_len);
	zassert_ok(ret);
	zassert_equal(copy.type, s_data.type);
	zassert_equal(copy.uid_len, s_data.uid_len);
	zassert_mem_equal(copy.uid, s_data.uid, copy.uid_len);
	zassert_mem_equal(copy.blocks[0], s_data.blocks[0], CLASSIC_BLOCK_SIZE);
}

ZTEST(classic_model, test_deserialize_bad_version)
{
	uint8_t bad[] = {0x02U, CLASSIC_TYPE_1K};

	classic_data_reset(&s_data);
	zassert_equal(classic_deserialize(&s_data, bad, sizeof(bad)), -EBADMSG);
}

ZTEST(classic_model, test_deserialize_truncated)
{
	classic_data_reset(&s_data);
	zassert_equal(classic_deserialize(&s_data, classic_MfClassic_1K_4b_model,
					 CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN - 1U),
		      -EBADMSG);
}

ZTEST(classic_model, test_serialize_enospc)
{
	size_t out_len = 0U;

	zassert_ok(classic_deserialize(&s_data, classic_MfClassic_1K_4b_model,
				       CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN));
	zassert_equal(classic_serialize(&s_data, s_out, 8U, &out_len), -ENOSPC);
}

ZTEST_SUITE(classic_model, NULL, NULL, NULL, NULL, NULL);
