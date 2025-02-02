#include "save_state.h"

#include <stdint.h>
#include <string.h>

#include "emulator.h"
#include "logging.h"
#include "status_code.h"

typedef struct
{
  apu_lfsr_state_t state;
  apu_lfsr_registers_t registers;
} apu_lfsr_saved_state_t;

typedef struct
{
  apu_pwm_state_t state;
  apu_pwm_registers_t registers;
} apu_pwm_saved_state_t;

typedef struct
{
  apu_wave_state_t state;
  apu_wave_registers_t registers;
  uint8_t wave_ram_data[16];
} apu_wave_saved_state_t;

typedef struct
{
  apu_pwm_saved_state_t ch1;
  apu_pwm_saved_state_t ch2;
  apu_wave_saved_state_t ch3;
  apu_lfsr_saved_state_t ch4;
  apu_registers_t registers;
  apu_frame_sequencer_counter_t frame_sequencer;
} apu_saved_state_t;

typedef struct
{
  registers_t registers;
  uint32_t m_cycles;
  cpu_run_mode_t run_mode;
  uint8_t next_ime_flag;
  uint8_t current_inst_m_cycle_count;
  interrupt_registers_t int_regs;
} cpu_saved_state_t;

typedef struct
{
  dma_state_t state;
  uint16_t starting_addr;
  uint8_t current_offset;
  uint8_t prep_delay;
} dma_saved_state_t;

typedef struct
{
  uint16_t rom_active_bank_num;
  uint8_t ram_active_bank_num;
  banking_mode_t banking_mode;
} mbc_saved_state_t;

typedef struct
{
  pxfifo_state_t fifo_state;
  pxfifo_buffer_t bg_fifo;
  pxfifo_counter_t counters;
  pixel_fetcher_state_t pixel_fetcher;
} pxfifo_saved_state_t;

typedef struct
{
  lcd_registers_t lcd;
  oam_entry_t oam_entries[OAM_ENTRY_SIZE];
  pxfifo_saved_state_t pxfifo;
  uint32_t current_frame;
  uint32_t line_ticks;
  video_buffer_t video_buffer;
} ppu_saved_state_t;

typedef struct
{
  wram_t wram;
  vram_t vram;
  hram_t hram;
} ram_saved_state_t;

typedef struct
{
  apu_saved_state_t apu;
  cpu_saved_state_t cpu;
  dma_saved_state_t dma;
  mbc_saved_state_t mbc;
  ppu_saved_state_t ppu;
  ram_saved_state_t ram;
  timer_registers_t tmr;
} emulator_saved_state_t;

static emulator_saved_state_t saved_state = {0};
// static emulator_t saved_state;

