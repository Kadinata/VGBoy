#include "mbc.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "bus_interface.h"
#include "callback.h"
#include "rom.h"
#include "rtc.h"
#include "status_code.h"

static status_code_t mbc_init_ext_ram(mbc_handle_t *const mbc);
static status_code_t mbc_init_battery(mbc_handle_t *const mbc);
static status_code_t mbc_init_rtc(mbc_handle_t *const mbc);
static status_code_t mbc_read(void *const resource, uint16_t address, uint8_t *const data);
static status_code_t mbc_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_ext_ram_read(void *const resource, uint16_t address, uint8_t *const data);
static status_code_t mbc_ext_ram_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_ext_ram_rtc_read(void *const resource, uint16_t address, uint8_t *const data);
static status_code_t mbc_ext_ram_rtc_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_rom_read(void *const resource, uint16_t address, uint8_t *const data);
static status_code_t mbc_no_rom_banking_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_1_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_2_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_3_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_5_write(void *const resource, uint16_t address, uint8_t const data);

static inline status_code_t mbc_switch_ext_ram_bank(mbc_handle_t *const mbc, uint8_t ram_bank_num, bool save_game);
static inline status_code_t mbc_switch_rom_bank(mbc_handle_t *const mbc, uint8_t rom_bank_num);
static inline void mbc_set_flag(mbc_handle_t *const mbc, mbc_flags_t const flags);
static inline void mbc_clear_flag(mbc_handle_t *const mbc, mbc_flags_t const flags);

status_code_t mbc_init(mbc_handle_t *const mbc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);

  status_code_t status = STATUS_OK;

  status = bus_interface_init(&mbc->bus_interface, mbc_read, mbc_write, mbc);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

status_code_t mbc_load_rom(mbc_handle_t *const mbc, uint8_t *const rom_data, const size_t size)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);

  status_code_t status = STATUS_OK;

  status = rom_load(&mbc->rom.content, rom_data, size);
  RETURN_STATUS_IF_NOT_OK(status);

  status = mbc_init_battery(mbc);
  RETURN_STATUS_IF_NOT_OK(status);

  status = mbc_init_rtc(mbc);
  RETURN_STATUS_IF_NOT_OK(status);

  status = mbc_init_ext_ram(mbc);
  RETURN_STATUS_IF_NOT_OK(status);

  mbc->rom.num_banks = (2 << (mbc->rom.content.header->rom_size & 0x0F));
  if (((mbc->rom.content.header->rom_size >> 4) & 0xF) == 5)
  {
    mbc->rom.num_banks += 64;
  }

  switch (mbc->rom.content.header->cartridge_type)
  {
  case ROM_ONLY:
  case ROM_RAM:
  case ROM_RAM_BATT:
    status = bus_interface_init(&mbc->rom.bus_interface, mbc_rom_read, mbc_no_rom_banking_write, mbc);
    break;
  case ROM_MBC1:
  case ROM_MBC1_RAM:
  case ROM_MBC1_RAM_BATT:
    status = bus_interface_init(&mbc->rom.bus_interface, mbc_rom_read, mbc_1_write, mbc);
    break;
  case ROM_MBC2:
  case ROM_MBC2_BATT:
    status = bus_interface_init(&mbc->rom.bus_interface, mbc_rom_read, mbc_2_write, mbc);
    break;
  case ROM_MBC3:
  case ROM_MBC3_RAM:
  case ROM_MBC3_RAM_BATT:
  case ROM_MBC3_TIMER_BATT:
  case ROM_MBC3_TIMER_RAM_BATT:
    status = bus_interface_init(&mbc->rom.bus_interface, mbc_rom_read, mbc_3_write, mbc);
    break;
  case ROM_MBC5:
  case ROM_MBC5_RAM:
  case ROM_MBC5_RAM_BATT:
  case ROM_MBC5_RUMBLE:
  case ROM_MBC5_RUMBLE_RAM:
  case ROM_MBC5_RUMBLE_RAM_BATT:
    status = bus_interface_init(&mbc->rom.bus_interface, mbc_rom_read, mbc_5_write, mbc);
    break;
  default:
    Log_E("Unsupported cartridge type: 0x%02X", mbc->rom.content.header->cartridge_type);
    return STATUS_ERR_UNSUPPORTED;
    break;
  }
  RETURN_STATUS_IF_NOT_OK(status);

  status = mbc_switch_rom_bank(mbc, 1);
  RETURN_STATUS_IF_NOT_OK(status);

  status = mbc_load_saved_game(mbc);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

