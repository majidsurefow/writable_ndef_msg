#include "emv_poller.h"
#include "nfc_session_mock.h"
#include "Emv_mock.h"
#include <zephyr/ztest.h>

static emv_card_image_t s_data;
static nfc_reader_session_t s_session;

static void setup(void *f)
{
	ARG_UNUSED(f);
	nfc_session_mock_reset();
	s_session.active = true;
	emv_card_image_reset(&s_data);
}

ZTEST(emv_poller, test_detect_ok)
{
	setup(NULL);
	nfc_session_mock_load(emv_Emv_read_steps, 1U);
	zassert_ok(emv_poller_detect(&s_session));
}

ZTEST(emv_poller, test_read_partial)
{
	setup(NULL);
	nfc_session_mock_load(emv_Emv_read_steps, EMV_EMV_READ_STEP_COUNT);
	zassert_ok(emv_poller_read(&s_session, &s_data));
	zassert_equal(s_data.record_count, 1U);
}

ZTEST_SUITE(emv_poller, NULL, NULL, setup, NULL, NULL);
