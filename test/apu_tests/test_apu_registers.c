#include "unity.h"

#include <string.h>

#include "apu.h"
#include "apu_pwm.h"
#include "apu_lfsr.h"
#include "apu_wave.h"
#include "audio_playback_samples.h"
#include "bus_interface.h"
#include "status_code.h"

#include "mock_callback.h"

TEST_FILE("apu.c")
TEST_FILE("apu_pwm.c")
TEST_FILE("apu_lfsr.c")
TEST_FILE("apu_wave.c")

static const uint8_t apu_reg_masks[] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF, /* NR10 - NR14 */
    0xFF, 0x3F, 0x00, 0xFF, 0xBF, /* NR20 - NR24 */
    0x7F, 0xFF, 0x9F, 0xFF, 0xBF, /* NR30 - NR34 */
    0xFF, 0xFF, 0x00, 0x00, 0xBF, /* NR40 - NR44 */
    0x00, 0x00, 0x70,             /* NR50 - NR52 */
};

static apu_handle_t apu;

void setUp(void)
{
  memset(&apu, 0, sizeof(apu_handle_t));
  apu.bus_interface.offset = 0xFF10;

  callback_init_IgnoreAndReturn(STATUS_OK);
  apu_init(&apu);
}

void tearDown(void)
{
  callback_init_StopIgnore();
}

void test_apu_registers_initial_values(void)
{
  uint16_t address = 0xFF10;
  uint8_t data = 0;

  for (uint8_t i = 0; i < sizeof(apu_reg_masks); i++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, address + i, &data));
    TEST_ASSERT_EQUAL_HEX8(apu_reg_masks[i], data);
  }
}

void test_wave_ram_initial_values(void)
{
  uint8_t data = 0;

  for (uint16_t addr = 0xFF30; addr < 0xFF40; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(0, data);
  }
}

void test_unused_regions_initial_values(void)
{
  uint8_t data = 0;

  for (uint16_t addr = 0xFF27; addr < 0xFF30; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(0xFF, data);
  }
}

void test_registers_read_and_write(void)
{
  uint8_t data = 0;
  uint8_t index = 0;

  /** Make sure the APU is enabled to enable writing to registers */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, 0x80));

  for (uint16_t addr = 0xFF10; addr < 0xFF26; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, addr, 0x55));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(0x55 | apu_reg_masks[index++], data);
  }

  index = 0;

  for (uint16_t addr = 0xFF10; addr < 0xFF26; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, addr, 0xAA));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(0xAA | apu_reg_masks[index++], data);
  }

  /** NR52 specific test */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, 0x00));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, 0xFF26, &data));
  TEST_ASSERT_EQUAL_HEX8(apu_reg_masks[22], data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, 0xFF));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, 0xFF26, &data));
  TEST_ASSERT_EQUAL_HEX8(APU_ACTL_AUDIO_EN | apu_reg_masks[22], data);
}

void test_wave_ram_read_and_write(void)
{
  uint8_t data = 0;

  for (uint16_t addr = 0xFF30; addr < 0xFF40; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, addr, 0x55));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(0x55, data);
  }

  for (uint16_t addr = 0xFF30; addr < 0xFF40; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, addr, 0xAA));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(0xAA, data);
  }
}

void test_unused_regions_read_and_write(void)
{
  uint8_t data = 0;

  for (uint16_t addr = 0xFF27; addr < 0xFF30; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, addr, 0x55));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(0xFF, data);
  }

  for (uint16_t addr = 0xFF27; addr < 0xFF30; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, addr, 0xAA));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(0xFF, data);
  }
}

void test_disabling_apu_should_preserve_wave_ram_contents(void)
{
  uint8_t data = 0;

  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, APU_ACTL_AUDIO_EN));

  for (uint16_t addr = 0xFF30; addr < 0xFF40; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, addr, 0xAA));
  }

  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, 0x00));

  for (uint16_t addr = 0xFF30; addr < 0xFF40; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(0xAA, data);
  }
}

void test_disabling_apu_should_reset_register_values(void)
{
  uint8_t data = 0;
  uint8_t index = 0;

  /** Make sure the APU is enabled to enable writing to registers */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, 0x80));

  /** Set all register bits to 1 except for NR52 */
  for (uint16_t addr = 0xFF10; addr < 0xFF26; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, addr, 0xFF));
  }

  /** Disable, then re-enable the APU */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, 0x00));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, APU_ACTL_AUDIO_EN));

  /** Register values should revert to their initial values */
  for (uint16_t addr = 0xFF10; addr < 0xFF26; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(apu_reg_masks[index++], data);
  }
}

void test_disabling_apu_should_disable_register_writes(void)
{
  uint8_t data = 0;
  uint8_t index = 0;

  /** Make sure the APU is disabled */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, 0x00));

  /** Set all register bits to 1 except for NR52 */
  for (uint16_t addr = 0xFF10; addr < 0xFF26; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, addr, 0xFF));
  }

  /** Re-enable the APU */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&apu.bus_interface, 0xFF26, APU_ACTL_AUDIO_EN));

  /** Register values should retain their initial values */
  for (uint16_t addr = 0xFF10; addr < 0xFF26; addr++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&apu.bus_interface, addr, &data));
    TEST_ASSERT_EQUAL_HEX8(apu_reg_masks[index++], data);
  }
}
