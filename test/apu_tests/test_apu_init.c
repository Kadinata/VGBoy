#include "unity.h"
#include "apu.h"
#include "audio_playback_samples.h"
#include "status_code.h"

#include "bus_interface_test_helper.h"

#include "mock_apu_pwm.h"
#include "mock_apu_lfsr.h"
#include "mock_apu_wave.h"
#include "mock_bus_interface.h"
#include "mock_callback.h"

TEST_FILE("apu.c")

void setUp(void)
{
}

void tearDown(void)
{
}

void test_apu_init(void)
{
  apu_handle_t apu = {0};

  apu_pwm_init_ExpectAndReturn(&apu.ch1, true, STATUS_OK);
  apu_pwm_init_ExpectAndReturn(&apu.ch2, false, STATUS_OK);
  apu_wave_init_ExpectAndReturn(&apu.ch3, STATUS_OK);
  apu_lfsr_init_ExpectAndReturn(&apu.ch4, STATUS_OK);

  callback_init_ExpectAndReturn(&apu.playback_cb, NULL, &apu, STATUS_OK);
  callback_init_IgnoreArg_callback_fn();

  bus_interface_init_ExpectAndReturn(&apu.bus_interface, NULL, NULL, &apu, STATUS_OK);
  bus_interface_init_IgnoreArg_read_fn();
  bus_interface_init_IgnoreArg_write_fn();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, apu_init(&apu));
}

void test_apu_init__null_ptr(void)
{
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, apu_init(NULL));
}