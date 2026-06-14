#include "aliro_listener.h"
#include "aliro_vectors.h"
#include "nfc_response_spy.h"
#include "nfc_test_apdu.h"
#include "nfc_work_spy.h"

#include <string.h>

#include <zephyr/ztest.h>

static void reset(void *f)
{
	ARG_UNUSED(f);
	nfc_response_spy_reset();
	nfc_work_spy_reset();
	(void)aliro_listener_shutdown();
	zassert_ok(aliro_listener_init(NULL));
}

static void send_apdu(const uint8_t *apdu, size_t len)
{
	uint8_t storage[256];
	nfc_apdu_t parsed;

	zassert_ok(nfc_test_apdu_from_bytes(apdu, len, storage, sizeof(storage), &parsed));
	aliro_listener_get()->on_apdu(&parsed, NULL);
}

static void select_expedited(void)
{
	aliro_listener_get()->on_select(aliro_expedited_aid, ALIRO_AID_LEN, NULL);
}

static uint16_t last_sw(void)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;

	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_true(len >= 2U);
	return (uint16_t)((buf[len - 2U] << 8U) | buf[len - 1U]);
}

ZTEST(aliro_listener, test_select_expedited)
{
	reset(NULL);
	select_expedited();
	zassert_equal(aliro_listener_state(), ALIRO_STATE_AWAIT_AUTH0);
}

ZTEST(aliro_listener, test_select_stepup_returns_6a81)
{
	reset(NULL);
	aliro_listener_get()->on_select(aliro_stepup_aid, ALIRO_AID_LEN, NULL);
	zassert_equal(last_sw(), 0x6A81U);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_IDLE);
}

ZTEST(aliro_listener, test_auth0_submits_work)
{
	static const uint8_t auth0[] = {0x80U, 0x80U, 0x00U, 0x00U, 0x10U,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	reset(NULL);
	select_expedited();
	send_apdu(auth0, sizeof(auth0));
	zassert_equal(nfc_work_spy_submit_count(), 1U);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_AWAIT_AUTH1);
}

ZTEST(aliro_listener, test_auth1_in_await_auth0_rejects)
{
	static const uint8_t auth1[] = {0x80U, 0x81U, 0x00U, 0x00U, 0x40U};

	reset(NULL);
	select_expedited();
	send_apdu(auth1, sizeof(auth1));
	zassert_equal(last_sw(), NFC_SW_CONDITIONS_NOT_SAT);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_ERROR);
}

ZTEST(aliro_listener, test_auth0_work_builds_response_83bytes)
{
	uint8_t auth0[5U + ALIRO_AUTH0_DATA_SIZE];
	size_t auth0_len = 0U;
	const uint8_t *buf = NULL;
	size_t len = 0U;

	reset(NULL);
	select_expedited();
	aliro_vectors_build_auth0_tx(auth0, &auth0_len);
	send_apdu(auth0, auth0_len);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, ALIRO_AUTH0_RSP_SIZE);
}

ZTEST(aliro_listener, test_auth0_work_clears_inflight)
{
	uint8_t auth0[5U + ALIRO_AUTH0_DATA_SIZE];
	size_t auth0_len = 0U;

	reset(NULL);
	select_expedited();
	aliro_vectors_build_auth0_tx(auth0, &auth0_len);
	send_apdu(auth0, auth0_len);
	zassert_false(aliro_listener_crypto_inflight());
}

ZTEST(aliro_listener, test_exchange_in_await_auth1_rejects)
{
	uint8_t auth0[5U + ALIRO_AUTH0_DATA_SIZE];
	size_t auth0_len = 0U;
	static const uint8_t exchange[] = {0x80U, 0x82U, 0x00U, 0x00U, 0x00U};

	reset(NULL);
	select_expedited();
	aliro_vectors_build_auth0_tx(auth0, &auth0_len);
	send_apdu(auth0, auth0_len);
	send_apdu(exchange, sizeof(exchange));
	zassert_equal(last_sw(), NFC_SW_CONDITIONS_NOT_SAT);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_ERROR);
}

ZTEST(aliro_listener, test_unknown_ins_rejects_6d00)
{
	static const uint8_t bad[] = {0x80U, 0xFFU, 0x00U, 0x00U, 0x00U};

	reset(NULL);
	select_expedited();
	send_apdu(bad, sizeof(bad));
	zassert_equal(last_sw(), NFC_SW_INS_NOT_SUPPORTED);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_ERROR);
}

ZTEST(aliro_listener, test_auth1_in_await_auth1_submits_work)
{
	uint8_t auth0[5U + ALIRO_AUTH0_DATA_SIZE];
	uint8_t auth1[5U + ALIRO_AUTH1_DATA_SIZE];
	size_t auth0_len = 0U;
	size_t auth1_len = 0U;

	reset(NULL);
	select_expedited();
	aliro_vectors_build_auth0_tx(auth0, &auth0_len);
	send_apdu(auth0, auth0_len);
	nfc_work_spy_reset();
	aliro_vectors_build_auth1_tx(auth1, &auth1_len);
	send_apdu(auth1, auth1_len);
	zassert_equal(nfc_work_spy_submit_count(), 1U);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_AWAIT_EXCHANGE);
}

