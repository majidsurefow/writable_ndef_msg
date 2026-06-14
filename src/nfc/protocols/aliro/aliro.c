/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aliro.h"

#include "router/service.h"

#include <errno.h>
#include <string.h>

const uint8_t aliro_expedited_aid[ALIRO_AID_LEN] = {
	0xA0U, 0x00U, 0x00U, 0x08U, 0x58U, 0x01U, 0x01U, 0x01U, 0x00U,
};

const uint8_t aliro_stepup_aid[ALIRO_AID_LEN] = {
	0xA0U, 0x00U, 0x00U, 0x08U, 0x58U, 0x01U, 0x01U, 0x02U, 0x00U,
};

void aliro_data_reset(aliro_data_t *data)
{
	if (data != NULL) {
		(void)memset(data, 0, sizeof(*data));
	}
}

uint8_t aliro_persist_id(void)
{
	return NFC_PERSIST_ID_ALIRO;
}

int aliro_serialize(const aliro_data_t *data, uint8_t *out, size_t out_max, size_t *out_len)
{
	size_t need;

	if ((data == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	need = 1U + 2U + 2U + ALIRO_P256_PUBLIC_KEY_SIZE + 2U + data->transcript_len;
	if (out_max < need) {
		return -ENOSPC;
	}

	out[0] = ALIRO_FORMAT_VERSION;
	out[1] = (uint8_t)(data->features & 0xFFU);
	out[2] = (uint8_t)((data->features >> 8U) & 0xFFU);
	out[3] = data->protocol_major;
	out[4] = data->protocol_minor;
	(void)memcpy(&out[5], data->credential_pubkey, ALIRO_P256_PUBLIC_KEY_SIZE);
	out[5U + ALIRO_P256_PUBLIC_KEY_SIZE] = (uint8_t)(data->transcript_len & 0xFFU);
	out[6U + ALIRO_P256_PUBLIC_KEY_SIZE] = (uint8_t)((data->transcript_len >> 8U) & 0xFFU);
	if (data->transcript_len > 0U) {
		(void)memcpy(&out[7U + ALIRO_P256_PUBLIC_KEY_SIZE], data->transcript,
			     data->transcript_len);
	}

	*out_len = need;
	return 0;
}

int aliro_deserialize(aliro_data_t *data, const uint8_t *in, size_t in_len)
{
	size_t offset = 0U;
	uint16_t tlen;

	if ((data == NULL) || (in == NULL)) {
		return -EINVAL;
	}

	aliro_data_reset(data);
	if (in_len < (1U + 2U + 2U + ALIRO_P256_PUBLIC_KEY_SIZE + 2U)) {
		return -EBADMSG;
	}
	if (in[offset++] != ALIRO_FORMAT_VERSION) {
		return -EBADMSG;
	}

	data->features = (uint16_t)in[offset] | ((uint16_t)in[offset + 1U] << 8U);
	offset += 2U;
	data->protocol_major = in[offset++];
	data->protocol_minor = in[offset++];
	(void)memcpy(data->credential_pubkey, &in[offset], ALIRO_P256_PUBLIC_KEY_SIZE);
	offset += ALIRO_P256_PUBLIC_KEY_SIZE;
	tlen = (uint16_t)in[offset] | ((uint16_t)in[offset + 1U] << 8U);
	offset += 2U;
	if (tlen > CONFIG_NFC_ALIRO_MAX_TRANSCRIPT) {
		return -EBADMSG;
	}
	if ((offset + tlen) != in_len) {
		return -EBADMSG;
	}

	data->transcript_len = tlen;
	if (tlen > 0U) {
		(void)memcpy(data->transcript, &in[offset], tlen);
	}

	return 0;
}
