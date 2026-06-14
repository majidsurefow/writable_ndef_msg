/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Aliro credential data model — cookbook §5.6 / wave5-aliro.
 */

#ifndef NFC_PROTOCOLS_ALIRO_H_
#define NFC_PROTOCOLS_ALIRO_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_NFC_ALIRO_MAX_TRANSCRIPT
#define CONFIG_NFC_ALIRO_MAX_TRANSCRIPT 512
#endif

#define ALIRO_FORMAT_VERSION           0x01U
#define ALIRO_AID_LEN                  9U
#define ALIRO_P256_PUBLIC_KEY_SIZE     65U
#define ALIRO_NONCE_SIZE               16U
#define ALIRO_INS_AUTH0                  0x80U
#define ALIRO_INS_AUTH1                  0x81U
#define ALIRO_INS_EXCHANGE               0x82U
#define ALIRO_CLA_PROP                   0x80U

extern const uint8_t aliro_expedited_aid[ALIRO_AID_LEN];
extern const uint8_t aliro_stepup_aid[ALIRO_AID_LEN];

typedef enum {
	ALIRO_STATE_IDLE = 0,
	ALIRO_STATE_AWAIT_AUTH0,
	ALIRO_STATE_AWAIT_AUTH1,
	ALIRO_STATE_AWAIT_EXCHANGE,
	ALIRO_STATE_DONE,
	ALIRO_STATE_ERROR,
} aliro_state_t;

typedef struct {
	uint16_t features;
	uint8_t protocol_major;
	uint8_t protocol_minor;
	uint8_t credential_pubkey[ALIRO_P256_PUBLIC_KEY_SIZE];
	uint16_t transcript_len;
	uint8_t transcript[CONFIG_NFC_ALIRO_MAX_TRANSCRIPT];
} aliro_data_t;

void aliro_data_reset(aliro_data_t *data);
uint8_t aliro_persist_id(void);

int aliro_serialize(const aliro_data_t *data, uint8_t *out, size_t out_max, size_t *out_len);
int aliro_deserialize(aliro_data_t *data, const uint8_t *in, size_t in_len);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ALIRO_H_ */
