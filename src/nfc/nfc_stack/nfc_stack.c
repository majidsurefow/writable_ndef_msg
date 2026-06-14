/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nfc_stack/nfc_stack.h"

#include "../../stats.h"

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/net_buf.h>

#include "framing/apdu_assembler.h"
#include "protocols/ndef/ndef_listener.h"
#include "router/aid_router.h"
#include "router/service.h"
#include "store/nfc_store.h"
#if IS_ENABLED(CONFIG_NFC_STORE_RAM)
#include "store/nfc_store_ram.h"
#endif

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_DESFIRE)
#include "protocols/desfire/desfire.h"
#include "protocols/desfire/desfire_listener.h"
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_EMV)
#include "protocols/emv/emv.h"
#include "protocols/emv/emv_listener.h"
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ALIRO)
#include "protocols/aliro/aliro.h"
#include "protocols/aliro/aliro_listener.h"
#endif

LOG_MODULE_REGISTER(nfc_stack, CONFIG_LOG_DEFAULT_LEVEL);

static const uint8_t NDEF_AID[] = { 0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x01U };

static nfc_stack_config_t s_config = NFC_STACK_CONFIG_DEFAULT;
static nfc_stack_stats_t s_stats;
static struct k_spinlock s_stats_lock;
static nfc_stack_state_t s_state = NFC_STACK_STATE_UNINITIALIZED;

static char s_active_tag[CONFIG_NFC_STORE_MAX_TAG_LEN];

static void nfc_stack_on_field_on(void *user_ctx);
static void nfc_stack_on_field_off(void *user_ctx);
static void nfc_stack_on_apdu(struct net_buf *apdu, void *user_ctx);

static nfc_transport_ops_t s_transport_ops = {
	.on_field_on = nfc_stack_on_field_on,
	.on_field_off = nfc_stack_on_field_off,
	.on_apdu = nfc_stack_on_apdu,
};

static size_t nfc_stack_listen_services(const nfc_service_t **svcs, size_t max)
{
	size_t n = 0U;

	if ((svcs == NULL) || (max == 0U)) {
		return 0U;
	}

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
	if (n < max) {
		svcs[n++] = ndef_listener_get();
	}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_DESFIRE)
	if (n < max) {
		svcs[n++] = desfire_listener_get();
	}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_EMV)
	if (n < max) {
		svcs[n++] = emv_listener_get();
	}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ALIRO)
	if (n < max) {
		svcs[n++] = aliro_listener_get();
	}
#endif

	return n;
}

static int stack_register_listen_profiles(void)
{
	int ret;

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
	ret = aid_router_register(NDEF_AID, sizeof(NDEF_AID), ndef_listener_get());
	if (ret != 0) {
		return ret;
	}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_EMV)
	ret = aid_router_register(emv_ppse_aid, EMV_PPSE_AID_LEN, emv_listener_get());
	if (ret != 0) {
		return ret;
	}

	ret = aid_router_register(emv_service_app_aid, EMV_SERVICE_APP_AID_LEN, emv_listener_get());
	if (ret != 0) {
		return ret;
	}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_DESFIRE)
	ret = aid_router_register(desfire_aid, DESFIRE_AID_LEN, desfire_listener_get());
	if (ret != 0) {
		return ret;
	}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ALIRO)
	ret = aid_router_register(aliro_expedited_aid, ALIRO_AID_LEN, aliro_listener_get());
	if (ret != 0) {
		return ret;
	}

	ret = aid_router_register(aliro_stepup_aid, ALIRO_AID_LEN, aliro_listener_get());
	if (ret != 0) {
		return ret;
	}
#endif

	return 0;
}

static int stack_init_protocol_listeners(void)
{
	int ret;

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
	ret = ndef_listener_init(NULL, NULL);
	if (ret != 0 && ret != -EALREADY) {
		return ret;
	}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_DESFIRE)
	ret = desfire_listener_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		return ret;
	}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_EMV)
	ret = emv_listener_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		return ret;
	}
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ALIRO)
	ret = aliro_listener_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		return ret;
	}
#endif

	return 0;
}

