/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/felica/felica_poller_i.c
 */

#include "protocols/felica/felica_poller_i.h"

#include "helpers/felica_crc.h"

#include <errno.h>
#include <string.h>

int felica_poller_frame_exchange(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				 uint8_t *rx, size_t rx_max, size_t *rx_len,
				 k_timeout_t timeout)
{
	uint8_t tx_framed[FELICA_POLLER_MAX_BUFFER_SIZE];
	size_t framed_len = tx_len;
	int ret;

	if ((session == NULL) || (tx == NULL) || (rx == NULL) || (rx_len == NULL)) {
		return -EINVAL;
	}

	if ((tx_len == 0U) || ((tx_len + FELICA_CRC_SIZE) > sizeof(tx_framed))) {
		return -EINVAL;
	}

	(void)memcpy(tx_framed, tx, tx_len);
	felica_crc_append(tx_framed, &framed_len);

	ret = nfc_reader_session_transceive(session, tx_framed, framed_len, rx, rx_max, rx_len,
					    timeout);
	if (ret != 0) {
		return ret;
	}

	if (!felica_crc_check(rx, *rx_len)) {
		return -EIO;
	}

	felica_crc_trim(rx, rx_len);
	return 0;
}

int felica_poller_polling(nfc_reader_session_t *session,
			  const felica_poller_polling_command_t *cmd,
			  felica_poller_polling_response_t *resp, k_timeout_t timeout)
{
	uint8_t tx[6U];
	uint8_t rx[FELICA_POLLER_MAX_BUFFER_SIZE];
	size_t rx_len = 0U;
	int ret;

	if ((session == NULL) || (cmd == NULL) || (resp == NULL)) {
		return -EINVAL;
	}

	tx[0] = (uint8_t)(sizeof(felica_poller_polling_command_t) + FELICA_CRC_SIZE);
	tx[1] = FELICA_POLLER_CMD_POLLING_REQ_CODE;
	tx[2] = (uint8_t)(cmd->system_code & 0xFFU);
	tx[3] = (uint8_t)((cmd->system_code >> 8U) & 0xFFU);
	tx[4] = cmd->request_code;
	tx[5] = cmd->time_slot;

	ret = felica_poller_frame_exchange(session, tx, sizeof(tx), rx, sizeof(rx), &rx_len,
					   timeout);
	if (ret != 0) {
		return ret;
	}

	if ((rx_len < (2U + FELICA_IDM_SIZE + FELICA_PMM_SIZE)) ||
	    (rx[1] != FELICA_POLLER_CMD_POLLING_RESP_CODE)) {
		return -EIO;
	}

	(void)memcpy(resp->idm, &rx[2], FELICA_IDM_SIZE);
	(void)memcpy(resp->pmm, &rx[2U + FELICA_IDM_SIZE], FELICA_PMM_SIZE);
	return 0;
}

void felica_poller_prepare_tx_buffer(uint8_t *tx, size_t *tx_len, const uint8_t idm[FELICA_IDM_SIZE],
				     uint8_t command, uint16_t service_code, uint8_t block_count,
				     const uint8_t *block_numbers)
{
	felica_command_header_t cmd;
	felica_block_list_element_t block_list[4];
	size_t block_list_size;
	size_t total_size;
	size_t offset = 0U;

	if ((tx == NULL) || (tx_len == NULL) || (idm == NULL) || (block_numbers == NULL)) {
		return;
	}

	cmd.code = command;
	(void)memcpy(cmd.idm, idm, FELICA_IDM_SIZE);
	cmd.service_num = 1U;
	cmd.service_code = service_code;
	cmd.block_count = block_count;

	for (uint8_t i = 0U; i < block_count; i++) {
		block_list[i].service_code = 0U;
		block_list[i].access_mode = 0U;
		block_list[i].length = 1U;
		block_list[i].block_number = block_numbers[i];
	}

	block_list_size = (size_t)block_count * sizeof(felica_block_list_element_t);
	total_size = sizeof(felica_command_header_t) + 1U + block_list_size;

	tx[offset++] = (uint8_t)total_size;
	(void)memcpy(&tx[offset], &cmd, sizeof(cmd));
	offset += sizeof(cmd);
	(void)memcpy(&tx[offset], block_list, block_list_size);
	offset += block_list_size;
	*tx_len = offset;
}

