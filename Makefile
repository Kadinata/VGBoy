CC = gcc
CFLAGS = -Icore/include -Ilib/include -Icommon -Icommon/ring_buf -Icommon/callback -pedantic -Wall -Wextra -Wno-gnu-statement-expression -std=c11
CFLAGS += -fsanitize=address
CFLAGS += -fsanitize=undefined
CFLAGS += -g3
CFLAGS += -O2

LDFLAGS = -L/usr/local/lib

SOURCES = core/src/cpu.c
SOURCES += core/src/rom.c
SOURCES += main.c
SOURCES += core/src/data_bus.c
SOURCES += core/src/emulator.c
SOURCES += core/src/ram.c
SOURCES += core/src/oam.c
SOURCES += core/src/io.c
SOURCES += core/src/interrupt.c
SOURCES += core/src/timer.c
SOURCES += core/src/debug_serial.c
SOURCES += core/src/bus_interface.c
SOURCES += core/src/dma.c
SOURCES += core/src/lcd.c
SOURCES += core/src/ppu.c
SOURCES += core/src/pixel_fifo.c
SOURCES += core/src/pixel_fetcher.c
SOURCES += core/src/joypad.c
SOURCES += core/src/mbc.c
SOURCES += core/src/apu.c
SOURCES += lib/src/display.c
SOURCES += lib/src/key_input.c
SOURCES += lib/src/window_manager.c
SOURCES += lib/src/main_window.c
SOURCES += lib/src/tile_debug_window.c
SOURCES += lib/src/fps_sync.c
SOURCES += common/ring_buf/ring_buf.c
SOURCES += common/callback/callback.c

HEADERS = common/status_code.h
HEADERS += common/color.h
HEADERS += common/logging.h
HEADERS += core/include/cpu.h
HEADERS += core/include/rom.h
HEADERS += core/include/data_bus.h
HEADERS += core/include/emulator.h
HEADERS += core/include/ram.h
HEADERS += core/include/oam.h
HEADERS += core/include/interrupt.h
HEADERS += core/include/io.h
HEADERS += core/include/timer.h
HEADERS += core/include/debug_serial.h
HEADERS += core/include/bus_interface.h
HEADERS += core/include/dma.h
HEADERS += core/include/lcd.h
HEADERS += core/include/ppu.h
HEADERS += core/include/pixel_fifo.h
HEADERS += core/include/pixel_fetcher.h
HEADERS += core/include/joypad.h
HEADERS += core/include/mbc.h
HEADERS += core/include/apu.h
HEADERS += lib/include/display.h
HEADERS += lib/include/key_input.h
HEADERS += lib/include/window_manager.h
HEADERS += lib/include/main_window.h
HEADERS += lib/include/tile_debug_window.h
HEADERS += lib/include/fps_sync.h
HEADERS += common/ring_buf/ring_buf.h
HEADERS += common/callback/callback.h

OBJS = objects/core/cpu.o 
OBJS += objects/core/rom.o
OBJS += objects/main.o
OBJS += objects/core/data_bus.o
OBJS += objects/core/emulator.o
OBJS += objects/core/ram.o
OBJS += objects/core/oam.o
OBJS += objects/core/io.o
OBJS += objects/core/interrupt.o
OBJS += objects/core/timer.o
OBJS += objects/core/debug_serial.o
OBJS += objects/core/bus_interface.o
OBJS += objects/core/dma.o
OBJS += objects/core/lcd.o
OBJS += objects/core/ppu.o
OBJS += objects/core/pixel_fifo.o
OBJS += objects/core/pixel_fetcher.o
OBJS += objects/core/joypad.o
OBJS += objects/core/mbc.o
OBJS += objects/core/apu.o
OBJS += objects/lib/display.o
OBJS += objects/lib/key_input.o
OBJS += objects/lib/window_manager.o
OBJS += objects/lib/main_window.o
OBJS += objects/lib/tile_debug_window.o
OBJS += objects/lib/fps_sync.o
OBJS += objects/common/ring_buf.o
OBJS += objects/common/callback.o

LIBS = -lSDL2

all: bin/gb_emu.out

bin/gb_emu.out: $(OBJS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

objects/core/%.o: core/src/%.c
	@mkdir -p objects/core
	$(CC) -c $< $(CFLAGS) -o$@

objects/lib/%.o: lib/src/%.c
	@mkdir -p objects/lib
	$(CC) -c $< $(CFLAGS) -o$@

objects/common/%.o: common/**/%.c
	@mkdir -p objects/common
	$(CC) -c $< $(CFLAGS) -o$@

objects/%.o: %.c
	@mkdir -p objects
	$(CC) -c $< $(CFLAGS) -o$@

clean:
	rm -rf bin objects