static void stack_shutdown_protocol_listeners(void)
{
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_ALIRO)
	(void)aliro_listener_shutdown();
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_EMV)
	(void)emv_listener_shutdown();
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_DESFIRE)
	(void)desfire_listener_shutdown();
#endif
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
	(void)ndef_listener_shutdown();
#endif
}

static void nfc_stack_on_field_on(void *user_ctx)
{
	const nfc_transport_ops_t *asm_ops = apdu_assembler_get_ops();

	ARG_UNUSED(user_ctx);

	if ((asm_ops != NULL) && (asm_ops->on_field_on != NULL)) {
		asm_ops->on_field_on(NULL);
	}
}

static void nfc_stack_on_field_off(void *user_ctx)
{
	const nfc_transport_ops_t *asm_ops = apdu_assembler_get_ops();

	ARG_UNUSED(user_ctx);

	if ((asm_ops != NULL) && (asm_ops->on_field_off != NULL)) {
		asm_ops->on_field_off(NULL);
	}
}

static void nfc_stack_on_apdu(struct net_buf *apdu, void *user_ctx)
{
	const nfc_transport_ops_t *asm_ops = apdu_assembler_get_ops();

	ARG_UNUSED(user_ctx);

	if ((asm_ops != NULL) && (asm_ops->on_apdu != NULL)) {
		asm_ops->on_apdu(apdu, NULL);
	} else if (apdu != NULL) {
		net_buf_unref(apdu);
	}
}

int nfc_stack_init(const nfc_stack_config_t *cfg)
{
	int ret;

	if (s_state != NFC_STACK_STATE_UNINITIALIZED) {
		return -EALREADY;
	}

	STATS_RESET(s_stats);
	s_config = (cfg != NULL) ? *cfg : NFC_STACK_CONFIG_DEFAULT;
	if (s_config.default_profile == NFC_PROFILE_NONE) {
		s_config.default_profile = (nfc_profile_t)CONFIG_NFC_STACK_DEFAULT_PROFILE;
	}

	s_state = NFC_STACK_STATE_INITIALIZED;
	s_active_tag[0] = '\0';

	ret = apdu_assembler_init(NULL);
	if (ret != 0) {
		goto fail;
	}

	ret = aid_router_init();
	if (ret != 0) {
		goto fail_asm;
	}

	ret = nfc_store_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		goto fail_router;
	}

#if IS_ENABLED(CONFIG_NFC_STORE_RAM)
	ret = nfc_store_ram_init();
	if (ret != 0) {
		goto fail_store;
	}
#endif

	ret = nfc_transport_init();
	if (ret != 0 && ret != -EALREADY) {
		goto fail_store;
	}

	ret = stack_init_protocol_listeners();
	if (ret != 0) {
		goto fail_transport;
	}

	ret = nfc_transport_register_callbacks(&s_transport_ops, NULL);
	if (ret != 0) {
		goto fail_listeners;
	}

	ret = stack_register_listen_profiles();
	if (ret != 0) {
		goto fail_listeners;
	}

	return 0;

fail_listeners:
	stack_shutdown_protocol_listeners();
fail_transport:
	(void)nfc_transport_shutdown();
fail_store:
	(void)nfc_store_shutdown();
fail_router:
	(void)aid_router_shutdown();
fail_asm:
	(void)apdu_assembler_shutdown();
fail:
	s_state = NFC_STACK_STATE_UNINITIALIZED;
	STATS_ERROR(&s_stats_lock, s_stats, ret);
	return -EIO;
}

int nfc_stack_start(const nfc_uid_t *uid)
{
	int ret;

	if (s_state == NFC_STACK_STATE_STARTED) {
		return 0;
	}

	if ((s_state != NFC_STACK_STATE_INITIALIZED) && (s_state != NFC_STACK_STATE_STOPPED)) {
		return -ENODEV;
	}

	if (uid != NULL) {
		if ((uid->len != NFC_UID_LEN_SINGLE) && (uid->len != NFC_UID_LEN_DOUBLE) &&
		    (uid->len != NFC_UID_LEN_TRIPLE)) {
			return -EINVAL;
		}
	}

	s_state = NFC_STACK_STATE_STARTED;
	ret = nfc_transport_start(uid);
	if (ret != 0) {
		s_state = NFC_STACK_STATE_ERROR;
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return -EIO;
	}

	STATS_INC(&s_stats_lock, s_stats, start_count);
	return 0;
}

