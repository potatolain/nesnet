# This attempts to build the game for you. 
# Note: This is a pretty poor excuse for a makefile. I'd look elsewhere for better examples. 
# Prequisites:
# - A few fairly standard unix applications available; Gow/Cygwin installed for Windows.
# - ca65 binaries in the tools folder

### USER EDITABLE STUFF STARTS HERE

ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

MAIN_COMPILER=./tools/cc65/bin/cc65
MAIN_ASM_COMPILER=./tools/cc65/bin/ca65
MAIN_LINKER=./tools/cc65/bin/ld65
SPLITTER=tools/readnes3/readnes3
MAIN_EMULATOR=tools/fceux/fceux
DEBUG_EMULATOR=tools/nintendulatordx/nintendulator
SPACE_CHECKER=tools/nessc/nessc
CONFIG_FILE=$(ROOT_DIR)/cfg/game.cfg
TEXT2DATA=sound/text2data
NSF2DATA=sound/nsf2data
VERSION=0.1

### USER EDITABLE STUFF ENDS HERE

all: hello.s lib/crt0.o hello.o hello.nes

hello.s: hello.c 
	$(MAIN_COMPILER) -Oi hello.c --add-source --include-dir ./tools/cc65/include
	
lib/crt0.o: lib/crt0.asm ../../src/nesnet.asm ../../src/nesnet.h
	$(MAIN_ASM_COMPILER) lib/crt0.asm

hello.o: hello.s
	$(MAIN_ASM_COMPILER) hello.s

hello.nes: lib/crt0.o hello.o 
	$(MAIN_LINKER) -C $(CONFIG_FILE) -o hello.nes lib/crt0.o hello.o lib/runtime.lib

fceux:
	$(MAIN_EMULATOR) hello.nes
	
run: fceux

nintendulator:
	$(DEBUG_EMULATOR) hello.nes

space_check:
	$(SPACE_CHECKER) hello.nes

clean:
	-rm -f *.nes
	-rm -f *.o
	-rm -f lib/*.o
	-rm -f hello.s