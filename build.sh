#!/bin/sh
# Builds 8tris from source. Needs gcc and libsdl2-dev for Linux; x86_64-w64-mingw32-gcc for the Windows .exe.
# Linux:    ./build.sh linux     -> ./8tris
# Windows:  ./build.sh windows   -> ./8tris.exe   (icon.ico is embedded)
set -e
W="-Wall -Wno-unused-function -Wno-misleading-indentation -Wno-unused-variable -Wno-format-truncation"
case "$1" in
 linux)   gcc -O2 $W -o 8tris src/plat_sdl.c -lSDL2 -lm -lpthread ;;
 windows) x86_64-w64-mingw32-windres icon.rc -O coff -o icon_res.o
          x86_64-w64-mingw32-gcc -O2 $W -static -mwindows -o 8tris.exe src/plat_win.c icon_res.o -lwinmm -lgdi32 -luser32 -lopengl32 -lm ;;
 *) echo "usage: $0 linux|windows"; exit 1 ;;
esac
