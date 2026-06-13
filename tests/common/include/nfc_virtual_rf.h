/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Software loopback bus — poller TX → listener RX (cookbook §14.12 Phase 1b).
 */

#ifndef TESTS_COMMON_NFC_VIRTUAL_RF_H_
#define TESTS_COMMON_NFC_VIRTUAL_RF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "reader/nfc_reader_engine.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief True while virtual RF intercepts nfc_reader_session_transceive(). */
bool nfc_virtual_rf_is_enabled(void);

/**
 * @brief Wire listener service into the virtual RF bus.
 *
 * SELECT-by-AID C-APDUs invoke @p listener_svc->on_select; all others go to on_apdu.
 * Responses are captured from nfc_transport_send_response() (nfc_response_spy).
 */
int nfc_virtual_rf_enable(const nfc_service_t *listener_svc);

/** @brief Restore scripted nfc_session_mock transceive behaviour. */
void nfc_virtual_rf_disable(void);

/**
 * @brief Loopback transceive — called from nfc_session_mock when virtual RF is on.
 */
int nfc_virtual_rf_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
			      uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* TESTS_COMMON_NFC_VIRTUAL_RF_H_ */
