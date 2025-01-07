#include "ppu.h"

#include <stdint.h>
#include <string.h>

#include "oam.h"
#include "lcd.h"
#include "interrupt.h"
#include "pixel_fifo.h"
#include "pixel_fetcher.h"
#include "logging.h"
#include "status_code.h"

#define OAM_SCAN_DURATION_TICKS (80)
#define LINES_PER_FRAME (154)
#define TICKS_PER_LINE (456)

static inline void load_video_buffer(ppu_handle_t *const ppu, pixel_data_t *const pixel_data)
{
  uint16_t index = (pixel_data->screen_x + (pixel_data->screen_y * SCREEN_WIDTH));

  if (pixel_data->data_valid && index <= (SCREEN_WIDTH * SCREEN_HEIGHT))
  {
    ppu->video_buffer[index] = pixel_data->color.as_hex;
  }
}

static inline status_code_t fps_sync(ppu_handle_t *const ppu)
{
  if (ppu->fps_sync_handler.callback)
  {
    return ppu->fps_sync_handler.callback(ppu->fps_sync_handler.ctx);
  }
  return STATUS_OK;
}

static inline void lcd_set_mode(lcd_handle_t *const lcd_handle, lcd_mode_t mode)
{
  lcd_handle->registers.lcd_stat &= ~LCD_STAT_PPU_MODE;
  lcd_handle->registers.lcd_stat |= (mode & LCD_STAT_PPU_MODE);
}

