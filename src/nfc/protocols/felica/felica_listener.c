/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/felica/felica_listener.c
 */

#include "protocols/felica/felica_listener.h"

#include "helpers/felica_crc.h"
#include "protocols/felica/felica_poller_i.h"

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#define FELICA_LISTENER_CMD_POLLING      0x00U
#define FELICA_LISTENER_RESP_POLLING     0x01U
#define FELICA_LISTENER_RESP_READ        0x07U

static felica_data_t s_model;
static bool s_initialized;
static uint8_t s_resp[FELICA_POLLER_MAX_BUFFER_SIZE];

static bool felica_listener_block_exists(uint8_t wire_block)
{
	if ((wire_block > FELICA_BLOCK_INDEX_REG) && (wire_block < FELICA_BLOCK_INDEX_RC)) {
		return false;
	}
	if ((wire_block > FELICA_BLOCK_INDEX_MC) && (wire_block < FELICA_BLOCK_INDEX_WCNT)) {
		return false;
	}
	if ((wire_block > FELICA_BLOCK_INDEX_STATE) && (wire_block < FELICA_BLOCK_INDEX_CRC_CHECK)) {
		return false;
	}
	if (wire_block > FELICA_BLOCK_INDEX_CRC_CHECK) {
		return false;
	}

	return true;
}

static void felica_listener_send(const uint8_t *data, size_t len)
{
	(void)nfc_transport_send_response(data, len);
}

static void felica_listener_handle_poll(void)
{
	size_t offset = 0U;

	s_resp[offset++] = (uint8_t)(2U + FELICA_IDM_SIZE + FELICA_PMM_SIZE);
	s_resp[offset++] = FELICA_LISTENER_RESP_POLLING;
	(void)memcpy(&s_resp[offset], s_model.idm, FELICA_IDM_SIZE);
	offset += FELICA_IDM_SIZE;
	(void)memcpy(&s_resp[offset], s_model.pmm, FELICA_PMM_SIZE);
	offset += FELICA_PMM_SIZE;

	felica_crc_append(s_resp, &offset);
	felica_listener_send(s_resp, offset);
}

static void felica_listener_handle_read(const uint8_t *frame, size_t frame_len)
{
	const felica_command_header_t *hdr;
	const felica_block_list_element_t *block_item;
	uint8_t dump_idx;
	size_t offset = 0U;
	size_t need;

	if (frame_len < (1U + sizeof(felica_command_header_t) + sizeof(felica_block_list_element_t))) {
		return;
	}

	hdr = (const felica_command_header_t *)&frame[1];
	if (memcmp(hdr->idm, s_model.idm, FELICA_IDM_SIZE) != 0) {
		return;
	}
	if ((hdr->service_num != 1U) || (hdr->block_count != 1U) ||
	    (hdr->service_code != FELICA_SERVICE_RO_ACCESS)) {
		return;
	}

	block_item = (const felica_block_list_element_t *)&frame[1U + sizeof(felica_command_header_t)];
	if ((block_item->service_code != 0U) || (block_item->access_mode != 0U) ||
	    (block_item->length != 1U) ||
	    !felica_listener_block_exists(block_item->block_number)) {
		return;
	}

	need = 2U + FELICA_IDM_SIZE + 3U + FELICA_BLOCK_DATA_SIZE;
	s_resp[offset++] = (uint8_t)need;
	s_resp[offset++] = FELICA_LISTENER_RESP_READ;
	(void)memcpy(&s_resp[offset], s_model.idm, FELICA_IDM_SIZE);
	offset += FELICA_IDM_SIZE;
	s_resp[offset++] = 0U;
	s_resp[offset++] = 0U;
	s_resp[offset++] = 1U;

	if (block_item->block_number == FELICA_BLOCK_INDEX_RC) {
		(void)memset(&s_resp[offset], 0, FELICA_BLOCK_DATA_SIZE);
	} else {
		dump_idx = felica_poller_lite_wire_to_dump_index(block_item->block_number);
		if (dump_idx >= s_model.blocks_total) {
			return;
		}
		(void)memcpy(&s_resp[offset], s_model.blocks[dump_idx].data, FELICA_BLOCK_DATA_SIZE);
	}
	offset += FELICA_BLOCK_DATA_SIZE;

	felica_crc_append(s_resp, &offset);
	felica_listener_send(s_resp, offset);
}

static void felica_listener_on_tag_cmd(const uint8_t *tx, size_t tx_len, void *user_ctx)
{
	uint8_t frame[FELICA_POLLER_MAX_BUFFER_SIZE];
	size_t frame_len;

	ARG_UNUSED(user_ctx);

	if (!s_initialized || (tx == NULL) || (tx_len < (1U + FELICA_CRC_SIZE))) {
		return;
	}

	if (!felica_crc_check(tx, tx_len)) {
		return;
	}

	if (tx_len > sizeof(frame)) {
		return;
	}

	(void)memcpy(frame, tx, tx_len);
	frame_len = tx_len;
	felica_crc_trim(frame, &frame_len);

	if (frame_len < 2U) {
		return;
	}

	switch (frame[1]) {
	case FELICA_LISTENER_CMD_POLLING:
		felica_listener_handle_poll();
		break;
	case FELICA_CMD_READ_WITHOUT_ENCRYPTION:
		felica_listener_handle_read(frame, frame_len);
		break;
	default:
		break;
	}
}

static int felica_listener_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	return felica_serialize(&s_model, out, out_max, out_len);
}

static int felica_listener_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	int ret;

	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	ret = felica_deserialize(&s_model, in, in_len);
	return ret;
}

static const nfc_service_t s_felica_listener_service = {
	.on_tag_cmd = felica_listener_on_tag_cmd,
	.serialize = felica_listener_serialize,
	.deserialize = felica_listener_deserialize,
	.persist_id = NFC_PERSIST_ID_FELICA,
	.user_ctx = NULL,
};

int felica_listener_init(const felica_data_t *model)
{
	if (model != NULL) {
		s_model = *model;
	} else {
		felica_data_reset(&s_model);
	}

	s_initialized = true;
	return 0;
}

int felica_listener_shutdown(void)
{
	felica_data_reset(&s_model);
	s_initialized = false;
	return 0;
}

const nfc_service_t *felica_listener_get(void)
{
	return &s_felica_listener_service;
}