status_code_t save_game_state(emulator_t *const emulator, const uint8_t slot_num)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(slot_num > 9, STATUS_ERR_INVALID_ARG);

  /** Save APU states */
  memcpy(&saved_state.apu.ch1.registers, &emulator->apu.ch1.registers, sizeof(apu_pwm_registers_t));
  memcpy(&saved_state.apu.ch1.state, &emulator->apu.ch1.state, sizeof(apu_pwm_state_t));
  memcpy(&saved_state.apu.ch2.registers, &emulator->apu.ch2.registers, sizeof(apu_pwm_registers_t));
  memcpy(&saved_state.apu.ch2.state, &emulator->apu.ch2.state, sizeof(apu_pwm_state_t));
  memcpy(&saved_state.apu.ch3.registers, &emulator->apu.ch3.registers, sizeof(apu_wave_registers_t));
  memcpy(&saved_state.apu.ch3.state, &emulator->apu.ch3.state, sizeof(apu_wave_state_t));
  memcpy(saved_state.apu.ch3.wave_ram_data, emulator->apu.ch3.wave_ram.data, sizeof(emulator->apu.ch3.wave_ram.data));
  memcpy(&saved_state.apu.ch4.registers, &emulator->apu.ch4.registers, sizeof(apu_lfsr_registers_t));
  memcpy(&saved_state.apu.ch4.state, &emulator->apu.ch4.state, sizeof(apu_lfsr_state_t));
  memcpy(&saved_state.apu.registers, &emulator->apu.registers, sizeof(apu_registers_t));
  memcpy(&saved_state.apu.frame_sequencer, &emulator->apu.frame_sequencer, sizeof(apu_frame_sequencer_counter_t));

  /** Save CPU states */
  memcpy(&saved_state.cpu.registers, &emulator->cpu_state.registers, sizeof(registers_t));
  memcpy(&saved_state.cpu.int_regs, &emulator->cpu_state.interrupt.registers, sizeof(interrupt_registers_t));
  saved_state.cpu.m_cycles = emulator->cpu_state.m_cycles;
  saved_state.cpu.run_mode = emulator->cpu_state.run_mode;
  saved_state.cpu.next_ime_flag = emulator->cpu_state.next_ime_flag;
  saved_state.cpu.current_inst_m_cycle_count = emulator->cpu_state.current_inst_m_cycle_count;

  /** Save DMA states */
  saved_state.dma.state = emulator->dma.state;
  saved_state.dma.starting_addr = emulator->dma.starting_addr;
  saved_state.dma.current_offset = emulator->dma.current_offset;
  saved_state.dma.prep_delay = emulator->dma.prep_delay;

  /** Save MBC states */
  saved_state.mbc.rom_active_bank_num = emulator->mbc.rom.active_bank_num;
  saved_state.mbc.ram_active_bank_num = emulator->mbc.ext_ram.active_bank_num;
  saved_state.mbc.banking_mode = emulator->mbc.banking_mode;

  /** Save PPU states */
  memcpy(&saved_state.ppu.lcd, &emulator->ppu.lcd.registers, sizeof(lcd_registers_t));
  memcpy(&saved_state.ppu.oam_entries, &emulator->ppu.oam.entries, sizeof(emulator->ppu.oam.entries));
  memcpy(&saved_state.ppu.video_buffer, &emulator->ppu.video_buffer, sizeof(emulator->ppu.video_buffer));
  memcpy(&saved_state.ppu.pxfifo.bg_fifo, &emulator->ppu.pxfifo.bg_fifo, sizeof(pxfifo_buffer_t));
  memcpy(&saved_state.ppu.pxfifo.counters, &emulator->ppu.pxfifo.counters, sizeof(pxfifo_counter_t));
  memcpy(&saved_state.ppu.pxfifo.pixel_fetcher, &emulator->ppu.pxfifo.pixel_fetcher, sizeof(pixel_fetcher_state_t));
  saved_state.ppu.pxfifo.fifo_state = emulator->ppu.pxfifo.fifo_state;
  saved_state.ppu.current_frame = emulator->ppu.current_frame;
  saved_state.ppu.line_ticks = emulator->ppu.line_ticks;

  /* Save RAM states */
  memcpy(&saved_state.ram.wram, &emulator->ram.wram, sizeof(wram_t));
  memcpy(&saved_state.ram.vram, &emulator->ram.vram, sizeof(vram_t));
  memcpy(&saved_state.ram.hram, &emulator->ram.hram, sizeof(hram_t));

  /* Save timer states */
  memcpy(&saved_state.tmr, &emulator->tmr.registers, sizeof(timer_registers_t));

  Log_I("Game state saved to slot %d", slot_num);

  return STATUS_OK;
}

