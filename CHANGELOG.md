# Changelog

Versions are `major.minor.patch`. Until about ten changes have built up, new features are shipped as patch releases, so a patch number here can include features. This file is the real record of what changed.

## 0.7.8
- The repository no longer contains any shell scripts. The AppImage launcher is now a tiny static C program (`appimage/launcher.c`, same behavior: system SDL2 first, bundled copy as a fallback), and the two build commands are in the README instead of `build.sh`.
- New test: `test/compat.c` checks that settings files written by older versions (or damaged by hand) load without losing anything they contain, never disturb settings they do not mention, clamp out-of-range values, and that save then load is lossless for every key.

## 0.7.7
- The combo song's description no longer gives away what happens along the way (here and in the README too).

## 0.7.6
- New theme **8tris Dark:** black background, dark grey blocks with slightly lighter grey outlines, and a faint grey grid. Muted grey text, buttons and effects.
- New theme **Chocolate:** dark chocolate brown backgrounds with light (milk chocolate) brown pieces and outlines, a subtle brown grid, and cream text and effects.
- Both cover every part (page, side panel, menu, board frame, board, grid, pieces, landing outline, effect colors) and use the 8tris sound voice. Theme order is now 8tris, 8tris Light, 8tris Dark, Graphite, Chocolate, Classic, Solid Color, Swirl, Wild.

## 0.7.5
- **Combo song** (Preferences, Sound): a tick box that replaces the combo sounds with a song that builds for as long as your combo chain lasts, into the hundreds, with a few surprises along the way. Breaking the chain ends the song. It plays at every Effects level, in stereo, through its own limiter so it can get loud as it builds; all other sounds work as before. History records whether it was on.
- Sound engine: room for twice as many notes at once, held notes for pads and melodies, and a stereo bus with its own reverb for the song.
- New test: `test/song.c` (plays a chain offline, checks the mix, and can write it to a WAV file).

