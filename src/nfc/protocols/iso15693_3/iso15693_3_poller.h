/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NFC_PROTOCOLS_ISO15693_3_POLLER_H_
#define NFC_PROTOCOLS_ISO15693_3_POLLER_H_

#include "protocols/iso15693_3/iso15693_3.h"
#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISO15693_CMD_INVENTORY      0x01U
#define ISO15693_CMD_READ           0x20U
#define ISO15693_CMD_GET_SYS_INFO   0x2BU
#define ISO15693_INV_FLAGS          0x26U

int iso15693_3_poller_detect(const nfc_reader_session_t *session);
int iso15693_3_poller_inventory(nfc_reader_session_t *session, iso15693_3_data_t *data);
int iso15693_3_poller_read(const nfc_reader_session_t *session, iso15693_3_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ISO15693_3_POLLER_H_ */
