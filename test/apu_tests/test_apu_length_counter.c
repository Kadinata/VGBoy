#include "unity.h"

#include <string.h>

#include "apu.h"
#include "apu_pwm.h"
#include "apu_lfsr.h"
#include "apu_wave.h"
#include "apu_common.h"
#include "audio_playback_samples.h"
#include "bus_interface.h"
#include "status_code.h"

#include "mock_callback.h"

TEST_FILE("apu.c")
TEST_FILE("apu_pwm.c")
TEST_FILE("apu_lfsr.c")
TEST_FILE("apu_wave.c")

#define CPU_FREQ (1048576)
#define FRAME_SEQ_RATE (256)

#define TEST_ASSERT_CHANNEL_ENABLED(CH_NUM)                                                  \
  {                                                                                          \
    uint8_t data = 0;                                                                        \
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, 0xFF26, &data)); \
    TEST_ASSERT_TRUE(data &channel_enabled_mask[CH_NUM]);                                    \
  }

#define TEST_ASSERT_CHANNEL_DISABLED(CH_NUM)                                                 \
  {                                                                                          \
    uint8_t data = 0;                                                                        \
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, 0xFF26, &data)); \
    TEST_ASSERT_FALSE(data &channel_enabled_mask[CH_NUM]);                                   \
  }

#define TEST_ASSERT_CHANNEL_ALMOST_DISABLED(CH_NUM) \
  {                                                 \
    TEST_ASSERT_CHANNEL_ENABLED(CH_NUM);            \
    apu_delay(1);                                   \
    TEST_ASSERT_CHANNEL_DISABLED(CH_NUM);           \
  }

#define TEST_CH_LENGTH_CTR_RESET(CH_NUM)                \
  void test_channel_##CH_NUM##_length_timer_reset(void) \
  {                                                     \
    stub_test_channel_length_countdown_reset(CH_NUM);   \
  }

#define TEST_CH_CAN_RELOAD_LENGTH_ANYTIME(CH_NUM)              \
  void test_channel_##CH_NUM##_can_reload_length_anytime(void) \
  {                                                            \
    stub_test_can_reload_timer_anytime(CH_NUM);                \
  }

#define TEST_CH_LOAD_LENGTH_WITH_ZERO(CH_NUM)              \
  void test_channel_##CH_NUM##_load_length_with_zero(void) \
  {                                                        \
    stub_test_load_length_with_zero(CH_NUM);               \
  }

#define TEST_CH_TRIGGER_SHOULD_NOT_AFFECT_LENGTH(CH_NUM)              \
  void test_channel_##CH_NUM##_trigger_should_not_affect_length(void) \
  {                                                                   \
    stub_test_trigger_should_not_affect_length(CH_NUM);               \
  }

#define TEST_CH_TRIGGER_WITH_ZERO_LENGTH(CH_NUM)              \
  void test_channel_##CH_NUM##_trigger_with_zero_length(void) \
  {                                                           \
    stub_test_trigger_with_zero_length(CH_NUM);               \
  }

#define TEST_CH_TRIGGER_WITH_LENGTH_DISABLED(CH_NUM)              \
  void test_channel_##CH_NUM##_trigger_with_length_disabled(void) \
  {                                                               \
    stub_test_trigger_with_length_disabled(CH_NUM);               \
  }

#define TEST_CH_DISABLING_LENGTH_SHOULD_NOT_REENABLE_CHANNEL(CH_NUM)   \
  void test_disabling_length_should_not_reenable_channel##CH_NUM(void) \
  {                                                                    \
    stub_test_disabling_length_should_not_reenable_channel(CH_NUM);    \
  }

#define TEST_CH_LENGTH_SHOULD_NOT_TICK_WHEN_LENGTH_IS_DISABLED(CH_NUM)              \
  void test_channel_##CH_NUM##_length_should_not_tick_when_length_is_disabled(void) \
  {                                                                                 \
    stub_test_length_should_not_tick_when_length_is_disabled(CH_NUM);               \
  }

