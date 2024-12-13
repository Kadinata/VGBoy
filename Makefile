CC = gcc
CFLAGS = -Icore/include -pedantic -Wall -Wextra -std=c11
LDFLAGS = -L/usr/local/lib

SOURCES = core/src/cpu.c
SOURCES += core/src/memory.c

HEADERS = core/include/cpu.h
HEADERS += core/include/memory.h
HEADERS += core/include/sys_def.h
HEADERS += core/include/status_code.h

OBJS = objects/core/cpu.o 
OBJS += objects/core/memory.o

all: bin/gameboy_emu.out

bin/gameboy_emu.out: $(OBJS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

objects/core/%.o: core/src/%.c
	@mkdir -p objects/core
	$(CC) -c $< $(CFLAGS) -o$@

clean:
	rm -rf bin objects