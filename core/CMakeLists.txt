target_include_directories(${PROJECT_NAME} PRIVATE
  include/
)

target_sources(${PROJECT_NAME} PRIVATE
  src/apu_lfsr.c
  src/apu_pwm.c
  src/apu_wave.c
  src/apu.c
  src/bus_interface.c
  src/cpu.c
  src/data_bus.c
  src/debug_serial.c
  src/dma.c
  src/emulator.c
  src/interrupt.c
  src/io.c
  src/joypad.c
  src/lcd.c
  src/mbc.c
  src/oam.c
  src/pixel_fetcher.c
  src/pixel_fifo.c
  src/ppu.c
  src/ram.c
  src/rom.c
  src/timer.c
)
