#ifndef __TILE_DEBUG_WINDOW_H__
#define __TILE_DEBUG_WINDOW_H__

#include <stdint.h>

#include "bus_interface.h"
#include "status_code.h"

status_code_t tile_debug_window_init(bus_interface_t const data_bus_interface);
void tile_debug_window_update(void);
void tile_debug_window_cleanup(void);

#endif /* __TILE_DEBUG_WINDOW_H__ */