status_code_t mbc_register_callbacks(mbc_handle_t *const mbc, mbc_callbacks_t *const callbacks)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(callbacks);

  mbc->callbacks.save_game = callbacks->save_game;
  mbc->callbacks.load_game = callbacks->load_game;

  return STATUS_OK;
}

status_code_t mbc_cleanup(mbc_handle_t *const mbc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);

  mbc_save_game(mbc);

  free(mbc->ext_ram.data);
  mbc->ext_ram.data = NULL;

  return rom_unload(&mbc->rom.content);
}

static status_code_t mbc_init_ext_ram(mbc_handle_t *const mbc)
{
  mbc->ext_ram.enabled = 0;
  mbc->ext_ram.num_banks = 0;

  switch (mbc->rom.content.header->ram_size)
  {
  case MBC_EXT_RAM_SIZE_NO_RAM:
  case MBC_EXT_RAM_SIZE_UNUSED:
    break;
  case MBC_EXT_RAM_SIZE_8K:
    mbc->ext_ram.num_banks = 1;
    break;
  case MBC_EXT_RAM_SIZE_32K:
    mbc->ext_ram.num_banks = 4;
    break;
  case MBC_EXT_RAM_SIZE_64K:
    mbc->ext_ram.num_banks = 8;
    break;
  case MBC_EXT_RAM_SIZE_128K:
    mbc->ext_ram.num_banks = 16;
    break;
  default:
    break;
  }

  if (mbc->ext_ram.num_banks > 0)
  {
    mbc->ext_ram.data = calloc(mbc->ext_ram.num_banks, 0x2000);
    VERIFY_COND_RETURN_STATUS_IF_TRUE(mbc->ext_ram.data == NULL, STATUS_ERR_NO_MEMORY);
  }

  mbc->ext_ram.active_bank_num = 0;
  mbc->ext_ram.active_bank = mbc->ext_ram.data;
  mbc->ext_ram.bus_interface.offset = 0xA000;

  if (rtc_is_present(&mbc->rtc))
  {
    return bus_interface_init(&mbc->ext_ram.bus_interface, mbc_ext_ram_rtc_read, mbc_ext_ram_rtc_write, mbc);
  }

  return bus_interface_init(&mbc->ext_ram.bus_interface, mbc_ext_ram_read, mbc_ext_ram_write, mbc);
}

static status_code_t mbc_init_battery(mbc_handle_t *const mbc)
{
  mbc->batt.has_unsaved_data = false;
  mbc->batt.present = false;

  switch (mbc->rom.content.header->cartridge_type)
  {
  case ROM_MBC1_RAM_BATT:
  case ROM_MBC2_BATT:
  case ROM_RAM_BATT:
  case ROM_MMM01_RAM_BATT:
  case ROM_MBC3_TIMER_BATT:
  case ROM_MBC3_TIMER_RAM_BATT:
  case ROM_MBC3_RAM_BATT:
  case ROM_MBC5_RAM_BATT:
  case ROM_MBC5_RUMBLE_RAM_BATT:
  case ROM_MBC7_SENSOR_RUMBLE_RAM_BATT:
  case ROM_HUC1_RAM_BATT:
    mbc->batt.present = true;
    break;
  default:
    break;
  }

  return STATUS_OK;
}

static status_code_t mbc_init_rtc(mbc_handle_t *const mbc)
{
  switch (mbc->rom.content.header->cartridge_type)
  {
  case ROM_MBC3_TIMER_BATT:
  case ROM_MBC3_TIMER_RAM_BATT:
    return rtc_init(&mbc->rtc);
  default:
    break;
  }

  return STATUS_OK;
}

