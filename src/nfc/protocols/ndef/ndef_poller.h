/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Type-4 poller — detect/read via reader session (cookbook §5.1).
 */

#ifndef NFC_PROTOCOLS_NDEF_POLLER_H_
#define NFC_PROTOCOLS_NDEF_POLLER_H_

#include "ndef.h"
#include "reader/nfc_reader_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Probe for NDEF Type-4 application on an active ISO-DEP session.
 *
 * @nfc_stack_wq
 *
 * @return 0 if NDEF AID v2.0 or v1.0 matches; -ENOTSUP if not NDEF;
 *         -EINVAL if @p session is NULL or inactive; other negative errno on I/O.
 */
int ndef_poller_detect(const nfc_reader_session_t *session);

/**
 * @brief Read CC + NDEF file into @p out via NXP RW_NDEF_T4T sequence.
 *
 * @nfc_stack_wq
 *
 * @return 0 on success; -EINVAL if @p session or @p out is NULL or session inactive;
 *         -ENOSPC if NLEN exceeds CONFIG_NFC_NDEF_MAX_SIZE; -EIO on SW/length errors.
 */
int ndef_poller_read(const nfc_reader_session_t *session, ndef_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_NDEF_POLLER_H_ */
