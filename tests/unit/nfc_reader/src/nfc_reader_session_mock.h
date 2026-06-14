/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared nfc_reader_session mock for nfc_reader unit tests.
 */

#ifndef NFC_READER_SESSION_MOCK_H_
#define NFC_READER_SESSION_MOCK_H_

#include "reader/nfc_reader_engine.h"

/**
 * @brief Set the mock session to return from nfc_reader_session_get().
 * @param session Session data to copy. NULL clears the mock.
 */
void nfc_reader_session_mock_set(const nfc_reader_session_t *session);

/**
 * @brief Clear the mock session (nfc_reader_session_get() will return NULL).
 */
void nfc_reader_session_mock_clear(void);

#endif /* NFC_READER_SESSION_MOCK_H_ */
