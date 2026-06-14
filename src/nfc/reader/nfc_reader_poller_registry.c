/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "reader/nfc_reader_poller_registry.h"

#include "hal/nfc_transport.h"

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
#include "protocols/ndef/ndef_listener.h"
#include "protocols/ndef/ndef_poller.h"
#endif

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ULTRALIGHT)
#include "protocols/ultralight/ultralight_listener.h"
#include "protocols/ultralight/ultralight_poller.h"
#endif

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_CLASSIC)
#include "protocols/classic/classic.h"
#include "protocols/classic/classic_poller.h"
#endif

#if IS_ENABLED(CONFIG_NFC_STORE)
#include "store/nfc_store.h"
#endif

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(nfc_reader, CONFIG_LOG_DEFAULT_LEVEL);

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ULTRALIGHT)
static int ultralight_poller_read_wrap(const nfc_reader_session_t *session, void *out)
{
	return ultralight_poller_read(session, out);
}

static int ultralight_poller_clone(const nfc_reader_session_t *session, const char *tag)
{
	ultralight_data_t data;
	const nfc_service_t *svcs[] = { ultralight_listener_get() };
	int ret;

	ultralight_data_reset(&data);
	ret = ultralight_poller_read(session, &data);
	if (ret != 0) {
		LOG_WRN("Ultralight poller read failed: %d", ret);
		return ret;
	}

	ret = ultralight_listener_init();
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("ultralight_listener_init failed: %d", ret);
		return ret;
	}

	ret = ultralight_listener_load(&data);
	if (ret != 0) {
		LOG_ERR("ultralight_listener_load failed: %d", ret);
		return ret;
	}

	ret = nfc_store_save(tag, svcs, ARRAY_SIZE(svcs));
	if (ret != 0) {
		LOG_ERR("nfc_store_save failed: %d", ret);
	}

	return ret;
}
#else
static int ultralight_poller_detect_stub(const nfc_reader_session_t *session)
{
	ARG_UNUSED(session);

	return -ENOTSUP;
}

static int ultralight_poller_read_stub(const nfc_reader_session_t *session, void *out)
{
	ARG_UNUSED(session);
	ARG_UNUSED(out);

	return -ENOTSUP;
}
#endif

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_CLASSIC)
static int classic_poller_read_wrap(const nfc_reader_session_t *session, void *out)
{
	return classic_poller_read(session, out);
}

static int classic_poller_clone(const nfc_reader_session_t *session, const char *tag)
{
	classic_data_t data;
	const nfc_service_t *svcs[] = { classic_service_get() };
	int ret;

	classic_data_reset(&data);
	ret = classic_poller_read(session, &data);
	if (ret != 0) {
		LOG_WRN("Classic poller read failed: %d", ret);
		return ret;
	}

	ret = nfc_store_save(tag, svcs, ARRAY_SIZE(svcs));
	if (ret != 0) {
		LOG_ERR("nfc_store_save failed: %d", ret);
	}

	return ret;
}
#else
static int classic_poller_detect_stub(const nfc_reader_session_t *session)
{
	ARG_UNUSED(session);

	return -ENOTSUP;
}

static int classic_poller_read_stub(const nfc_reader_session_t *session, void *out)
{
	ARG_UNUSED(session);
	ARG_UNUSED(out);

	return -ENOTSUP;
}
#endif

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
static int nfc_reader_poller_ndef_read(const nfc_reader_session_t *session, void *out)
{
	return ndef_poller_read(session, out);
}

static int nfc_reader_poller_ndef_clone(const nfc_reader_session_t *session, const char *tag)
{
	ndef_data_t data;
	const nfc_service_t *svcs[] = { ndef_listener_get() };
	int ret;

	ndef_data_reset(&data);
	ret = ndef_poller_read(session, &data);
	if (ret != 0) {
		LOG_WRN("NDEF poller read failed: %d", ret);
		return ret;
	}

	ret = ndef_listener_init(&data, NULL);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("ndef_listener_init failed: %d", ret);
		return ret;
	}

	ret = nfc_store_save(tag, svcs, ARRAY_SIZE(svcs));
	if (ret != 0) {
		LOG_ERR("nfc_store_save failed: %d", ret);
	}

	return ret;
}
#endif

static const nfc_reader_poller_entry_t s_pollers[] = {
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
	{
		.name = "ndef",
		.persist_id = NFC_PERSIST_ID_NDEF,
		.tech_mask = NFC_TECH_ISO_DEP_A,
		.detect = ndef_poller_detect,
		.read = nfc_reader_poller_ndef_read,
		.listener_get = ndef_listener_get,
		.clone_fn = nfc_reader_poller_ndef_clone,
	},
#endif
	{
		.name = "ultralight",
		.persist_id = NFC_PERSIST_ID_ULTRALIGHT,
		.tech_mask = NFC_TECH_TYPE2,
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ULTRALIGHT)
		.detect = ultralight_poller_detect,
		.read = ultralight_poller_read_wrap,
		.listener_get = ultralight_listener_get,
		.clone_fn = ultralight_poller_clone,
#else
		.detect = ultralight_poller_detect_stub,
		.read = ultralight_poller_read_stub,
		.listener_get = NULL,
		.clone_fn = NULL,
#endif
	},
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_CLASSIC)
	{
		.name = "classic",
		.persist_id = NFC_PERSIST_ID_CLASSIC,
		.tech_mask = NFC_TECH_ISO14443_3A_RAW,
		.detect = classic_poller_detect,
		.read = classic_poller_read_wrap,
		.listener_get = NULL,
		.clone_fn = classic_poller_clone,
	},
#else
	{
		.name = "classic",
		.persist_id = NFC_PERSIST_ID_CLASSIC,
		.tech_mask = NFC_TECH_ISO14443_3A_RAW,
		.detect = classic_poller_detect_stub,
		.read = classic_poller_read_stub,
		.listener_get = NULL,
		.clone_fn = NULL,
	},
#endif
	{
		.detect = NULL,
	},
};

const nfc_reader_poller_entry_t *nfc_reader_pollers_get(void)
{
	return s_pollers;
}

int nfc_reader_pollers_run(const char *tag)
{
#if IS_ENABLED(CONFIG_NFC_STORE)
	const nfc_reader_session_t *session = nfc_reader_session_get();
	int ret;
	bool matched = false;

	if (session == NULL) {
		LOG_WRN("Poller walk skipped: no active reader session (scan first)");
		return -ENODEV;
	}

	for (const nfc_reader_poller_entry_t *e = s_pollers; e->detect != NULL; e++) {
		ret = e->detect(session);
		if (ret != 0) {
			continue;
		}

		matched = true;

		if (e->clone_fn == NULL) {
			LOG_WRN("Poller \"%s\" matched but has no clone hook (stub)", e->name);
			ret = -ENOTSUP;
			break;
		}

		ret = e->clone_fn(session, tag);
		if (ret == 0) {
			LOG_INF("Clone saved tag \"%s\" via %s poller", tag, e->name);
		}
		break;
	}

	if (!matched) {
		LOG_WRN("No poller matched active tag");
		ret = -ENOTSUP;
	}

	nfc_reader_session_end();
	return ret;
#else
	ARG_UNUSED(tag);
	LOG_WRN("Poller walk not built (enable CONFIG_NFC_STORE)");
	if (nfc_reader_session_get() != NULL) {
		nfc_reader_session_end();
	}
	return -ENOTSUP;
#endif
}
