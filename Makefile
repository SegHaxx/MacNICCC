CFLAGS=-std=gnu99 -Wall -Os -mshort
#CFLAGS=-std=gnu99 -Wall -Os -mtune=68020-40

# path to RETRO68
RETRO68=/home/seg/src/Retro68-build/toolchain
PREFIX=$(RETRO68)/bin/m68k-apple-macos
CC=$(PREFIX)-gcc
CXX=$(PREFIX)-g++
REZ=$(RETRO68)/bin/Rez

LDFLAGS=-Wl,-gc-sections
RINCLUDES=$(RETRO68)/m68k-apple-macos/RIncludes
REZFLAGS=-I$(RINCLUDES)

TARGETS=MacNICCC.code.bin MacNICCC.bin MacNICCC.img

src = $(wildcard *.c)
obj = $(src:.c=.o)

all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(obj) $(TARGETS) *.code.bin.gdb

MacNICCC.code.bin: macniccc.o
	$(CC) $< -o $@ $(LDFLAGS)

macniccc.s: macniccc.c
	$(CC) $(CFLAGS) -S -o $@ $<

MacNICCC.APPL: MacNICCC.code.bin
	$(REZ) $(REZFLAGS) --copy $< "$(RINCLUDES)/Retro68APPL.r" -t APPL -c "????" -o $@

MacNICCC.bin: MacNICCC.code.bin
	$(REZ) $(REZFLAGS) --copy $< "$(RINCLUDES)/Retro68APPL.r" -t APPL -c "????" -o $@
	@du -b $@

MacNICCC.img: MacNICCC.bin
	@truncate -s 800k MacNICCC.img
	@hformat -l MacNICCC MacNICCC.img
	@hcopy MacNICCC.bin :
	@hcopy -r scene1.bin :
	@humount MacNICCC.img

MacNICCC.720k.img: MacNICCC.bin
	@truncate -s 720k $@
	@hformat -l MacNICCC $@
	@hcopy MacNICCC.bin :
	@hcopy -r scene1.bin :
	@humount $@
