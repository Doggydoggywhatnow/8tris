#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/p07/";} const char* plat_home_dir(void){return "/tmp/p07/";}
static u32*fb;static double t=1000;static int w=1000,h=720;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void ppm(const char*name){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
int main(void){fb=calloc(1920*1080,4);system("rm -rf /tmp/p07;mkdir -p /tmp/p07");app_init();app_resize(w,h);run(20);
 printf("defaults: fall %.2f rows/s lvl=%d grid %dx%d theming=%d fx=%d vol=%.2f next=%d music=%d timer=%d clock=%d\n",rateOf(fallV),lvlSys,W,H,themeOn,fxMaster,snd_vol,NQ,musMode,CLK[0].on,CLK[1].on);
 int i;for(i=0;i<3;i++){app_key('d',1,0);app_key('d',0,0);}paused=0;over=0;run(200);
 CLK[0].on=1;CLK[0].pos=2;CLK[1].on=1;CLK[1].pos=2;clkTzH=-5;clkDst=1;stackV=1;boardPX=20;boardPY=50;NQ=3;prefBtn=3;
 fltTint=40;fltSat=130;fltHue=30;run(3);ppm("/tmp/p07/a.ppm");
 stackV=0;stackFirst=1;CLK[1].sz=40;CLK[0].fam=4;CLK[0].bold=1;CLK[1].pos=2;run(3);ppm("/tmp/p07/b.ppm");
 CLK[0].pos=6;CLK[1].pos=6;fltTint=0;fltHue=0;fltSplit=6;fltInv=1;run(3);ppm("/tmp/p07/c.ppm");
 fltInv=0;fltSplit=0;fltSat=100;menu=1;sec_open[0]=0;sec_open[2]=1;themeOn=0;run(5);
 {int k;for(k=0;k<5;k++){sec_open[2]=1;wheel_acc=0;}}
 app_wheel(60);run(5);ppm("/tmp/p07/m.ppm");
 /* filter perf at 1080p */
 w=1920;h=1080;app_resize(w,h);menu=0;fltBri=10;fltCon=110;fltSat=120;fltHue=40;fltTint=20;run(10);clock_t c0=clock();run(100);printf("1080p with filters ms/frame %.2f\n",(double)(clock()-c0)*1000/CLOCKS_PER_SEC/100);
 fltSplit=5;run(5);c0=clock();run(100);printf("with split %.2f\n",(double)(clock()-c0)*1000/CLOCKS_PER_SEC/100);
 fltBri=0;fltCon=100;fltSat=100;fltHue=0;fltTint=0;fltSplit=0;run(5);c0=clock();run(100);printf("filters off %.2f\n",(double)(clock()-c0)*1000/CLOCKS_PER_SEC/100);
 return 0;}
