#ifndef __MBC_TEST_HELPER_H__
#define __MBC_TEST_HELPER_H__

#include "rom.h"

void *create_rom(cartridge_type_t const cartridge_type, uint8_t const rom_size, uint8_t const ram_size);

#endif