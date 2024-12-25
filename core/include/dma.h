#ifndef __DMG_DMA_H__
#define __DMG_DMA_H__

#include <stdint.h>
#include "bus_interface.h"
#include "status_code.h"

typedef enum
{
  DMA_IDLE,
  DMA_PREPARING,
  DMA_XFER_ACTIVE,
} dma_state_t;

typedef struct
{
  dma_state_t state;
  uint16_t starting_addr;
  uint8_t current_offset;
  uint8_t prep_delay;
  bus_interface_t bus_interface;
} dma_handle_t;

status_code_t dma_init(dma_handle_t *const handle, bus_interface_t const bus_interface);
status_code_t dma_tick(dma_handle_t *const handle);
status_code_t dma_start(dma_handle_t *const handle, uint8_t const offset);

#endif /* __DMG_DMA_H__ */