status_code_t load_game_state(emulator_t *const emulator, const uint8_t slot_num)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(slot_num > 9, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;

  /** Load APU states */
  memcpy(&emulator->apu.ch1.registers, &saved_state.apu.ch1.registers, sizeof(apu_pwm_registers_t));
  memcpy(&emulator->apu.ch1.state, &saved_state.apu.ch1.state, sizeof(apu_pwm_state_t));
  memcpy(&emulator->apu.ch2.registers, &saved_state.apu.ch2.registers, sizeof(apu_pwm_registers_t));
  memcpy(&emulator->apu.ch2.state, &saved_state.apu.ch2.state, sizeof(apu_pwm_state_t));
  memcpy(&emulator->apu.ch3.registers, &saved_state.apu.ch3.registers, sizeof(apu_wave_registers_t));
  memcpy(&emulator->apu.ch3.state, &saved_state.apu.ch3.state, sizeof(apu_wave_state_t));
  memcpy(emulator->apu.ch3.wave_ram.data, saved_state.apu.ch3.wave_ram_data, sizeof(emulator->apu.ch3.wave_ram.data));
  memcpy(&emulator->apu.ch4.registers, &saved_state.apu.ch4.registers, sizeof(apu_lfsr_registers_t));
  memcpy(&emulator->apu.ch4.state, &saved_state.apu.ch4.state, sizeof(apu_lfsr_state_t));
  memcpy(&emulator->apu.registers, &saved_state.apu.registers, sizeof(apu_registers_t));
  memcpy(&emulator->apu.frame_sequencer, &saved_state.apu.frame_sequencer, sizeof(apu_frame_sequencer_counter_t));

  /** Load CPU states */
  memcpy(&emulator->cpu_state.registers, &saved_state.cpu.registers, sizeof(registers_t));
  memcpy(&emulator->cpu_state.interrupt.registers, &saved_state.cpu.int_regs, sizeof(interrupt_registers_t));
  emulator->cpu_state.m_cycles = saved_state.cpu.m_cycles;
  emulator->cpu_state.run_mode = saved_state.cpu.run_mode;
  emulator->cpu_state.next_ime_flag = saved_state.cpu.next_ime_flag;
  emulator->cpu_state.current_inst_m_cycle_count = saved_state.cpu.current_inst_m_cycle_count;

  /** Load DMA states */
  emulator->dma.state = saved_state.dma.state;
  emulator->dma.starting_addr = saved_state.dma.starting_addr;
  emulator->dma.current_offset = saved_state.dma.current_offset;
  emulator->dma.prep_delay = saved_state.dma.prep_delay;

  /** Load MBC states */
  emulator->mbc.rom.active_bank_num = saved_state.mbc.rom_active_bank_num;
  emulator->mbc.ext_ram.active_bank_num = saved_state.mbc.ram_active_bank_num;
  emulator->mbc.banking_mode = saved_state.mbc.banking_mode;

  /** Load PPU states */
  memcpy(&emulator->ppu.lcd.registers, &saved_state.ppu.lcd, sizeof(lcd_registers_t));
  memcpy(&emulator->ppu.oam.entries, &saved_state.ppu.oam_entries, sizeof(emulator->ppu.oam.entries));
  memcpy(&emulator->ppu.video_buffer, &saved_state.ppu.video_buffer, sizeof(emulator->ppu.video_buffer));
  memcpy(&emulator->ppu.pxfifo.bg_fifo, &saved_state.ppu.pxfifo.bg_fifo, sizeof(pxfifo_buffer_t));
  memcpy(&emulator->ppu.pxfifo.counters, &saved_state.ppu.pxfifo.counters, sizeof(pxfifo_counter_t));
  memcpy(&emulator->ppu.pxfifo.pixel_fetcher, &saved_state.ppu.pxfifo.pixel_fetcher, sizeof(pixel_fetcher_state_t));
  emulator->ppu.pxfifo.fifo_state = saved_state.ppu.pxfifo.fifo_state;
  emulator->ppu.current_frame = saved_state.ppu.current_frame;
  emulator->ppu.line_ticks = saved_state.ppu.line_ticks;

  /* Load RAM states */
  memcpy(&emulator->ram.wram, &saved_state.ram.wram, sizeof(wram_t));
  memcpy(&emulator->ram.vram, &saved_state.ram.vram, sizeof(vram_t));
  memcpy(&emulator->ram.hram, &saved_state.ram.hram, sizeof(hram_t));

  /* Load timer states */
  memcpy(&emulator->tmr.registers, &saved_state.tmr, sizeof(timer_registers_t));

  status = mbc_reload_banks(&emulator->mbc);
  RETURN_STATUS_IF_NOT_OK(status);

  Log_I("Game state loaded from slot %d", slot_num);

  return STATUS_OK;
}
