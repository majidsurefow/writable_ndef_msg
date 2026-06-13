/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Tier A — data model serialize/deserialize (cookbook §14.2).
 */

#include "protocols/ndef/ndef.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static ndef_data_t s_data;
static uint8_t s_out[1U + NFC_NDEF_CC_LEN + NFC_NDEF_NLEN_FIELD_LEN + CONFIG_NFC_NDEF_MAX_SIZE];
static uint8_t s_round[sizeof(s_out)];

static void fill_cc(ndef_data_t *data)
{
	for (uint16_t i = 0U; i < NFC_NDEF_CC_LEN; i++) {
		data->cc[i] = (uint8_t)(0xE1U + (i % 3U));
	}
	data->cc_len = NFC_NDEF_CC_LEN;
}

static void set_empty_ndef_file(ndef_data_t *data)
{
	data->ndef_file[0] = 0U;
	data->ndef_file[1] = 0U;
	data->ndef_file_len = NFC_NDEF_NLEN_FIELD_LEN;
}

static void set_uri_ndef_file(ndef_data_t *data)
{
	static const uint8_t uri[] = {0xD1U, 0x01U, 0x03U, 0x55U, 0x01U};

	data->ndef_file[0] = 0U;
	data->ndef_file[1] = (uint8_t)sizeof(uri);
	(void)memcpy(&data->ndef_file[2], uri, sizeof(uri));
	data->ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + sizeof(uri));
}

ZTEST(ndef_model, test_model_reset)
{
	fill_cc(&s_data);
	set_uri_ndef_file(&s_data);
	ndef_data_reset(&s_data);
	zassert_equal(s_data.cc_len, 0U);
	zassert_equal(s_data.ndef_file_len, 0U);
	zassert_equal(s_data.cc[0], 0U);
	zassert_equal(s_data.ndef_file[0], 0U);
}

ZTEST(ndef_model, test_serialize_empty)
{
	size_t out_len = 0U;
	int ret;

	ndef_data_reset(&s_data);
	fill_cc(&s_data);
	set_empty_ndef_file(&s_data);

	ret = ndef_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_equal(out_len, 1U + NFC_NDEF_CC_LEN + NFC_NDEF_NLEN_FIELD_LEN);
	zassert_equal(s_out[0], NFC_NDEF_FORMAT_VERSION);
	zassert_mem_equal(&s_out[1U + NFC_NDEF_CC_LEN], s_data.ndef_file, NFC_NDEF_NLEN_FIELD_LEN);
}

ZTEST(ndef_model, test_serialize_roundtrip)
{
	size_t out_len = 0U;
	ndef_data_t copy;
	int ret;

	ndef_data_reset(&s_data);
	fill_cc(&s_data);
	set_uri_ndef_file(&s_data);

	ret = ndef_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);

	ret = ndef_deserialize(&copy, s_out, out_len);
	zassert_ok(ret);
	zassert_mem_equal(copy.cc, s_data.cc, NFC_NDEF_CC_LEN);
	zassert_equal(copy.cc_len, s_data.cc_len);
	zassert_equal(copy.ndef_file_len, s_data.ndef_file_len);
	zassert_mem_equal(copy.ndef_file, s_data.ndef_file, s_data.ndef_file_len);
}

ZTEST(ndef_model, test_serialize_roundtrip_generated)
{
	size_t out_len = 0U;
	ndef_data_t copy;
	int ret;

	ndef_data_reset(&s_data);
	fill_cc(&s_data);
	s_data.ndef_file[0] = (uint8_t)(CONFIG_NFC_NDEF_MAX_SIZE >> 8);
	s_data.ndef_file[1] = (uint8_t)(CONFIG_NFC_NDEF_MAX_SIZE & 0xFFU);
	for (uint16_t i = 0U; i < CONFIG_NFC_NDEF_MAX_SIZE; i++) {
		s_data.ndef_file[2U + i] = (uint8_t)(i & 0xFFU);
	}
	s_data.ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + CONFIG_NFC_NDEF_MAX_SIZE);

	ret = ndef_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);

	ret = ndef_deserialize(&copy, s_out, out_len);
	zassert_ok(ret);
	zassert_mem_equal(&copy, &s_data, sizeof(copy));
}

