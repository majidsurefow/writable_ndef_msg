/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "emv.h"

#include "router/service.h"

#include <errno.h>
#include <string.h>

const uint8_t emv_ppse_aid[EMV_PPSE_AID_LEN] = {
	0x32U, 0x50U, 0x41U, 0x59U, 0x2EU, 0x53U, 0x59U, 0x53U,
	0x2EU, 0x44U, 0x44U, 0x46U, 0x30U, 0x31U,
};

const uint8_t emv_service_app_aid[EMV_SERVICE_APP_AID_LEN] = {
	0xA0U, 0x00U, 0x00U, 0x00U, 0x03U, 0x10U, 0x10U,
};

typedef struct {
	uint8_t *buf;
	size_t cap;
	size_t pos;
	bool error;
} emv_tlv_writer_t;

static void emv_tlv_init(emv_tlv_writer_t *w, uint8_t *buf, size_t cap)
{
	w->buf = buf;
	w->cap = cap;
	w->pos = 0U;
	w->error = false;
}

static void emv_tlv_put_u8(emv_tlv_writer_t *w, uint8_t v)
{
	if (w->error || (w->pos >= w->cap)) {
		w->error = true;
		return;
	}

	w->buf[w->pos++] = v;
}

static void emv_tlv_put_bytes(emv_tlv_writer_t *w, const uint8_t *data, size_t len)
{
	for (size_t i = 0U; i < len; i++) {
		emv_tlv_put_u8(w, data[i]);
	}
}

static void emv_tlv_put_len(emv_tlv_writer_t *w, size_t len)
{
	emv_tlv_put_u8(w, (uint8_t)len);
}

static void emv_tlv_open(emv_tlv_writer_t *w, uint8_t tag)
{
	emv_tlv_put_u8(w, tag);
	emv_tlv_put_u8(w, 0U);
}

static void emv_tlv_close(emv_tlv_writer_t *w, size_t start)
{
	size_t len = w->pos - start - 1U;

	if (w->error || (len > 0x7FU)) {
		w->error = true;
		return;
	}

	w->buf[start] = (uint8_t)len;
}

static int emv_build_record0(emv_card_image_t *image)
{
	emv_tlv_writer_t w;
	size_t start;
	size_t pan_bytes;

	emv_tlv_init(&w, image->record_data[0], CONFIG_NFC_EMV_RECORD_SIZE);
	emv_tlv_open(&w, EMV_TAG_RECORD_TMPL);
	start = w.pos;
	pan_bytes = (size_t)image->pan_len / 2U;
	if (pan_bytes > EMV_PAN_BYTES) {
		pan_bytes = EMV_PAN_BYTES;
	}
	if (pan_bytes > 0U) {
		emv_tlv_put_u8(&w, 0x5AU);
		emv_tlv_put_len(&w, pan_bytes);
		emv_tlv_put_bytes(&w, image->pan, pan_bytes);
	}
	emv_tlv_put_u8(&w, 0x57U);
	emv_tlv_put_len(&w, image->track2_len);
	emv_tlv_put_bytes(&w, image->track2, image->track2_len);
	emv_tlv_close(&w, start);
	if (w.error) {
		return -ENOSPC;
	}

	image->record_len[0] = (uint8_t)w.pos;
	image->record_count = 1U;
	return 0;
}

void emv_card_image_reset(emv_card_image_t *image)
{
	if (image != NULL) {
		(void)memset(image, 0, sizeof(*image));
	}
}

void emv_card_image_load_default(emv_card_image_t *image)
{
	static const uint8_t pan[] = {0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U};
	static const uint8_t expiry[] = {0x99U, 0x12U, 0x31U};
	static const uint8_t name[] = "TEST/CARD                 ";
	static const uint8_t track2[] = {0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
					 0xD9U, 0x12U, 0x31U, 0x00U, 0x00U, 0x00U, 0x00U, 0x0FU};

	if (image == NULL) {
		return;
	}

	emv_card_image_reset(image);
	(void)memcpy(image->pan, pan, sizeof(pan));
	image->pan_len = 16U;
	(void)memcpy(image->expiry, expiry, sizeof(expiry));
	(void)memcpy(image->name, name, sizeof(name) - 1U);
	(void)memcpy(image->track2, track2, sizeof(track2));
	image->track2_len = (uint8_t)sizeof(track2);
	image->aip[0] = 0x00U;
	image->aip[1] = 0x00U;
	image->afl[0] = 0x08U;
	image->afl[1] = 0x01U;
	image->afl[2] = 0x01U;
	image->afl[3] = 0x00U;
	(void)memcpy(image->app_aid, emv_service_app_aid, EMV_SERVICE_APP_AID_LEN);
	image->app_aid_len = EMV_SERVICE_APP_AID_LEN;
	(void)emv_build_record0(image);
}

