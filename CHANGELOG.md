# Changelog

Versions are `major.minor.patch`. Until about ten changes have built up, new features are shipped as patch releases, so a patch number here can include features. This file is the real record of what changed.

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
