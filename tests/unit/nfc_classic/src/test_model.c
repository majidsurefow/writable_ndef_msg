/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Classic Tier A — model scaffold (cookbook §14.13 Step 3 / §5.3).
 */

#include "protocols/classic/classic.h"

#include <zephyr/ztest.h>

static classic_data_t s_data;

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

ZTEST(classic_model, test_serialize_not_implemented)
{
	size_t out_len = 0U;
	uint8_t out[16];

	classic_data_reset(&s_data);
	s_data.type = CLASSIC_TYPE_1K;
	zassert_equal(classic_serialize(&s_data, out, sizeof(out), &out_len), -ENOTSUP);
}

ZTEST(classic_model, test_deserialize_not_implemented)
{
	static const uint8_t blob[] = {CLASSIC_FORMAT_VERSION, CLASSIC_TYPE_1K};

	classic_data_reset(&s_data);
	zassert_equal(classic_deserialize(&s_data, blob, sizeof(blob)), -ENOTSUP);
}

ZTEST_SUITE(classic_model, NULL, NULL, NULL, NULL, NULL);