static status_code_t increment_ly(ppu_handle_t *const ppu)
{
  status_code_t status = STATUS_OK;
  lcd_registers_t *const lcd_regs = &ppu->lcd.registers;

  if (lcd_window_visible(&ppu->lcd) && (lcd_regs->ly >= lcd_regs->window_y) && (lcd_regs->ly < lcd_regs->window_y + SCREEN_HEIGHT))
  {
    ppu->pxfifo.pixel_fetcher.window_line++;
  }

  lcd_regs->ly++;

  if (lcd_regs->ly != lcd_regs->ly_comp)
  {
    lcd_regs->lcd_stat &= ~LCD_STAT_LY_EQ_LYC;
    return STATUS_OK;
  }

  lcd_regs->lcd_stat |= LCD_STAT_LY_EQ_LYC;

  if (lcd_regs->lcd_stat & LCD_STAT_LYC_INT_SEL)
  {
    status = request_interrupt(ppu->interrupt, INT_LCD);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return STATUS_OK;
}

static status_code_t handle_mode_oam_scan(ppu_handle_t *const ppu)
{
  status_code_t status = STATUS_OK;

  if (ppu->line_ticks >= 80)
  {
    lcd_set_mode(&ppu->lcd, MODE_XFER);
    status = pxfifo_reset(&ppu->pxfifo);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  if (ppu->line_ticks == 1)
  {
    status = oam_scan(
        &ppu->oam,
        ppu->lcd.registers.ly,
        lcd_ctrl_obj_size(&ppu->lcd),
        &(ppu->pxfifo.pixel_fetcher.oam_scanned_sprites)); /** TODO: simplify */
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return STATUS_OK;
}

static status_code_t handle_mode_xfer(ppu_handle_t *const ppu)
{
  status_code_t status = STATUS_OK;
  pixel_data_t pixel_out = {0};

  status = pxfifo_shift_pixel(&ppu->pxfifo, &pixel_out);
  RETURN_STATUS_IF_NOT_OK(status);

  load_video_buffer(ppu, &pixel_out);

  if (pixel_out.screen_x < (SCREEN_WIDTH - 1))
  {
    return STATUS_OK;
  }

  lcd_set_mode(&ppu->lcd, MODE_HBLANK);

  if (ppu->lcd.registers.lcd_stat & LCD_STAT_HBLANK_MODE_INT_SEL)
  {
    status = request_interrupt(ppu->interrupt, INT_LCD);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return STATUS_OK;
}

static status_code_t handle_mode_vblank(ppu_handle_t *const ppu)
{
  status_code_t status = STATUS_OK;

  if (ppu->line_ticks < TICKS_PER_LINE)
  {
    return STATUS_OK;
  }

  status = increment_ly(ppu);
  RETURN_STATUS_IF_NOT_OK(status);

  if (ppu->lcd.registers.ly >= LINES_PER_FRAME)
  {
    lcd_set_mode(&ppu->lcd, MODE_OAM_SCAN);
    ppu->lcd.registers.ly = 0;
    ppu->pxfifo.pixel_fetcher.window_line = 0;
  }

  ppu->line_ticks = 0;

  return STATUS_OK;
}

static status_code_t handle_mode_hblank(ppu_handle_t *const ppu)
{
  status_code_t status = STATUS_OK;

  if (ppu->line_ticks < TICKS_PER_LINE)
  {
    return STATUS_OK;
  }

  status = increment_ly(ppu);
  RETURN_STATUS_IF_NOT_OK(status);

  if (ppu->lcd.registers.ly >= SCREEN_HEIGHT)
  {
    lcd_set_mode(&ppu->lcd, MODE_VBLANK);

    status = request_interrupt(ppu->interrupt, INT_VBLANK);
    RETURN_STATUS_IF_NOT_OK(status);

    if (ppu->lcd.registers.lcd_stat & LCD_STAT_VBLANK_MODE_INT_SEL)
    {
      status = request_interrupt(ppu->interrupt, INT_LCD);
      RETURN_STATUS_IF_NOT_OK(status);
    }

    ppu->current_frame++;

    status = fps_sync(ppu);
    RETURN_STATUS_IF_NOT_OK(status);
  }
  else
  {
    lcd_set_mode(&ppu->lcd, MODE_OAM_SCAN);
  }

  ppu->line_ticks = 0;

  return STATUS_OK;
}

status_code_t ppu_init(ppu_handle_t *const ppu, ppu_init_param_t *const param)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(ppu);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(param);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(param->interrupt, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(param->bus_interface, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;

  pxfifo_init_param_t pxfifo_init_params = (pxfifo_init_param_t){
      .bus_interface = param->bus_interface,
      .lcd_handle = &ppu->lcd,
  };

  // ppu_handle->lcd_handle = param->lcd_handle;
  ppu->interrupt = param->interrupt;
  // ppu_handle->oam_handle = param->oam_handle;
  ppu->current_frame = 0;
  ppu->line_ticks = 0;
  memset(ppu->video_buffer, 0, SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint32_t));

  status = oam_init(&ppu->oam);
  RETURN_STATUS_IF_NOT_OK(status);

  status = lcd_init(&ppu->lcd);
  RETURN_STATUS_IF_NOT_OK(status);

  status = pxfifo_init(&ppu->pxfifo, &pxfifo_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  lcd_set_mode(&ppu->lcd, MODE_OAM_SCAN);

  return STATUS_OK;
}

status_code_t ppu_tick(ppu_handle_t *const ppu)
{
  status_code_t status = STATUS_OK;

  ppu->line_ticks++;

  switch (ppu->lcd.registers.lcd_stat & LCD_STAT_PPU_MODE)
  {
  case MODE_HBLANK:
    status = handle_mode_hblank(ppu);
    break;
  case MODE_VBLANK:
    status = handle_mode_vblank(ppu);
    break;
  case MODE_OAM_SCAN:
    status = handle_mode_oam_scan(ppu);
    break;
  case MODE_XFER:
    status = handle_mode_xfer(ppu);
    break;
  default:
    break;
  }

  return status;
}

status_code_t ppu_register_fps_sync_handler(ppu_handle_t *const ppu, fps_sync_handler_t fps_sync_handler)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(ppu);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(fps_sync_handler.callback, STATUS_ERR_INVALID_ARG);

  memcpy(&ppu->fps_sync_handler, &fps_sync_handler, sizeof(fps_sync_handler_t));

  return STATUS_OK;
}
