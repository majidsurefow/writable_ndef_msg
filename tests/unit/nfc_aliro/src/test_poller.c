#include "aliro_poller.h"
#include "nfc_session_mock.h"
#include "Aliro_mock.h"
#include <zephyr/ztest.h>

static aliro_data_t s_data;
static nfc_reader_session_t s_session;

static void setup(void *f)
{
	ARG_UNUSED(f);
	nfc_session_mock_reset();
	s_session.active = true;
	aliro_data_reset(&s_data);
}

ZTEST(aliro_poller, test_detect_ok)
{
	setup(NULL);
	nfc_session_mock_load(aliro_Aliro_read_steps, ALIRO_ALIRO_READ_STEP_COUNT);
	zassert_ok(aliro_poller_detect(&s_session));
}

ZTEST(aliro_poller, test_read_partial)
{
	setup(NULL);
	nfc_session_mock_load(aliro_Aliro_read_steps, ALIRO_ALIRO_READ_STEP_COUNT);
	zassert_ok(aliro_poller_read(&s_session, &s_data));
	zassert_true(s_data.transcript_len > 0U);
}

ZTEST(aliro_poller, test_read_auth_chain_transcript)
{
	setup(NULL);
	nfc_session_mock_load(aliro_Aliro_auth_chain_steps, ALIRO_ALIRO_AUTH_CHAIN_STEP_COUNT);
	zassert_ok(aliro_poller_read(&s_session, &s_data));
	zassert_equal(s_data.transcript_len,
		      (uint16_t)(sizeof(aliro_select_ok) + sizeof(aliro_auth0_rsp) +
				 sizeof(aliro_auth1_rsp)));
	zassert_equal(nfc_session_mock_tx_count(), 3U);
}

ZTEST(aliro_poller, test_inactive_session)
{
	setup(NULL);
	s_session.active = false;
	zassert_equal(aliro_poller_detect(&s_session), -EINVAL);
}

ZTEST_SUITE(aliro_poller, NULL, NULL, setup, NULL, NULL);
