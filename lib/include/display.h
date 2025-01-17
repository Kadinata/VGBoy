#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdint.h>
#include "status_code.h"
#include "ppu.h"
#include "bus_interface.h"

status_code_t display_init(bus_interface_t const data_bus_interface, ppu_handle_t *const ppu_handle);
status_code_t handle_events(void);
void update_display(void);
void display_cleanup(void);

#endif /* __DISPLAY_H__ */
