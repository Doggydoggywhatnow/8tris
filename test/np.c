#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/np/";} const char* plat_home_dir(void){return "/tmp/np/";}
static u32*fb;static double t=1000;static int w=1000,h=720;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void ppm(const char*name){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
int main(void){fb=calloc(w*h,4);app_init();app_resize(w,h);run(5);musMode=2;musVol=40;NP.on=1;NP.pos=3;NP.sz=20;run(5);paused=0;over=0;
 printf("status: %s | title '%s' artist '%s' art %dx%d\n",mus_status,cur_meta.title,cur_meta.artist,cur_meta.aw,cur_meta.ah);ppm("/tmp/np/a.ppm");
 NP.pos=2;NP.fam=5;NP.bold=1;npArtSz=120;NP.sz=26;run(3);ppm("/tmp/np/b.ppm");
 menu=1;sec_open[0]=0;sec_open[3]=1;run(5);app_wheel(10);run(5);ppm("/tmp/np/c.ppm");return 0;}
