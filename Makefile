CPU = 68000
TARGET = zsm

ARCH?=$(CPU)
TUNE?=$(CPU)
EXTRA_CFLAGS?=
DEFINES:=$(DEFINES)

ifeq ($(OS),Windows_NT)
	CROSSDIR = C:/dev/lang/m68k-elf-os
else
	CROSSDIR = /opt/m68k-elf-9
endif
BIN = $(CROSSDIR)/bin

BINARY=$(TARGET).exe
BINARY_ELF=$(TARGET).elf
MAP=$(TARGET).map

CC = $(BIN)/m68k-elf-gcc
AS = $(BIN)/vasmm68k_mot
LD = $(BIN)/m68k-elf-ld
OBJCOPY = $(BIN)/m68k-elf-objcopy
GCC_VERSION = $(shell $(CC) -dumpversion)

CFLAGS=-std=c11 -Wall -Wpedantic -s     \
       -I. -Iinclude -I..//os-programs/DOSLib -mcpu=$(CPU) -march=$(ARCH) -mtune=$(TUNE) -O2   \
       -ffunction-sections -fdata-sections -mstrict-align -fomit-frame-pointer \
	   -fno-unwind-tables -fno-asynchronous-unwind-tables -ffast-math \
	   -Wa,--register-prefix-optional  $(DEFINES)

LDFLAGS= -Map=$(MAP) -print-memory-usage
ASFLAGS=-Felf -m$(CPU) -quiet $(DEFINES)
LIBS = -L$(CROSSDIR)/m68k-elf/lib -lm -L$(CROSSDIR)/lib/gcc/m68k-elf/$(GCC_VERSION) -lgcc \
	   -L../os-programs/DOSLib -ldos \
	   -T $(CROSSDIR)/m68k-elf/lib/ddraig.ld

OBJECTS = zsmplay.o ym2151.o zsmplayer68k.o zfxfm68k.o ym2151_ding68k.o vgacard.o

%.o : %.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $<

%.o : %.S
	$(CC) -x assembler-with-cpp -c $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $<

%.o : %.asm
	$(AS) $(ASFLAGS) -o $@ $<

$(BINARY) : $(BINARY_ELF)
	$(OBJCOPY) --strip-debug $< $@

$(BINARY_ELF) : $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)


.PHONY: all clean dump tools

all: $(BINARY)

clean:
ifeq ($(OS),Windows_NT)
	cmd /c del /q /f *.lst
	cmd /c del /q /f *.o
	cmd /c del /q /f src\*.o
	cmd /c del /q /f *.map
	cmd /c del /q /f *.hex
	cmd /c del /q /f *.bin
	cmd /c del /q /f *.elf
	cmd /c del /q /f *.exe
else
	rm -f *.lst *.LST
	rm -f *.o src/*.o
	rm -f *.a
	rm -f *.map
	rm -f *.hex
	rm -f *.bin *.BIN
	rm -f *.elf
endif
