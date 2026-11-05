#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-tm/";} const char* plat_home_dir(void){return "/tmp/8tris-tm/";}
static u32*fb;static double t=1000;static int w=1000,h=720;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void ppm(const char*name){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
int main(void){fb=calloc(w*h,4);system("rm -rf /tmp/8tris-tm;mkdir -p /tmp/8tris-tm");app_init();app_resize(w,h);run(20);
 timerOn=1;timerPos=2;timerSz=28;timerOp=80;paused=0;over=0;
 int i;for(i=0;i<3;i++){app_key('d',1,0);app_key('d',0,0);}
 run(300);printf("playMs %.0f\n",playMs);ppm("/tmp/8tris-tm/a.ppm");
 timerPos=6;timerSz=60;timerFam=4;timerBold=1;run(2);ppm("/tmp/8tris-tm/b.ppm");
 timerPos=3;timerSz=120;timerOp=30;run(2);ppm("/tmp/8tris-tm/c.ppm");
 /* save/load */ timerPos=4;timerSz=44;timerOp=55;timerFam=2;timerBold=0;save_settings();timerOn=0;timerPos=0;timerSz=12;load_settings();
 printf("loaded on=%d pos=%d sz=%d op=%d fam=%d bold=%d\n",timerOn,timerPos,timerSz,timerOp,timerFam,timerBold);
 /* menu page */ timerOn=1;menu=1;sec_open[0]=0;sec_open[2]=1;themeOn=0;run(5);ppm("/tmp/8tris-tm/m.ppm");
 return 0;}
