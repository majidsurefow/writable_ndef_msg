/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_classic/mf_classic.c
 */

#include "protocols/classic/classic.h"
#include "protocols/classic/classic_i.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>

static int classic_serialize_cb(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	return classic_serialize(user_ctx, out, out_max, out_len);
}

static int classic_deserialize_cb(const uint8_t *in, size_t in_len, void *user_ctx)
{
	return classic_deserialize(user_ctx, in, in_len);
}

static nfc_service_t s_classic_svc = {
	.serialize = classic_serialize_cb,
	.deserialize = classic_deserialize_cb,
	.persist_id = NFC_PERSIST_ID_CLASSIC,
};

void classic_data_reset(classic_data_t *data)
{
	if (data == NULL) {
		return;
	}

	(void)memset(data, 0, sizeof(*data));
	data->type = CLASSIC_TYPE_UNKNOWN;
}

uint8_t classic_persist_id(void)
{
	return NFC_PERSIST_ID_CLASSIC;
}

int classic_serialize(const classic_data_t *data, uint8_t *out, size_t out_max, size_t *out_len)
{
	size_t offset = 0U;
	uint16_t blocks_total;
	size_t need;

	if ((data == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	if ((data->type == CLASSIC_TYPE_UNKNOWN) || (data->uid_len == 0U)) {
		return -EINVAL;
	}

	blocks_total = classic_get_total_blocks(data->type);
	if (blocks_total == 0U) {
		return -EINVAL;
	}

	need = 1U + 1U + 1U + (size_t)data->uid_len + 1U + 2U +
	       (sizeof(uint32_t) * CLASSIC_BLOCK_MASK_WORDS) + sizeof(uint64_t) * 2U +
	       (size_t)blocks_total * CLASSIC_BLOCK_SIZE;

	if (out_max < need) {
		return -ENOSPC;
	}

	out[offset++] = CLASSIC_FORMAT_VERSION;
	out[offset++] = (uint8_t)data->type;
	out[offset++] = data->uid_len;
	(void)memcpy(&out[offset], data->uid, data->uid_len);
	offset += data->uid_len;
	out[offset++] = data->sak;
	out[offset++] = data->atqa[0];
	out[offset++] = data->atqa[1];

	for (size_t i = 0U; i < CLASSIC_BLOCK_MASK_WORDS; i++) {
		uint32_t w = data->block_read_mask[i];

		out[offset++] = (uint8_t)(w & 0xFFU);
		out[offset++] = (uint8_t)((w >> 8U) & 0xFFU);
		out[offset++] = (uint8_t)((w >> 16U) & 0xFFU);
		out[offset++] = (uint8_t)((w >> 24U) & 0xFFU);
	}

	for (size_t i = 0U; i < sizeof(uint64_t); i++) {
		out[offset++] = (uint8_t)((data->key_a_mask >> (8U * i)) & 0xFFU);
	}
	for (size_t i = 0U; i < sizeof(uint64_t); i++) {
		out[offset++] = (uint8_t)((data->key_b_mask >> (8U * i)) & 0xFFU);
	}

	for (uint16_t block = 0U; block < blocks_total; block++) {
		(void)memcpy(&out[offset], data->blocks[block], CLASSIC_BLOCK_SIZE);
		offset += CLASSIC_BLOCK_SIZE;
	}

	*out_len = offset;
	return 0;
}

int classic_deserialize(classic_data_t *data, const uint8_t *in, size_t in_len)
{
	size_t offset = 0U;
	uint16_t blocks_total;

	if ((data == NULL) || (in == NULL)) {
		return -EINVAL;
	}

	classic_data_reset(data);

	if (in_len < 4U) {
		return -EBADMSG;
	}

	if (in[offset++] != CLASSIC_FORMAT_VERSION) {
		return -EBADMSG;
	}

	data->type = (classic_type_t)in[offset++];
	if ((data->type == CLASSIC_TYPE_UNKNOWN) ||
	    (data->type > CLASSIC_TYPE_4K)) {
		return -EBADMSG;
	}

	data->uid_len = in[offset++];
	if ((data->uid_len == 0U) || (data->uid_len > sizeof(data->uid))) {
		return -EBADMSG;
	}

	if (in_len < offset + (size_t)data->uid_len + 3U) {
		return -EBADMSG;
	}

	(void)memcpy(data->uid, &in[offset], data->uid_len);
	offset += data->uid_len;
	data->sak = in[offset++];
	data->atqa[0] = in[offset++];
	data->atqa[1] = in[offset++];

	if (in_len < offset + (sizeof(uint32_t) * CLASSIC_BLOCK_MASK_WORDS) +
			     (sizeof(uint64_t) * 2U)) {
		return -EBADMSG;
	}

	for (size_t i = 0U; i < CLASSIC_BLOCK_MASK_WORDS; i++) {
		uint32_t w = (uint32_t)in[offset] |
			     ((uint32_t)in[offset + 1U] << 8U) |
			     ((uint32_t)in[offset + 2U] << 16U) |
			     ((uint32_t)in[offset + 3U] << 24U);

		data->block_read_mask[i] = w;
		offset += 4U;
	}

	data->key_a_mask = 0ULL;
	for (size_t i = 0U; i < sizeof(uint64_t); i++) {
		data->key_a_mask |= (uint64_t)in[offset++] << (8U * i);
	}
	data->key_b_mask = 0ULL;
	for (size_t i = 0U; i < sizeof(uint64_t); i++) {
		data->key_b_mask |= (uint64_t)in[offset++] << (8U * i);
	}

	blocks_total = classic_get_total_blocks(data->type);
	if (in_len != offset + (size_t)blocks_total * CLASSIC_BLOCK_SIZE) {
		return -EBADMSG;
	}

	for (uint16_t block = 0U; block < blocks_total; block++) {
		(void)memcpy(data->blocks[block], &in[offset], CLASSIC_BLOCK_SIZE);
		offset += CLASSIC_BLOCK_SIZE;
	}

	return 0;
}

const nfc_service_t *classic_service_get(void)
{
	return &s_classic_svc;
}
