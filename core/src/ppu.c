#include "ppu.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "bus_interface.h"
#include "callback.h"
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

static inline status_code_t fps_sync(ppu_handle_t *const ppu);
static status_code_t lcd_set_mode(ppu_handle_t *const ppu, lcd_mode_t mode);
static status_code_t lyc_interrupt_check(ppu_handle_t *const ppu);
static status_code_t increment_ly(ppu_handle_t *const ppu);
static status_code_t reset_ly(ppu_handle_t *const ppu);

/** PPU FSM mode handler functions */
static status_code_t handle_mode_oam_scan(ppu_handle_t *const ppu);
static status_code_t handle_mode_xfer(ppu_handle_t *const ppu);
static status_code_t handle_mode_vblank(ppu_handle_t *const ppu);
static status_code_t handle_mode_hblank(ppu_handle_t *const ppu);

status_code_t ppu_init(ppu_handle_t *const ppu, ppu_init_param_t *const param)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(ppu);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(param);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(param->interrupt, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(param->bus_interface, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;

  pxfifo_init_param_t pxfifo_init_params = (pxfifo_init_param_t){
      .bus_interface = param->bus_interface,
      .lcd = &ppu->lcd,
  };

  ppu->interrupt = param->interrupt;
  ppu->current_frame = 0;
  ppu->line_ticks = 0;
  memset(ppu->video_buffer.buffer, 0, sizeof(ppu->video_buffer.buffer));

  status = oam_init(&ppu->oam);
  RETURN_STATUS_IF_NOT_OK(status);

  status = lcd_init(&ppu->lcd);
  RETURN_STATUS_IF_NOT_OK(status);

  status = pxfifo_init(&ppu->pxfifo, &pxfifo_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

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

status_code_t ppu_register_fps_sync_callback(ppu_handle_t *const ppu, callback_t *const fps_sync_callback)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(ppu);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(fps_sync_callback->callback_fn, STATUS_ERR_NOT_INITIALIZED);

  memcpy(&ppu->fps_sync_callback, fps_sync_callback, sizeof(callback_t));

  return STATUS_OK;
}

static status_code_t handle_mode_oam_scan(ppu_handle_t *const ppu)
{
  status_code_t status = STATUS_OK;

  if (ppu->line_ticks >= OAM_SCAN_DURATION_TICKS)
  {
    status = lcd_set_mode(ppu, MODE_XFER);
    RETURN_STATUS_IF_NOT_OK(status);

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

  if (pixel_out.data_valid)
  {
    ppu->video_buffer.matrix[pixel_out.screen_y][pixel_out.screen_x] = pixel_out.color.as_hex;
  }

  if (pixel_out.screen_x < (SCREEN_WIDTH - 1))
  {
    return STATUS_OK;
  }

  status = lcd_set_mode(ppu, MODE_HBLANK);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

static status_code_t handle_mode_vblank(ppu_handle_t *const ppu)
{
  status_code_t status = STATUS_OK;

  /** If LY == 153, it resets to 0 after 4 ticks */
  if ((ppu->lcd.registers.ly == 153) && (ppu->line_ticks == 4))
  {
    status = reset_ly(ppu);
  }

  if (ppu->line_ticks < TICKS_PER_LINE)
  {
    return STATUS_OK;
  }

  if (ppu->lcd.registers.ly == 0)
  {
    status = lcd_set_mode(ppu, MODE_OAM_SCAN);
    RETURN_STATUS_IF_NOT_OK(status);
  }
  else
  {
    status = increment_ly(ppu);
    RETURN_STATUS_IF_NOT_OK(status);
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
    status = lcd_set_mode(ppu, MODE_VBLANK);
    RETURN_STATUS_IF_NOT_OK(status);

    ppu->current_frame++;

    status = fps_sync(ppu);
    RETURN_STATUS_IF_NOT_OK(status);
  }
  else
  {
    status = lcd_set_mode(ppu, MODE_OAM_SCAN);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  ppu->line_ticks = 0;

  return STATUS_OK;
}

static status_code_t lcd_set_mode(ppu_handle_t *const ppu, lcd_mode_t mode)
{
  VERIFY_COND_RETURN_STATUS_IF_TRUE((ppu->lcd.registers.lcd_stat & LCD_STAT_PPU_MODE) == mode, STATUS_OK);

  ppu->lcd.registers.lcd_stat &= ~(LCD_STAT_PPU_MODE);
  ppu->lcd.registers.lcd_stat |= (mode & LCD_STAT_PPU_MODE);

  status_code_t status = STATUS_OK;
  uint8_t const lcd_stat = ppu->lcd.registers.lcd_stat;
  bool trigger_stat_interrupt = false;

  trigger_stat_interrupt |= !!((mode == MODE_OAM_SCAN) && (lcd_stat & LCD_STAT_OAM_SCAN_MODE_INT_SEL));
  trigger_stat_interrupt |= !!((mode == MODE_HBLANK) && (lcd_stat & LCD_STAT_HBLANK_MODE_INT_SEL));
  trigger_stat_interrupt |= !!((mode == MODE_VBLANK) && (lcd_stat & LCD_STAT_VBLANK_MODE_INT_SEL));

  if (trigger_stat_interrupt)
  {
    status = request_interrupt(ppu->interrupt, INT_LCD);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  if (mode == MODE_VBLANK)
  {
    status = request_interrupt(ppu->interrupt, INT_VBLANK);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return STATUS_OK;
}

static inline status_code_t fps_sync(ppu_handle_t *const ppu)
{
  if (ppu->fps_sync_callback.callback_fn)
  {
    return callback_call(&ppu->fps_sync_callback, NULL);
  }
  return STATUS_OK;
}

static status_code_t lyc_interrupt_check(ppu_handle_t *const ppu)
{
  status_code_t status = STATUS_OK;
  lcd_registers_t *const lcd_regs = &ppu->lcd.registers;

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

static status_code_t increment_ly(ppu_handle_t *const ppu)
{
  lcd_registers_t *const lcd_regs = &ppu->lcd.registers;

  if (lcd_window_visible(&ppu->lcd) && (lcd_regs->ly >= lcd_regs->window_y) && (lcd_regs->ly < lcd_regs->window_y + SCREEN_HEIGHT))
  {
    ppu->pxfifo.pixel_fetcher.window_line++;
  }

  lcd_regs->ly++;

  return lyc_interrupt_check(ppu);
}

static status_code_t reset_ly(ppu_handle_t *const ppu)
{
  ppu->lcd.registers.ly = 0;
  ppu->pxfifo.pixel_fetcher.window_line = 0;
  return lyc_interrupt_check(ppu);
}