uint8_t emv_persist_id(void)
{
	return NFC_PERSIST_ID_EMV;
}

int emv_rebuild_caches(const emv_card_image_t *image)
{
	if (image == NULL) {
		return -EINVAL;
	}

	if ((image->app_aid_len == 0U) || (image->app_aid_len > EMV_APP_AID_MAX_BYTES)) {
		return -EBADMSG;
	}

	return 0;
}

int emv_serialize(const emv_card_image_t *image, uint8_t *out, size_t out_max, size_t *out_len)
{
	size_t offset = 0U;
	size_t need = 84U;

	if ((image == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	for (uint8_t i = 0U; i < image->record_count; i++) {
		need += 1U + image->record_len[i];
	}
	if (out_max < need) {
		return -ENOSPC;
	}

	out[offset++] = EMV_FORMAT_VERSION;
	out[offset++] = image->pan_len;
	(void)memcpy(&out[offset], image->pan, EMV_PAN_BYTES);
	offset += EMV_PAN_BYTES;
	(void)memcpy(&out[offset], image->expiry, EMV_EXPIRY_BYTES);
	offset += EMV_EXPIRY_BYTES;
	(void)memcpy(&out[offset], image->name, EMV_NAME_BYTES);
	offset += EMV_NAME_BYTES;
	out[offset++] = image->track2_len;
	(void)memcpy(&out[offset], image->track2, EMV_TRACK2_MAX_BYTES);
	offset += EMV_TRACK2_MAX_BYTES;
	(void)memcpy(&out[offset], image->aip, EMV_AIP_BYTES);
	offset += EMV_AIP_BYTES;
	(void)memcpy(&out[offset], image->afl, EMV_AFL_BYTES);
	offset += EMV_AFL_BYTES;
	out[offset++] = image->app_aid_len;
	(void)memcpy(&out[offset], image->app_aid, EMV_APP_AID_MAX_BYTES);
	offset += EMV_APP_AID_MAX_BYTES;
	out[offset++] = image->record_count;
	for (uint8_t i = 0U; i < image->record_count; i++) {
		out[offset++] = image->record_len[i];
		(void)memcpy(&out[offset], image->record_data[i], image->record_len[i]);
		offset += image->record_len[i];
	}

	*out_len = offset;
	return 0;
}

int emv_deserialize(emv_card_image_t *image, const uint8_t *in, size_t in_len)
{
	size_t offset = 0U;

	if ((image == NULL) || (in == NULL) || (in_len < 84U)) {
		return -EINVAL;
	}

	emv_card_image_reset(image);
	if (in[offset++] != EMV_FORMAT_VERSION) {
		return -EBADMSG;
	}

	image->pan_len = in[offset++];
	(void)memcpy(image->pan, &in[offset], EMV_PAN_BYTES);
	offset += EMV_PAN_BYTES;
	(void)memcpy(image->expiry, &in[offset], EMV_EXPIRY_BYTES);
	offset += EMV_EXPIRY_BYTES;
	(void)memcpy(image->name, &in[offset], EMV_NAME_BYTES);
	offset += EMV_NAME_BYTES;
	image->track2_len = in[offset++];
	(void)memcpy(image->track2, &in[offset], EMV_TRACK2_MAX_BYTES);
	offset += EMV_TRACK2_MAX_BYTES;
	(void)memcpy(image->aip, &in[offset], EMV_AIP_BYTES);
	offset += EMV_AIP_BYTES;
	(void)memcpy(image->afl, &in[offset], EMV_AFL_BYTES);
	offset += EMV_AFL_BYTES;
	image->app_aid_len = in[offset++];
	(void)memcpy(image->app_aid, &in[offset], EMV_APP_AID_MAX_BYTES);
	offset += EMV_APP_AID_MAX_BYTES;
	if (image->app_aid_len > EMV_APP_AID_MAX_BYTES) {
		return -EBADMSG;
	}

	image->record_count = in[offset++];
	for (uint8_t i = 0U; i < image->record_count; i++) {
		if (offset >= in_len) {
			return -EBADMSG;
		}
		image->record_len[i] = in[offset++];
		if ((offset + image->record_len[i]) > in_len) {
			return -EBADMSG;
		}
		(void)memcpy(image->record_data[i], &in[offset], image->record_len[i]);
		offset += image->record_len[i];
	}

	return (offset == in_len) ? emv_rebuild_caches(image) : -EBADMSG;
}
