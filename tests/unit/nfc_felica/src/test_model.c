/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * FeliCa Tier A — model serialize/deserialize (cookbook §14.2 / §5.7).
 */

#include "protocols/felica/felica.h"

#include "Felica_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static felica_data_t s_data;
static uint8_t s_out[2048];

ZTEST(felica_model, test_model_reset)
{
	felica_data_reset(&s_data);
	s_data.blocks_total = 4U;
	felica_data_reset(&s_data);
	zassert_equal(s_data.blocks_total, 0U);
}

ZTEST(felica_model, test_persist_id)
{
	zassert_equal(felica_persist_id(), NFC_PERSIST_ID_FELICA);
	zassert_equal(felica_service_get()->persist_id, NFC_PERSIST_ID_FELICA);
}

ZTEST(felica_model, test_serialize_roundtrip_golden)
{
	size_t out_len = 0U;
	felica_data_t copy;
	int ret;

	ret = felica_deserialize(&s_data, felica_Felica_model, FELICA_FELICA_MODEL_LEN);
	zassert_ok(ret);
	zassert_equal(s_data.blocks_total, 28U);

	ret = felica_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_equal(out_len, FELICA_FELICA_MODEL_LEN);
	zassert_mem_equal(s_out, felica_Felica_model, out_len);

	ret = felica_deserialize(&copy, s_out, out_len);
	zassert_ok(ret);
	zassert_mem_equal(copy.idm, s_data.idm, FELICA_IDM_SIZE);
	zassert_mem_equal(copy.blocks[14].data, s_data.blocks[14].data, FELICA_BLOCK_DATA_SIZE);
}

ZTEST(felica_model, test_deserialize_bad_version)
{
	uint8_t bad[] = {0x01U};

	felica_data_reset(&s_data);
	zassert_equal(felica_deserialize(&s_data, bad, sizeof(bad)), -EBADMSG);
}

ZTEST(felica_model, test_serialize_enospc)
{
	size_t out_len = 0U;

	zassert_ok(felica_deserialize(&s_data, felica_Felica_model, FELICA_FELICA_MODEL_LEN));
	zassert_equal(felica_serialize(&s_data, s_out, 8U, &out_len), -ENOSPC);
}

ZTEST_SUITE(felica_model, NULL, NULL, NULL, NULL, NULL);
