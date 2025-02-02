#include "snapshot.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "logging.h"
#include "status_code.h"

typedef struct
{
  apu_lfsr_state_t state;
  apu_lfsr_registers_t registers;
} apu_lfsr_snapshot_t;

typedef struct
{
  apu_pwm_state_t state;
  apu_pwm_registers_t registers;
} apu_pwm_snapshot_t;

typedef struct
{
  apu_wave_state_t state;
  apu_wave_registers_t registers;
  uint8_t wave_ram_data[16];
} apu_wave_snapshot_t;

typedef struct
{
  apu_pwm_snapshot_t ch1;
  apu_pwm_snapshot_t ch2;
  apu_wave_snapshot_t ch3;
  apu_lfsr_snapshot_t ch4;
  apu_registers_t registers;
  apu_frame_sequencer_counter_t frame_sequencer;
} apu_snapshot_t;

typedef struct
{
  registers_t registers;
  uint32_t m_cycles;
  cpu_run_mode_t run_mode;
  uint8_t next_ime_flag;
  uint8_t current_inst_m_cycle_count;
  interrupt_registers_t int_regs;
} cpu_snapshot_t;

typedef struct
{
  dma_state_t state;
  uint16_t starting_addr;
  uint8_t current_offset;
  uint8_t prep_delay;
} dma_snapshot_t;

typedef struct
{
  uint16_t rom_active_bank_num;
  uint8_t ram_active_bank_num;
  banking_mode_t banking_mode;
} mbc_snapshot_t;

typedef struct
{
  pxfifo_state_t fifo_state;
  pxfifo_buffer_t bg_fifo;
  pxfifo_counter_t counters;
  pixel_fetcher_state_t pixel_fetcher;
} pxfifo_snapshot_t;

typedef struct
{
  lcd_registers_t lcd;
  oam_entry_t oam_entries[OAM_ENTRY_SIZE];
  pxfifo_snapshot_t pxfifo;
  uint32_t current_frame;
  uint32_t line_ticks;
  video_buffer_t video_buffer;
} ppu_snapshot_t;

typedef struct
{
  wram_t wram;
  vram_t vram;
  hram_t hram;
} ram_snapshot_t;

typedef struct
{
  apu_snapshot_t apu;
  cpu_snapshot_t cpu;
  dma_snapshot_t dma;
  mbc_snapshot_t mbc;
  ppu_snapshot_t ppu;
  ram_snapshot_t ram;
  timer_registers_t tmr;
} emulator_snapshot_t;

typedef struct
{
  bool requested;
  uint8_t slot_num;
} snapshot_request_t;

typedef struct
{
  snapshot_request_t save;
  snapshot_request_t load;
} snapshot_requests_t;

static emulator_snapshot_t snapshot = {0};
static snapshot_requests_t snapshot_requests;

static status_code_t save_snapshot(emulator_t *const emulator, const uint8_t slot_num);
static status_code_t load_snapshot(emulator_t *const emulator, const uint8_t slot_num);

status_code_t request_snapshot(const uint8_t slot_num, const game_state_mode_t mode)
{
  VERIFY_COND_RETURN_STATUS_IF_TRUE(slot_num > 9, STATUS_ERR_INVALID_ARG);

  switch (mode)
  {
  case MODE_SAVE_SNAPSHOT:
    snapshot_requests.save.requested = true;
    snapshot_requests.save.slot_num = slot_num;
    break;
  case MODE_LOAD_SNAPSHOT:
    snapshot_requests.load.requested = true;
    snapshot_requests.load.slot_num = slot_num;
    break;
  default:
    break;
  }

  return STATUS_OK;
}