## 0.7.4
- **Theming tab is shorter:** the per-part theme choices moved into a **Theme each part** popup, and the filters into a **Filters** popup. Both open from the Theming tab, show changes live, and close with Done or Esc. The presets stay on the tab.
- **Fall rate is one control:** drag the slider, or click the number next to it and type an exact rate in rows per second (typed rates can go past the slider's ends, as before).
- New test: `test/popups.c`; `test/order.c` now checks the per-part list in its popup.

## 0.7.3
Saving, privacy and a code audit. No features were removed.
- **History is now a plain text file** (`history.txt`, one run per line). Older and newer versions can read each other's history: unknown details are skipped and missing ones are left blank, instead of the whole history being ignored. The old `history.bin` is converted on first launch, checked, and then removed, so "Clear history" really clears everything.
- **Crash-safe saving:** preferences, history and history exports are written to a temporary file and swapped in with one rename, so a crash or power cut can no longer leave a half-written file.
- **Windows:** the data folder moved from `%APPDATA%\8tris` (which can be synced to a server on work and school networks) to `%LOCALAPPDATA%\8tris`, which stays on this computer. The old folder, music included, is moved over once on first launch.
- **Linux:** the data folder is now readable only by your own account (it was readable by other accounts on the same computer).
- **Linux AppImage runs on older systems:** it now uses the system's SDL2 when there is one, and the game itself needs glibc 2.34 instead of 2.38 (it falls back to its bundled libraries otherwise).
- `--selftest` no longer adds a test game to your history or reads your preferences.
- Fixed: a rare timing race could start a sound before it was fully set up, which could silence all sound until restart. Sound also can no longer be stuck silent by a single bad sample.
- Fixed: four dropdowns could lose your choice: Window size, the clock's UTC minutes, the music Track, and the History date offset minutes. They handed the menu a value that no longer existed by the time your click was applied. New test: `test/dropdowns.c`.
- Fixed: music files whose names contain control characters (such as a line break) are skipped instead of risking the preferences file.
- Faster: sound synthesis is about 3 times cheaper (same output), and the Wild theme takes about 40% less time to draw at 1080p.
- Preferences are now written at most a few times per second while you drag a slider (before: every frame), and right away when the menu closes.
- Trimmed: unused variables, duplicated lists of font and position names, a leftover debug test, and the old downloads (0.6.5 to 0.7.1). Test programs that no longer built were fixed. New test: `test/histfmt.c`. The click test now always starts from default preferences.

## 0.7.2
- Repository only, no new program build (the 0.7.1 downloads are still the current ones): added `.gitattributes` so GitHub shows 8tris as the C program it is. Header files were being counted as C++, and bundled third-party libraries and generated font data are now marked as vendored and generated.

## 0.7.1
- **Privacy fix:** dates in History (and the name of an exported history file) no longer use the device's timezone. They are shown in UTC plus the offset you type in, the same offset the real-time clock uses (default: UTC). Set it in Preferences, History or under Theming, Real-time clock. 8tris now never reads the device's timezone or location anywhere.
- Added `test/tz.c`, which checks that dates come out identical whatever timezone the device is set to.

## 0.7.0
- **Music from your own files.** Preferences, Sound: put .mp3, .ogg, .wav or .flac files (up to 10 minutes each) in the music folder, then play one track on repeat, all in order, or shuffled, with its own volume. Files are decoded locally and never copied or sent anywhere.
- **Now playing on screen** (optional): title, artist and album, plus album art, read from ID3v2 and ID3v1 (mp3), Vorbis comments and pictures (flac, ogg) and LIST INFO (wav); falls back to the file name. Position, size, opacity, font and bold are adjustable.
- **Filters** in Theming: brightness, contrast, saturation, hue shift, color split, tint (any color), picture opacity and invert. They apply over any theme, not to the menu.
- **Game timer and real-time clock** in Theming, each with position, size, opacity, font and bold. The clock uses a UTC offset you type in (plus a daylight saving switch, 12 or 24 hour, optional seconds). The two can stack side by side or one above the other.
- **Board position:** slide the board anywhere in the play area (Display).
- **Preferences button** can be moved (Display), and every "Settings" in the interface now reads "Preferences". Its label was changed in 0.6.6.
- **Next pieces shown:** choose 0 to 10 upcoming pieces (Game).
- **Default first impression is now minimal:** slow fall rate, level system off, 10 x 20 board, the 8tris theme, effects and audio at "A bit". Saved preferences from older versions are kept as they were.
- Numbers in the menu fields now accept a minus sign where negative values make sense.
- Third-party code added: dr_mp3, dr_flac, dr_wav, stb_vorbis and stb_image (public domain or MIT-0).

## 0.6.7
- History now shows the length of each game right in the list, next to the date (it was only visible after opening an entry). Games over an hour show as h:mm:ss. Paused time is not counted.

## 0.6.6
- The **Settings** button in the side panel now reads **Preferences** (slightly wider button, smaller text so it fits).
- Bug hunt: ran the game under memory and undefined-behaviour checkers with randomised settings files, damaged history files, random window sizes and thousands of random key and mouse events, plus 300 random games checking the rules. No crashes found.
- Fixed: a damaged history file with an out-of-range theme number could read past the theme list when shown in History.
- Fixed: a non-numeric value such as `nan` in a hand-edited settings file could stall the fall speed.
- Added fuzz, rules-invariant and settings round-trip tests to `test/`.

## 0.6.5
- Theme **Classic colors** is now just **Classic**, and **Solid color** is now **Solid Color**.
- Themes are listed in a new order everywhere: 8tris, 8tris Light, Graphite, Classic, Solid Color, Swirl, Wild. Saved settings are unaffected.

## 0.6.4
- New **Graphite** theme, available for every part (page, side panel, settings menu, board frame, board, grid, pieces, landing outline, effects, sound): the 8tris look in light grey on white, with a light grey grid.

## 0.6.3
- Swirl theme is much faster: backgrounds are built on a worker thread and the frame only copies them. At 1080p with everything on Swirl, the average frame went from about 12 ms to about 3 ms, and from about 23 ms to about 6 ms with the settings menu open.
- Full-screen alpha blending (such as the dim behind the settings menu) is about four times faster.
- Swirl code rewritten into clearly labeled sections.
- Fix: S and Z pieces (and every other piece) in the next-piece preview and hold area now show in the orientation they spawn in.

## 0.6.2
- Swirl theme cleaned up: calmer board background, nearly straight grid lines, thicker landing outline, tidier braided borders at the corners, fewer background rings, readable Settings button.

## 0.6.1
- New **8tris Light** theme (white with black pieces and borders), available for every part.
- Settings menu is larger and wider, with about 25% bigger text.
- Settings sections now have quick-jump tabs, work like an accordion, and scroll with PgUp, PgDn, arrows, Home and End.
- New **Appearance** section: settings menu font (DejaVu Sans, Liberation Sans, Carlito, Liberation Serif, Liberation Mono, Atkinson Hyperlegible, OpenDyslexic), bold, italic, text size in pt, menu width, line spacing, high contrast menu, landing outline strength, and a Reduce motion shortcut. None of this affects text during play.

## 0.6.0
- New **Solid color** theme for pieces and the landing outline, with a hex field and a full color picker. Optional separate colors for each in Advanced effect options.

## 0.5.0
- Swirl theme runs about twice as fast and its backgrounds are smoother (no banding).
- New **Display** settings: window size, full screen, render resolution, refresh rate and a frame counter.

## 0.4.0
- T-spins score the same as tetrises.
- New **Swirl** theme: curves, loops and spirals, with its own odd random palette each launch and its own sounds.

## 0.3.0
- Period added to the epilepsy caution.
- Fall rate can be typed exactly.
- Advanced effect options moved into a popup inside Theming.
- "Reset to 10 x 20" row for the standard board size.

## 0.2.1
- Fix: Apply and restart, Resume and the other settings buttons now work.
- Fix: restarting the game works.

## 0.2.0
- Rewritten as a native C program: no Chromium or Electron, about 2 MB, with a Linux AppImage and a Windows exe.

## 0.1.0
- First version, a web page packaged with Electron.
