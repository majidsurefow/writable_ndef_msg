/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight.c
 */

#include "ultralight.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>

#define ULTRALIGHT_NDEF_SCAN_MAX 564U

void ultralight_data_reset(ultralight_data_t *data)
{
	if (data == NULL) {
		return;
	}

	(void)memset(data, 0, sizeof(*data));
	data->type = UL_TYPE_UNKNOWN;
	data->variant = UL_VARIANT_UNKNOWN;
}

uint8_t ultralight_persist_id(void)
{
	return NFC_PERSIST_ID_ULTRALIGHT;
}

uint16_t ultralight_pages_total_for_type(ultralight_type_t type)
{
	switch (type) {
	case UL_TYPE_ORIGIN:
		return 16U;
	case UL_TYPE_UL11:
		return 20U;
	case UL_TYPE_UL21:
		return 41U;
	case UL_TYPE_NTAG203:
		return 42U;
	case UL_TYPE_NTAG213:
		return 45U;
	case UL_TYPE_NTAG215:
		return 135U;
	case UL_TYPE_NTAG216:
		return 231U;
	case UL_TYPE_MFUL_C:
		return 48U;
	default:
		return 0U;
	}
}

static ultralight_variant_t variant_from_type(ultralight_type_t type)
{
	switch (type) {
	case UL_TYPE_UL11:
		return UL_VARIANT_UL11;
	case UL_TYPE_UL21:
		return UL_VARIANT_UL21;
	case UL_TYPE_ORIGIN:
		return UL_VARIANT_ORIGIN;
	default:
		return UL_VARIANT_UNKNOWN;
	}
}

static ultralight_type_t type_from_pages_total(uint16_t pages_total)
{
	switch (pages_total) {
	case 16U:
		return UL_TYPE_ORIGIN;
	case 20U:
		return UL_TYPE_UL11;
	case 41U:
		return UL_TYPE_UL21;
	case 42U:
		return UL_TYPE_NTAG203;
	case 45U:
		return UL_TYPE_NTAG213;
	case 48U:
		return UL_TYPE_MFUL_C;
	case 135U:
		return UL_TYPE_NTAG215;
	case 231U:
		return UL_TYPE_NTAG216;
	default:
		return UL_TYPE_UNKNOWN;
	}
}

static size_t ultralight_blob_pages_len(uint16_t pages_total)
{
	return (size_t)pages_total * ULTRALIGHT_PAGE_SIZE;
}

