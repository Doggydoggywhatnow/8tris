# 8tris

A minimal, endlessly customizable Tetris. One small native program (less than 5 MB), no browser engine inside.

You open it to a black board and plain gray blocks. Everything else is opt-in: any board size, pieces from 1 to 10 blocks or letters, themes, effects, sounds, fonts, your own keys and your own pace.

**Created by Claude.**

## Download

Latest version: **0.7.13**. Download the file for your system from the [`downloads`](downloads) folder (click the file, then the download button).

| Platform | File | Notes |
|---|---|---|
| Linux (x86_64) | [`8tris-0.7.13-x86_64.AppImage`](downloads/8tris-0.7.13-x86_64.AppImage) | `chmod +x` it and run. Uses your system's SDL2 when it has one (then glibc 2.34 or newer is enough), otherwise its own bundled copy (glibc 2.38 or newer). |
| Windows (x86_64) | [`8tris-0.7.13.exe`](downloads/8tris-0.7.13.exe) | Not code-signed, so SmartScreen may warn. Choose "More info", then "Run anyway". |

See the [changelog](CHANGELOG.md) for what changed in each version.

Preferences and score history are saved in a folder on your computer and are never sent anywhere: `~/.local/share/8tris` on Linux (readable only by your account) and `%LOCALAPPDATA%\8tris` on Windows. Both are plain text files you can open and read.

## Philosophy

8tris starts from how modern Tetris plays and asks what happens when you can change almost everything.

- **Minimal first.** Themes, effects and extras are opt-in, so the first experience is just the game.
- **Open ended.** Any board size from 4 to 500, pieces of 1 to 10 blocks or letters A to Z, up to 10 hold slots, your own keys, your own fall rate. Presets are starting points, not rules.
- **Modern by default.** Where it can, 8tris plays like current Tetris (SRS rotation, 7-bag, hold, ghost piece, back-to-back, T-spins, perfect clears). Where it does not, the About screen lists exactly how it differs.
- **Honest scores.** Scoring adjusts for board width and piece size, and the history records every setting, so runs can be compared fairly.
- **Yours alone.** Preferences and history stay on your computer. It is a plain native program with no browser, no network access and no telemetry.

## Features

- **Themes, mixed per part.** The page, side panel, preferences menu, board frame, board, grid, pieces, landing outline, effects and sound each take their own theme: 8tris, 8tris Light, 8tris Dark, Graphite, Chocolate, Milk, Classic, Solid Color (any hex color, with a full color picker), Swirl and Wild.
- **Sound.** A table-driven synth with a different voice per theme, combo and perfect-clear sounds that climb with your combo.
- **Combo song.** An optional mode (Sound tab) where a combo chain plays a song that builds for as long as the chain lasts. Keep it going and see where it goes.
- **Your own music.** Drop .mp3, .ogg, .wav or .flac files in the music folder (shown in Preferences, Sound) and play one track, all in order, or shuffled. Optionally show the track title, artist, album and cover art on screen, read from the file's own tags. Files are only read locally.
- **Brightness for the whole game.** One slider in Appearance dims or brightens everything on screen, menus included. Every slider's value is also a box you can click and type an exact number into.
- **Game timer** can show milliseconds (a tickbox in its theming options).
- **Filters.** Brightness, contrast, saturation, hue shift, color split, tint, picture opacity and invert, over any theme.
- **Timer and clock.** An optional play timer and a real-time clock (you type your own UTC offset; 8tris never looks up your timezone), each with its own position, size, opacity, font and bold, and able to stack.
- **Layout.** Move the board anywhere in the play area, move the Preferences button, and choose how many upcoming pieces to see (0 to 10).
- **Effects ladder.** Off, A bit, Moderate, High, Intense, with a flash limiter and a Reduce motion switch. Advanced options let combo and perfect-clear visuals and sounds differ.
- **Display.** Window size, full screen, render resolution, refresh rate and a frame counter.
- **Appearance.** Preferences menu font (DejaVu Sans, Liberation Sans, Carlito, Liberation Serif, Liberation Mono, Atkinson Hyperlegible, OpenDyslexic), bold, italic, text size, menu width, line spacing and a high contrast menu. These never change the game's own text.
- **History.** Every finished run is saved with the preferences it used, and can be exported as text.

## Epilepsy caution

No guarantee is made that all themes are safe for people who are sensitive to flashing light. The High and Intense effects add bright flashes and screen shake. Use Preview, keep the default "A bit" level, or turn on Reduce motion.

## Build from source

Everything is plain C in a single translation unit (`src/8tris.c` pulls in the rest).

```sh
# Linux: needs gcc and libsdl2-dev
gcc -O2 -o 8tris src/plat_sdl.c -lSDL2 -lm -lpthread

# Windows exe, cross-compiled: needs x86_64-w64-mingw32-gcc
x86_64-w64-mingw32-windres icon.rc -O coff -o icon_res.o
x86_64-w64-mingw32-gcc -O2 -static -mwindows -o 8tris.exe src/plat_win.c icon_res.o -lwinmm -lgdi32 -luser32 -lopengl32 -lm
```

The AppImage's launcher (`appimage/launcher.c`, built static as `AppRun`) uses your system's SDL2 when it works and the bundled copy otherwise.

`./8tris --selftest` plays a scripted game with the menu and effects, mixes audio and checks the result. The `test/` folder holds the scripted UI, scoring and rendering tests; each is a single `.c` file that includes the game (`gcc -O1 -o t test/click.c -lm`).

Sound is table-driven: to add a voice, add a row to `VOICES` in `src/audio.c`. To add a theme, add its bit to `AVAIL` in `src/look.c`.

## Fonts and licenses

The program's text uses DejaVu Sans (free Bitstream Vera license). The preferences menu can also use Liberation, Carlito, Atkinson Hyperlegible and OpenDyslexic, bundled as Latin subsets under the SIL Open Font License. See `FONTS.txt`. `stb_truetype.h` is public domain. No license has been chosen yet for the 8tris code itself.
