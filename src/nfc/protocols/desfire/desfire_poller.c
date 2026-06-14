/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "desfire_poller.h"
#include "desfire_poller_i.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(desfire_poller, CONFIG_LOG_DEFAULT_LEVEL);

static const uint8_t SELECT_AID[] = {
	0x00U, 0xA4U, 0x04U, 0x00U, DESFIRE_AID_LEN,
	0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x00U,
};

static const uint8_t GET_KEY_VERSION[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_KEY_VERSION,
					  0x00U, 0x00U, 0x01U, 0x00U};
static const uint8_t GET_VERSION[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_VERSION,
				      0x00U, 0x00U, 0x00U};
static const uint8_t GET_FREE_MEMORY[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_FREE_MEMORY,
					  0x00U, 0x00U, 0x00U};
static const uint8_t GET_KEY_SETTINGS[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_KEY_SETTINGS,
					   0x00U, 0x00U, 0x00U};

typedef enum {
	DESFIRE_POLLER_STATE_READ_VERSION = 0,
	DESFIRE_POLLER_STATE_READ_FREE_MEMORY,
	DESFIRE_POLLER_STATE_READ_MASTER_KEY_SETTINGS,
	DESFIRE_POLLER_STATE_READ_APPLICATION_IDS,
	DESFIRE_POLLER_STATE_READ_APPLICATIONS,
	DESFIRE_POLLER_STATE_DONE,
} desfire_poller_state_t;

static bool desfire_poller_status_ok(const uint8_t *rx, size_t rx_len, uint8_t expect_sw2)
{
	if (rx_len < 2U) {
		return false;
	}

	return (rx[rx_len - 2U] == DESFIRE_SW1) && (rx[rx_len - 1U] == expect_sw2);
}

