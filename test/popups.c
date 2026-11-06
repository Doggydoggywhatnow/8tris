#include "../src/8tris.c"
// Fall rate control (slider + typed number in one row) and the "Theme each part" / "Filters" popups on the Theming tab.
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;}
int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-pp/";} const char* plat_home_dir(void){return "/tmp/8tris-pp/";}
static u32*fb;static double t=1000;static int bad=0,w=1000,h=900;
#define CHK(c) do{if(!(c)){bad++;printf("FAIL line %d: %s\n",__LINE__,#c);}}while(0)
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void click(int x,int y){app_mouse_move(x,y);run(1);app_mouse_button(x,y,1);run(1);app_mouse_button(x,y,0);run(1);}
static void key(int k){app_key(k,1,0);app_key(k,0,0);run(1);}
static void ppm(const char*name){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
static double MZ(void){return UZ*(1.25*uiPt/14.0);}   // the menu's own zoom
int main(void){fb=calloc(w*h,4);system("rm -rf /tmp/8tris-pp;mkdir -p /tmp/8tris-pp");app_init();app_resize(w,h);run(5);menu=1;run(3);
  // find the fall rate number box: the right end of the control column, scanning down the Game section
  int bx=PX+PW-(int)(36*MZ()),y,found=0;
  for(y=DY+(int)(150*MZ());y<DY+DHH-(int)(70*MZ())&&!found;y+=4){click(bx,y);if(focus_id&&f_dptr==&fallV)found=y;else if(focus_id)key(K_ESC);}
  CHK(found);ppm("/tmp/8tris-pp/game.ppm");
  key(K_BKSP);key('3');key('.');key('5');key(K_ENTER);CHK(fabs(rateOf(fallV)-3.5)<1e-6);
  // drag the slider part of the same row to the far left: slowest rate on the slider
  {int sx=PX+PW-CWd()+(int)(4*MZ());app_mouse_move(sx+20,found);run(1);app_mouse_button(sx+20,found,1);run(1);app_mouse_move(sx-200,found);run(2);app_mouse_button(sx-200,found,0);run(1);CHK(fallV==0);}
  // a typed rate beyond the slider's end is kept
  click(bx,found);key(K_BKSP);key('2');key('0');key('0');key(K_ENTER);CHK(fabs(rateOf(fallV)-200)<1e-6&&fallV>100);
  // Theming tab: the popups open, keep the menu input away from the panel behind, and Esc closes only the popup
  int k;for(k=0;k<8;k++)sec_open[k]=0;sec_open[2]=1;themeOn=1;run(3);ppm("/tmp/8tris-pp/theming.ppm");
  partsp_open=1;run(3);ppm("/tmp/8tris-pp/parts.ppm");key(K_ESC);CHK(!partsp_open&&menu);
  fltp_open=1;fltTint=50;run(3);ppm("/tmp/8tris-pp/filters.ppm");key(K_ESC);CHK(!fltp_open&&menu);fltTint=0;
  key(K_ESC);CHK(!menu);
  printf("popups test bad=%d\n",bad);return bad;}
