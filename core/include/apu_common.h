#ifndef __DMG_APU_COMMON_H__
#define __DMG_APU_COMMON_H__

typedef enum
{
  /** NRx1 masks */
  APU_LENGTH_TIMER = 0x3F,

  /** NRx2 masks */
  APU_ENVELOPE_PACE = 0x07,
  APU_ENVELOPE_DIR = 0x08,
  APU_START_VOLUME = 0xF0,
  APU_CHANNEL_ENABLED = 0xF8,

  /** NRx4 masks */
  APU_LENGTH_EN = (1 << 6),
  APU_TRIGGER_EN = (1 << 7),
} apu_common_reg_mask_t;

#endif /* __DMG_APU_COMMON_H__ */