static int desfire_poller_select(nfc_reader_session_t *session)
{
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = desfire_poller_i_transceive(session, SELECT_AID, sizeof(SELECT_AID), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if ((rx_len < 2U) || (rx[rx_len - 2U] != 0x90U) || (rx[rx_len - 1U] != 0x00U)) {
		return -EIO;
	}

	return 0;
}

static int desfire_poller_parse_version(const uint8_t *buf, size_t len, desfire_data_t *out)
{
	if (len < DESFIRE_VERSION_TOTAL_BYTES) {
		return -EBADMSG;
	}

	(void)memcpy(out->hw_version, buf, DESFIRE_VERSION_SIZE);
	(void)memcpy(out->sw_version, &buf[DESFIRE_VERSION_SIZE], DESFIRE_VERSION_SIZE);
	(void)memcpy(out->uid, &buf[DESFIRE_VERSION_SIZE * 2U], DESFIRE_UID_SIZE);
	(void)memcpy(out->batch, &buf[DESFIRE_VERSION_SIZE * 2U + DESFIRE_UID_SIZE],
		     DESFIRE_BATCH_SIZE);
	out->prod_week = buf[DESFIRE_VERSION_SIZE * 2U + DESFIRE_UID_SIZE + DESFIRE_BATCH_SIZE];
	out->prod_year = buf[DESFIRE_VERSION_SIZE * 2U + DESFIRE_UID_SIZE + DESFIRE_BATCH_SIZE + 1U];
	return 0;
}

static bool desfire_poller_free_directory_list(uint8_t key_settings)
{
	return (key_settings & 0x08U) != 0U;
}

static int desfire_poller_read_application(nfc_reader_session_t *session,
					    desfire_application_t *app)
{
	uint8_t file_ids[CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP];
	uint8_t file_count = 0U;
	uint8_t key_settings = 0U;
	uint8_t max_keys = 0U;
	int ret;

	ret = desfire_poller_i_read_key_settings(session, &key_settings, &max_keys);
	if (ret == DESFIRE_POLLER_I_AUTH) {
		LOG_DBG("Auth required for app key settings; partial app shell");
		app->key_settings = (uint8_t)(key_settings & (uint8_t)~0x08U);
		app->key_count = 0U;
		app->file_count = 0U;
		return 0;
	}
	if (ret != 0) {
		return ret;
	}

	app->key_settings = key_settings;
	app->key_count = max_keys;

	ret = desfire_poller_i_read_file_ids(session, file_ids, sizeof(file_ids), &file_count);
	if (ret != 0) {
		return ret;
	}

	app->file_count = 0U;

	for (uint8_t fi = 0U; fi < file_count; fi++) {
		desfire_file_settings_t fs;
		uint32_t file_size = 0U;
		uint8_t slot = app->file_count;

		if (slot >= CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP) {
			break;
		}

		ret = desfire_poller_i_read_file_settings(session, file_ids[fi], &fs, &file_size);
		if (ret != 0) {
			LOG_DBG("Skip file %u settings: %d", file_ids[fi], ret);
			continue;
		}

		if ((fs.type != DESFIRE_FILE_TYPE_STANDARD) &&
		    (fs.type != DESFIRE_FILE_TYPE_BACKUP)) {
			LOG_DBG("Skip unsupported file type %u", (unsigned int)fs.type);
			continue;
		}

		if (!desfire_access_free_read(fs.access_rights)) {
			LOG_DBG("Can't read file %u without authentication", file_ids[fi]);
			continue;
		}

		if (file_size > CONFIG_NFC_DESFIRE_MAX_FILE_SIZE) {
			LOG_DBG("Skip oversized file %u", file_ids[fi]);
			continue;
		}

		app->file_ids[slot] = file_ids[fi];
		app->file_settings[slot] = fs;
		app->file_data_len[slot] = (uint16_t)file_size;

		if (file_size > 0U) {
			size_t read_len = 0U;

			ret = desfire_poller_i_read_file_data(session, file_ids[fi], 0U, file_size,
							    app->file_data[slot],
							    CONFIG_NFC_DESFIRE_MAX_FILE_SIZE,
							    &read_len);
			if (ret != 0) {
				LOG_DBG("Skip file %u data: %d", file_ids[fi], ret);
				continue;
			}

			app->file_data_len[slot] = (uint16_t)read_len;
		}

		app->file_count++;
	}

	return 0;
}

int desfire_poller_detect(const nfc_reader_session_t *session)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	ret = desfire_poller_select(mut);
	if (ret != 0) {
		return -ENOTSUP;
	}

	ret = desfire_poller_i_transceive(mut, GET_KEY_VERSION, sizeof(GET_KEY_VERSION), rx, &rx_len);
	if ((ret != 0) || !desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK)) {
		return -ENOTSUP;
	}

	ret = desfire_poller_i_transceive(mut, GET_VERSION, sizeof(GET_VERSION), rx, &rx_len);
	if (ret != 0) {
		return -ENOTSUP;
	}

	if (!desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK) &&
	    !desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_ADDITIONAL_FRAME)) {
		return -ENOTSUP;
	}

	return 0;
}

