#include "ring_buf.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define BUF_SIZE(buf) ((buf->write_ptr - buf->read_ptr + buf->capacity) % buf->capacity)
#define VERIFY_COND(cond) \
  if (!(cond))            \
  {                       \
    return false;         \
  }

bool ring_buffer_init(ring_buffer_t *const buf, void *const storage, size_t const item_size, size_t const capacity_bytes)
{
  VERIFY_COND((buf != NULL) && (storage != NULL));
  VERIFY_COND(capacity_bytes > 0 && capacity_bytes < BUF_MAX_SIZE_BYTES);
  VERIFY_COND(item_size > 0);
  VERIFY_COND((capacity_bytes % item_size) == 0);

  buf->write_ptr = 0;
  buf->read_ptr = 0;
  buf->capacity = capacity_bytes;
  buf->item_size = item_size;
  buf->status = BUFFER_EMPTY;
  buf->data = storage;
  memset(buf->data, 0, buf->capacity);

  return true;
}

bool ring_buffer_write(ring_buffer_t *const buf, void *const data)
{
  VERIFY_COND((buf != NULL) && (data != NULL));
  VERIFY_COND(buf->status != BUFFER_FULL);

  memcpy(&buf->data[buf->write_ptr], data, buf->item_size);
  buf->write_ptr += buf->item_size;
  buf->write_ptr %= buf->capacity;
  buf->status = (buf->write_ptr == buf->read_ptr) ? BUFFER_FULL : 0;

  return true;
}

bool ring_buffer_read(ring_buffer_t *const buf, void *const data)
{
  VERIFY_COND((buf != NULL) && (data != NULL));
  VERIFY_COND(buf->status != BUFFER_EMPTY);

  memcpy(data, &buf->data[buf->read_ptr % buf->capacity], buf->item_size);
  buf->read_ptr += buf->item_size;
  buf->read_ptr %= buf->capacity;
  buf->status = (buf->write_ptr == buf->read_ptr) ? BUFFER_EMPTY : 0;

  return true;
}

bool ring_buffer_peek(ring_buffer_t *const buf, void *const data)
{
  VERIFY_COND((buf != NULL) && (data != NULL));
  VERIFY_COND(buf->status != BUFFER_EMPTY);

  memcpy(data, &buf->data[buf->read_ptr], buf->item_size);

  return true;
}

bool ring_buffer_full(ring_buffer_t *const buf)
{
  return (buf != NULL) && (buf->status == BUFFER_FULL);
}

bool ring_buffer_empty(ring_buffer_t *const buf)
{
  return (buf != NULL) && (buf->status == BUFFER_EMPTY);
}

size_t ring_buffer_size(ring_buffer_t *const buf)
{
  if (buf == NULL)
  {
    return 0;
  }
  else if (buf->status == BUFFER_FULL)
  {
    return buf->capacity / buf->item_size;
  }
  return (BUF_SIZE(buf) / buf->item_size);
}
