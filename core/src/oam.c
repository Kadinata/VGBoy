#include "oam.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bus_interface.h"
#include "status_code.h"

typedef struct
{
  oam_entry_t entry;
  uint8_t index;
} indexed_oam_entry_t;

static status_code_t oam_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t oam_write(void *const resource, uint16_t const address, uint8_t const data);
static inline bool scanline_intersects_sprite(oam_entry_t *oam_entry, uint8_t line_y, obj_size_t obj_size);
static int sort_compare(const void *value_1, const void *value_2);

/**
 * TODO: Disable OAM read/write by the CPU during DMA transfer
 * read returns 0xFF
 * write is a nop
 */

status_code_t oam_init(oam_handle_t *const oam_handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(oam_handle);

  memset(oam_handle->entries, 0, sizeof(oam_handle->entries));

  return bus_interface_init(&oam_handle->bus_interface, oam_read, oam_write, oam_handle);
}

status_code_t oam_scan(oam_handle_t *const oam_handle, uint8_t const line_y, obj_size_t const sprite_size, oam_scanned_sprites_t *const scan_results)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(oam_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(scan_results);
  VERIFY_COND_RETURN_STATUS_IF_TRUE((sprite_size != OBJ_SIZE_LARGE) && (sprite_size != OBJ_SIZE_SMALL), STATUS_ERR_INVALID_ARG);

  uint8_t index = 0;
  uint8_t entry_count = 0;
  indexed_oam_entry_t entries[10];

  memset(scan_results, 0, sizeof(oam_scanned_sprites_t));

  // Scan until the end of OAM is reached or 10 sprites have been collected
  while ((index < OAM_ENTRY_SIZE) && (entry_count < MAX_SPRITES_PER_LINE))
  {
    oam_entry_t *const oam_entry = &oam_handle->entries[index++];

    if (scanline_intersects_sprite(oam_entry, line_y, sprite_size))
    {
      entries[entry_count].entry = *oam_entry;
      entries[entry_count].index = entry_count;
      entry_count++;
    }
  }

  /**
   * Sort the sprites based on their X coordinate in ascending order,
   * and then based on their index in ascending order if the entries
   * have the same X coordinate.
   */
  qsort(entries, entry_count, sizeof(indexed_oam_entry_t), sort_compare);

  // Copy the sorted entries to the scan results
  for (uint8_t i = 0; i < entry_count; i++)
  {
    scan_results->sprite_attributes[scan_results->sprite_count++] = entries[i].entry;
  }

  return STATUS_OK;
}

static status_code_t oam_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  oam_handle_t *const oam_handle = (oam_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(oam_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= sizeof(oam_handle->entries), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  *data = oam_handle->oam_buf[address];

  return STATUS_OK;
}

static status_code_t oam_write(void *const resource, uint16_t const address, uint8_t const data)
{
  oam_handle_t *const oam_handle = (oam_handle_t *)resource;
  VERIFY_PTR_RETURN_ERROR_IF_NULL(oam_handle);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= sizeof(oam_handle->entries), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  oam_handle->oam_buf[address] = data;

  return STATUS_OK;
}

static inline bool scanline_intersects_sprite(oam_entry_t *oam_entry, uint8_t line_y, obj_size_t sprite_size)
{
  return (((oam_entry->y_pos <= (line_y + 16)) && (oam_entry->y_pos + sprite_size) > (line_y + 16)) && (oam_entry->x_pos != 0));
}

static int sort_compare(const void *value_1, const void *value_2)
{
  uint8_t const x_pos_1 = ((indexed_oam_entry_t *)value_1)->entry.x_pos;
  uint8_t const x_pos_2 = ((indexed_oam_entry_t *)value_2)->entry.x_pos;
  uint8_t const index_1 = ((indexed_oam_entry_t *)value_1)->index;
  uint8_t const index_2 = ((indexed_oam_entry_t *)value_2)->index;

  int const index_ordering = (index_1 > index_2) - (index_1 < index_2);
  int const x_pos_ordering = (x_pos_1 > x_pos_2) - (x_pos_1 < x_pos_2);

  return (x_pos_1 == x_pos_2) ? index_ordering : x_pos_ordering;
}