ZTEST(ndef_model, test_deserialize_bad_version)
{
	uint8_t bad[1U + NFC_NDEF_CC_LEN + NFC_NDEF_NLEN_FIELD_LEN];

	(void)memset(bad, 0, sizeof(bad));
	bad[0] = 0x02U;

	ndef_data_reset(&s_data);
	zassert_equal(ndef_deserialize(&s_data, bad, sizeof(bad)), -EBADMSG);
}

ZTEST(ndef_model, test_deserialize_truncated)
{
	size_t out_len = 0U;
	int ret;

	ndef_data_reset(&s_data);
	fill_cc(&s_data);
	set_uri_ndef_file(&s_data);
	ret = ndef_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);
	zassert_true(out_len > 1U);

	ret = ndef_deserialize(&s_data, s_out, out_len - 1U);
	zassert_equal(ret, -EBADMSG);
}

ZTEST(ndef_model, test_deserialize_oversize_field)
{
	uint8_t blob[1U + NFC_NDEF_CC_LEN + NFC_NDEF_NLEN_FIELD_LEN + 1U];

	(void)memset(blob, 0, sizeof(blob));
	blob[0] = NFC_NDEF_FORMAT_VERSION;
	blob[1U + NFC_NDEF_CC_LEN] = 0xFFU;
	blob[2U + NFC_NDEF_CC_LEN] = 0xFFU;

	ndef_data_reset(&s_data);
	zassert_equal(ndef_deserialize(&s_data, blob, sizeof(blob)), -EBADMSG);
}

ZTEST(ndef_model, test_deserialize_bad_nlen)
{
	uint8_t blob[1U + NFC_NDEF_CC_LEN + NFC_NDEF_NLEN_FIELD_LEN];

	(void)memset(blob, 0, sizeof(blob));
	blob[0] = NFC_NDEF_FORMAT_VERSION;
	blob[1U + NFC_NDEF_CC_LEN] = 0x00U;
	blob[2U + NFC_NDEF_CC_LEN] = 0x05U;

	ndef_data_reset(&s_data);
	zassert_equal(ndef_deserialize(&s_data, blob, sizeof(blob)), -EBADMSG);
}

ZTEST(ndef_model, test_serialize_enospc)
{
	size_t out_len = 0U;

	ndef_data_reset(&s_data);
	fill_cc(&s_data);
	set_empty_ndef_file(&s_data);
	zassert_equal(ndef_serialize(&s_data, s_out, 1U, &out_len), -ENOSPC);
}

ZTEST(ndef_model, test_serialize_null_out)
{
	size_t out_len = 0U;

	ndef_data_reset(&s_data);
	fill_cc(&s_data);
	set_empty_ndef_file(&s_data);
	zassert_equal(ndef_serialize(&s_data, NULL, sizeof(s_out), &out_len), -EINVAL);
}

ZTEST(ndef_model, test_deserialize_null_in)
{
	ndef_data_reset(&s_data);
	zassert_equal(ndef_deserialize(&s_data, NULL, 0U), -EINVAL);
}

ZTEST(ndef_model, test_persist_id_stable)
{
	zassert_equal(ndef_persist_id(), NFC_PERSIST_ID_NDEF);
}

ZTEST(ndef_model, test_cc_preserved_on_roundtrip)
{
	size_t out_len = 0U;
	ndef_data_t copy;
	int ret;

	ndef_data_reset(&s_data);
	fill_cc(&s_data);
	set_empty_ndef_file(&s_data);

	ret = ndef_serialize(&s_data, s_out, sizeof(s_out), &out_len);
	zassert_ok(ret);

	ndef_data_reset(&copy);
	ret = ndef_deserialize(&copy, s_out, out_len);
	zassert_ok(ret);
	zassert_mem_equal(copy.cc, s_data.cc, NFC_NDEF_CC_LEN);
}

ZTEST(ndef_model, test_fixture_empty_bin)
{
	static const uint8_t golden[] = {
		0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
		0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	};
	size_t out_len = 0U;
	int ret;

	ndef_data_reset(&s_data);
	set_empty_ndef_file(&s_data);
	s_data.cc_len = NFC_NDEF_CC_LEN;

	ret = ndef_serialize(&s_data, s_round, sizeof(s_round), &out_len);
	zassert_ok(ret);
	zassert_equal(out_len, sizeof(golden));
	zassert_mem_equal(s_round, golden, sizeof(golden));
}

ZTEST_SUITE(ndef_model, NULL, NULL, NULL, NULL, NULL);
