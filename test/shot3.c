#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
static void ppm(const char*name,u32*fb,int w,int h){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
static u32*fb;static double t=1000;static int w=900,h=1700;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void shot(const char*n){char b[100];snprintf(b,100,"/tmp/8tris-test-pref/%s.ppm",n);ppm(b,fb,w,h);}
int main(int argc,char**argv){
  fb=calloc(w*h,4);system("mkdir -p /tmp/8tris-test-pref");
  app_init();app_resize(w,h);run(30);
  menu=1;int i;for(i=0;i<6;i++)sec_open[i]=1;adv_open=1;themeOn=argc>1;weights[2]=1;pWeights[2]=1;
  hist_load();
  run(3);scr_main.scroll=0;shot("p_all");
  scr_main.scroll=1400;run(3);shot("p_all2");
  freq_open=1;run(3);shot("p_freq");freq_open=0;about_open=1;run(3);shot("p_about");
  return 0;
}
