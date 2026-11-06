#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
static void ppm(const char*name,u32*fb,int w,int h){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
static u32*fb;static double t=1000;static int w=1000,h=900;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void click(int x,int y){app_mouse_move(x,y);run(1);app_mouse_button(x,y,1);run(1);app_mouse_button(x,y,0);run(1);}
int main(void){fb=calloc(w*h,4);remove("/tmp/8tris-test-pref/settings.txt");app_init();app_resize(w,h);run(30);
 menu=1;sec_open[0]=0;sec_open[2]=1;themeOn=1;run(5);ppm("/tmp/8tris-test-pref/ord.ppm",fb,w,h);
 // the per-part dropdowns live in the "Theme each part" popup; the list must follow THEME_ORDER and a pick must set the right theme
 partsp_open=1;run(3);
 {int y,ok=0;double mz=UZ*(1.25*uiPt/14.0);int ih=(int)(26*mz);
  for(y=DY+(int)(60*mz);y<DY+DHH&&!ok;y+=4){click(PX+PW-CWd()/2,y);
    if(sel_open&&sel_n==N_THEMES&&!strcmp(sel_names[1],"8tris Light")){ok=y;ppm("/tmp/8tris-test-pref/ord2.ppm",fb,w,h);
      click(sel_x+10,sel_py+5*ih+ih/2);run(3);printf("page=%d (want %d swirl)\n",TC[C_PAGE],TH_SWIRL);
      click(PX+PW-CWd()/2,y);run(2);click(sel_x+10,sel_py+2*ih+ih/2);run(3);printf("page=%d (want %d graphite)\n",TC[C_PAGE],TH_GRAPHITE);}
    else if(sel_open)click(5,5);}
  if(!ok)printf("FAIL: page background dropdown not found\n");}
 int i;for(i=0;i<N_THEMES;i++)printf("%s ",THEME_NAME[THEME_ORDER[i]]);printf("\n");return 0;}
