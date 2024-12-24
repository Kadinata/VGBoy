#ifndef __DMG_TIMING_SYNC_H__
#define __DMG_TIMING_SYNC_H__

#include <stdint.h>
#include "status_code.h"

typedef status_code_t (*sync_handle_callback_fn)(void *const ctx, uint8_t const m_cycle_count);

typedef struct
{
  sync_handle_callback_fn sync_callback;
  void *callback_ctx;
} timing_sync_handle_t;

status_code_t timing_sync_init(timing_sync_handle_t *const sync_handle, sync_handle_callback_fn const callback, void *const callback_ctx);
status_code_t timing_m_cycle_sync(timing_sync_handle_t *const sync_handle, uint8_t const m_cycle_count);

#endif /* __DMG_TIMING_SYNC_H__ */
