/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_classic/mf_classic.c
 */

#include "protocols/classic/classic.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>

static int classic_serialize_cb(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	return classic_serialize(user_ctx, out, out_max, out_len);
}

static int classic_deserialize_cb(const uint8_t *in, size_t in_len, void *user_ctx)
{
	return classic_deserialize(user_ctx, in, in_len);
}

static nfc_service_t s_classic_svc = {
	.serialize = classic_serialize_cb,
	.deserialize = classic_deserialize_cb,
	.persist_id = NFC_PERSIST_ID_CLASSIC,
};

void classic_data_reset(classic_data_t *data)
{
	if (data == NULL) {
		return;
	}

	(void)memset(data, 0, sizeof(*data));
	data->type = CLASSIC_TYPE_UNKNOWN;
}

uint8_t classic_persist_id(void)
{
	return NFC_PERSIST_ID_CLASSIC;
}

int classic_serialize(const classic_data_t *data, uint8_t *out, size_t out_max, size_t *out_len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(out);
	ARG_UNUSED(out_max);
	ARG_UNUSED(out_len);

	return -ENOTSUP;
}

int classic_deserialize(classic_data_t *data, const uint8_t *in, size_t in_len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(in);
	ARG_UNUSED(in_len);

	return -ENOTSUP;
}

const nfc_service_t *classic_service_get(void)
{
	return &s_classic_svc;
}