#define TEST_CH_RELOADING_SHOULD_NOT_REENABLE_CHANNEL(CH_NUM)    \
  void test_reloading_should_not_reenable_channel_##CH_NUM(void) \
  {                                                              \
    stub_test_reloading_should_not_reenable_channel(CH_NUM);     \
  }

#define TEST_CH_LENGTH_SHOULD_TICK_WHILE_CHANNEL_IS_DISABLED(CH_NUM)     \
  void test_length_should_tick_when_channel_##CH_NUM##_is_disabled(void) \
  {                                                                      \
    stub_length_should_tick_while_channel_is_disabled(CH_NUM);           \
  }

#define TEST_CH_DISABLED_CHANNEL_WITH_ZERO_LENGTH(CH_NUM)      \
  void test_disabled_channel_##CH_NUM##_with_zero_length(void) \
  {                                                            \
    stub_test_disabled_channel_with_zero_length(CH_NUM);       \
  }

#define TEST_CH_CHANNEL_IS_DISABLED_WHEN_DAC_IS_DISABLED(CH_NUM)      \
  void test_channel_##CH_NUM##_is_disabled_when_dac_is_disabled(void) \
  {                                                                   \
    stub_test_channel_is_disabled_when_dac_is_disabled(CH_NUM);       \
  }

#define TEST_CH_DISABLED_DAC_PREVENTS_CHANNEL_ENABLE_AT_TRIGGER(CH_NUM)      \
  void test_disabled_dac_prevents_channel_##CH_NUM##_enable_at_trigger(void) \
  {                                                                          \
    stub_test_disabled_dac_should_prevent_enable_at_trigger(CH_NUM);         \
  }

#define TEST_CH_ENABLING_DAC_SHOULD_NOT_REENABLE_CHANNEL(CH_NUM)    \
  void test_enabling_DAC_should_not_reenable_channel_##CH_NUM(void) \
  {                                                                 \
    stub_test_enabling_DAC_should_not_reenable_channel(CH_NUM);     \
  }

static apu_handle_t apu;

static const uint8_t channel_enabled_mask[] = {
    [1] = APU_ACTL_CH1_EN,
    [2] = APU_ACTL_CH2_EN,
    [3] = APU_ACTL_CH3_EN,
    [4] = APU_ACTL_CH4_EN,
};

static void apu_delay(uint32_t count)
{
  count *= (CPU_FREQ / FRAME_SEQ_RATE);

  while (count--)
  {
    apu_tick(&apu);
  }
}

static inline uint16_t get_reg_addr(uint8_t ch_num, uint8_t reg_num)
{
  return 0xFF10 + (5 * (ch_num - 1)) + (reg_num % 5);
}

static void enable_dac(uint8_t channel_num)
{
  if (channel_num == 3)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 0), APU_WAVE_DAC_EN));
  }
  else
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 2), 0x10));
  }
}

static void disable_dac(uint8_t channel_num)
{
  if (channel_num == 3)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 0), 0x00));
  }
  else
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 2), 0x00));
  }
}

static void load_length_timer(uint8_t channel_num, uint8_t ticks)
{
  uint8_t timer_val = (channel_num == 3) ? 255 : 64;
  timer_val -= ticks;
  timer_val += (channel_num == 3) ? 1 : 0;

  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 1), timer_val));
}

static void stub_test_channel_length_countdown_reset(uint8_t channel_num)
{
  /* Load the length timer */
  load_length_timer(channel_num, 4);

  /* Silence the channel without disabling it */
  enable_dac(channel_num);

  /* Enable the channel and length counter */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Verify the channel is now enabled */
  TEST_ASSERT_CHANNEL_ENABLED(channel_num);

  /* Wait for 3 frame sequences */
  apu_delay(3);

  /* Verify the channel is still enabled */
  TEST_ASSERT_CHANNEL_ENABLED(channel_num);

  /* Wait for another frame sequence */
  apu_delay(1);

  /* Verify the channel should be disabled now */
  TEST_ASSERT_CHANNEL_DISABLED(channel_num);
}