int nfc_stack_stop(void)
{
	int ret;

	if (s_state == NFC_STACK_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if ((s_state == NFC_STACK_STATE_INITIALIZED) || (s_state == NFC_STACK_STATE_STOPPED)) {
		return 0;
	}

	ret = nfc_transport_stop();
	if (ret != 0) {
		s_state = NFC_STACK_STATE_ERROR;
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return -EIO;
	}

	s_state = NFC_STACK_STATE_STOPPED;
	STATS_INC(&s_stats_lock, s_stats, stop_count);
	return 0;
}

int nfc_stack_shutdown(void)
{
	if (s_state == NFC_STACK_STATE_UNINITIALIZED) {
		return 0;
	}

	if (s_state == NFC_STACK_STATE_STARTED) {
		(void)nfc_stack_stop();
	}

	(void)nfc_transport_shutdown();
	stack_shutdown_protocol_listeners();
	(void)nfc_store_shutdown();
	(void)aid_router_shutdown();
	(void)apdu_assembler_shutdown();

	s_state = NFC_STACK_STATE_UNINITIALIZED;
	return 0;
}

int nfc_stack_load(const char *tag)
{
	const nfc_service_t *svcs[6];
	size_t n;
	int ret;

	if (s_state == NFC_STACK_STATE_STARTED) {
		return -EBUSY;
	}

	if ((s_state != NFC_STACK_STATE_INITIALIZED) && (s_state != NFC_STACK_STATE_STOPPED)) {
		return -ENODEV;
	}

	if (tag == NULL) {
		return -EINVAL;
	}

	n = nfc_stack_listen_services(svcs, ARRAY_SIZE(svcs));
	if (n == 0U) {
		return -ENODEV;
	}

	ret = nfc_store_load(tag, svcs, n);
	if (ret != 0) {
		return ret;
	}

	(void)strncpy(s_active_tag, tag, sizeof(s_active_tag) - 1U);
	s_active_tag[sizeof(s_active_tag) - 1U] = '\0';
	STATS_INC(&s_stats_lock, s_stats, load_count);
	return 0;
}

int nfc_stack_save(const char *tag)
{
	const nfc_service_t *svcs[6];
	size_t n;
	int ret;

	if (s_state == NFC_STACK_STATE_STARTED) {
		return -EBUSY;
	}

	if ((s_state != NFC_STACK_STATE_INITIALIZED) && (s_state != NFC_STACK_STATE_STOPPED)) {
		return -ENODEV;
	}

	if (tag == NULL) {
		return -EINVAL;
	}

	n = nfc_stack_listen_services(svcs, ARRAY_SIZE(svcs));
	if (n == 0U) {
		return -ENODEV;
	}

	ret = nfc_store_save(tag, svcs, n);
	if (ret != 0) {
		return ret;
	}

	(void)strncpy(s_active_tag, tag, sizeof(s_active_tag) - 1U);
	s_active_tag[sizeof(s_active_tag) - 1U] = '\0';
	STATS_INC(&s_stats_lock, s_stats, save_count);
	return 0;
}

void nfc_stack_on_service_dirty(const nfc_service_t *svc)
{
#if IS_ENABLED(CONFIG_NFC_STORE)
	if ((svc == NULL) || (s_active_tag[0] == '\0')) {
		return;
	}

	(void)nfc_store_on_dirty(svc, s_active_tag);
#else
	ARG_UNUSED(svc);
#endif
}

const nfc_stack_config_t *nfc_stack_get_config(void)
{
	return &s_config;
}

int nfc_stack_get_stats(nfc_stack_stats_t *out)
{
	return STATS_COPY_OUT(&s_stats_lock, s_stats, out);
}

nfc_stack_state_t nfc_stack_get_state(void)
{
	return s_state;
}
