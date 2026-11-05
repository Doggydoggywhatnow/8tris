#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
static void ppm(const char*name,u32*fb,int w,int h){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
static u32*fb;static double t=1000;static int w=1000,h=720;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
int main(void){fb=calloc(w*h,4);remove("/tmp/8tris-test-pref/settings.txt");app_init();app_resize(w,h);run(30);
 int i;for(i=0;i<9;i++){app_key(i%2?'a':'d',1,0);app_key(i%2?'a':'d',0,0);app_key(' ',1,0);app_key(' ',0,0);run(4);}
 themeOn=1;for(i=0;i<N_COMPS;i++)TC[i]=TH_SWIRL;fxMaster=4;resolve_fx();paused=0;over=0;run(5);
 clock_t c0=clock();run(60);printf("ms/frame %.2f\n",(double)(clock()-c0)*1000/CLOCKS_PER_SEC/60);
 ppm("/tmp/8tris-test-pref/sw1.ppm",fb,w,h);
 pv_start();run(110);ppm("/tmp/8tris-test-pref/sw2.ppm",fb,w,h);run(150);ppm("/tmp/8tris-test-pref/sw3.ppm",fb,w,h);run(80);pv_stop();
 menu=1;run(3);ppm("/tmp/8tris-test-pref/sw4.ppm",fb,w,h);return 0;}