static inline status_code_t mbc_switch_ext_ram_bank(mbc_handle_t *const mbc, uint8_t ram_bank_num, bool save_game)
{
  VERIFY_COND_RETURN_STATUS_IF_TRUE(ram_bank_num >= mbc->ext_ram.num_banks, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;

  if (save_game)
  {
    status = mbc_save_game(mbc);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  mbc->ext_ram.active_bank_num = ram_bank_num;
  mbc->ext_ram.active_bank = &(mbc->ext_ram.data[0x2000 * mbc->ext_ram.active_bank_num]);

  return STATUS_OK;
}

static inline status_code_t mbc_switch_rom_bank(mbc_handle_t *const mbc, uint8_t rom_bank_num)
{
  VERIFY_COND_RETURN_STATUS_IF_TRUE(rom_bank_num >= mbc->rom.num_banks, STATUS_ERR_INVALID_ARG);

  mbc->rom.active_bank_num = rom_bank_num;
  mbc->rom.active_switchable_bank = &(mbc->rom.content.data[0x4000 * mbc->rom.active_bank_num]);

  return STATUS_OK;
}

status_code_t mbc_load_saved_game(mbc_handle_t *const mbc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);

  if (!mbc->batt.present || !mbc->callbacks.load_game)
  {
    return STATUS_OK;
  }

  saved_game_data_t saved_data = {
      .ram_data_size = 0x2000 * mbc->ext_ram.num_banks,
      .ram_data = mbc->ext_ram.data,
      .rtc = &mbc->rtc,
  };

  return mbc->callbacks.load_game(&saved_data);
}

status_code_t mbc_save_game(mbc_handle_t *const mbc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);

  if (!mbc->batt.present || !mbc->batt.has_unsaved_data || !mbc->callbacks.save_game)
  {
    return STATUS_OK;
  }

  saved_game_data_t saved_data = {
      .ram_data_size = 0x2000 * mbc->ext_ram.num_banks,
      .ram_data = mbc->ext_ram.data,
      .rtc = &mbc->rtc,
  };

  return mbc->callbacks.save_game(&saved_data);
}

status_code_t mbc_reload_banks(mbc_handle_t *const mbc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);

  status_code_t status = STATUS_OK;

  status = mbc_switch_ext_ram_bank(mbc, mbc->ext_ram.active_bank_num, false);
  RETURN_STATUS_IF_NOT_OK(status);

  status = mbc_switch_rom_bank(mbc, mbc->rom.active_bank_num);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

static status_code_t mbc_read(void *const resource, uint16_t address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  if (address < 0x8000)
  {
    return bus_interface_read(&mbc->rom.bus_interface, address, data);
  }
  else if ((address >= 0xA000) & (address < 0xC000))
  {
    return bus_interface_read(&mbc->ext_ram.bus_interface, address, data);
  }

  return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
}

static status_code_t mbc_write(void *const resource, uint16_t address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  if (address < 0x8000)
  {
    return bus_interface_write(&mbc->rom.bus_interface, address, data);
  }
  if ((address >= 0xA000) & (address < 0xC000))
  {
    return bus_interface_write(&mbc->ext_ram.bus_interface, address, data);
  }

  return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
}

static status_code_t mbc_rom_read(void *const resource, uint16_t address, uint8_t *const data)
{
  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  if (address < 0x4000)
  {
    /** 0x0000 - 0x3FFF: ROM Bank 0 */
    *data = mbc->rom.content.data[address];
  }
  else if ((address >= 0x4000) && (address < 0x8000))
  {
    /** 0x4000 - 0x7FFF: Switchable ROM bank */
    *data = mbc->rom.active_switchable_bank[address - 0x4000];
  }

  return STATUS_OK;
}

static status_code_t mbc_no_rom_banking_write(void *const __attribute__((unused)) resource, uint16_t __attribute__((unused)) address, uint8_t const __attribute__((unused)) data)
{
  return STATUS_OK;
}

