#include "../src/8tris.c"
// Screenshots of every Preferences tab, page by page, to /tmp/8tris-prefs/: ./shot_prefs [prefix]
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-prefs/";} const char* plat_home_dir(void){return "/tmp/8tris-prefs/";}
static void ppm(const char*name,u32*fb,int w,int h){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
static u32*fb;static double t=1000;static int w=1000,h=900;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void key(int k){app_key(k,1,0);app_key(k,0,0);run(1);}
int main(int argc,char**argv){const char*pre=argc>1?argv[1]:"p";system("mkdir -p /tmp/8tris-prefs");store_off=1;fb=calloc(w*h,4);app_init();app_resize(w,h);run(10);
 themeOn=1;CLK[0].on=1;CLK[1].on=1;NP.on=1;musMode=1;menu=1;if(argc>2){int g;for(g=0;g<G_N;g++)grp_open[g]=1;}run(3);
 int s,pg,k;for(s=0;s<8;s++){for(k=0;k<8;k++)sec_open[k]=0;sec_open[s]=1;run(3);key(K_HOME);run(2);
   for(pg=0;pg<6;pg++){char p[120];snprintf(p,sizeof p,"/tmp/8tris-prefs/%s-s%d-%d.ppm",pre,s,pg);ppm(p,fb,w,h);int before=scr_main.scroll;key(K_PGDN);run(2);if(scr_main.scroll==before)break;}}
 // the popups
 for(k=0;k<8;k++)sec_open[k]=0;sec_open[2]=1;run(2);partsp_open=1;run(3);{char p[120];snprintf(p,sizeof p,"/tmp/8tris-prefs/%s-parts.ppm",pre);ppm(p,fb,w,h);}partsp_open=0;
 fltp_open=1;run(3);{char p[120];snprintf(p,sizeof p,"/tmp/8tris-prefs/%s-filters.ppm",pre);ppm(p,fb,w,h);}fltp_open=0;
 return 0;}