int felica_poller_read_blocks(nfc_reader_session_t *session, const uint8_t idm[FELICA_IDM_SIZE],
			      const uint8_t block_count, const uint8_t *block_numbers,
			      uint16_t service_code, felica_poller_read_response_t *resp,
			      k_timeout_t timeout)
{
	uint8_t tx[FELICA_POLLER_MAX_BUFFER_SIZE];
	uint8_t rx[FELICA_POLLER_MAX_BUFFER_SIZE];
	size_t tx_len = 0U;
	size_t rx_len = 0U;
	size_t need;
	int ret;

	if ((session == NULL) || (idm == NULL) || (block_numbers == NULL) || (resp == NULL)) {
		return -EINVAL;
	}

	if ((block_count == 0U) || (block_count > 4U)) {
		return -EINVAL;
	}

	felica_poller_prepare_tx_buffer(tx, &tx_len, idm, FELICA_CMD_READ_WITHOUT_ENCRYPTION,
					service_code, block_count, block_numbers);

	ret = felica_poller_frame_exchange(session, tx, tx_len, rx, sizeof(rx), &rx_len, timeout);
	if (ret != 0) {
		return ret;
	}

	need = 2U + FELICA_IDM_SIZE + 3U + FELICA_BLOCK_DATA_SIZE;
	if ((rx_len < need) || (rx[1] != FELICA_CMD_READ_RESP_CODE)) {
		return -EIO;
	}

	resp->length = rx[0];
	resp->response_code = rx[1];
	(void)memcpy(resp->idm, &rx[2], FELICA_IDM_SIZE);
	resp->sf1 = rx[2U + FELICA_IDM_SIZE];
	resp->sf2 = rx[2U + FELICA_IDM_SIZE + 1U];
	resp->block_count = rx[2U + FELICA_IDM_SIZE + 2U];
	(void)memcpy(resp->data, &rx[2U + FELICA_IDM_SIZE + 3U], FELICA_BLOCK_DATA_SIZE);
	return 0;
}

void felica_poller_prepare_tx_buffer_raw(uint8_t *tx, size_t *tx_len,
					 const uint8_t idm[FELICA_IDM_SIZE], uint8_t command,
					 const uint8_t *data, uint8_t data_length)
{
	size_t total_size;
	size_t offset = 0U;

	if ((tx == NULL) || (tx_len == NULL) || (idm == NULL)) {
		return;
	}

	total_size = 2U + FELICA_IDM_SIZE + (size_t)data_length;
	tx[offset++] = (uint8_t)total_size;
	tx[offset++] = command;
	(void)memcpy(&tx[offset], idm, FELICA_IDM_SIZE);
	offset += FELICA_IDM_SIZE;
	if ((data_length > 0U) && (data != NULL)) {
		(void)memcpy(&tx[offset], data, data_length);
		offset += data_length;
	}
	*tx_len = offset;
}

uint8_t felica_poller_lite_next_block_index(uint8_t block_index)
{
	block_index++;

	if (block_index == (FELICA_BLOCK_INDEX_REG + 1U)) {
		return FELICA_BLOCK_INDEX_RC;
	}
	if (block_index == (FELICA_BLOCK_INDEX_MC + 1U)) {
		return FELICA_BLOCK_INDEX_WCNT;
	}
	if (block_index == (FELICA_BLOCK_INDEX_STATE + 1U)) {
		return FELICA_BLOCK_INDEX_CRC_CHECK;
	}

	return block_index;
}
