/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/felica/felica_poller.c
 */

#include "protocols/felica/felica_poller.h"

#include "protocols/felica/felica_poller_i.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>

#define FELICA_POLLER_TIMEOUT K_MSEC(5000)

static int felica_poller_session_valid(const nfc_reader_session_t *session)
{
	if ((session == NULL) || !session->active) {
		return -EINVAL;
	}

	return 0;
}

static int felica_poller_poll(nfc_reader_session_t *session, felica_data_t *data)
{
	const felica_poller_polling_command_t cmd = {
		.system_code = FELICA_SYSTEM_CODE_ANY,
		.request_code = 0U,
		.time_slot = FELICA_TIME_SLOT_1,
	};
	felica_poller_polling_response_t resp;
	int ret;

	ret = felica_poller_polling(session, &cmd, &resp, FELICA_POLLER_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	(void)memcpy(data->idm, resp.idm, FELICA_IDM_SIZE);
	(void)memcpy(data->pmm, resp.pmm, FELICA_PMM_SIZE);
	return 0;
}

static int felica_poller_read_lite_blocks(nfc_reader_session_t *session, felica_data_t *data)
{
	uint8_t block_index = 0U;
	uint8_t block_list[1];
	int ret;

	while (data->blocks_total < FELICA_BLOCKS_LITE_TOTAL) {
		felica_poller_read_response_t response;

		block_list[0] = block_index;
		block_index = felica_poller_lite_next_block_index(block_index);

		ret = felica_poller_read_blocks(session, data->idm, 1U, block_list,
						FELICA_SERVICE_RO_ACCESS, &response,
						FELICA_POLLER_TIMEOUT);
		if (ret != 0) {
			return ret;
		}

		data->blocks[data->blocks_total].sf1 = response.sf1;
		data->blocks[data->blocks_total].sf2 = response.sf2;
		if (response.sf1 == 0U) {
			(void)memcpy(data->blocks[data->blocks_total].data, response.data,
				     FELICA_BLOCK_DATA_SIZE);
			data->blocks_read++;
		} else {
			(void)memset(data->blocks[data->blocks_total].data, 0,
				     FELICA_BLOCK_DATA_SIZE);
		}

		data->blocks_total++;
	}

	return 0;
}

int felica_poller_detect(const nfc_reader_session_t *session)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	felica_data_t scratch;
	int ret;

	ret = felica_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}

	felica_data_reset(&scratch);
	ret = felica_poller_poll(mut, &scratch);
	if (ret != 0) {
		return -ENOTSUP;
	}

	return 0;
}

int felica_poller_read(const nfc_reader_session_t *session, felica_data_t *out)
{
	nfc_reader_session_t *mut = (nfc_reader_session_t *)session;
	int ret;

	ret = felica_poller_session_valid(session);
	if (ret != 0) {
		return ret;
	}
	if (out == NULL) {
		return -EINVAL;
	}

	felica_data_reset(out);
	ret = felica_poller_poll(mut, out);
	if (ret != 0) {
		return ret;
	}

	if (felica_get_workflow_type(out->pmm) != FELICA_WORKFLOW_LITE) {
		return -ENOTSUP;
	}

	ret = felica_poller_read_lite_blocks(mut, out);
	if (ret != 0) {
		return ret;
	}

	if (out->blocks_total == 0U) {
		return -EIO;
	}

	return 0;
}
