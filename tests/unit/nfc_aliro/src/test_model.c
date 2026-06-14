#include "aliro.h"
#include "router/service.h"
#include <errno.h>
#include <string.h>
#include <zephyr/ztest.h>

static aliro_data_t s_data;
static aliro_data_t s_copy; /* Static to avoid stack allocation */
static uint8_t s_out[768];

ZTEST(aliro_model, test_reset)
{
	aliro_data_reset(&s_data);
	s_data.transcript_len = 4U;
	aliro_data_reset(&s_data);
	zassert_equal(s_data.transcript_len, 0U);
}

ZTEST(aliro_model, test_persist_id)
{
	zassert_equal(aliro_persist_id(), NFC_PERSIST_ID_ALIRO);
}

ZTEST(aliro_model, test_serialize_roundtrip)
{
	size_t len = 0U;

	(void)memset(&s_copy, 0, sizeof(s_copy));

	s_data.protocol_major = 0U;
	s_data.protocol_minor = 9U;
	s_data.credential_pubkey[0] = 0x04U;
	s_data.transcript_len = 2U;
	s_data.transcript[0] = 0xAAU;
	s_data.transcript[1] = 0xBBU;
	zassert_ok(aliro_serialize(&s_data, s_out, sizeof(s_out), &len));
	zassert_ok(aliro_deserialize(&s_copy, s_out, len));
	zassert_equal(s_copy.transcript_len, 2U);
}

ZTEST(aliro_model, test_deserialize_bad_version)
{
	uint8_t bad[] = {0xFFU};

	zassert_equal(aliro_deserialize(&s_data, bad, sizeof(bad)), -EBADMSG);
}

ZTEST_SUITE(aliro_model, NULL, NULL, NULL, NULL, NULL);
