#include "mbc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "bus_interface.h"
#include "rom.h"
#include "status_code.h"

static status_code_t mbc_init_ext_ram(mbc_handle_t *const mbc, rom_header_t *const rom_header);
static status_code_t mbc_read(void *const resource, uint16_t address, uint8_t *const data);
static status_code_t mbc_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_ext_ram_read(void *const resource, uint16_t address, uint8_t *const data);
static status_code_t mbc_ext_ram_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_no_banking_read(void *const resource, uint16_t address, uint8_t *const data);
static status_code_t mbc_no_banking_write(void *const resource, uint16_t address, uint8_t const data);
static status_code_t mbc_1_read(void *const resource, uint16_t address, uint8_t *const data);
static status_code_t mbc_1_write(void *const resource, uint16_t address, uint8_t const data);

/** TODO: Implement battery + saving */

status_code_t mbc_init(mbc_handle_t *const mbc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);

  status_code_t status = STATUS_OK;

  status = bus_interface_init(&mbc->bus_interface, mbc_read, mbc_write, mbc);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

status_code_t mbc_load_rom(mbc_handle_t *const mbc, const char *file)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(file);

  status_code_t status = STATUS_OK;

  status = rom_load(&mbc->rom.content, file);
  RETURN_STATUS_IF_NOT_OK(status);

  status = mbc_init_ext_ram(mbc, mbc->rom.content.header);
  RETURN_STATUS_IF_NOT_OK(status);

  switch (mbc->rom.content.header->cartridge_type)
  {
  case ROM_ONLY:
  case ROM_RAM:
  case ROM_RAM_BATT:
    status = bus_interface_init(&mbc->rom.bus_interface, mbc_no_banking_read, mbc_no_banking_write, mbc);
    break;
  case ROM_MBC1:
  case ROM_MBC1_RAM:
  case ROM_MBC1_RAM_BATT:
    status = bus_interface_init(&mbc->rom.bus_interface, mbc_1_read, mbc_1_write, mbc);
    break;
  default:
    Log_E("Unsupported cartridge type: 0x%02X", mbc->rom.content.header->cartridge_type);
    return STATUS_ERR_UNSUPPORTED;
    break;
  }

  RETURN_STATUS_IF_NOT_OK(status);
  return STATUS_OK;
}

status_code_t mbc_cleanup(mbc_handle_t *const mbc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);

  for (uint8_t i = 0; i < MAX_RAM_BANKS; i++)
  {
    if (mbc->ext_ram.banks[i])
    {
      free(mbc->ext_ram.banks[i]);
      mbc->ext_ram.banks[i] = NULL;
    }
  }

  return rom_unload(&mbc->rom.content);
}

static status_code_t mbc_init_ext_ram(mbc_handle_t *const mbc, rom_header_t *const rom_header)
{
  mbc->ext_ram.enabled = 0;
  mbc->ext_ram.num_banks = 0;
  memset(mbc->ext_ram.banks, 0, sizeof(mbc->ext_ram.banks));

  switch (rom_header->ram_size)
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

  for (uint8_t i = 0; i < mbc->ext_ram.num_banks; i++)
  {
    mbc->ext_ram.banks[i] = malloc(0x2000);
    memset(mbc->ext_ram.banks[i], 0, 0x2000);
  }

  mbc->ext_ram.active_bank = mbc->ext_ram.banks[0];
  mbc->ext_ram.bus_interface.offset = 0xA000;

  return bus_interface_init(&mbc->ext_ram.bus_interface, mbc_ext_ram_read, mbc_ext_ram_write, &mbc->ext_ram);
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

  return STATUS_OK;
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

  return STATUS_OK;
}

static status_code_t mbc_no_banking_read(void *const resource, uint16_t address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  *data = mbc->rom.content.data[address];
  return STATUS_OK;
}

static status_code_t mbc_no_banking_write(void *const __attribute__((unused)) resource, uint16_t __attribute__((unused)) address, uint8_t const __attribute__((unused)) data)
{
  return STATUS_OK;
}

static status_code_t mbc_1_read(void *const resource, uint16_t address, uint8_t *const data)
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

static status_code_t mbc_1_write(void *const resource, uint16_t address, uint8_t const data)
{
  mbc_handle_t *const mbc = (mbc_handle_t *)resource;

  if (address < 0x2000)
  {
    /** 0x0000 - 0x1FFF: RAM enable (Write only) */
    mbc->ext_ram.enabled = ((data & 0x0F) == 0xA);
  }
  else if ((address >= 0x2000) && (address < 0x4000))
  {
    /** 0x2000 - 0x3FFF: ROM bank number (Write only) */
    mbc->rom.active_bank_num = data ? data : 1;
    mbc->rom.active_bank_num &= 0x1F;
    mbc->rom.active_switchable_bank = &(mbc->rom.content.data[0x4000 * mbc->rom.active_bank_num]);
  }
  else if ((address >= 0x4000) && (address < 0x6000) && (mbc->banking_mode == BANK_MODE_ADVANCED))
  {
    /** 0x4000 - 0x5FFF: RAM bank number or upper bits of ROM bank number (Write only) */
    if (mbc->ext_ram.num_banks == MBC_EXT_RAM_SIZE_32K)
    {
      mbc->ext_ram.active_bank = mbc->ext_ram.banks[(data & 0x3)];
    }
    else if (mbc->rom.content.header->rom_size > 0x4)
    {
      // mbc
      /** TODO: Implement */
    }
  }
  else if ((address >= 0x6000) && (address < 0x8000))
  {
    /** 0x6000 - 0x7FFF: Banking mode select (Write only) */
    mbc->banking_mode = (data & 0x1) ? BANK_MODE_ADVANCED : BANK_MODE_SIMPLE;
    if (mbc->banking_mode == BANK_MODE_ADVANCED)
    {
      if (mbc->ext_ram.num_banks == MBC_EXT_RAM_SIZE_32K)
      {
        mbc->ext_ram.active_bank = mbc->ext_ram.banks[(data & 0x3)];
      }
    }
  }

  return STATUS_OK;
}

static status_code_t mbc_ext_ram_read(void *const resource, uint16_t address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= 0x2000, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  mbc_ext_ram_t *const ext_ram = (mbc_ext_ram_t *)resource;

  *data = (ext_ram->active_bank && ext_ram->enabled) ? ext_ram->active_bank[address] : 0xFF;

  return STATUS_OK;
}

static status_code_t mbc_ext_ram_write(void *const resource, uint16_t address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= 0x2000, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  mbc_ext_ram_t *const ext_ram = (mbc_ext_ram_t *)resource;

  if (ext_ram->active_bank && ext_ram->enabled)
  {
    ext_ram->active_bank[address] = data;
  }

  return STATUS_OK;
}