static status_code_t mbc_1_write(void *const resource, uint16_t address, uint8_t const data)
{
  mbc_handle_t *const mbc = (mbc_handle_t *)resource;
  status_code_t status = STATUS_OK;

  if (address < 0x2000)
  {
    /** 0x0000 - 0x1FFF: RAM enable (Write only) */
    mbc->ext_ram.enabled = ((data & 0x0F) == 0xA);
  }
  else if ((address >= 0x2000) && (address < 0x4000))
  {
    /** 0x2000 - 0x3FFF: ROM bank number (Write only) */
    uint16_t rom_bank = (mbc->rom.active_bank_num & ~(0x1F)) | ((data ? data : 1) & 0x1F);
    status = mbc_switch_rom_bank(mbc, rom_bank);
    RETURN_STATUS_IF_NOT_OK(status);
  }
  else if ((address >= 0x4000) && (address < 0x6000) && (mbc->flags & MBC_FLAGS_BANK_MODE_ADVANCED))
  {
    /** 0x4000 - 0x5FFF: RAM bank number or upper bits of ROM bank number (Write only) */
    if (mbc->rom.content.header->ram_size == MBC_EXT_RAM_SIZE_32K)
    {
      status = mbc_switch_ext_ram_bank(mbc, data & 0x3, true);
      RETURN_STATUS_IF_NOT_OK(status);
    }
    else if (mbc->rom.num_banks > 32)
    {
      uint16_t rom_bank = (mbc->rom.active_bank_num & ~(0x3 << 5)) | ((data & 0x3) << 5);
      status = mbc_switch_rom_bank(mbc, rom_bank);
      RETURN_STATUS_IF_NOT_OK(status);
    }
  }
  else if ((address >= 0x6000) && (address < 0x8000))
  {
    /** 0x6000 - 0x7FFF: Banking mode select (Write only) */
    ((data & 0x1) ? mbc_set_flag(mbc, MBC_FLAGS_BANK_MODE_ADVANCED) : mbc_clear_flag(mbc, MBC_FLAGS_BANK_MODE_ADVANCED));
    if (mbc->flags & MBC_FLAGS_BANK_MODE_ADVANCED)
    {
      if (mbc->rom.content.header->ram_size == MBC_EXT_RAM_SIZE_32K)
      {
        status = mbc_switch_ext_ram_bank(mbc, data & 0x3, true);
        RETURN_STATUS_IF_NOT_OK(status);
      }

      // TODO: Handle ROM Bank 0 switching
    }
  }

  return STATUS_OK;
}

static status_code_t mbc_2_write(void *const resource, uint16_t address, uint8_t const data)
{
  mbc_handle_t *const mbc = (mbc_handle_t *)resource;
  status_code_t status = STATUS_OK;

  if (address < 0x4000)
  {
    /** 0x0000 - 0x3FFF: RAM Enable, ROM Bank Number (Write only) */
    if (address & 0x0100)
    {
      uint16_t rom_bank_num = (data ? data : 1) & 0x0F;
      status = mbc_switch_rom_bank(mbc, rom_bank_num);
      RETURN_STATUS_IF_NOT_OK(status);
    }
    else
    {
      mbc->ext_ram.enabled = ((data & 0x0F) == 0xA);
    }
  }
  return STATUS_OK;
}

