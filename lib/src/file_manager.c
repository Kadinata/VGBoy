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

typedef enum
{
  SAVED_GAME_SECTION_BATT_RAM = 0,
  SAVED_GAME_SECTION_RTC,
  SAVED_GAME_SECTION_MAX,
} saved_game_data_section_t;

typedef struct
{
  char filename[512];
  uint8_t *data;
} cartridge_handle_t;

typedef struct
{
  void *data_ptr;
  size_t data_size;
} file_data_section_t;

static status_code_t save_file(const char *filename, file_data_section_t *const sections, const size_t section_count, size_t *const bytes_written);
static status_code_t load_file(const char *filename, file_data_section_t *const sections, const size_t section_count, size_t *const bytes_read);
static status_code_t save_game(saved_game_data_t *const data);
static status_code_t load_game(saved_game_data_t *const data);

static inline size_t get_file_size(const char *filename);

static cartridge_handle_t cartridge_handle;

status_code_t setup_mbc_callbacks(mbc_handle_t *const mbc)
{
  mbc_callbacks_t callbacks = {
      .save_game = save_game,
      .load_game = load_game,
  };

  return mbc_register_callbacks(mbc, &callbacks);
}

status_code_t load_cartridge(mbc_handle_t *const mbc, const char *file)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(mbc);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(file);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(cartridge_handle.data != NULL, STATUS_ERR_ALREADY_INITIALIZED);

  status_code_t status = STATUS_OK;
  size_t file_size = get_file_size(file);
  size_t bytes_read = 0;

  Log_I("Loading ROM file: %s", file);
  Log_I("Allocating %zu bytes for ROM contents", file_size);

  cartridge_handle.data = calloc(1, file_size);

  if (cartridge_handle.data == NULL)
  {
    Log_E("Failed to allocate memory for ROM data");
    return STATUS_ERR_NO_MEMORY;
  }

  file_data_section_t section = {
      .data_ptr = cartridge_handle.data,
      .data_size = file_size,
  };

  status = load_file(file, &section, 1, &bytes_read);

  if (status != STATUS_OK)
  {
    Log_E("Failed to open file (%d)", status);
    return STATUS_ERR_FILE_NOT_FOUND;
  }

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
  return (stat(filename, &st) == 0) ? st.st_size : 0;
}

static status_code_t save_file(const char *filename, file_data_section_t *const sections, const size_t section_count, size_t *const bytes_written)
{
  FILE *fp = fopen(filename, "wb");
  VERIFY_COND_RETURN_STATUS_IF_TRUE(fp == NULL, STATUS_ERR_FILE_NOT_FOUND);

  size_t total_bytes = 0;

  for (size_t index = 0; index < section_count; index++)
  {
    total_bytes += fwrite(sections[index].data_ptr, 1, sections[index].data_size, fp);
  }

  fclose(fp);

  if (bytes_written)
  {
    *bytes_written = total_bytes;
  }

  return STATUS_OK;
}

static status_code_t load_file(const char *filename, file_data_section_t *const sections, const size_t section_count, size_t *const bytes_read)
{
  FILE *fp = fopen(filename, "rb");
  VERIFY_COND_RETURN_STATUS_IF_TRUE(fp == NULL, STATUS_ERR_FILE_NOT_FOUND);

  size_t total_bytes = 0;

  for (size_t index = 0; index < section_count; index++)
  {
    size_t read_size = fread(sections[index].data_ptr, 1, sections[index].data_size, fp);
    Log_I("Read data of size %zu", read_size);
    total_bytes += read_size;
  }

  fclose(fp);

  if (bytes_read)
  {
    *bytes_read = total_bytes;
  }

  return STATUS_OK;
}

static status_code_t save_game(saved_game_data_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(data->ram_data_size <= 0, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;
  char filename[530];

  file_data_section_t file_sections[SAVED_GAME_SECTION_MAX] = {
      [SAVED_GAME_SECTION_BATT_RAM] = {.data_ptr = data->ram_data, .data_size = data->ram_data_size},
      [SAVED_GAME_SECTION_RTC] = {.data_ptr = data->rtc, .data_size = sizeof(rtc_handle_t)},
  };

  snprintf(filename, sizeof(filename), "%s.gbsav", cartridge_handle.filename);

  status = save_file(filename, file_sections, SAVED_GAME_SECTION_MAX, NULL);
  if (status != STATUS_OK)
  {
    Log_E("Failed to open save file.");
    return status;
  }

  Log_I("Game state saved");
  return STATUS_OK;
}

static status_code_t load_game(saved_game_data_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(data->ram_data_size <= 0, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;
  char filename[530];

  file_data_section_t file_sections[SAVED_GAME_SECTION_MAX] = {
      [SAVED_GAME_SECTION_BATT_RAM] = {.data_ptr = data->ram_data, .data_size = data->ram_data_size},
      [SAVED_GAME_SECTION_RTC] = {.data_ptr = data->rtc, .data_size = sizeof(rtc_handle_t)},
  };

  snprintf(filename, sizeof(filename), "%s.gbsav", cartridge_handle.filename);

  status = load_file(filename, file_sections, SAVED_GAME_SECTION_MAX, NULL);
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
  uint8_t *const compressed_data = calloc(1, size);

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

  file_data_section_t section = {
      .data_ptr = compressed_data,
      .data_size = compressed_size,
  };

  snprintf(filename, sizeof(filename), "%s.%u.gbstate", cartridge_handle.filename, slot_num);
  status = save_file(filename, &section, 1, NULL);
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
  uint8_t *const compressed_data = calloc(1, file_size + 1);
  if (compressed_data == NULL)
  {
    Log_E("Failed to allocate memory for compressed data while saving snapshot to slot %d", slot_num);
    return STATUS_ERR_NO_MEMORY;
  }

  file_data_section_t section = {
      .data_ptr = compressed_data,
      .data_size = file_size,
  };

  status = load_file(filename, &section, 1, NULL);
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