status_code_t handle_snapshot_request(emulator_t *const emulator)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);

  status_code_t status = STATUS_OK;

  if (snapshot_requests.save.requested)
  {
    snapshot_requests.save.requested = false;
    status = save_snapshot(emulator, snapshot_requests.save.slot_num);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  if (snapshot_requests.load.requested)
  {
    snapshot_requests.load.requested = false;
    status = load_snapshot(emulator, snapshot_requests.load.slot_num);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return STATUS_OK;
}

static status_code_t save_snapshot(emulator_t *const emulator, const uint8_t slot_num)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(slot_num > 9, STATUS_ERR_INVALID_ARG);

  /** Save APU states */
  memcpy(&snapshot.apu.ch1.registers, &emulator->apu.ch1.registers, sizeof(apu_pwm_registers_t));
  memcpy(&snapshot.apu.ch1.state, &emulator->apu.ch1.state, sizeof(apu_pwm_state_t));
  memcpy(&snapshot.apu.ch2.registers, &emulator->apu.ch2.registers, sizeof(apu_pwm_registers_t));
  memcpy(&snapshot.apu.ch2.state, &emulator->apu.ch2.state, sizeof(apu_pwm_state_t));
  memcpy(&snapshot.apu.ch3.registers, &emulator->apu.ch3.registers, sizeof(apu_wave_registers_t));
  memcpy(&snapshot.apu.ch3.state, &emulator->apu.ch3.state, sizeof(apu_wave_state_t));
  memcpy(snapshot.apu.ch3.wave_ram_data, emulator->apu.ch3.wave_ram.data, sizeof(emulator->apu.ch3.wave_ram.data));
  memcpy(&snapshot.apu.ch4.registers, &emulator->apu.ch4.registers, sizeof(apu_lfsr_registers_t));
  memcpy(&snapshot.apu.ch4.state, &emulator->apu.ch4.state, sizeof(apu_lfsr_state_t));
  memcpy(&snapshot.apu.registers, &emulator->apu.registers, sizeof(apu_registers_t));
  memcpy(&snapshot.apu.frame_sequencer, &emulator->apu.frame_sequencer, sizeof(apu_frame_sequencer_counter_t));

  /** Save CPU states */
  memcpy(&snapshot.cpu.registers, &emulator->cpu_state.registers, sizeof(registers_t));
  memcpy(&snapshot.cpu.int_regs, &emulator->cpu_state.interrupt.registers, sizeof(interrupt_registers_t));
  snapshot.cpu.m_cycles = emulator->cpu_state.m_cycles;
  snapshot.cpu.run_mode = emulator->cpu_state.run_mode;
  snapshot.cpu.next_ime_flag = emulator->cpu_state.next_ime_flag;
  snapshot.cpu.current_inst_m_cycle_count = emulator->cpu_state.current_inst_m_cycle_count;

  /** Save DMA states */
  snapshot.dma.state = emulator->dma.state;
  snapshot.dma.starting_addr = emulator->dma.starting_addr;
  snapshot.dma.current_offset = emulator->dma.current_offset;
  snapshot.dma.prep_delay = emulator->dma.prep_delay;

  /** Save MBC states */
  snapshot.mbc.rom_active_bank_num = emulator->mbc.rom.active_bank_num;
  snapshot.mbc.ram_active_bank_num = emulator->mbc.ext_ram.active_bank_num;
  snapshot.mbc.banking_mode = emulator->mbc.banking_mode;

  /** Save PPU states */
  memcpy(&snapshot.ppu.lcd, &emulator->ppu.lcd.registers, sizeof(lcd_registers_t));
  memcpy(&snapshot.ppu.oam_entries, &emulator->ppu.oam.entries, sizeof(emulator->ppu.oam.entries));
  memcpy(&snapshot.ppu.video_buffer, &emulator->ppu.video_buffer, sizeof(emulator->ppu.video_buffer));
  memcpy(&snapshot.ppu.pxfifo.bg_fifo, &emulator->ppu.pxfifo.bg_fifo, sizeof(pxfifo_buffer_t));
  memcpy(&snapshot.ppu.pxfifo.counters, &emulator->ppu.pxfifo.counters, sizeof(pxfifo_counter_t));
  memcpy(&snapshot.ppu.pxfifo.pixel_fetcher, &emulator->ppu.pxfifo.pixel_fetcher, sizeof(pixel_fetcher_state_t));
  snapshot.ppu.pxfifo.fifo_state = emulator->ppu.pxfifo.fifo_state;
  snapshot.ppu.current_frame = emulator->ppu.current_frame;
  snapshot.ppu.line_ticks = emulator->ppu.line_ticks;

  /* Save RAM states */
  memcpy(&snapshot.ram.wram, &emulator->ram.wram, sizeof(wram_t));
  memcpy(&snapshot.ram.vram, &emulator->ram.vram, sizeof(vram_t));
  memcpy(&snapshot.ram.hram, &emulator->ram.hram, sizeof(hram_t));

  /* Save timer states */
  memcpy(&snapshot.tmr, &emulator->tmr.registers, sizeof(timer_registers_t));

  Log_I("State snapshot saved to slot %d", slot_num);

  return STATUS_OK;
}

