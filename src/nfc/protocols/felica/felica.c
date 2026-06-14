/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/felica/felica.h"

#include <errno.h>
#include <string.h>

static int felica_serialize_cb(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	return felica_serialize(user_ctx, out, out_max, out_len);
}

static int felica_deserialize_cb(const uint8_t *in, size_t in_len, void *user_ctx)
{
	return felica_deserialize(user_ctx, in, in_len);
}

static nfc_service_t s_felica_svc = {
	.serialize = felica_serialize_cb,
	.deserialize = felica_deserialize_cb,
	.persist_id = NFC_PERSIST_ID_FELICA,
};

void felica_data_reset(felica_data_t *data)
{
	if (data == NULL) {
		return;
	}

	(void)memset(data, 0, sizeof(*data));
}

felica_workflow_type_t felica_get_workflow_type(const uint8_t pmm[FELICA_PMM_SIZE])
{
	if (pmm == NULL) {
		return FELICA_WORKFLOW_UNKNOWN;
	}

	switch (pmm[1]) {
	case 0xF0U:
	case 0xF1U:
	case 0xF2U:
		return FELICA_WORKFLOW_LITE;
	case 0xA2U:
		return FELICA_WORKFLOW_STANDARD;
	default:
		if (pmm[1] <= 0x48U) {
			return FELICA_WORKFLOW_STANDARD;
		}
		return FELICA_WORKFLOW_UNKNOWN;
	}
}

uint8_t felica_persist_id(void)
{
	return NFC_PERSIST_ID_FELICA;
}

int felica_serialize(const felica_data_t *data, uint8_t *out, size_t out_max, size_t *out_len)
{
	size_t offset = 0U;
	size_t need;

	if ((data == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	if ((data->blocks_total == 0U) || (data->blocks_total > FELICA_BLOCKS_MAX)) {
		return -EINVAL;
	}

	need = 1U + FELICA_IDM_SIZE + FELICA_PMM_SIZE + 4U +
	       ((size_t)data->blocks_total * (2U + FELICA_BLOCK_DATA_SIZE));
	if (out_max < need) {
		return -ENOSPC;
	}

	out[offset++] = FELICA_FORMAT_VERSION;
	(void)memcpy(&out[offset], data->idm, FELICA_IDM_SIZE);
	offset += FELICA_IDM_SIZE;
	(void)memcpy(&out[offset], data->pmm, FELICA_PMM_SIZE);
	offset += FELICA_PMM_SIZE;
	out[offset++] = (uint8_t)(data->blocks_total & 0xFFU);
	out[offset++] = (uint8_t)((data->blocks_total >> 8U) & 0xFFU);
	out[offset++] = (uint8_t)(data->blocks_read & 0xFFU);
	out[offset++] = (uint8_t)((data->blocks_read >> 8U) & 0xFFU);

	for (uint16_t i = 0U; i < data->blocks_total; i++) {
		out[offset++] = data->blocks[i].sf1;
		out[offset++] = data->blocks[i].sf2;
		(void)memcpy(&out[offset], data->blocks[i].data, FELICA_BLOCK_DATA_SIZE);
		offset += FELICA_BLOCK_DATA_SIZE;
	}

	*out_len = offset;
	return 0;
}

int felica_deserialize(felica_data_t *data, const uint8_t *in, size_t in_len)
{
	size_t offset = 0U;
	uint16_t blocks_total;
	size_t need;

	if ((data == NULL) || (in == NULL)) {
		return -EINVAL;
	}

	felica_data_reset(data);

	if (in_len < (1U + FELICA_IDM_SIZE + FELICA_PMM_SIZE + 4U)) {
		return -EBADMSG;
	}

	if (in[offset++] != FELICA_FORMAT_VERSION) {
		return -EBADMSG;
	}

	(void)memcpy(data->idm, &in[offset], FELICA_IDM_SIZE);
	offset += FELICA_IDM_SIZE;
	(void)memcpy(data->pmm, &in[offset], FELICA_PMM_SIZE);
	offset += FELICA_PMM_SIZE;
	blocks_total = (uint16_t)in[offset] | ((uint16_t)in[offset + 1U] << 8U);
	offset += 2U;
	data->blocks_read = (uint16_t)in[offset] | ((uint16_t)in[offset + 1U] << 8U);
	offset += 2U;

	if ((blocks_total == 0U) || (blocks_total > FELICA_BLOCKS_MAX)) {
		return -EBADMSG;
	}

	need = offset + ((size_t)blocks_total * (2U + FELICA_BLOCK_DATA_SIZE));
	if (in_len < need) {
		return -EBADMSG;
	}

	data->blocks_total = blocks_total;
	for (uint16_t i = 0U; i < blocks_total; i++) {
		data->blocks[i].sf1 = in[offset++];
		data->blocks[i].sf2 = in[offset++];
		(void)memcpy(data->blocks[i].data, &in[offset], FELICA_BLOCK_DATA_SIZE);
		offset += FELICA_BLOCK_DATA_SIZE;
	}

	return 0;
}

const nfc_service_t *felica_service_get(void)
{
	return &s_felica_svc;
}
