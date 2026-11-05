#include <pthread.h>
#include <unistd.h>
#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} 
static void*tr(void*a){void**p=(void**)a;((void(*)(void*))p[0])(p[1]);return 0;}
int plat_thread(void(*fn)(void*),void*a){static void*p[2];p[0]=(void*)fn;p[1]=a;pthread_t t;return pthread_create(&t,0,tr,p)==0;} void plat_sleep(int ms){usleep(ms*1000);}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
#include <time.h>
static u32*fb;static double t=1000;
static double now_ms(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return ts.tv_sec*1000.0+ts.tv_nsec/1e6;}
static double worst=0;static double bench(void){int i;double tot=0;worst=0;for(i=0;i<120;i++){double a=now_ms();t+=16.7;app_frame(t,fb);double d=now_ms()-a;tot+=d;if(d>worst)worst=d;}return tot/120;}
int main(int argc,char**argv){
  int w=argc>1?atoi(argv[1]):1920,h=argc>2?atoi(argv[2]):1080;fb=calloc((size_t)w*h,4);remove("/tmp/8tris-test-pref/settings.txt");
  app_init();
#ifdef RENDERH
  cfg_render_h=0;
#endif
  app_resize(w,h);int i;for(i=0;i<8;i++){app_key(' ',1,0);app_key(' ',0,0);app_frame(t+=16,fb);}paused=0;
  printf("%dx%d plain 8tris look: %.2f ms\n",w,h,bench());
  themeOn=1;int c;
  for(c=0;c<N_COMPS;c++)TC[c]=TH_MIN;
  for(c=0;c<N_COMPS;c++){TC[c]=TH_SWIRL;printf("  only %-18s swirl: %.2f ms\n",COMP_NAME[c],bench());TC[c]=TH_MIN;}
  for(c=0;c<N_COMPS;c++)TC[c]=TH_SWIRL;{double a=bench();printf("all swirl: avg %.2f ms, worst %.2f ms\n",a,worst);}
  menu=1;{double a=bench();printf("all swirl + menu: avg %.2f ms, worst %.2f ms\n",a,worst);}
  return 0;}
