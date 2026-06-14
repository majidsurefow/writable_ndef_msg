/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "desfire.h"

#include <errno.h>
#include <string.h>

const uint8_t desfire_aid[DESFIRE_AID_LEN] = {
	0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x00U,
};

void desfire_data_reset(desfire_data_t *data)
{
	if (data == NULL) {
		return;
	}

	(void)memset(data, 0, sizeof(*data));
}

uint8_t desfire_persist_id(void)
{
	return NFC_PERSIST_ID_DESFIRE;
}

uint16_t desfire_rot_left_16(uint16_t value, uint8_t shift)
{
	shift &= 0x0FU;

	return (uint16_t)(((uint32_t)value << shift) | ((uint32_t)value >> (16U - shift)));
}

bool desfire_access_free_read(uint16_t access_rights)
{
	uint8_t read_nibble = (uint8_t)(access_rights & 0x0FU);
	uint8_t rw_nibble = (uint8_t)((access_rights >> 4U) & 0x0FU);

	return (read_nibble == 0x0EU) || (rw_nibble == 0x0EU);
}

void desfire_build_version_bytes(const desfire_data_t *data, uint8_t *out)
{
	if ((data == NULL) || (out == NULL)) {
		return;
	}

	(void)memcpy(out, data->hw_version, DESFIRE_VERSION_SIZE);
	(void)memcpy(&out[DESFIRE_VERSION_SIZE], data->sw_version, DESFIRE_VERSION_SIZE);
	(void)memcpy(&out[DESFIRE_VERSION_SIZE * 2U], data->uid, DESFIRE_UID_SIZE);
	(void)memcpy(&out[DESFIRE_VERSION_SIZE * 2U + DESFIRE_UID_SIZE], data->batch,
		     DESFIRE_BATCH_SIZE);
	out[DESFIRE_VERSION_SIZE * 2U + DESFIRE_UID_SIZE + DESFIRE_BATCH_SIZE] = data->prod_week;
	out[DESFIRE_VERSION_SIZE * 2U + DESFIRE_UID_SIZE + DESFIRE_BATCH_SIZE + 1U] =
		data->prod_year;
}

static int desfire_write_file_settings(uint8_t *out, const desfire_file_settings_t *fs)
{
	(void)memset(out, 0, 8U);
	out[0] = (uint8_t)fs->type;
	out[1] = (uint8_t)fs->comm;
	out[2] = (uint8_t)(fs->access_rights & 0xFFU);
	out[3] = (uint8_t)((fs->access_rights >> 8U) & 0xFFU);

	switch (fs->type) {
	case DESFIRE_FILE_TYPE_STANDARD:
	case DESFIRE_FILE_TYPE_BACKUP:
		out[4] = (uint8_t)(fs->settings.data.size & 0xFFU);
		out[5] = (uint8_t)((fs->settings.data.size >> 8U) & 0xFFU);
		out[6] = (uint8_t)((fs->settings.data.size >> 16U) & 0xFFU);
		break;
	case DESFIRE_FILE_TYPE_VALUE:
		(void)memcpy(&out[4], &fs->settings.value.lo_limit, 4U);
		break;
	default:
		out[4] = (uint8_t)(fs->settings.record.record_size & 0xFFU);
		out[5] = (uint8_t)((fs->settings.record.record_size >> 8U) & 0xFFU);
		break;
	}

	return 0;
}

static int desfire_read_file_settings(const uint8_t *in, desfire_file_settings_t *fs)
{
	fs->type = (desfire_file_type_t)in[0];
	fs->comm = (desfire_comm_t)in[1];
	fs->access_rights = (uint16_t)in[2] | ((uint16_t)in[3] << 8U);

	switch (fs->type) {
	case DESFIRE_FILE_TYPE_STANDARD:
	case DESFIRE_FILE_TYPE_BACKUP:
		fs->settings.data.size = (uint32_t)in[4] | ((uint32_t)in[5] << 8U) |
					 ((uint32_t)in[6] << 16U);
		break;
	case DESFIRE_FILE_TYPE_VALUE:
		(void)memcpy(&fs->settings.value.lo_limit, &in[4], 4U);
		break;
	default:
		fs->settings.record.record_size = (uint32_t)in[4] | ((uint32_t)in[5] << 8U);
		break;
	}

	return 0;
}

