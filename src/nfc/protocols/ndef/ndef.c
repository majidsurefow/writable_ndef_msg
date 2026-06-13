/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ndef.h"

#include <errno.h>
#include <string.h>

void ndef_data_reset(ndef_data_t *data)
{
	if (data == NULL) {
		return;
	}

	(void)memset(data, 0, sizeof(*data));
}

uint8_t ndef_persist_id(void)
{
	return NFC_PERSIST_ID_NDEF;
}

static size_t ndef_blob_payload_len(const ndef_data_t *data)
{
	return (size_t)NFC_NDEF_CC_LEN + (size_t)data->ndef_file_len;
}

static size_t ndef_blob_total_len(const ndef_data_t *data)
{
	return 1U + ndef_blob_payload_len(data);
}

int ndef_serialize(const ndef_data_t *data, uint8_t *out, size_t out_max, size_t *out_len)
{
	size_t need;

	if ((data == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	if (out == NULL) {
		return -EINVAL;
	}

	if (data->cc_len != NFC_NDEF_CC_LEN) {
		return -EINVAL;
	}

	if (data->ndef_file_len > (NFC_NDEF_NLEN_FIELD_LEN + CONFIG_NFC_NDEF_MAX_SIZE)) {
		return -EINVAL;
	}

	need = ndef_blob_total_len(data);
	if (out_max < need) {
		return -ENOSPC;
	}

	out[0] = NFC_NDEF_FORMAT_VERSION;
	(void)memcpy(&out[1], data->cc, NFC_NDEF_CC_LEN);
	(void)memcpy(&out[1U + NFC_NDEF_CC_LEN], data->ndef_file, data->ndef_file_len);
	*out_len = need;
	return 0;
}

static int ndef_nlen_from_file(const uint8_t *ndef_file, uint16_t ndef_file_len, uint16_t *nlen)
{
	if (ndef_file_len < NFC_NDEF_NLEN_FIELD_LEN) {
		return -EBADMSG;
	}

	*nlen = (uint16_t)(((uint16_t)ndef_file[0] << 8) | (uint16_t)ndef_file[1]);

	if (*nlen > CONFIG_NFC_NDEF_MAX_SIZE) {
		return -EBADMSG;
	}

	if ((uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + *nlen) != ndef_file_len) {
		return -EBADMSG;
	}

	return 0;
}

int ndef_deserialize(ndef_data_t *data, const uint8_t *in, size_t in_len)
{
	uint16_t nlen;
	size_t payload_len;

	if ((data == NULL) || (in == NULL)) {
		return -EINVAL;
	}

	if (in_len < (1U + NFC_NDEF_CC_LEN + NFC_NDEF_NLEN_FIELD_LEN)) {
		return -EBADMSG;
	}

	if (in[0] != NFC_NDEF_FORMAT_VERSION) {
		return -EBADMSG;
	}

	payload_len = in_len - 1U;
	if (payload_len < (NFC_NDEF_CC_LEN + NFC_NDEF_NLEN_FIELD_LEN)) {
		return -EBADMSG;
	}

	ndef_data_reset(data);
	data->cc_len = NFC_NDEF_CC_LEN;
	(void)memcpy(data->cc, &in[1], NFC_NDEF_CC_LEN);

	data->ndef_file_len = (uint16_t)(payload_len - NFC_NDEF_CC_LEN);
	if (data->ndef_file_len > sizeof(data->ndef_file)) {
		return -EBADMSG;
	}

	(void)memcpy(data->ndef_file, &in[1U + NFC_NDEF_CC_LEN], data->ndef_file_len);

	return ndef_nlen_from_file(data->ndef_file, data->ndef_file_len, &nlen);
}