int desfire_poller_read(const nfc_reader_session_t *session, desfire_data_t *out)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	uint8_t version_buf[DESFIRE_VERSION_TOTAL_BYTES];
	uint8_t app_ids[CONFIG_NFC_DESFIRE_MAX_APPS * DESFIRE_APP_ID_SIZE];
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	uint8_t app_id_count = 0U;
	size_t version_len = 0U;
	size_t rx_len = 0U;
	bool free_directory = false;
	desfire_poller_state_t state = DESFIRE_POLLER_STATE_READ_VERSION;
	int ret;

	if ((session == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	desfire_data_reset(out);

	ret = desfire_poller_select(mut);
	if (ret != 0) {
		return ret;
	}

	ret = desfire_poller_i_transceive(mut, GET_KEY_VERSION, sizeof(GET_KEY_VERSION), rx, &rx_len);
	if ((ret != 0) || !desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK)) {
		return -EIO;
	}

	while (state != DESFIRE_POLLER_STATE_DONE) {
		switch (state) {
		case DESFIRE_POLLER_STATE_READ_VERSION:
			ret = desfire_poller_i_send_chunks(mut, GET_VERSION, sizeof(GET_VERSION),
							   version_buf, sizeof(version_buf),
							   &version_len);
			if (ret != 0) {
				return ret;
			}

			ret = desfire_poller_parse_version(version_buf, version_len, out);
			if (ret != 0) {
				return ret;
			}

			state = DESFIRE_POLLER_STATE_READ_FREE_MEMORY;
			break;

		case DESFIRE_POLLER_STATE_READ_FREE_MEMORY:
			ret = desfire_poller_i_transceive(mut, GET_FREE_MEMORY,
							  sizeof(GET_FREE_MEMORY), rx, &rx_len);
			if ((ret == 0) && desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK) &&
			    (rx_len >= 6U)) {
				out->free_memory = (uint32_t)rx[0] | ((uint32_t)rx[1] << 8U) |
						   ((uint32_t)rx[2] << 16U) |
						   ((uint32_t)rx[3] << 24U);
			}

			state = DESFIRE_POLLER_STATE_READ_MASTER_KEY_SETTINGS;
			break;

		case DESFIRE_POLLER_STATE_READ_MASTER_KEY_SETTINGS:
			ret = desfire_poller_i_transceive(mut, GET_KEY_SETTINGS,
							  sizeof(GET_KEY_SETTINGS), rx, &rx_len);
			if ((ret == 0) &&
			    desfire_poller_status_ok(rx, rx_len, DESFIRE_STATUS_OK) &&
			    (rx_len >= 3U)) {
				out->master_key_settings = rx[0];
				free_directory = desfire_poller_free_directory_list(rx[0]);
			} else if ((ret == 0) && (rx_len >= 2U) &&
				   (rx[rx_len - 1U] == DESFIRE_STATUS_AUTH_ERROR)) {
				LOG_DBG("Auth required for master key settings");
				out->master_key_settings = 0U;
				free_directory = false;
			} else if (ret != 0) {
				return ret;
			}

			if (free_directory) {
				state = DESFIRE_POLLER_STATE_READ_APPLICATION_IDS;
			} else {
				state = DESFIRE_POLLER_STATE_DONE;
			}
			break;

		case DESFIRE_POLLER_STATE_READ_APPLICATION_IDS:
			ret = desfire_poller_i_read_application_ids(mut, app_ids, sizeof(app_ids),
								    &app_id_count);
			if (ret == DESFIRE_POLLER_I_AUTH) {
				LOG_DBG("Auth required for application IDs; PICC partial only");
				state = DESFIRE_POLLER_STATE_DONE;
				break;
			}
			if (ret != 0) {
				return ret;
			}

			if (app_id_count == 0U) {
				state = DESFIRE_POLLER_STATE_DONE;
			} else {
				state = DESFIRE_POLLER_STATE_READ_APPLICATIONS;
			}
			break;

		case DESFIRE_POLLER_STATE_READ_APPLICATIONS:
			out->app_count = 0U;

			for (uint8_t ai = 0U; ai < app_id_count; ai++) {
				desfire_application_t *app;
				const uint8_t *aid = &app_ids[(size_t)ai * DESFIRE_APP_ID_SIZE];

				if (out->app_count >= CONFIG_NFC_DESFIRE_MAX_APPS) {
					break;
				}

				ret = desfire_poller_i_select_application(mut, aid);
				if (ret != 0) {
					LOG_DBG("Skip app %u select: %d", ai, ret);
					continue;
				}

				app = &out->apps[out->app_count];
				(void)memcpy(app->app_id, aid, DESFIRE_APP_ID_SIZE);

				ret = desfire_poller_read_application(mut, app);
				if (ret != 0) {
					LOG_DBG("Skip app %u read: %d", ai, ret);
					continue;
				}

				out->app_count++;
			}

			state = DESFIRE_POLLER_STATE_DONE;
			break;

		default:
			state = DESFIRE_POLLER_STATE_DONE;
			break;
		}
	}

	return 0;
}
