#ifndef NFC_PROTOCOLS_ALIRO_LISTENER_H_
#define NFC_PROTOCOLS_ALIRO_LISTENER_H_

#include "aliro.h"
#include "router/service.h"

int aliro_listener_init(const aliro_data_t *model);
int aliro_listener_shutdown(void);
const nfc_service_t *aliro_listener_get(void);
aliro_state_t aliro_listener_state(void);
bool aliro_listener_crypto_inflight(void);

#endif
