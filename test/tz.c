#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-tz/";} const char* plat_home_dir(void){return "/tmp/8tris-tz/";}
// History dates must not depend on the device's timezone (run this under different TZ values: the output must be identical).
int main(void){char d[32];i64 t=1767225600;   /* 2026-01-01 00:00:00 UTC */
 clkTzH=0;clkTzM=0;clkDst=0;fmt_date(t,d,32);printf("utc:%s ",d);
 clkTzH=-5;fmt_date(t,d,32);printf("-5:%s ",d);clkTzH=5;clkTzM=30;fmt_date(t,d,32);printf("+5:30:%s ",d);clkDst=1;fmt_date(t,d,32);printf("dst:%s\n",d);return 0;}
