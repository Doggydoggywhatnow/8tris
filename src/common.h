// 8tris native: shared types, math helpers and the small platform interface.
// The whole game is one translation unit (see app.c); platform files (plat_sdl.c, plat_win.c) only provide a window, input and audio output.
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#ifdef _WIN32
#include <io.h>        // _commit, used for crash-safe saving
#else
#include <unistd.h>    // fsync
#include <sys/stat.h>  // mkdir, chmod
#endif

// Linux builds ask for the long-standing versions of a few math functions. Newer glibc releases only re-versioned them
// (same results), and pinning the old names lets one build run on older Linux systems too.
#if defined(__linux__)&&defined(__x86_64__)&&defined(__GLIBC__)
__asm__(".symver fmod,fmod@GLIBC_2.2.5");
__asm__(".symver hypot,hypot@GLIBC_2.2.5");
__asm__(".symver exp,exp@GLIBC_2.2.5");
__asm__(".symver log,log@GLIBC_2.2.5");
__asm__(".symver pow,pow@GLIBC_2.2.5");
#endif
typedef uint32_t u32; typedef uint8_t u8; typedef int64_t i64;
#define N_(a) ((int)(sizeof(a)/sizeof((a)[0])))
#undef RGB
#define RGB(r,g,b) (0xFF000000u|((u32)(r)<<16)|((u32)(g)<<8)|(u32)(b))
#define HEX(h) (0xFF000000u|(u32)(h))
static inline int imin(int a,int b){return a<b?a:b;}
static inline int imax(int a,int b){return a>b?a:b;}
static inline int iclamp(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
static inline double dmin(double a,double b){return a<b?a:b;}
static inline double dmax(double a,double b){return a>b?a:b;}
// floor/ceil to int without a library call (the generic x86-64 build has no rounding instruction, so floor() is a function call).
static inline int ifloor(double x){int i=(int)x;return i-(x<(double)i);}
static inline int iceil(double x){int i=(int)x;return i+(x>(double)i);}
static inline double dclamp(double v,double lo,double hi){return v!=v?lo:v<lo?lo:v>hi?hi:v;}

static u32 rs_=0x9E3779B9u;
static inline u32 rnd32(void){rs_^=rs_<<13;rs_^=rs_>>17;rs_^=rs_<<5;return rs_;}
static inline double rnd(void){return rnd32()/4294967296.0;}

// CSS-style hsl(h, s, l): h in degrees, s and l in 0..1.
static u32 hsl(double h,double s,double l){
  h=fmod(h,360.0);if(h<0)h+=360;
  double c=(1-fabs(2*l-1))*s,x=c*(1-fabs(fmod(h/60.0,2)-1)),m=l-c/2,r=0,g=0,b=0;
  if(h<60){r=c;g=x;}else if(h<120){r=x;g=c;}else if(h<180){g=c;b=x;}else if(h<240){g=x;b=c;}else if(h<300){r=x;b=c;}else{r=c;b=x;}
  return RGB((int)((r+m)*255+.5),(int)((g+m)*255+.5),(int)((b+m)*255+.5));
}

// Keys are small ints: printable keys are their lowercase ASCII character, special keys are listed here.
enum{K_NONE=0,K_LEFT=1001,K_RIGHT,K_UP,K_DOWN,K_ESC,K_BKSP,K_DEL,K_ENTER,K_TAB,K_F11,K_PGUP,K_PGDN,K_HOME,K_END,K_SHIFT,K_CTRL,K_ALT};

// ---- What the engine asks of the platform layer ----
void plat_lock(void);            // guards the audio voice list shared with the audio thread
void plat_unlock(void);
void plat_fullscreen(int on);
void plat_quit(void);
void plat_window_size(int w,int h);
int plat_thread(void(*fn)(void*),void*arg);   // starts a background thread; returns 1 on success, 0 if threads are unavailable
void plat_sleep(int ms); // resize the window (ignored in full screen)
const char* plat_pref_dir(void); // writable folder for settings and history (ends with a slash)
const char* plat_home_dir(void); // where exported text files go (ends with a slash)

// ---- What the platform layer calls on the engine (app.c) ----
void app_init(void);
void app_resize(int w,int h);
void app_key(int key,int down,int repeat);
void app_mouse_move(int x,int y);
void app_mouse_button(int x,int y,int down);
void app_wheel(int dy);
void app_focus_lost(void);
void app_frame(double now_ms,u32* fb);   // draws one frame into an ARGB buffer of the current size
void app_audio(float* out,int frames);   // mixes stereo float audio at 44100 Hz
void app_shutdown(void);
int  app_fps_target(void);              // refresh-rate setting: 0 = match the display (vsync), -1 = unlimited, otherwise frames per second
u32* app_render(double now_ms,int* w,int* h); // draws a frame into the game's own buffer (which may be smaller than the window) and returns it
