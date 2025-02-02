#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include <stdint.h>

#include "mbc.h"
#include "status_code.h"

status_code_t setup_mbc_callbacks(mbc_handle_t *const mbc);
status_code_t load_cartridge(mbc_handle_t *const mbc, const char *file);
status_code_t unload_cartridge(mbc_handle_t *const mbc);
status_code_t save_snapshot_file(void *const data, size_t const size, uint8_t const slot_num);
status_code_t load_snapshot_file(void *const data, size_t const size, uint8_t const slot_num);

#endif /* __FILE_MANAGER_H__ */
