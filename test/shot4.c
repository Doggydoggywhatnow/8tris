#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
static void ppm(const char*name,u32*fb,int w,int h){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
static u32*fb;static double t=1000;static int w=900,h=900;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
int main(void){fb=calloc(w*h,4);app_init();app_resize(w,h);run(5);menu=1;sec_open[2]=1;themeOn=1;run(3);scr_main.scroll=0;run(2);ppm("/tmp/8tris-test-pref/q1.ppm",fb,w,h);
 scr_main.scroll=500;run(3);ppm("/tmp/8tris-test-pref/q2.ppm",fb,w,h);advp_open=1;run(3);ppm("/tmp/8tris-test-pref/q3.ppm",fb,w,h);return 0;}