static status_code_t load_snapshot(emulator_t *const emulator, const uint8_t slot_num)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(slot_num > 9, STATUS_ERR_INVALID_ARG);

  status_code_t status = STATUS_OK;

  /** Load APU states */
  memcpy(&emulator->apu.ch1.registers, &snapshot.apu.ch1.registers, sizeof(apu_pwm_registers_t));
  memcpy(&emulator->apu.ch1.state, &snapshot.apu.ch1.state, sizeof(apu_pwm_state_t));
  memcpy(&emulator->apu.ch2.registers, &snapshot.apu.ch2.registers, sizeof(apu_pwm_registers_t));
  memcpy(&emulator->apu.ch2.state, &snapshot.apu.ch2.state, sizeof(apu_pwm_state_t));
  memcpy(&emulator->apu.ch3.registers, &snapshot.apu.ch3.registers, sizeof(apu_wave_registers_t));
  memcpy(&emulator->apu.ch3.state, &snapshot.apu.ch3.state, sizeof(apu_wave_state_t));
  memcpy(emulator->apu.ch3.wave_ram.data, snapshot.apu.ch3.wave_ram_data, sizeof(emulator->apu.ch3.wave_ram.data));
  memcpy(&emulator->apu.ch4.registers, &snapshot.apu.ch4.registers, sizeof(apu_lfsr_registers_t));
  memcpy(&emulator->apu.ch4.state, &snapshot.apu.ch4.state, sizeof(apu_lfsr_state_t));
  memcpy(&emulator->apu.registers, &snapshot.apu.registers, sizeof(apu_registers_t));
  memcpy(&emulator->apu.frame_sequencer, &snapshot.apu.frame_sequencer, sizeof(apu_frame_sequencer_counter_t));

  /** Load CPU states */
  memcpy(&emulator->cpu_state.registers, &snapshot.cpu.registers, sizeof(registers_t));
  memcpy(&emulator->cpu_state.interrupt.registers, &snapshot.cpu.int_regs, sizeof(interrupt_registers_t));
  emulator->cpu_state.m_cycles = snapshot.cpu.m_cycles;
  emulator->cpu_state.run_mode = snapshot.cpu.run_mode;
  emulator->cpu_state.next_ime_flag = snapshot.cpu.next_ime_flag;
  emulator->cpu_state.current_inst_m_cycle_count = snapshot.cpu.current_inst_m_cycle_count;

  /** Load DMA states */
  emulator->dma.state = snapshot.dma.state;
  emulator->dma.starting_addr = snapshot.dma.starting_addr;
  emulator->dma.current_offset = snapshot.dma.current_offset;
  emulator->dma.prep_delay = snapshot.dma.prep_delay;

  /** Load MBC states */
  emulator->mbc.rom.active_bank_num = snapshot.mbc.rom_active_bank_num;
  emulator->mbc.ext_ram.active_bank_num = snapshot.mbc.ram_active_bank_num;
  emulator->mbc.banking_mode = snapshot.mbc.banking_mode;

  /** Load PPU states */
  memcpy(&emulator->ppu.lcd.registers, &snapshot.ppu.lcd, sizeof(lcd_registers_t));
  memcpy(&emulator->ppu.oam.entries, &snapshot.ppu.oam_entries, sizeof(emulator->ppu.oam.entries));
  memcpy(&emulator->ppu.video_buffer, &snapshot.ppu.video_buffer, sizeof(emulator->ppu.video_buffer));
  memcpy(&emulator->ppu.pxfifo.bg_fifo, &snapshot.ppu.pxfifo.bg_fifo, sizeof(pxfifo_buffer_t));
  memcpy(&emulator->ppu.pxfifo.counters, &snapshot.ppu.pxfifo.counters, sizeof(pxfifo_counter_t));
  memcpy(&emulator->ppu.pxfifo.pixel_fetcher, &snapshot.ppu.pxfifo.pixel_fetcher, sizeof(pixel_fetcher_state_t));
  emulator->ppu.pxfifo.fifo_state = snapshot.ppu.pxfifo.fifo_state;
  emulator->ppu.current_frame = snapshot.ppu.current_frame;
  emulator->ppu.line_ticks = snapshot.ppu.line_ticks;

  /* Load RAM states */
  memcpy(&emulator->ram.wram, &snapshot.ram.wram, sizeof(wram_t));
  memcpy(&emulator->ram.vram, &snapshot.ram.vram, sizeof(vram_t));
  memcpy(&emulator->ram.hram, &snapshot.ram.hram, sizeof(hram_t));

  /* Load timer states */
  memcpy(&emulator->tmr.registers, &snapshot.tmr, sizeof(timer_registers_t));

  status = mbc_reload_banks(&emulator->mbc);
  RETURN_STATUS_IF_NOT_OK(status);

  Log_I("State snapshot loaded from slot %d", slot_num);

  return STATUS_OK;
}
