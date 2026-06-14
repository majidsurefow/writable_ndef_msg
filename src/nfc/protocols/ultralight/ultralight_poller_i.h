/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight_poller_i.c
 */

#ifndef NFC_PROTOCOLS_ULTRALIGHT_POLLER_I_H_
#define NFC_PROTOCOLS_ULTRALIGHT_POLLER_I_H_

#include "ultralight.h"
#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

int ultralight_poller_i_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				   uint8_t *rx, size_t *rx_len);

int ultralight_poller_i_read_page(nfc_reader_session_t *session, uint8_t page,
				  uint8_t out[ULTRALIGHT_READ_RESP_LEN]);

int ultralight_poller_i_get_version(nfc_reader_session_t *session, uint8_t ver[8]);

int ultralight_poller_i_read_signature(nfc_reader_session_t *session, uint8_t sig[32]);

int ultralight_poller_i_read_counter(nfc_reader_session_t *session, uint8_t index, uint8_t out[3]);

int ultralight_poller_i_read_tearing(nfc_reader_session_t *session, uint8_t index, uint8_t *flag);

int ultralight_poller_i_pwd_auth(nfc_reader_session_t *session, const uint8_t pwd[4]);

/** Flipper mf_ultralight_poller_authentication_test — probe 0x1A/0x00. */
int ultralight_poller_i_authentication_test(nfc_reader_session_t *session);

/** Full Ultralight C 3DES auth; key is 16-byte 2-key DES material. */
int ultralight_poller_i_authenticate(nfc_reader_session_t *session,
				     const uint8_t key[ULTRALIGHT_C_AUTH_DES_KEY_SIZE],
				     bool *authenticated);

#if defined(CONFIG_ZTEST)
void ultralight_poller_i_test_set_fixed_rnda(const uint8_t rnda[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE]);
#endif

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ULTRALIGHT_POLLER_I_H_ */
