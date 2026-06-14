#ifndef TESTS_FIXTURES_ALIRO_ALIRO_MOCK_H_
#define TESTS_FIXTURES_ALIRO_ALIRO_MOCK_H_

#include "nfc_session_mock.h"

static const uint8_t aliro_select_ok[] = {0x00U, 0x00U, 0x11U, 0x22U, 0x90U, 0x00U};

static const nfc_session_mock_step_t aliro_Aliro_read_steps[] = {
	{.rx = aliro_select_ok, .rx_len = sizeof(aliro_select_ok), .err = 0},
};

#define ALIRO_ALIRO_READ_STEP_COUNT ARRAY_SIZE(aliro_Aliro_read_steps)

#endif
