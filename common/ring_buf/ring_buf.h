#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUF_MAX_SIZE_BYTES (0xFFFF)
#define INIT_RING_BUFFER(buffer, storage) (ring_buffer_init(&buffer, storage, sizeof(storage[0]), sizeof(storage)))

typedef enum
{
  BUFFER_EMPTY = 1,
  BUFFER_FULL = 2,
} buffer_status_t;

typedef struct
{
  size_t capacity;
  size_t read_ptr;
  size_t write_ptr;
  size_t item_size;
  uint8_t status;
  void *data;
} ring_buffer_t;

bool ring_buffer_init(ring_buffer_t *const buf, void *const storage, size_t const item_size, size_t const capacity_bytes);

bool ring_buffer_write(ring_buffer_t *const buf, void *const data);
bool ring_buffer_read(ring_buffer_t *const buf, void *const data);
bool ring_buffer_peek(ring_buffer_t *const buf, void *const data);

bool ring_buffer_full(ring_buffer_t *const buf);
bool ring_buffer_empty(ring_buffer_t *const buf);
size_t ring_buffer_size(ring_buffer_t *const buf);

#endif /* __RING_BUFFER_H__ */