int ultralight_serialize(const ultralight_data_t *data, uint8_t *out, size_t out_max,
			 size_t *out_len)
{
	size_t offset = 0U;
	size_t need;

	if ((data == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	if ((data->pages_total == 0U) ||
	    (data->pages_total > CONFIG_NFC_ULTRALIGHT_MAX_PAGES)) {
		return -EINVAL;
	}

	need = 4U + ultralight_blob_pages_len(data->pages_total) + 1U;
	if (data->has_version) {
		need += ULTRALIGHT_VERSION_SIZE;
	}
	need += 1U;
	if (data->has_signature) {
		need += ULTRALIGHT_SIGNATURE_SIZE;
	}
	need += 1U + (size_t)data->counter_count * 3U;
	need += 1U + (size_t)data->tearing_flag_count;

	if (out_max < need) {
		return -ENOSPC;
	}

	out[offset++] = ULTRALIGHT_FORMAT_VERSION;
	out[offset++] = (uint8_t)data->variant;
	out[offset++] = (uint8_t)(data->pages_total & 0xFFU);
	out[offset++] = (uint8_t)((data->pages_total >> 8U) & 0xFFU);

	for (uint16_t i = 0U; i < data->pages_total; i++) {
		(void)memcpy(&out[offset], data->pages[i], ULTRALIGHT_PAGE_SIZE);
		offset += ULTRALIGHT_PAGE_SIZE;
	}

	out[offset++] = data->has_version ? 1U : 0U;
	if (data->has_version) {
		(void)memcpy(&out[offset], data->version, ULTRALIGHT_VERSION_SIZE);
		offset += ULTRALIGHT_VERSION_SIZE;
	}

	out[offset++] = data->has_signature ? 1U : 0U;
	if (data->has_signature) {
		(void)memcpy(&out[offset], data->signature, ULTRALIGHT_SIGNATURE_SIZE);
		offset += ULTRALIGHT_SIGNATURE_SIZE;
	}

	out[offset++] = data->counter_count;
	for (uint8_t i = 0U; i < data->counter_count; i++) {
		(void)memcpy(&out[offset], data->counters[i], 3U);
		offset += 3U;
	}

	out[offset++] = data->tearing_flag_count;
	for (uint8_t i = 0U; i < data->tearing_flag_count; i++) {
		out[offset++] = data->tearing_flags[i];
	}

	*out_len = offset;
	return 0;
}

int ultralight_deserialize(ultralight_data_t *data, const uint8_t *in, size_t in_len)
{
	size_t offset = 0U;
	uint16_t pages_total;
	uint8_t has_version;
	uint8_t has_signature;
	uint8_t counter_count;
	uint8_t tearing_count;

	if ((data == NULL) || (in == NULL)) {
		return -EINVAL;
	}

	if (in_len < 4U) {
		return -EBADMSG;
	}

	if (in[offset++] != ULTRALIGHT_FORMAT_VERSION) {
		return -EBADMSG;
	}

	ultralight_data_reset(data);
	data->variant = (ultralight_variant_t)in[offset++];

	pages_total = (uint16_t)in[offset] | ((uint16_t)in[offset + 1U] << 8U);
	offset += 2U;

	if ((pages_total == 0U) || (pages_total > CONFIG_NFC_ULTRALIGHT_MAX_PAGES)) {
		return -EBADMSG;
	}

	if (in_len < (4U + ultralight_blob_pages_len(pages_total))) {
		return -EBADMSG;
	}

	data->pages_total = pages_total;
	data->type = type_from_pages_total(pages_total);
	if (data->variant == UL_VARIANT_UNKNOWN) {
		data->variant = variant_from_type(data->type);
	}
	for (uint16_t i = 0U; i < pages_total; i++) {
		(void)memcpy(data->pages[i], &in[offset], ULTRALIGHT_PAGE_SIZE);
		offset += ULTRALIGHT_PAGE_SIZE;
	}

	if (offset >= in_len) {
		return -EBADMSG;
	}

	has_version = in[offset++];
	if (has_version != 0U) {
		if ((offset + ULTRALIGHT_VERSION_SIZE) > in_len) {
			return -EBADMSG;
		}

		data->has_version = true;
		(void)memcpy(data->version, &in[offset], ULTRALIGHT_VERSION_SIZE);
		offset += ULTRALIGHT_VERSION_SIZE;
	}

	if (offset >= in_len) {
		return -EBADMSG;
	}

	has_signature = in[offset++];
	if (has_signature != 0U) {
		if ((offset + ULTRALIGHT_SIGNATURE_SIZE) > in_len) {
			return -EBADMSG;
		}

		data->has_signature = true;
		(void)memcpy(data->signature, &in[offset], ULTRALIGHT_SIGNATURE_SIZE);
		offset += ULTRALIGHT_SIGNATURE_SIZE;
	}

	if (offset >= in_len) {
		return -EBADMSG;
	}

	counter_count = in[offset++];
	if (counter_count > ULTRALIGHT_COUNTER_NUM) {
		return -EBADMSG;
	}

	if ((offset + ((size_t)counter_count * 3U)) > in_len) {
		return -EBADMSG;
	}

	data->counter_count = counter_count;
	for (uint8_t i = 0U; i < counter_count; i++) {
		(void)memcpy(data->counters[i], &in[offset], 3U);
		offset += 3U;
	}

	if (offset >= in_len) {
		return -EBADMSG;
	}

	tearing_count = in[offset++];
	if (tearing_count > ULTRALIGHT_TEARING_FLAG_NUM) {
		return -EBADMSG;
	}

	if ((offset + tearing_count) > in_len) {
		return -EBADMSG;
	}

	data->tearing_flag_count = tearing_count;
	for (uint8_t i = 0U; i < tearing_count; i++) {
		data->tearing_flags[i] = in[offset++];
	}

	data->pages_read = pages_total;
	return 0;
}

int ultralight_scan_ndef_tlv(const uint8_t *data, size_t data_len, const uint8_t **msg_out,
			     size_t *msg_len_out)
{
	size_t offset = 0U;

	if ((data == NULL) || (msg_out == NULL) || (msg_len_out == NULL)) {
		return -EINVAL;
	}

	*msg_out = NULL;
	*msg_len_out = 0U;

	while (offset < data_len) {
		uint8_t tag = data[offset];
		size_t tlv_len = 0U;

		if (tag == ULTRALIGHT_TLV_NULL) {
			offset++;
			continue;
		}

		if (tag == ULTRALIGHT_TLV_TERMINATOR) {
			return 0;
		}

		if ((offset + 1U) >= data_len) {
			return -ENODATA;
		}

		offset++;
		if (data[offset] == ULTRALIGHT_TLV_LONG_FORM) {
			if ((offset + 2U) >= data_len) {
				return -ENODATA;
			}

			tlv_len = ((size_t)data[offset + 1U] << 8U) | (size_t)data[offset + 2U];
			offset += 3U;
		} else {
			tlv_len = (size_t)data[offset];
			offset++;
		}

		if ((offset + tlv_len) > data_len) {
			return -ENODATA;
		}

		if (tag == ULTRALIGHT_TLV_NDEF) {
			*msg_out = &data[offset];
			*msg_len_out = tlv_len;
			return 0;
		}

		offset += tlv_len;
	}

	return 0;
}

int ultralight_extract_ndef(const ultralight_data_t *data, const uint8_t **msg, size_t *msg_len)
{
	uint8_t flat[ULTRALIGHT_NDEF_SCAN_MAX];
	size_t flat_len = 0U;
	size_t scan_max = sizeof(flat);
	int ret;

	if ((data == NULL) || (msg == NULL) || (msg_len == NULL)) {
		return -EINVAL;
	}

	*msg = NULL;
	*msg_len = 0U;

	if (data->pages_total <= ULTRALIGHT_CC_PAGE) {
		return 0;
	}

	if (data->pages[ULTRALIGHT_CC_PAGE][0] != 0xE1U) {
		return 0;
	}

	for (uint16_t page = ULTRALIGHT_DATA_START_PAGE; page < data->pages_total; page++) {
		if ((flat_len + ULTRALIGHT_PAGE_SIZE) > scan_max) {
			break;
		}

		(void)memcpy(&flat[flat_len], data->pages[page], ULTRALIGHT_PAGE_SIZE);
		flat_len += ULTRALIGHT_PAGE_SIZE;
	}

	ret = ultralight_scan_ndef_tlv(flat, flat_len, msg, msg_len);
	if (ret == -ENODATA) {
		*msg = NULL;
		*msg_len = 0U;
		return 0;
	}

	return ret;
}
