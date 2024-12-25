CC = gcc
CFLAGS = -Icore/include -pedantic -Wall -Wextra -Wno-gnu-statement-expression -std=c11
LDFLAGS = -L/usr/local/lib

SOURCES = core/src/cpu.c
SOURCES += core/src/rom.c
SOURCES += main.c
SOURCES += core/src/data_bus.c
SOURCES += core/src/emulator.c
SOURCES += core/src/ram.c
SOURCES += core/src/io.c
SOURCES += core/src/interrupt.c
SOURCES += core/src/timer.c
SOURCES += core/src/timing_sync.c
SOURCES += core/src/debug_serial.c
SOURCES += core/src/bus_interface.c

HEADERS += core/include/logging.h
HEADERS = core/include/cpu.h
HEADERS += core/include/rom.h
HEADERS += core/include/status_code.h
HEADERS += core/include/data_bus.h
HEADERS += core/include/emulator.h
HEADERS += core/include/ram.h
HEADERS += core/include/interrupt.h
HEADERS += core/include/io.h
HEADERS += core/include/timer.h
HEADERS += core/include/timing_sync.h
HEADERS += core/include/debug_serial.h
HEADERS += core/include/bus_interface.h

OBJS = objects/core/cpu.o 
OBJS += objects/core/rom.o
OBJS += objects/main.o
OBJS += objects/core/data_bus.o
OBJS += objects/core/emulator.o
OBJS += objects/core/ram.o
OBJS += objects/core/io.o
OBJS += objects/core/interrupt.o
OBJS += objects/core/timer.o
OBJS += objects/core/timing_sync.o
OBJS += objects/core/debug_serial.o
OBJS += objects/core/bus_interface.o

all: bin/gb_emu.out

bin/gb_emu.out: $(OBJS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

objects/core/%.o: core/src/%.c
	@mkdir -p objects/core
	$(CC) -c $< $(CFLAGS) -o$@

objects/%.o: %.c
	@mkdir -p objects
	$(CC) -c $< $(CFLAGS) -o$@

clean:
	rm -rf bin objects