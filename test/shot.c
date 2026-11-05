// Headless check: renders frames of the game to PPM files and an audio WAV, with a simple scripted play-through.
#include "../src/8tris.c"
static int frames_written=0;
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
static void ppm(const char*name,u32*fb,int w,int h){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
int main(int argc,char**argv){
  int w=1000,h=720;u32*fb=calloc(w*h,4);system("mkdir -p /tmp/8tris-test-pref");
  app_init();app_resize(w,h);double t=1000;int i;
  for(i=0;i<600;i++){t+=16.7;if(i%7==0)app_key('d',1,0),app_key('d',0,0);if(i%11==0)app_key('j',1,0),app_key('j',0,0);if(i%23==0)app_key(' ',1,0),app_key(' ',0,0);app_frame(t,fb);}
  ppm("/tmp/8tris-test-pref/game.ppm",fb,w,h);
  app_key(K_ESC,1,0);app_key(K_ESC,0,0);for(i=0;i<5;i++){t+=16.7;app_frame(t,fb);}ppm("/tmp/8tris-test-pref/menu.ppm",fb,w,h);
  (void)argc;(void)argv;return 0;
}
