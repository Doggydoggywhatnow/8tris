#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-hh/";} const char* plat_home_dir(void){return "/tmp/8tris-hh/";}
int main(void){system("rm -rf /tmp/8tris-hh;mkdir -p /tmp/8tris-hh");u32*fb=calloc(1000*900,4);double t=1000;app_init();app_resize(1000,900);int k;
 nhist=3;memset(hist,0,sizeof hist[0]*3);hist[0].t=time(NULL);hist[0].time=3725;hist[0].score=12345;hist[1].t=time(NULL)-9000;hist[1].time=95;hist[1].score=800;hist[2].t=time(NULL)-99999;hist[2].time=7;
 menu=1;sec_open[0]=0;sec_open[6]=1;for(k=0;k<40;k++){t+=16;app_frame(t,fb);}
 FILE*f=fopen("/tmp/8tris-hh/h.ppm","wb");fprintf(f,"P6\n1000 900\n255\n");for(k=0;k<1000*900;k++){u32 x=fb[k];fputc(x>>16,f);fputc(x>>8,f);fputc(x,f);}fclose(f);return 0;}