void stub_test_can_reload_timer_anytime(uint8_t channel_num)
{
  /* Load the length timer */
  load_length_timer(channel_num, 4);

  /* Silence the channel without disabling it */
  enable_dac(channel_num);

  /* Enable the channel and length counter */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait 1 frame */
  apu_delay(1);

  /** Verify the channel is still enabled */
  TEST_ASSERT_CHANNEL_ENABLED(channel_num);

  /* Reload the length counter */
  load_length_timer(channel_num, 10);

  /* Wait 9 frame */
  apu_delay(9);

  /** Verify the channel is 1 frame away from becoming disabled */
  TEST_ASSERT_CHANNEL_ALMOST_DISABLED(channel_num);
}

void stub_test_load_length_with_zero(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 1), 0));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for N-1 frames */
  apu_delay((channel_num == 3) ? 255 : 63);

  /** Verify the channel is 1 frame away from becoming disabled */
  TEST_ASSERT_CHANNEL_ALMOST_DISABLED(channel_num);
}

void stub_test_trigger_should_not_affect_length(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for 1 frame */
  apu_delay(1);

  /* Trigger the channel again */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for 2 frames */
  apu_delay(2);

  /** Verify the channel is 1 frame away from becoming disabled */
  TEST_ASSERT_CHANNEL_ALMOST_DISABLED(channel_num);
}

void stub_test_trigger_with_zero_length(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for the length timer to expire */
  apu_delay(4);

  /* Trigger the channel again */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for N-1 frames */
  apu_delay((channel_num == 3) ? 255 : 63);

  /** Verify the channel is 1 frame away from becoming disabled */
  TEST_ASSERT_CHANNEL_ALMOST_DISABLED(channel_num);
}

void stub_test_trigger_with_length_disabled(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for the length timer to expire */
  apu_delay(4);

  /* Disable length */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), 0x00));

  /* Trigger the channel only */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN));

  /* Enable length */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_LENGTH_EN));

  /* Wait for N-1 frames */
  apu_delay((channel_num == 3) ? 255 : 63);

  /** Verify the channel is 1 frame away from becoming disabled */
  TEST_ASSERT_CHANNEL_ALMOST_DISABLED(channel_num);
}

void stub_test_disabling_length_should_not_reenable_channel(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for the length timer to expire */
  apu_delay(4);

  /* Verify the channel is disabled */
  TEST_ASSERT_CHANNEL_DISABLED(channel_num);

  /* Disable length */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), 0x00));

  /* Verify the channel is still disabled */
  TEST_ASSERT_CHANNEL_DISABLED(channel_num);
}

void stub_test_length_should_not_tick_when_length_is_disabled(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and trigger the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN));

  /* Wait for 4 frames */
  apu_delay(4);

  /* Enable length */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_LENGTH_EN));

  /* Wait for 3 frames */
  apu_delay(3);

  /** Verify the channel is 1 frame away from becoming disabled */
  TEST_ASSERT_CHANNEL_ALMOST_DISABLED(channel_num);
}

void stub_test_reloading_should_not_reenable_channel(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for the length timer to expire */
  apu_delay(4);

  /* Verify the channel is disabled */
  TEST_ASSERT_CHANNEL_DISABLED(channel_num);

  /* Reload the length timer */
  load_length_timer(channel_num, 2);

  /* Verify the channel is still disabled */
  TEST_ASSERT_CHANNEL_DISABLED(channel_num);
}

void stub_length_should_tick_while_channel_is_disabled(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for the length timer to expire */
  apu_delay(4);

  /* Reload the timer */
  load_length_timer(channel_num, 8);

  /* Wait for 4 frames */
  apu_delay(4);

  /* Enable the channel again */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for 3 frames */
  apu_delay(3);

  /** Verify the channel is 1 frame away from becoming disabled */
  TEST_ASSERT_CHANNEL_ALMOST_DISABLED(channel_num);
}

