/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * ISO15693-3 Tier A — model serialize/deserialize (cookbook §14.2 / §5.8).
 */

#include "protocols/iso15693_3/iso15693_3.h"

#include "Slix_cap_default_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static iso15693_3_data_t s_data;
static iso15693_3_data_t s_copy;
static uint8_t s_out[4096];

ZTEST(iso15693_model, test_model_reset)
{
	iso15693_3_data_reset(&s_data);
	s_data.block_count = 4U;
	iso15693_3_data_reset(&s_data);
	zassert_equal(s_data.block_count, 0U);
}

ZTEST(iso15693_model, test_persist_id)
{
	zassert_equal(iso15693_3_persist_id(), NFC_PERSIST_ID_ISO15693);
	zassert_equal(iso15693_3_service_get()->persist_id, NFC_PERSIST_ID_ISO15693);
}

ZTEST(iso15693_model, test_uid_byte_reverse_golden)
{
	static const uint8_t expected_uid[ISO15693_UID_SIZE] = {
		0xE0U, 0x04U, 0x01U, 0x08U, 0x49U, 0xD0U, 0xDCU, 0x81U,
	};
	int ret;

	ret = iso15693_3_deserialize(&s_data, iso15693_Slix_cap_default_model,
				       ISO15693_SLIX_CAP_DEFAULT_MODEL_LEN);
	zassert_ok(ret);
	zassert_mem_equal(s_data.uid, expected_uid, ISO15693_UID_SIZE);
}

ZTEST(iso15693_model, test_block_security_roundtrip)
{
	size_t out_len = 0U;
	int ret;

	ret = iso15693_3_deserialize(&s_data, iso15693_Slix_cap_default_model,
				       ISO15693_SLIX_CAP_DEFAULT_MODEL_LEN);
	zassert_ok(ret);
	zassert_equal(s_data.block_count, 80U);

	ret = iso15693_3_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);

	ret = iso15693_3_deserialize(&s_copy, s_out, out_len);
	zassert_ok(ret);
	zassert_mem_equal(s_copy.block_security, s_data.block_security, s_data.block_count);
}

ZTEST(iso15693_model, test_serialize_roundtrip_golden)
{
	size_t out_len = 0U;
	int ret;

	ret = iso15693_3_deserialize(&s_data, iso15693_Slix_cap_default_model,
				       ISO15693_SLIX_CAP_DEFAULT_MODEL_LEN);
	zassert_ok(ret);
	zassert_equal(s_data.dsfid, 0x01U);
	zassert_equal(s_data.afi, 0x3DU);
	zassert_equal(s_data.block_size, 4U);

	ret = iso15693_3_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_equal(out_len, ISO15693_SLIX_CAP_DEFAULT_MODEL_LEN);
	zassert_mem_equal(s_out, iso15693_Slix_cap_default_model, out_len);

	ret = iso15693_3_deserialize(&s_copy, s_out, out_len);
	zassert_ok(ret);
	zassert_equal(s_copy.block_count, s_data.block_count);
}

ZTEST(iso15693_model, test_deserialize_bad_version)
{
	uint8_t bad[32] = {0x02U};

	zassert_equal(iso15693_3_deserialize(&s_data, bad, sizeof(bad)), -EBADMSG);
}

ZTEST_SUITE(iso15693_model, NULL, NULL, NULL, NULL, NULL);
