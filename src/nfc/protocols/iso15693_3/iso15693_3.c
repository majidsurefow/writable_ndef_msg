/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/iso15693_3/iso15693_3.h"

#include <errno.h>
#include <string.h>

static int iso15693_3_serialize_cb(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	return iso15693_3_serialize(user_ctx, out, out_max, out_len);
}

static int iso15693_3_deserialize_cb(const uint8_t *in, size_t in_len, void *user_ctx)
{
	return iso15693_3_deserialize(user_ctx, in, in_len);
}

static nfc_service_t s_iso15693_svc = {
	.serialize = iso15693_3_serialize_cb,
	.deserialize = iso15693_3_deserialize_cb,
	.persist_id = NFC_PERSIST_ID_ISO15693,
};

void iso15693_3_data_reset(iso15693_3_data_t *data)
{
	if (data == NULL) {
		return;
	}

	(void)memset(data, 0, sizeof(*data));
}

uint8_t iso15693_3_persist_id(void)
{
	return NFC_PERSIST_ID_ISO15693;
}

int iso15693_3_serialize(const iso15693_3_data_t *data, uint8_t *out, size_t out_max,
			 size_t *out_len)
{
	size_t offset = 0U;
	size_t data_len;
	size_t need;

	if ((data == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	if ((data->block_count == 0U) || (data->block_count > ISO15693_BLOCKS_MAX) ||
	    (data->block_size == 0U) || (data->block_size > ISO15693_BLOCK_SIZE_MAX)) {
		return -EINVAL;
	}

	data_len = (size_t)data->block_count * (size_t)data->block_size;
	need = 1U + ISO15693_UID_SIZE + 5U + 3U + data_len + (size_t)data->block_count;
	if (out_max < need) {
		return -ENOSPC;
	}

	out[offset++] = ISO15693_FORMAT_VERSION;
	(void)memcpy(&out[offset], data->uid, ISO15693_UID_SIZE);
	offset += ISO15693_UID_SIZE;
	out[offset++] = data->dsfid;
	out[offset++] = data->afi;
	out[offset++] = data->ic_ref;
	out[offset++] = data->lock_dsfid;
	out[offset++] = data->lock_afi;
	out[offset++] = (uint8_t)(data->block_count & 0xFFU);
	out[offset++] = (uint8_t)((data->block_count >> 8U) & 0xFFU);
	out[offset++] = data->block_size;
	(void)memcpy(&out[offset], data->block_data, data_len);
	offset += data_len;
	(void)memcpy(&out[offset], data->block_security, data->block_count);
	offset += data->block_count;

	*out_len = offset;
	return 0;
}

int iso15693_3_deserialize(iso15693_3_data_t *data, const uint8_t *in, size_t in_len)
{
	size_t offset = 0U;
	uint16_t block_count;
	size_t data_len;
	size_t need;

	if ((data == NULL) || (in == NULL)) {
		return -EINVAL;
	}

	iso15693_3_data_reset(data);

	if (in_len < (1U + ISO15693_UID_SIZE + 5U + 3U)) {
		return -EBADMSG;
	}

	if (in[offset++] != ISO15693_FORMAT_VERSION) {
		return -EBADMSG;
	}

	(void)memcpy(data->uid, &in[offset], ISO15693_UID_SIZE);
	offset += ISO15693_UID_SIZE;
	data->dsfid = in[offset++];
	data->afi = in[offset++];
	data->ic_ref = in[offset++];
	data->lock_dsfid = in[offset++];
	data->lock_afi = in[offset++];
	block_count = (uint16_t)in[offset] | ((uint16_t)in[offset + 1U] << 8U);
	offset += 2U;
	data->block_size = in[offset++];

	if ((block_count == 0U) || (block_count > ISO15693_BLOCKS_MAX) ||
	    (data->block_size == 0U) || (data->block_size > ISO15693_BLOCK_SIZE_MAX)) {
		return -EBADMSG;
	}

	data_len = (size_t)block_count * (size_t)data->block_size;
	need = offset + data_len + (size_t)block_count;
	if (in_len < need) {
		return -EBADMSG;
	}

	data->block_count = block_count;
	(void)memcpy(data->block_data, &in[offset], data_len);
	offset += data_len;
	(void)memcpy(data->block_security, &in[offset], block_count);

	return 0;
}

int iso15693_3_compare(const iso15693_3_data_t *expected, const iso15693_3_data_t *actual)
{
	size_t data_len;

	if ((expected == NULL) || (actual == NULL)) {
		return -EINVAL;
	}

	if (memcmp(expected->uid, actual->uid, ISO15693_UID_SIZE) != 0) {
		return -EBADMSG;
	}
	if (expected->dsfid != actual->dsfid) {
		return -EBADMSG;
	}
	if (expected->afi != actual->afi) {
		return -EBADMSG;
	}
	if (expected->ic_ref != actual->ic_ref) {
		return -EBADMSG;
	}
	if (expected->block_count != actual->block_count) {
		return -EBADMSG;
	}
	if (expected->block_size != actual->block_size) {
		return -EBADMSG;
	}

	data_len = (size_t)expected->block_count * (size_t)expected->block_size;
	if (memcmp(expected->block_data, actual->block_data, data_len) != 0) {
		return -EBADMSG;
	}
	if (memcmp(expected->block_security, actual->block_security, expected->block_count) != 0) {
		return -EBADMSG;
	}

	return 0;
}

const nfc_service_t *iso15693_3_service_get(void)
{
	return &s_iso15693_svc;
}