void stub_test_disabled_channel_with_zero_length(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for the length timer to expire */
  apu_delay(4);

  /* Load max timer length */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 1), 0));

  /* Wait for 32 frames */
  apu_delay(32);

  /* Reenable the channel */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for N-1 frames */
  apu_delay((channel_num == 3) ? (256 - 33) : (64 - 33));

  /** Verify the channel is 1 frame away from becoming disabled */
  TEST_ASSERT_CHANNEL_ALMOST_DISABLED(channel_num);
}

void stub_test_channel_is_disabled_when_dac_is_disabled(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Wait for 2 frames */
  apu_delay(2);

  /** Verify the channel is still enabled */
  TEST_ASSERT_CHANNEL_ENABLED(channel_num);

  /** Disable the DAC */
  disable_dac(channel_num);

  /** Verify the channel is now disabled */
  TEST_ASSERT_CHANNEL_DISABLED(channel_num);
}

void stub_test_disabled_dac_should_prevent_enable_at_trigger(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /* Disable the DAC */
  enable_dac(channel_num);

  /* Trigger the channel */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN));

  /** Verify the channel is still disabled */
  TEST_ASSERT_CHANNEL_ENABLED(channel_num);
}

void stub_test_enabling_DAC_should_not_reenable_channel(uint8_t channel_num)
{
  /** Enable the DAC */
  enable_dac(channel_num);

  /** Load length timer and enable the channel */
  load_length_timer(channel_num, 4);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, get_reg_addr(channel_num, 4), APU_TRIGGER_EN | APU_LENGTH_EN));

  /** Wait 2 audio frames */
  apu_delay(2);

  /** Verify the channel is still enabled */
  TEST_ASSERT_CHANNEL_ENABLED(channel_num);

  /** Disable the DAC */
  disable_dac(channel_num);

  /** Verify the channel is now disabled */
  TEST_ASSERT_CHANNEL_DISABLED(channel_num);

  /** Enable the DAC */
  enable_dac(channel_num);

  /** Verify the channel is still disabled */
  TEST_ASSERT_CHANNEL_DISABLED(channel_num);
}

void setUp(void)
{
  memset(&apu, 0, sizeof(apu_handle_t));
  apu.bus_interface.offset = 0xFF10;

  callback_init_IgnoreAndReturn(STATUS_OK);
  apu_init(&apu);

  /* Enable APU */
  bus_interface_write(&apu.bus_interface, 0xFF26, APU_ACTL_AUDIO_EN);
}

void tearDown(void)
{
  callback_init_StopIgnore();
}

TEST_CH_LENGTH_CTR_RESET(1);
TEST_CH_LENGTH_CTR_RESET(2);
TEST_CH_LENGTH_CTR_RESET(3);
TEST_CH_LENGTH_CTR_RESET(4);

TEST_CH_CAN_RELOAD_LENGTH_ANYTIME(1);
TEST_CH_CAN_RELOAD_LENGTH_ANYTIME(2);
TEST_CH_CAN_RELOAD_LENGTH_ANYTIME(3);
TEST_CH_CAN_RELOAD_LENGTH_ANYTIME(4);

TEST_CH_LOAD_LENGTH_WITH_ZERO(1);
TEST_CH_LOAD_LENGTH_WITH_ZERO(2);
TEST_CH_LOAD_LENGTH_WITH_ZERO(3);
TEST_CH_LOAD_LENGTH_WITH_ZERO(4);

TEST_CH_TRIGGER_SHOULD_NOT_AFFECT_LENGTH(1);
TEST_CH_TRIGGER_SHOULD_NOT_AFFECT_LENGTH(2);
TEST_CH_TRIGGER_SHOULD_NOT_AFFECT_LENGTH(3);
TEST_CH_TRIGGER_SHOULD_NOT_AFFECT_LENGTH(4);

TEST_CH_TRIGGER_WITH_ZERO_LENGTH(1);
TEST_CH_TRIGGER_WITH_ZERO_LENGTH(2);
TEST_CH_TRIGGER_WITH_ZERO_LENGTH(3);
TEST_CH_TRIGGER_WITH_ZERO_LENGTH(4);

