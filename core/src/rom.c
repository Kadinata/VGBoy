#include "rom.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "logging.h"
#include "status_code.h"

#define ROM_HEADER_ADDR (0x100)
#define ROM_SIZE_KB(rom_size) (32 * (1 << rom_size))

static status_code_t verify_header_checksum(rom_handle_t *const handle);

status_code_t rom_load(rom_handle_t *const handle, const char *file)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(file);

  if (handle->data != NULL)
  {
    return STATUS_ERR_ALREADY_INITIALIZED;
  }

  Log_I("Loading ROM file: %s", file);

  FILE *fp = fopen(file, "rb");

  if (fp == NULL)
  {
    Log_E("Failed to open file");
    return STATUS_ERR_FILE_NOT_FOUND;
  }

  struct stat st;
  stat(file, &st);
  size_t file_size = st.st_size;

  Log_I("ROM file loaded successfully. Allocating %zu bytes for ROM contents", file_size);

  handle->data = malloc(file_size);

  if (handle->data == NULL)
  {
    Log_E("Failed to allocate memory for ROM data");
    return STATUS_ERR_NO_MEMORY;
  }

  size_t bytes_read = fread(handle->data, 1, file_size, fp);

  fclose(fp);

  Log_I("Bytes read: %zu; bytes allocated: %zu", bytes_read, file_size);

  handle->header = (rom_header_t *)&handle->data[ROM_HEADER_ADDR];

  Log_I("ROM Metadata:");
  Log_I("- Title:        %s", handle->header->title);
  Log_I("- Type:         0x%02X", handle->header->cartridge_type);
  Log_I("- ROM size:     %u KiB", ROM_SIZE_KB(handle->header->rom_size));
  Log_I("- RAM size:     %u", handle->header->ram_size);
  Log_I("- License code: 0x%02X", handle->header->new_license_code);
  Log_I("- ROM version:  0x%02X", handle->header->mask_rom_version);

  return verify_header_checksum(handle);
}

status_code_t rom_unload(rom_handle_t *const handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);

  if (handle->data == NULL)
  {
    return STATUS_ERR_ALREADY_FREED;
  }

  free(handle->data);

  handle->data = NULL;
  handle->header = NULL;

  Log_I("ROM unloaded successfully");
  return STATUS_OK;
}

static status_code_t verify_header_checksum(rom_handle_t *const handle)
{
  uint8_t checksum = 0;

  for (uint16_t address = 0x134; address <= 0x14C; address++)
  {
    checksum = checksum - handle->data[address] - 1;
  }

  Log_I("ROM header checksum: 0x%02X", handle->header->header_checksum);

  if (checksum != handle->header->header_checksum)
  {
    Log_E("ROM header checksum check failed (0x%02X)", checksum);
    return STATUS_ERR_CHECKSUM_FAILURE;
  }

  Log_I("ROM header checksum check passed");
  return STATUS_OK;
}

status_code_t rom_read(rom_handle_t *const handle, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  if (handle->header == NULL || handle->data == NULL)
  {
    return STATUS_ERR_NOT_INITIALIZED;
  }

  *data = handle->data[address]; // TODO: do address verification
  return STATUS_OK;
}

status_code_t rom_write(rom_handle_t *const handle, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);

  if (handle->header == NULL || handle->data == NULL)
  {
    return STATUS_ERR_NOT_INITIALIZED;
  }

  handle->data[address] = data; // TODO: do address verification
  return STATUS_OK;
}
