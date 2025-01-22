#ifndef __DMG_APU_H__
#define __DMG_APU_H__

#include <stdint.h>

#include "bus_interface.h"
#include "status_code.h"

typedef struct __attribute__((packed))
{
  uint8_t sweep;  /* 0xFF10 — NR10: Channel 1 sweep */
  uint8_t tmrd;   /* 0xFF11 — NR11: Channel 1 length timer & duty cycle */
  uint8_t volenv; /* 0xFF12 — NR12: Channel 1 volume & envelope */
  uint8_t plow;   /* 0xFF13 — NR13: Channel 1 period low [write-only] */
  uint8_t phctl;  /* 0xFF14 — NR14: Channel 1 period high & control */
} apu_ch1_registers_t;

typedef struct __attribute__((packed))
{
  uint8_t _unused; /* 0xFF15 - Unused */
  uint8_t tmrd;    /* 0xFF16 — NR21: Channel 2 length timer & duty cycle */
  uint8_t volenv;  /* 0xFF17 — NR22: Channel 2 volume & envelope */
  uint8_t plow;    /* 0xFF18 — NR23: Channel 2 period low [write-only] */
  uint8_t phctl;   /* 0xFF19 — NR24: Channel 2 period high & control */
} apu_ch2_registers_t;

typedef struct __attribute__((packed))
{
  uint8_t dacen; /* 0xFF1A — NR30: Channel 3 DAC enable */
  uint8_t ltmr;  /* 0xFF1B — NR31: Channel 3 length timer [write-only] */
  uint8_t vol;   /* 0xFF1C — NR32: Channel 3 output level */
  uint8_t plow;  /* 0xFF1D — NR33: Channel 3 period low [write-only] */
  uint8_t phctl; /* 0xFF1E — NR34: Channel 3 period high & control */
} apu_ch3_registers_t;

typedef struct __attribute__((packed))
{
  uint8_t _unused; /* 0xFF1F - Unused */
  uint8_t ltmr;    /* 0xFF20 — NR41: Channel 4 length timer [write-only] */
  uint8_t volenv;  /* 0xFF21 — NR42: Channel 4 volume & envelope */
  uint8_t frqrand; /* 0xFF22 — NR43: Channel 4 frequency & randomness */
  uint8_t ctrl;    /* 0xFF23 — NR44: Channel 4 control */
} apu_ch4_registers_t;

typedef struct __attribute__((packed))
{
  uint8_t mvp;  /* 0xFF24 — NR50: Master volume & VIN panning */
  uint8_t sndp; /* 0xFF25 — NR51: Sound panning */
  uint8_t actl; /* 0xFF26 — NR52: Audio master control */
} apu_global_registers_t;

typedef union
{
  struct __attribute__((packed))
  {
    apu_ch1_registers_t ch1;       /* 0xFF10 - 0xFF14: Channel 1 registers */
    apu_ch2_registers_t ch2;       /* 0xFF15 - 0xFF19: Channel 2 registers */
    apu_ch3_registers_t ch3;       /* 0xFF1A - 0xFF1E: Channel 3 registers */
    apu_ch4_registers_t ch4;       /* 0xFF1F - 0xFF23: Channel 4 registers */
    apu_global_registers_t global; /* 0xFF24 - 0xFF26: Global APU registers */
    uint8_t _unused[9];            /* 0xFF27 - 0xFF2F: Unused */
    uint8_t wave_ram[16];          /* 0xFF30 – 0xFF3F — Wave pattern RAM */
  };
  uint8_t buffer[48]; /* APU registers as byte array */
} apu_registers_t;

typedef struct
{
  apu_registers_t registers;
  bus_interface_t bus_interface;
} apu_handle_t;

status_code_t apu_init(apu_handle_t *const apu);

#endif /* __DMG_APU_H__ */
