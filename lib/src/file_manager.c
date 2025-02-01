#include "file_manager.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "logging.h"
#include "status_code.h"

#define DEFAULT_FILE_NAME_SIZE (512)

typedef struct
{
  char filename[512];
  uint8_t *data;
} cartridge_handle_t;

static status_code_t save_game(uint8_t *const game_data, const size_t size);
static status_code_t load_game(uint8_t *const game_data, const size_t size);

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

  struct stat st;
  stat(file, &st);
  size_t file_size = st.st_size;

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

static status_code_t save_game(uint8_t *const game_data, const size_t size)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(game_data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(size <= 0, STATUS_ERR_INVALID_ARG);

  char filename[512];

  snprintf(filename, sizeof(filename), "%s.gbsav", cartridge_handle.filename);

  FILE *fp = fopen(filename, "wb");

  if (fp == NULL)
  {
    Log_E("Failed to open save file.");
    return STATUS_ERR_GENERIC;
  }

  fwrite(game_data, 1, 0x2000, fp);
  fclose(fp);

  Log_I("Game state saved");
  return STATUS_OK;
}

static status_code_t load_game(uint8_t *const game_data, const size_t size)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(game_data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(size <= 0, STATUS_ERR_INVALID_ARG);

  char filename[512];

  snprintf(filename, sizeof(filename), "%s.gbsav", cartridge_handle.filename);

  FILE *fp = fopen(filename, "rb");

  if (fp == NULL)
  {
    Log_I("No saved game found");
    return STATUS_OK;
  }

  fread(game_data, 1, size, fp);
  fclose(fp);

  return STATUS_OK;
}
