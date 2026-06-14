#ifndef NFC_PROTOCOLS_ALIRO_POLLER_H_
#define NFC_PROTOCOLS_ALIRO_POLLER_H_

#include "aliro.h"
#include "reader/nfc_reader_engine.h"

int aliro_poller_detect(const nfc_reader_session_t *session);
int aliro_poller_read(const nfc_reader_session_t *session, aliro_data_t *out);

#endif
