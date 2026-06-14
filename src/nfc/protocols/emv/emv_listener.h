/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NFC_PROTOCOLS_EMV_LISTENER_H_
#define NFC_PROTOCOLS_EMV_LISTENER_H_

#include "emv.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

int emv_listener_init(const emv_card_image_t *image);
int emv_listener_shutdown(void);
const nfc_service_t *emv_listener_get(void);
emv_session_state_t emv_listener_session_state(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_EMV_LISTENER_H_ */
