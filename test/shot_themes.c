#include "../src/8tris.c"
// Writes screenshots of the flat themes (board and menu) to /tmp/8tris-shots/ to look at: ./shot_themes
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-shots/";} const char* plat_home_dir(void){return "/tmp/8tris-shots/";}
static void ppm(const char*name,u32*fb,int w,int h){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
static u32*fb;static double t=1000;static int w=900,h=640;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
int main(void){system("mkdir -p /tmp/8tris-shots");store_off=1;fb=calloc(w*h,4);app_init();app_resize(w,h);run(20);
 const int ths[3]={TH_DARK,TH_CHOC,TH_MILK};const char*nm[3]={"dark","choc","milk"};int k,i;
 for(k=0;k<3;k++){themeOn=1;for(i=0;i<N_COMPS;i++)TC[i]=ths[k];menu=0;paused=0;
  for(i=0;i<14;i++){app_key(i%2?'a':'d',1,0);app_key(i%2?'a':'d',0,0);app_key(' ',1,0);app_key(' ',0,0);run(4);}
  run(10);char p[80];snprintf(p,sizeof p,"/tmp/8tris-shots/%s-board.ppm",nm[k]);ppm(p,fb,w,h);
  menu=1;run(4);snprintf(p,sizeof p,"/tmp/8tris-shots/%s-menu.ppm",nm[k]);ppm(p,fb,w,h);}
 return 0;}
