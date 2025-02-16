#ifndef __MBC_TEST_HELPER_H__
#define __MBC_TEST_HELPER_H__

#include "mbc.h"
#include "rom.h"

void *create_rom(cartridge_type_t const cartridge_type, uint8_t const rom_size, uint8_t const ram_size);
void stub_write_then_read_address_range(mbc_handle_t *const mbc, uint16_t const start_address, uint16_t const range, uint8_t const write_data, uint8_t const expected_data);
void stub_read_address_range(mbc_handle_t *const mbc, uint16_t const start_address, uint16_t const range, uint8_t const expected_data);
void stub_test_ram_returns_error_when_reading_outside_address_range(mbc_handle_t *const mbc);
void stub_test_ram_returns_error_when_writing_outside_address_range(mbc_handle_t *const mbc);

#endif