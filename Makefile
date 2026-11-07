# 8tris. `make` builds ./8tris (Linux). Needs gcc, libsdl2-dev and, for Liberated Peep (the WAV and FLAC decoder, in third_party/peep),
# nasm on x86-64 or GNU as on ARM64.
#   make            Linux binary ./8tris
#   make win        Windows exe ./8tris.exe (cross-compiled with mingw-w64 and nasm)
#   make peep       just the Liberated Peep library, build/libpeep.a
# The tests are single .c files that include the game; link them with the library:
#   gcc -O1 -o /tmp/t test/click.c build/libpeep.a -lm -lpthread
CC    ?= gcc
NASM  ?= nasm
MINGW ?= x86_64-w64-mingw32-gcc
WINDRES ?= x86_64-w64-mingw32-windres
P := third_party/peep
B := build
SRC := src/*.c src/*.h $(P)/peep.h

ifneq (,$(filter aarch64 arm64,$(shell uname -m)))
PEEPOBJ := $(B)/peep.o $(B)/flac.o $(B)/wav.o $(B)/wavc.o
$(B)/%.o: $(P)/arm64/%.s $(P)/arm64/abi_a64.inc | $(B); $(AS) -I $(P)/arm64 $< -o $@
else
PEEPOBJ := $(B)/peep.o $(B)/flac.o $(B)/wav.o $(B)/wavc.o
$(B)/%.o: $(P)/%.asm $(P)/abi.inc | $(B);               $(NASM) -f elf64 -I $(P)/ $< -o $@
endif
WOBJ := $(B)/peep.obj $(B)/flac.obj $(B)/wav.obj $(B)/wavc.obj
$(B)/%.obj: $(P)/%.asm $(P)/abi.inc | $(B);             $(NASM) -f win64 -DWIN64 -I $(P)/ $< -o $@

all: 8tris
peep: $(B)/libpeep.a
$(B):; mkdir -p $(B)
$(B)/libpeep.a: $(PEEPOBJ); rm -f $@; ar rcs $@ $^
8tris: $(SRC) $(B)/libpeep.a; $(CC) -O2 -o $@ src/plat_sdl.c $(B)/libpeep.a -lSDL2 -lm -lpthread
$(B)/icon_res.o: icon.rc icon.ico | $(B); $(WINDRES) icon.rc -O coff -o $@
win: 8tris.exe
8tris.exe: $(SRC) $(WOBJ) $(B)/icon_res.o; $(MINGW) -O2 -static -mwindows -o $@ src/plat_win.c $(B)/icon_res.o $(WOBJ) -lwinmm -lgdi32 -luser32 -lopengl32 -lm
clean:; rm -rf $(B) 8tris 8tris.exe
.PHONY: all peep win clean
