#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/** Upper limit for the possible number of bytes in a buffer */
#define BUF_MAX_SIZE_BYTES (0xFFFF)

/** Short-hand macro to initialize a ring buffer */
#define INIT_RING_BUFFER(buffer, storage) (ring_buffer_init(&buffer, storage, sizeof(storage[0]), sizeof(storage)))

/**
 * Possible states of the buffer
 */
typedef enum
{
  BUFFER_EMPTY = 1,
  BUFFER_FULL = 2,
} buffer_status_t;

/**
 * Ring buffer handle to keep track of the state of the buffer.
 */
typedef struct
{
  size_t capacity;  /** Total capacity of the buffer in bytes */
  size_t read_ptr;  /** Pointer to the next element to be read off the buffer */
  size_t write_ptr; /** Pointer to the next empty space for the next element to be written to */
  size_t item_size; /** The size of each buffer element in bytes */
  uint8_t status;   /** Buffer state to indicate whether the buffer is full, empty, or neither */
  uint8_t *data;    /** Pointer to the underlying byte-buffer storage */
} ring_buffer_t;

/**
 * Initializes a ring buffer.
 *
 * This function only sets up the ring buffer and does not allocate memory for the buffer. Rather,
 * the caller of must allocate the buffer storage and provide it via the `storage` parameter.
 * The capacity provided must be greater than 0 and in the multiples of `item_size`.
 *
 * @param buf Pointer to a ring buffer handle to initialize
 * @param storage Pointer to a byte buffer to be used as the underlying storage
 * @param item_size The size of each element in bytes that will be written to the buffer
 * @param capacity_bytes The total number of bytes in the storage; must be multiples of `item_size`
 *
 * @return `true` upon successful initialization, `false` if an error is encountered.
 */
bool ring_buffer_init(ring_buffer_t *const buf, void *const storage, size_t const item_size, size_t const capacity_bytes);

/**
 * Write an element to the end of the buffer.
 * This advances the write pointer. This assumes the size of
 * the data to be the same as the buffer's `item_size`.
 *
 * @param buf Pointer to the buffer to write the element to
 * @param data Pointer to the data to be written to the buffer
 *
 * @return `true` if the element is successfully written to the buffer, `false` otherwise.
 */
bool ring_buffer_write(ring_buffer_t *const buf, void *const data);

/**
 * Read and remove an element from the front of the buffer.
 * This advances the read pointer. This assumes the size of
 * the data to be the same as the buffer's `item_size`.
 *
 * @param buf Pointer to the buffer to read an element from
 * @param data Pointer to store the data read from the buffer
 *
 * @return `true` if the element is successfully read from the buffer, `false` otherwise.
 */
bool ring_buffer_read(ring_buffer_t *const buf, void *const data);

/**
 * Look at the element at the front of the buffer without removing it from the buffer.
 * This does NOT advance the read pointer. This assumes the size of
 * the data to be the same as the buffer's `item_size`.
 *
 * @param buf Pointer to the buffer to peek an element from
 * @param data Pointer to store the data peeked from the buffer
 *
 * @return `true` if the element is successfully peeked from the buffer, `false` otherwise.
 */
bool ring_buffer_peek(ring_buffer_t *const buf, void *const data);

/**
 * Determine whether or not the provided buffer is full.
 *
 * @param buf Pointer to the buffer to determine if it's full
 *
 * @return `true` if the buffer is full, `false` otherwise or if an error is encountered.
 */
bool ring_buffer_full(ring_buffer_t *const buf);

/**
 * Determine whether or not the provided buffer is empty.
 *
 * @param buf Pointer to the buffer to determine if it's empty
 *
 * @return `true` if the buffer is empty, `false` otherwise or if an error is encountered.
 */
bool ring_buffer_empty(ring_buffer_t *const buf);

/**
 * Determine the number of elements in the provided buffer.
 * This returns the total number of elements in the buffer and not the
 * number of bytes currently used in the buffer.
 *
 * @param buf Pointer to the buffer to determine the size of
 *
 * @return The number of elements currently stored in the buffer.
 */
size_t ring_buffer_size(ring_buffer_t *const buf);

/**
 * Empty out the provided buffer.
 * This is done lazily by advancing the read pointer to the write pointer.
 *
 * @param buf Pointer to the buffer to empty
 *
 * @return `true` if the buffer has been successfully emptied, `false` otherwise.
 */
bool ring_buffer_flush(ring_buffer_t *const buf);

#endif /* __RING_BUFFER_H__ */
