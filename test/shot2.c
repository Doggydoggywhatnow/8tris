#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
static void ppm(const char*name,u32*fb,int w,int h){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
static u32*fb;static double t=1000;static int w=1000,h=720;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void click(int x,int y){app_mouse_move(x,y);run(1);app_mouse_button(x,y,1);run(1);app_mouse_button(x,y,0);run(1);}
static void shot(const char*n){char b[100];snprintf(b,100,"/tmp/8tris-test-pref/%s.ppm",n);ppm(b,fb,w,h);}
int main(void){
  fb=calloc(w*h,4);system("mkdir -p /tmp/8tris-test-pref");
  app_init();app_resize(w,h);run(60);
  int i;for(i=0;i<14;i++){app_key(i%2?'a':'d',1,0);app_key(i%2?'a':'d',0,0);app_key(' ',1,0);app_key(' ',0,0);run(5);}
  click(960,700);run(3);                     // open settings
  click(500,87);                            // collapse Game
  click(500,121);                           // open Pieces
  shot("s_pieces");
  click(480,200);                           // maybe Set frequencies
  shot("s_freq");
  app_key(K_ESC,1,0);app_key(K_ESC,0,0);run(2);
  click(500,121);click(500,155);            // collapse pieces, open theming
  shot("s_theming");
  themeOn=1;for(i=0;i<N_COMPS;i++)TC[i]=TH_WILD;fxMaster=4;run(3);shot("s_wild");
  app_key(K_ESC,1,0);run(2);shot("s_wild_game");
  pv_start();run(60);shot("pv1");run(100);shot("pv2");run(60);shot("pv3");run(120);shot("pv4");run(200);shot("pv5");
  return 0;
}
