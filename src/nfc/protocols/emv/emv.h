/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMV contactless card image — cookbook §5.5 / wave5-emv.
 */

#ifndef NFC_PROTOCOLS_EMV_H_
#define NFC_PROTOCOLS_EMV_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_NFC_EMV_MAX_RECORDS
#define CONFIG_NFC_EMV_MAX_RECORDS 2
#endif
#ifndef CONFIG_NFC_EMV_RECORD_SIZE
#define CONFIG_NFC_EMV_RECORD_SIZE 64
#endif

#define EMV_FORMAT_VERSION        0x01U
#define EMV_PPSE_AID_LEN          14U
#define EMV_SERVICE_APP_AID_LEN    7U
#define EMV_PAN_BYTES              8U
#define EMV_EXPIRY_BYTES           3U
#define EMV_NAME_BYTES            26U
#define EMV_TRACK2_MAX_BYTES      19U
#define EMV_AIP_BYTES              2U
#define EMV_AFL_BYTES              4U
#define EMV_APP_AID_MAX_BYTES      16U

#define EMV_TAG_RECORD_TMPL        0x70U
#define EMV_TAG_RESP_TMPL_FMT1     0x80U

extern const uint8_t emv_ppse_aid[EMV_PPSE_AID_LEN];
extern const uint8_t emv_service_app_aid[EMV_SERVICE_APP_AID_LEN];

typedef struct {
	uint8_t pan[EMV_PAN_BYTES];
	uint8_t pan_len;
	uint8_t expiry[EMV_EXPIRY_BYTES];
	uint8_t name[EMV_NAME_BYTES];
	uint8_t track2[EMV_TRACK2_MAX_BYTES];
	uint8_t track2_len;
	uint8_t aip[EMV_AIP_BYTES];
	uint8_t afl[EMV_AFL_BYTES];
	uint8_t app_aid[EMV_APP_AID_MAX_BYTES];
	uint8_t app_aid_len;
	uint8_t record_count;
	uint8_t record_len[CONFIG_NFC_EMV_MAX_RECORDS];
	uint8_t record_data[CONFIG_NFC_EMV_MAX_RECORDS][CONFIG_NFC_EMV_RECORD_SIZE];
} emv_card_image_t;

typedef enum {
	EMV_SESSION_IDLE = 0,
	EMV_SESSION_PPSE_SELECTED,
	EMV_SESSION_APP_SELECTED,
	EMV_SESSION_GPO_DONE,
} emv_session_state_t;

void emv_card_image_reset(emv_card_image_t *image);
void emv_card_image_load_default(emv_card_image_t *image);
uint8_t emv_persist_id(void);

int emv_serialize(const emv_card_image_t *image, uint8_t *out, size_t out_max, size_t *out_len);
int emv_deserialize(emv_card_image_t *image, const uint8_t *in, size_t in_len);
int emv_rebuild_caches(const emv_card_image_t *image);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_EMV_H_ */
