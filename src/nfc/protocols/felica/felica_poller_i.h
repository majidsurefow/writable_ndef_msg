/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/felica/felica_poller_i.c
 */

#ifndef NFC_PROTOCOLS_FELICA_POLLER_I_H_
#define NFC_PROTOCOLS_FELICA_POLLER_I_H_

#include <stddef.h>
#include <stdint.h>

#include "protocols/felica/felica.h"
#include "reader/nfc_reader_engine.h"

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FELICA_POLLER_MAX_BUFFER_SIZE 256U
#define FELICA_POLLER_POLLING_FWT     200000U

#define FELICA_POLLER_CMD_POLLING_REQ_CODE  0x00U
#define FELICA_POLLER_CMD_POLLING_RESP_CODE 0x01U
#define FELICA_CMD_READ_RESP_CODE           0x07U

#define FELICA_SYSTEM_CODE_ANY 0xFFFFU
#define FELICA_TIME_SLOT_1     0x00U

#define FELICA_CMD_POLL                       0x00U
#define FELICA_CMD_READ_WITHOUT_ENCRYPTION    0x06U
#define FELICA_SERVICE_RO_ACCESS              0x000BU

#define FELICA_BLOCKS_LITE_TOTAL       28U
#define FELICA_BLOCK_INDEX_REG         0x0EU
#define FELICA_BLOCK_INDEX_RC          0x80U
#define FELICA_BLOCK_INDEX_MC          0x88U
#define FELICA_BLOCK_INDEX_WCNT        0x90U
#define FELICA_BLOCK_INDEX_STATE       0x92U
#define FELICA_BLOCK_INDEX_CRC_CHECK   0xA0U

typedef struct __packed {
	uint8_t code;
	uint8_t idm[FELICA_IDM_SIZE];
	uint8_t service_num;
	uint16_t service_code;
	uint8_t block_count;
} felica_command_header_t;

typedef struct __packed {
	uint8_t service_code : 4;
	uint8_t access_mode  : 3;
	uint8_t length       : 1;
	uint8_t block_number;
} felica_block_list_element_t;

typedef struct {
	uint16_t system_code;
	uint8_t request_code;
	uint8_t time_slot;
} felica_poller_polling_command_t;

typedef struct {
	uint8_t idm[FELICA_IDM_SIZE];
	uint8_t pmm[FELICA_PMM_SIZE];
} felica_poller_polling_response_t;

typedef struct {
	uint8_t length;
	uint8_t response_code;
	uint8_t idm[FELICA_IDM_SIZE];
	uint8_t sf1;
	uint8_t sf2;
	uint8_t block_count;
	uint8_t data[FELICA_BLOCK_DATA_SIZE];
} felica_poller_read_response_t;

int felica_poller_frame_exchange(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				 uint8_t *rx, size_t rx_max, size_t *rx_len,
				 k_timeout_t timeout);

int felica_poller_polling(nfc_reader_session_t *session,
			  const felica_poller_polling_command_t *cmd,
			  felica_poller_polling_response_t *resp, k_timeout_t timeout);

int felica_poller_read_blocks(nfc_reader_session_t *session, const uint8_t idm[FELICA_IDM_SIZE],
			      const uint8_t block_count, const uint8_t *block_numbers,
			      uint16_t service_code, felica_poller_read_response_t *resp,
			      k_timeout_t timeout);

void felica_poller_prepare_tx_buffer(uint8_t *tx, size_t *tx_len, const uint8_t idm[FELICA_IDM_SIZE],
				     uint8_t command, uint16_t service_code, uint8_t block_count,
				     const uint8_t *block_numbers);

void felica_poller_prepare_tx_buffer_raw(uint8_t *tx, size_t *tx_len,
					  const uint8_t idm[FELICA_IDM_SIZE], uint8_t command,
					  const uint8_t *data, uint8_t data_length);

uint8_t felica_poller_lite_next_block_index(uint8_t block_index);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_FELICA_POLLER_I_H_ */