int desfire_serialize(const desfire_data_t *data, uint8_t *out, size_t out_max, size_t *out_len)
{
	size_t offset = 0U;

	if ((data == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	if (data->app_count > CONFIG_NFC_DESFIRE_MAX_APPS) {
		return -EINVAL;
	}

	if (out_max < 51U) {
		return -ENOSPC;
	}

	out[offset++] = DESFIRE_FORMAT_VERSION;
	(void)memcpy(&out[offset], data->hw_version, DESFIRE_VERSION_SIZE);
	offset += DESFIRE_VERSION_SIZE;
	(void)memcpy(&out[offset], data->sw_version, DESFIRE_VERSION_SIZE);
	offset += DESFIRE_VERSION_SIZE;
	(void)memcpy(&out[offset], data->uid, DESFIRE_UID_SIZE);
	offset += DESFIRE_UID_SIZE;
	(void)memcpy(&out[offset], data->batch, DESFIRE_BATCH_SIZE);
	offset += DESFIRE_BATCH_SIZE;
	out[offset++] = data->prod_week;
	out[offset++] = data->prod_year;
	out[offset++] = (uint8_t)(data->free_memory & 0xFFU);
	out[offset++] = (uint8_t)((data->free_memory >> 8U) & 0xFFU);
	out[offset++] = (uint8_t)((data->free_memory >> 16U) & 0xFFU);
	out[offset++] = (uint8_t)((data->free_memory >> 24U) & 0xFFU);
	(void)memcpy(&out[offset], data->master_key, DESFIRE_AES_KEY_SIZE);
	offset += DESFIRE_AES_KEY_SIZE;
	out[offset++] = data->master_key_settings;
	out[offset++] = data->app_count;

	for (uint8_t ai = 0U; ai < data->app_count; ai++) {
		const desfire_application_t *app = &data->apps[ai];

		if (app->file_count > CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP) {
			return -EINVAL;
		}
		if (app->key_count > DESFIRE_MAX_KEYS) {
			return -EINVAL;
		}

		if (out_max < (offset + 6U + ((size_t)app->key_count * DESFIRE_AES_KEY_SIZE))) {
			return -ENOSPC;
		}

		(void)memcpy(&out[offset], app->app_id, DESFIRE_APP_ID_SIZE);
		offset += DESFIRE_APP_ID_SIZE;
		out[offset++] = app->key_settings;
		out[offset++] = app->key_count;
		out[offset++] = app->file_count;

		for (uint8_t ki = 0U; ki < app->key_count; ki++) {
			(void)memcpy(&out[offset], app->keys[ki], DESFIRE_AES_KEY_SIZE);
			offset += DESFIRE_AES_KEY_SIZE;
		}

		for (uint8_t fi = 0U; fi < app->file_count; fi++) {
			size_t need = 1U + 1U + 1U + 2U + 8U + 2U + app->file_data_len[fi];

			if (out_max < (offset + need)) {
				return -ENOSPC;
			}
			if (app->file_data_len[fi] > CONFIG_NFC_DESFIRE_MAX_FILE_SIZE) {
				return -EINVAL;
			}

			out[offset++] = app->file_ids[fi];
			(void)desfire_write_file_settings(&out[offset], &app->file_settings[fi]);
			offset += 8U;
			out[offset++] = (uint8_t)(app->file_data_len[fi] & 0xFFU);
			out[offset++] = (uint8_t)((app->file_data_len[fi] >> 8U) & 0xFFU);
			if (app->file_data_len[fi] > 0U) {
				(void)memcpy(&out[offset], app->file_data[fi],
					     app->file_data_len[fi]);
				offset += app->file_data_len[fi];
			}
		}
	}

	*out_len = offset;
	return 0;
}

int desfire_deserialize(desfire_data_t *data, const uint8_t *in, size_t in_len)
{
	size_t offset = 0U;

	if ((data == NULL) || (in == NULL)) {
		return -EINVAL;
	}

	desfire_data_reset(data);

	if (in_len < 51U) {
		return -EBADMSG;
	}

	if (in[offset++] != DESFIRE_FORMAT_VERSION) {
		return -EBADMSG;
	}

	(void)memcpy(data->hw_version, &in[offset], DESFIRE_VERSION_SIZE);
	offset += DESFIRE_VERSION_SIZE;
	(void)memcpy(data->sw_version, &in[offset], DESFIRE_VERSION_SIZE);
	offset += DESFIRE_VERSION_SIZE;
	(void)memcpy(data->uid, &in[offset], DESFIRE_UID_SIZE);
	offset += DESFIRE_UID_SIZE;
	(void)memcpy(data->batch, &in[offset], DESFIRE_BATCH_SIZE);
	offset += DESFIRE_BATCH_SIZE;
	data->prod_week = in[offset++];
	data->prod_year = in[offset++];
	data->free_memory = (uint32_t)in[offset] | ((uint32_t)in[offset + 1U] << 8U) |
			    ((uint32_t)in[offset + 2U] << 16U) |
			    ((uint32_t)in[offset + 3U] << 24U);
	offset += 4U;
	(void)memcpy(data->master_key, &in[offset], DESFIRE_AES_KEY_SIZE);
	offset += DESFIRE_AES_KEY_SIZE;
	data->master_key_settings = in[offset++];
	data->app_count = in[offset++];

	if (data->app_count > CONFIG_NFC_DESFIRE_MAX_APPS) {
		return -EBADMSG;
	}

	for (uint8_t ai = 0U; ai < data->app_count; ai++) {
		desfire_application_t *app = &data->apps[ai];

		if (in_len < (offset + 6U)) {
			return -EBADMSG;
		}

		(void)memcpy(app->app_id, &in[offset], DESFIRE_APP_ID_SIZE);
		offset += DESFIRE_APP_ID_SIZE;
		app->key_settings = in[offset++];
		app->key_count = in[offset++];
		app->file_count = in[offset++];

		if (app->key_count > DESFIRE_MAX_KEYS) {
			return -EBADMSG;
		}
		if (app->file_count > CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP) {
			return -EBADMSG;
		}

		if (in_len < (offset + ((size_t)app->key_count * DESFIRE_AES_KEY_SIZE))) {
			return -EBADMSG;
		}

		for (uint8_t ki = 0U; ki < app->key_count; ki++) {
			(void)memcpy(app->keys[ki], &in[offset], DESFIRE_AES_KEY_SIZE);
			offset += DESFIRE_AES_KEY_SIZE;
		}

		for (uint8_t fi = 0U; fi < app->file_count; fi++) {
			uint16_t data_len;

			if (in_len < (offset + 12U)) {
				return -EBADMSG;
			}

			app->file_ids[fi] = in[offset++];
			(void)desfire_read_file_settings(&in[offset], &app->file_settings[fi]);
			offset += 8U;
			data_len = (uint16_t)in[offset] | ((uint16_t)in[offset + 1U] << 8U);
			offset += 2U;

			if (data_len > CONFIG_NFC_DESFIRE_MAX_FILE_SIZE) {
				return -EBADMSG;
			}
			if (in_len < (offset + data_len)) {
				return -EBADMSG;
			}

			app->file_data_len[fi] = data_len;
			if (data_len > 0U) {
				(void)memcpy(app->file_data[fi], &in[offset], data_len);
				offset += data_len;
			}
		}
	}

	if (offset != in_len) {
		return -EBADMSG;
	}

	return 0;
}
