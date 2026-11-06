#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-bh/";} const char* plat_home_dir(void){return "/tmp/8tris-bh/";}
int main(void){system("rm -rf /tmp/8tris-bh;mkdir -p /tmp/8tris-bh");FILE*f=fopen("/tmp/8tris-bh/history.bin","wb");unsigned m=0x38544831u,sz=sizeof(Hist);int n=MAXHIST,i;fwrite(&m,4,1,f);fwrite(&sz,4,1,f);fwrite(&n,4,1,f);for(i=0;i<(int)(n*sizeof(Hist));i++)fputc(rand(),f);fclose(f);
 u32*fb=calloc(1000*900,4);double t=1000;app_init();app_resize(1000,900);menu=1;sec_open[0]=0;sec_open[6]=1;int k;
 for(k=0;k<60;k++){t+=16;app_frame(t,fb);}
 for(k=0;k<40;k++){int y=300+k*12;app_mouse_move(500,y);app_mouse_button(500,y,1);app_mouse_button(500,y,0);app_wheel(5);t+=16;app_frame(t,fb);app_frame(t+16,fb);t+=32;}
 char out[40][400];for(k=0;k<nhist;k++)entry_lines(&hist[k],out,40);
 puts("badhist ok");return 0;}
