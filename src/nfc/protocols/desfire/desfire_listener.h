/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * DESFire ISO-DEP listener — partial PICC (cookbook §5.4 / wave5-desfire).
 */

#ifndef NFC_PROTOCOLS_DESFIRE_LISTENER_H_
#define NFC_PROTOCOLS_DESFIRE_LISTENER_H_

#include "desfire.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

int desfire_listener_init(const desfire_data_t *model);
int desfire_listener_shutdown(void);
const nfc_service_t *desfire_listener_get(void);

desfire_auth_state_t desfire_listener_auth_state(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_DESFIRE_LISTENER_H_ */