static status_code_t mbc_3_write(void *const resource, uint16_t address, uint8_t const data)
{
  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  status_code_t status = STATUS_OK;

  if (address < 0x2000)
  {
    /** 0x0000 - 0x1FFF: RAM and timer enable (Write only) */
    mbc->ext_ram.enabled = ((data & 0x0F) == 0xA);

    status = rtc_enable(&mbc->rtc, (data & 0x0F) == 0xA);
    RETURN_STATUS_IF_NOT_OK(status);
  }
  else if ((address >= 0x2000) && (address < 0x4000))
  {
    /** 0x2000 - 0x3FFF: ROM bank number (Write only) */
    uint16_t rom_bank_num = (data ? data : 1) & 0x7F;
    status = mbc_switch_rom_bank(mbc, rom_bank_num);
    RETURN_STATUS_IF_NOT_OK(status);
  }
  else if ((address >= 0x4000) && (address < 0x6000))
  {
    /**
     * 0x4000 - 0x5FFF: RAM bank number or RTC register select (Write only)
     *
     * As for the MBC1s RAM Banking Mode, writing a value in range for $00-$03 maps the corresponding
     * external RAM Bank (if any) into memory at A000-BFFF. When writing a value of $08-$0C, this will
     * map the corresponding RTC register into memory at A000-BFFF. That register could then be
     * read/written by accessing any address in that area, typically that is done by using address A000.
     */
    if ((data >= 0x00) && (data <= 0x03))
    {
      status = mbc_switch_ext_ram_bank(mbc, data & 0x3, true);
      RETURN_STATUS_IF_NOT_OK(status);

      mbc_clear_flag(mbc, MBC_FLAGS_ACCESS_MODE_RTC);
    }
    else if ((data >= 0x08) && (data <= 0x0C) && rtc_is_present(&mbc->rtc))
    {
      status = rtc_select_reg(&mbc->rtc, data - 0x08);
      RETURN_STATUS_IF_NOT_OK(status);

      mbc_set_flag(mbc, MBC_FLAGS_ACCESS_MODE_RTC);
    }
  }
  else if ((address >= 0x6000) && (address < 0x8000) && rtc_is_present(&mbc->rtc))
  {
    /**
     * 0x6000 - 0x7FFF: Latch clock data (Write only)
     * When writing $00, and then $01 to this register, the current time becomes latched into the RTC registers.
     * The latched data will not change until it becomes latched again, by repeating the write $00->$01 procedure.
     * This provides a way to read the RTC registers while the clock keeps ticking.
     */
    status = rtc_latch(&mbc->rtc, data);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return STATUS_OK;
}

static status_code_t mbc_5_write(void *const resource, uint16_t address, uint8_t const data)
{
  mbc_handle_t *const mbc = (mbc_handle_t *)resource;
  status_code_t status = STATUS_OK;

  if (address < 0x2000)
  {
    /** 0x0000 - 0x1FFF: RAM enable (Write only) */
    mbc->ext_ram.enabled = ((data & 0x0F) == 0xA);
  }
  else if ((address >= 0x2000) && (address < 0x3000))
  {
    /** 0x2000 - 0x2FFF: ROM bank number LSB (Write only) */
    uint16_t rom_bank_num = (mbc->rom.active_bank_num & ~(0x00FF)) | (data & 0xFF);
    status = mbc_switch_rom_bank(mbc, (rom_bank_num & 0x1FF));
    RETURN_STATUS_IF_NOT_OK(status);
  }
  else if ((address >= 0x3000) && (address < 0x4000))
  {
    /** 0x3000 - 0x3FFF: ROM bank number 9th bit (Write only) */
    uint16_t rom_bank_num = (mbc->rom.active_bank_num & ~(0x100)) | ((data & 0x1) << 8);
    status = mbc_switch_rom_bank(mbc, (rom_bank_num & 0x1FF));
    RETURN_STATUS_IF_NOT_OK(status);
  }
  else if ((address >= 0x4000) && (address < 0x6000))
  {
    /** 0x4000 - 0x5FFF: RAM bank number (Write only) */
    status = mbc_switch_ext_ram_bank(mbc, data & 0xF, true);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return STATUS_OK;
}

static status_code_t mbc_ext_ram_read(void *const resource, uint16_t address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= 0x2000, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  *data = (mbc->ext_ram.active_bank && mbc->ext_ram.enabled) ? mbc->ext_ram.active_bank[address] : 0xFF;

  return STATUS_OK;
}

static status_code_t mbc_ext_ram_write(void *const resource, uint16_t address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= 0x2000, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  if (!(mbc->ext_ram.active_bank && mbc->ext_ram.enabled))
  {
    return STATUS_OK;
  }

  mbc->ext_ram.active_bank[address] = data;

  if (mbc->batt.present)
  {
    mbc->batt.has_unsaved_data = true;
  }

  return STATUS_OK;
}

static status_code_t mbc_ext_ram_rtc_read(void *const resource, uint16_t address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= 0x2000, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  if (mbc->flags & MBC_FLAGS_ACCESS_MODE_RTC)
  {
    return rtc_read(&mbc->rtc, data);
  }

  return mbc_ext_ram_read(resource, address, data);
}

static status_code_t mbc_ext_ram_rtc_write(void *const resource, uint16_t address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= 0x2000, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  if (mbc->flags & MBC_FLAGS_ACCESS_MODE_RTC)
  {
    return rtc_write(&mbc->rtc, data);
  }

  return mbc_ext_ram_write(resource, address, data);
}

static inline void mbc_set_flag(mbc_handle_t *const mbc, mbc_flags_t const flags)
{
  mbc->flags |= flags;
}

static inline void mbc_clear_flag(mbc_handle_t *const mbc, mbc_flags_t const flags)
{
  mbc->flags &= ~flags;
}
