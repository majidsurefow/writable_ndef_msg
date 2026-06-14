/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * DESFire poller internal I/O — APDU transceive + 0x91AF chunking.
 */

#ifndef NFC_PROTOCOLS_DESFIRE_POLLER_I_H_
#define NFC_PROTOCOLS_DESFIRE_POLLER_I_H_

#include "desfire.h"
#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DESFIRE_POLLER_I_AUTH (-EACCES)

int desfire_poller_i_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				uint8_t *rx, size_t *rx_len);

int desfire_poller_i_send_chunks(nfc_reader_session_t *session, const uint8_t *first_tx,
				 size_t first_tx_len, uint8_t *out, size_t out_max,
				 size_t *out_len);

int desfire_poller_i_read_application_ids(nfc_reader_session_t *session, uint8_t *ids_out,
					  size_t ids_max, uint8_t *count_out);

int desfire_poller_i_select_application(nfc_reader_session_t *session,
					const uint8_t app_id[DESFIRE_APP_ID_SIZE]);

int desfire_poller_i_read_key_settings(nfc_reader_session_t *session, uint8_t *settings_out,
				       uint8_t *max_keys_out);

int desfire_poller_i_read_file_ids(nfc_reader_session_t *session, uint8_t *ids_out,
				   size_t ids_max, uint8_t *count_out);

int desfire_poller_i_read_file_settings(nfc_reader_session_t *session, uint8_t file_id,
					desfire_file_settings_t *out, uint32_t *size_out);

int desfire_poller_i_read_file_data(nfc_reader_session_t *session, uint8_t file_id, uint32_t offset,
				    uint32_t length, uint8_t *out, size_t out_max,
				    size_t *out_len);

int desfire_poller_i_read_file_value(nfc_reader_session_t *session, uint8_t file_id, uint8_t *out,
				     size_t out_max, size_t *out_len);

int desfire_poller_i_read_file_records(nfc_reader_session_t *session, uint8_t file_id,
				       uint32_t offset, uint32_t length, uint8_t *out,
				       size_t out_max, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_DESFIRE_POLLER_I_H_ */
