/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/slix/slix.h"

#include <errno.h>
#include <string.h>

#define SLIX_EXT_TAIL_SIZE (2U + 1U + 1U + (SLIX_PASSWORD_COUNT * SLIX_PASSWORD_SIZE) + \
			    SLIX_SIGNATURE_SIZE + 5U)

static int slix_serialize_cb(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	return slix_serialize(user_ctx, out, out_max, out_len);
}

static int slix_deserialize_cb(const uint8_t *in, size_t in_len, void *user_ctx)
{
	return slix_deserialize(user_ctx, in, in_len);
}

static nfc_service_t s_slix_svc = {
	.serialize = slix_serialize_cb,
	.deserialize = slix_deserialize_cb,
	.persist_id = NFC_PERSIST_ID_SLIX,
};

void slix_data_reset(slix_data_t *data)
{
	if (data == NULL) {
		return;
	}

	(void)memset(data, 0, sizeof(*data));
	data->type = SLIX_TYPE_UNKNOWN;
}

uint8_t slix_persist_id(void)
{
	return NFC_PERSIST_ID_SLIX;
}

slix_type_t slix_type_from_uid(const uint8_t uid[ISO15693_UID_SIZE])
{
	if ((uid == NULL) || (uid[0] != 0xE0U) || (uid[1] != SLIX_NXP_MFG_CODE)) {
		return SLIX_TYPE_UNKNOWN;
	}

	switch (uid[2]) {
	case 0x01U:
		return SLIX_TYPE_SLIX;
	case 0x02U:
		return SLIX_TYPE_SLIX_S;
	case 0x03U:
		return SLIX_TYPE_SLIX_L;
	case 0x04U:
		return SLIX_TYPE_SLIX2;
	default:
		return SLIX_TYPE_SLIX;
	}
}

int slix_serialize(const slix_data_t *data, uint8_t *out, size_t out_max, size_t *out_len)
{
	size_t parent_len = 0U;
	size_t offset;
	int ret;

	if ((data == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	ret = iso15693_3_serialize(&data->parent, out, out_max, &parent_len);
	if (ret != 0) {
		return ret;
	}

	if ((out_max - parent_len) < SLIX_EXT_TAIL_SIZE) {
		return -ENOSPC;
	}

	offset = parent_len;
	out[offset++] = (uint8_t)SLIX_EXT_MAGIC_0;
	out[offset++] = (uint8_t)SLIX_EXT_MAGIC_1;
	out[offset++] = SLIX_EXT_VERSION;
	out[offset++] = (uint8_t)data->capabilities;

	for (size_t i = 0U; i < SLIX_PASSWORD_COUNT; i++) {
		(void)memcpy(&out[offset], data->passwords[i], SLIX_PASSWORD_SIZE);
		offset += SLIX_PASSWORD_SIZE;
	}

	(void)memcpy(&out[offset], data->signature, SLIX_SIGNATURE_SIZE);
	offset += SLIX_SIGNATURE_SIZE;
	out[offset++] = data->privacy_mode;
	out[offset++] = data->protection_pointer;
	out[offset++] = data->protection_condition;
	out[offset++] = data->lock_eas;
	out[offset++] = data->lock_ppl;

	*out_len = offset;
	return 0;
}

int slix_deserialize(slix_data_t *data, const uint8_t *in, size_t in_len)
{
	size_t offset;
	int ret;

	if ((data == NULL) || (in == NULL)) {
		return -EINVAL;
	}

	slix_data_reset(data);
	ret = iso15693_3_deserialize(&data->parent, in, in_len);
	if (ret != 0) {
		return ret;
	}

	if (in_len < (1U + ISO15693_UID_SIZE + 5U + 3U)) {
		return -EBADMSG;
	}

	offset = 1U + ISO15693_UID_SIZE + 5U + 3U;
	offset += (size_t)data->parent.block_count * (size_t)data->parent.block_size;
	offset += data->parent.block_count;

	if (in_len < (offset + SLIX_EXT_TAIL_SIZE)) {
		return -EBADMSG;
	}

	if ((in[offset] != (uint8_t)SLIX_EXT_MAGIC_0) || (in[offset + 1U] != (uint8_t)SLIX_EXT_MAGIC_1)) {
		return -EBADMSG;
	}
	offset += 2U;

	if (in[offset++] != SLIX_EXT_VERSION) {
		return -EBADMSG;
	}

	data->capabilities = (slix_capabilities_t)in[offset++];
	data->type = slix_type_from_uid(data->parent.uid);

	for (size_t i = 0U; i < SLIX_PASSWORD_COUNT; i++) {
		(void)memcpy(data->passwords[i], &in[offset], SLIX_PASSWORD_SIZE);
		offset += SLIX_PASSWORD_SIZE;
	}

	(void)memcpy(data->signature, &in[offset], SLIX_SIGNATURE_SIZE);
	offset += SLIX_SIGNATURE_SIZE;
	data->privacy_mode = in[offset++];
	data->protection_pointer = in[offset++];
	data->protection_condition = in[offset++];
	data->lock_eas = in[offset++];
	data->lock_ppl = in[offset++];

	return 0;
}

int slix_compare(const slix_data_t *expected, const slix_data_t *actual)
{
	int ret;

	if ((expected == NULL) || (actual == NULL)) {
		return -EINVAL;
	}

	ret = iso15693_3_compare(&expected->parent, &actual->parent);
	if (ret != 0) {
		return ret;
	}

	if (expected->capabilities != actual->capabilities) {
		return -EBADMSG;
	}
	if (expected->type != actual->type) {
		return -EBADMSG;
	}
	if (memcmp(expected->passwords, actual->passwords, sizeof(expected->passwords)) != 0) {
		return -EBADMSG;
	}
	if (memcmp(expected->signature, actual->signature, SLIX_SIGNATURE_SIZE) != 0) {
		return -EBADMSG;
	}
	if (expected->privacy_mode != actual->privacy_mode) {
		return -EBADMSG;
	}
	if (expected->protection_pointer != actual->protection_pointer) {
		return -EBADMSG;
	}
	if (expected->protection_condition != actual->protection_condition) {
		return -EBADMSG;
	}
	if (expected->lock_eas != actual->lock_eas) {
		return -EBADMSG;
	}
	if (expected->lock_ppl != actual->lock_ppl) {
		return -EBADMSG;
	}

	return 0;
}

const nfc_service_t *slix_service_get(void)
{
	return &s_slix_svc;
}