ZTEST(aliro_listener, test_auth1_work_builds_response_66bytes)
{
	uint8_t auth0[5U + ALIRO_AUTH0_DATA_SIZE];
	uint8_t auth1[5U + ALIRO_AUTH1_DATA_SIZE];
	size_t auth0_len = 0U;
	size_t auth1_len = 0U;
	const uint8_t *buf = NULL;
	size_t len = 0U;

	reset(NULL);
	select_expedited();
	aliro_vectors_build_auth0_tx(auth0, &auth0_len);
	send_apdu(auth0, auth0_len);
	aliro_vectors_build_auth1_tx(auth1, &auth1_len);
	send_apdu(auth1, auth1_len);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, ALIRO_AUTH1_RSP_SIZE);
}

ZTEST(aliro_listener, test_exchange_in_await_exchange_submits_work)
{
	uint8_t auth0[5U + ALIRO_AUTH0_DATA_SIZE];
	uint8_t auth1[5U + ALIRO_AUTH1_DATA_SIZE];
	uint8_t exchange[5U + ALIRO_EXCHANGE_DATA_SIZE];
	size_t auth0_len = 0U;
	size_t auth1_len = 0U;
	size_t exchange_len = 0U;

	reset(NULL);
	select_expedited();
	aliro_vectors_build_auth0_tx(auth0, &auth0_len);
	send_apdu(auth0, auth0_len);
	aliro_vectors_build_auth1_tx(auth1, &auth1_len);
	send_apdu(auth1, auth1_len);
	nfc_work_spy_reset();
	aliro_vectors_build_exchange_tx(exchange, &exchange_len);
	send_apdu(exchange, exchange_len);
	zassert_equal(nfc_work_spy_submit_count(), 1U);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_DONE);
}

ZTEST(aliro_listener, test_exchange_work_builds_response_31bytes)
{
	uint8_t auth0[5U + ALIRO_AUTH0_DATA_SIZE];
	uint8_t auth1[5U + ALIRO_AUTH1_DATA_SIZE];
	uint8_t exchange[5U + ALIRO_EXCHANGE_DATA_SIZE];
	size_t auth0_len = 0U;
	size_t auth1_len = 0U;
	size_t exchange_len = 0U;
	const uint8_t *buf = NULL;
	size_t len = 0U;

	reset(NULL);
	select_expedited();
	aliro_vectors_build_auth0_tx(auth0, &auth0_len);
	send_apdu(auth0, auth0_len);
	aliro_vectors_build_auth1_tx(auth1, &auth1_len);
	send_apdu(auth1, auth1_len);
	aliro_vectors_build_exchange_tx(exchange, &exchange_len);
	send_apdu(exchange, exchange_len);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, ALIRO_EXCHANGE_RSP_SIZE);
}

ZTEST(aliro_listener, test_exchange_bad_payload_sends_6982)
{
	uint8_t auth0[5U + ALIRO_AUTH0_DATA_SIZE];
	uint8_t auth1[5U + ALIRO_AUTH1_DATA_SIZE];
	static const uint8_t bad_exchange[] = {0x80U, 0x82U, 0x00U, 0x00U, 0x1DU,
					       0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
					       0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
					       0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
					       0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
					       0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
					       0xFFU, 0xFFU, 0xFFU, 0xFFU};

	size_t auth0_len = 0U;
	size_t auth1_len = 0U;

	reset(NULL);
	select_expedited();
	aliro_vectors_build_auth0_tx(auth0, &auth0_len);
	send_apdu(auth0, auth0_len);
	aliro_vectors_build_auth1_tx(auth1, &auth1_len);
	send_apdu(auth1, auth1_len);
	send_apdu(bad_exchange, sizeof(bad_exchange));
	zassert_equal(last_sw(), 0x6982U);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_ERROR);
}

ZTEST(aliro_listener, test_field_off_resets)
{
	reset(NULL);
	select_expedited();
	aliro_listener_get()->on_field_off(NULL);
	zassert_equal(aliro_listener_state(), ALIRO_STATE_IDLE);
}

ZTEST(aliro_listener, test_unknown_cla_rejects)
{
	static const uint8_t bad[] = {0x00U, 0x80U, 0x00U, 0x00U, 0x00U};

	reset(NULL);
	send_apdu(bad, sizeof(bad));
	zassert_equal(last_sw(), NFC_SW_CLA_NOT_SUPPORTED);
}

ZTEST_SUITE(aliro_listener, NULL, NULL, reset, NULL, NULL);
