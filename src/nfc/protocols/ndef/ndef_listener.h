/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Type-4 listener — T4T FSM (cookbook §5.1 Gate 3).
 */

#ifndef NFC_PROTOCOLS_NDEF_LISTENER_H_
#define NFC_PROTOCOLS_NDEF_LISTENER_H_

#include <stdbool.h>

#include "ndef.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool writable;
} ndef_listener_config_t;

#define NDEF_LISTENER_CONFIG_DEFAULT \
	((ndef_listener_config_t){ .writable = true })

/**
 * @brief Bind listener to an NDEF model and build emulated CC/NDEF file view.
 *
 * @param model  Cloned tag data; NULL → empty NDEF (NLEN=0) with default CC.
 * @param cfg    NULL → writable default.
 */
int ndef_listener_init(const ndef_data_t *model, const ndef_listener_config_t *cfg);

/** @brief Clear session state; model bytes retained until next init. */
int ndef_listener_shutdown(void);

/** @brief Service vtable for aid_router / unit tests. */
const nfc_service_t *ndef_listener_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_NDEF_LISTENER_H_ */
