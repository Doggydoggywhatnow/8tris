// 8tris: the whole game is one translation unit so every module can see the others' state.
// Order matters: each file may use what the files above it define.
#include "common.h"
#include "gfx.c"     // software renderer and text
#include "audio.c"   // sound synthesis
#include "music.c"   // playing music files from the user's music folder
#include "game.c"    // rules: pieces, bags, movement, scoring
#include "song.c"    // the combo song (a song that builds as a combo chain grows)
#include "swirl.c"   // the Swirl theme's drawing helpers
#include "look.c"    // themes, board drawing, combo / perfect clear effects
#include "store.c"   // key bindings, history, saved settings
#include "ui.c"      // settings screen and popups
#include "app.c"     // layout, input routing, the entry points the platform layers call
