#include "file_manager.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

#include "logging.h"
#include "status_code.h"

#define DEFAULT_FILE_NAME_SIZE (512)

typedef struct
{
  char filename[512];
  uint8_t *data;
} cartridge_handle_t;

static status_code_t save_file(const char *filename, void *const data, const size_t size);
static status_code_t load_file(const char *filename, void *const data, const size_t size);
static status_code_t save_game(uint8_t *const game_data, const size_t size);
static status_code_t load_game(uint8_t *const game_data, const size_t size);

static inline size_t get_file_size(const char *filename);

static cartridge_handle_t cartridge_handle;

status_code_t setup_mbc_callbacks(mbc_handle_t *const mbc)
{
  return mbc_register_callbacks(mbc, save_game, load_game);
}

status_code_t load_cartridge(mbc_handle_t *const mbc, const char *file)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(file);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(cartridge_handle.data != NULL, STATUS_ERR_ALREADY_INITIALIZED);

  Log_I("Loading ROM file: %s", file);

  size_t file_size = get_file_size(file);

  FILE *fp = fopen(file, "rb");

  if (fp == NULL)
  {
    Log_E("Failed to open file");
    return STATUS_ERR_FILE_NOT_FOUND;
  }

  Log_I("ROM file loaded successfully. Allocating %zu bytes for ROM contents", file_size);

  cartridge_handle.data = malloc(file_size);

  if (cartridge_handle.data == NULL)
  {
    Log_E("Failed to allocate memory for ROM data");
    fclose(fp);
    return STATUS_ERR_NO_MEMORY;
  }

  size_t bytes_read = fread(cartridge_handle.data, 1, file_size, fp);
  fclose(fp);

  if (bytes_read != file_size)
  {
    Log_E("Bytes read (%zu) doesn't match file size (%zu)!", bytes_read, file_size);
    return STATUS_ERR_GENERIC;
  }

  snprintf(cartridge_handle.filename, sizeof(cartridge_handle.filename), "%s", file);

  Log_I("Bytes read: %zu; bytes allocated: %zu", bytes_read, file_size);

  return mbc_load_rom(mbc, cartridge_handle.data, file_size);
}

status_code_t unload_cartridge(mbc_handle_t *const mbc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);

  if (cartridge_handle.data)
  {
    free(cartridge_handle.data);
    cartridge_handle.data = NULL;
  }

  return mbc_cleanup(mbc);
}

static inline size_t get_file_size(const char *filename)
{
  struct stat st;
  stat(filename, &st);
  return st.st_size;
}

static status_code_t save_file(const char *filename, void *const data, const size_t size)
{
  FILE *fp = fopen(filename, "wb");
  VERIFY_COND_RETURN_STATUS_IF_TRUE(fp == NULL, STATUS_ERR_FILE_NOT_FOUND);

  fwrite(data, 1, size, fp);
  fclose(fp);

  return STATUS_OK;
}

static status_code_t load_file(const char *filename, void *const data, const size_t size)
{
  FILE *fp = fopen(filename, "rb");
  VERIFY_COND_RETURN_STATUS_IF_TRUE(fp == NULL, STATUS_ERR_FILE_NOT_FOUND);

  fread(data, 1, size, fp);
  fclose(fp);

  return STATUS_OK;
}

static status_code_t save_game(uint8_t *const game_data, const size_t size)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(game_data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(size <= 0, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;
  char filename[530];

  snprintf(filename, sizeof(filename), "%s.gbsav", cartridge_handle.filename);

  status = save_file(filename, game_data, size);
  if (status != STATUS_OK)
  {
    Log_E("Failed to open save file.");
    return status;
  }

  Log_I("Game state saved");
  return STATUS_OK;
}

static status_code_t load_game(uint8_t *const game_data, const size_t size)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(game_data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(size <= 0, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;
  char filename[530];

  snprintf(filename, sizeof(filename), "%s.gbsav", cartridge_handle.filename);

  status = load_file(filename, game_data, size);
  if (status == STATUS_ERR_FILE_NOT_FOUND)
  {
    Log_I("No saved game found");
    return STATUS_OK;
  }

  return status;
}

status_code_t save_snapshot_file(void *const data, size_t const size, uint8_t const slot_num)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(size <= 0, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;
  char filename[530];

  /** Compress data */
  size_t compressed_size = compressBound(size);
  uint8_t *const compressed_data = malloc(size);

  if (compressed_data == NULL)
  {
    Log_E("Failed to allocate memory for compressed data while saving snapshot to slot %d", slot_num);
    return STATUS_ERR_NO_MEMORY;
  }

  int z_status = compress(compressed_data, &compressed_size, data, size);
  if (z_status != Z_OK)
  {
    Log_E("An error occurred while compressing snapshot data for slot %d (%d)", slot_num, z_status);
    free(compressed_data);
    return STATUS_ERR_GENERIC;
  }

  snprintf(filename, sizeof(filename), "%s.%u.gbstate", cartridge_handle.filename, slot_num);
  status = save_file(filename, compressed_data, compressed_size);
  free(compressed_data);

  if (status != STATUS_OK)
  {
    Log_E("Failed to open state snapshot file for slot #%u.", slot_num);
    return STATUS_ERR_GENERIC;
  }

  Log_I("Snapshot state saved to slot #%u", slot_num);
  return STATUS_OK;
}

status_code_t load_snapshot_file(void *const data, size_t const size, uint8_t const slot_num)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(size <= 0, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;
  char filename[530];

  snprintf(filename, sizeof(filename), "%s.%u.gbstate", cartridge_handle.filename, slot_num);

  size_t file_size = get_file_size(filename);

  if (file_size == 0)
  {
    Log_I("No state snapshot file found for slot# %u.", slot_num);
    return STATUS_ERR_FILE_NOT_FOUND;
  }

  size_t uncompressed_size = size;
  uint8_t *const compressed_data = malloc(file_size + 1);
  if (compressed_data == NULL)
  {
    Log_E("Failed to allocate memory for compressed data while saving snapshot to slot %d", slot_num);
    return STATUS_ERR_NO_MEMORY;
  }

  status = load_file(filename, compressed_data, file_size);
  if (status != STATUS_OK)
  {
    Log_I("An error occurred while reading snapshot file for slot# %u (%d).", slot_num, status);
    free(compressed_data);
    return status;
  }

  int z_status = uncompress(data, &uncompressed_size, compressed_data, file_size);
  free(compressed_data);

  if (z_status != Z_OK)
  {
    Log_E("An error occurred while uncompressing snapshot data from slot %d (%d)", slot_num, z_status);
    return STATUS_ERR_GENERIC;
  }
  if (uncompressed_size != size)
  {
    Log_E("Uncompressed data size doesn't match the expected size (%zu vs %zu)", uncompressed_size, size);
    return STATUS_ERR_GENERIC;
  }

  Log_I("Snapshot state loaded from slot #%u", slot_num);
  return STATUS_OK;
}