TEST_CH_TRIGGER_WITH_LENGTH_DISABLED(1);
TEST_CH_TRIGGER_WITH_LENGTH_DISABLED(2);
TEST_CH_TRIGGER_WITH_LENGTH_DISABLED(3);
TEST_CH_TRIGGER_WITH_LENGTH_DISABLED(4);

TEST_CH_DISABLING_LENGTH_SHOULD_NOT_REENABLE_CHANNEL(1);
TEST_CH_DISABLING_LENGTH_SHOULD_NOT_REENABLE_CHANNEL(2);
TEST_CH_DISABLING_LENGTH_SHOULD_NOT_REENABLE_CHANNEL(3);
TEST_CH_DISABLING_LENGTH_SHOULD_NOT_REENABLE_CHANNEL(4);

TEST_CH_LENGTH_SHOULD_NOT_TICK_WHEN_LENGTH_IS_DISABLED(1);
TEST_CH_LENGTH_SHOULD_NOT_TICK_WHEN_LENGTH_IS_DISABLED(2);
TEST_CH_LENGTH_SHOULD_NOT_TICK_WHEN_LENGTH_IS_DISABLED(3);
TEST_CH_LENGTH_SHOULD_NOT_TICK_WHEN_LENGTH_IS_DISABLED(4);

TEST_CH_RELOADING_SHOULD_NOT_REENABLE_CHANNEL(1);
TEST_CH_RELOADING_SHOULD_NOT_REENABLE_CHANNEL(2);
TEST_CH_RELOADING_SHOULD_NOT_REENABLE_CHANNEL(3);
TEST_CH_RELOADING_SHOULD_NOT_REENABLE_CHANNEL(4);

TEST_CH_LENGTH_SHOULD_TICK_WHILE_CHANNEL_IS_DISABLED(1);
TEST_CH_LENGTH_SHOULD_TICK_WHILE_CHANNEL_IS_DISABLED(2);
TEST_CH_LENGTH_SHOULD_TICK_WHILE_CHANNEL_IS_DISABLED(3);
TEST_CH_LENGTH_SHOULD_TICK_WHILE_CHANNEL_IS_DISABLED(4);

TEST_CH_DISABLED_CHANNEL_WITH_ZERO_LENGTH(1);
TEST_CH_DISABLED_CHANNEL_WITH_ZERO_LENGTH(2);
TEST_CH_DISABLED_CHANNEL_WITH_ZERO_LENGTH(3);
TEST_CH_DISABLED_CHANNEL_WITH_ZERO_LENGTH(4);

TEST_CH_CHANNEL_IS_DISABLED_WHEN_DAC_IS_DISABLED(1);
TEST_CH_CHANNEL_IS_DISABLED_WHEN_DAC_IS_DISABLED(2);
TEST_CH_CHANNEL_IS_DISABLED_WHEN_DAC_IS_DISABLED(3);
TEST_CH_CHANNEL_IS_DISABLED_WHEN_DAC_IS_DISABLED(4);

TEST_CH_DISABLED_DAC_PREVENTS_CHANNEL_ENABLE_AT_TRIGGER(1);
TEST_CH_DISABLED_DAC_PREVENTS_CHANNEL_ENABLE_AT_TRIGGER(2);
TEST_CH_DISABLED_DAC_PREVENTS_CHANNEL_ENABLE_AT_TRIGGER(3);
TEST_CH_DISABLED_DAC_PREVENTS_CHANNEL_ENABLE_AT_TRIGGER(4);

TEST_CH_ENABLING_DAC_SHOULD_NOT_REENABLE_CHANNEL(1);
TEST_CH_ENABLING_DAC_SHOULD_NOT_REENABLE_CHANNEL(2);
TEST_CH_ENABLING_DAC_SHOULD_NOT_REENABLE_CHANNEL(3);
TEST_CH_ENABLING_DAC_SHOULD_NOT_REENABLE_CHANNEL(4);
