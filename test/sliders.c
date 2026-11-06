#include "../src/8tris.c"
// Every slider's value box accepts a typed number: board position (left/right, top/bottom), sound effects (a value that is derived
// each frame), brightness, and negative ranges. Typing past the ends clamps; Escape cancels; the colour picker's RGB sliders stay drag-only.
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;}
int plat_thread(void(*fn)(void*),void*a){fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-sl/";} const char* plat_home_dir(void){return "/tmp/8tris-sl/";}
static u32*fb;static double t=1000;static int bad=0;
#define CHK(c) do{if(!(c)){bad++;printf("FAIL line %d: %s\n",__LINE__,#c);}}while(0)
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void click(int x,int y){app_mouse_move(x,y);run(1);app_mouse_button(x,y,1);run(1);app_mouse_button(x,y,0);run(1);}
static void type(const char*s){for(;*s;s++){app_key(*s,1,0);app_key(*s,0,0);}run(1);}
static void key(int k){app_key(k,1,0);app_key(k,0,0);run(1);}
// Clicks down the value column of one open section until the box showing 'shown' is focused (sections that fit the window need no scrolling).
static int find(int sct,const char*shown,int lo,int hi){
  int y,k;for(k=0;k<8;k++)sec_open[k]=0;sec_open[sct]=1;run(3);
  for(y=DY+U(60);y<DY+DHH-U(40);y+=14){if(!menu)menu=1;int s0[8];memcpy(s0,sec_open,sizeof s0);click(PX+PW-U(36),y);
    if(sel_open)key(K_ESC);if(memcmp(s0,sec_open,sizeof s0)){memcpy(sec_open,s0,sizeof s0);run(2);}
    if(focus_id&&f_ptr&&!f_dptr&&f_lo==lo&&f_hi==hi&&!strcmp(fbuf,shown))return 1;
    if(focus_id){key(K_ESC);}}
  return 0;}
// The same typing and commit path, started directly for sliders that sit below the visible part of their section.
static void direct(int*p,int lo,int hi,const char*digits,int endkey){
  focus_id=0x1234;f_ptr=p;f_dptr=NULL;f_lo=lo;f_hi=hi;f_fresh=1;snprintf(fbuf,sizeof fbuf,"%d",*p);
  type(digits);key(endkey);run(2);}
int main(void){fb=calloc(1000*900,4);system("mkdir -p /tmp/8tris-sl");store_off=1;app_init();app_resize(1000,900);run(5);menu=1;
  // Sound effects (a value derived each frame): the real slider, clicked
  snd_vol=.5;run(3);CHK(find(3,"50",0,100));type("73");key(K_ENTER);run(3);CHK(fabs(snd_vol-.73)<1e-9);
  CHK(find(3,"73",0,100));type("0");key(K_ENTER);run(3);CHK(snd_vol==0);
  CHK(find(3,"0",0,100));type("250");key(K_ENTER);run(3);CHK(fabs(snd_vol-1)<1e-9);   // past the end clamps
  CHK(find(3,"100",0,100));type("40");key(K_ESC);run(3);CHK(fabs(snd_vol-1)<1e-9);     // Escape cancels
  // Board position, brightness (whole game) and a negative range: the typing path on their real variables
  boardPX=37;boardPY=63;gBright=100;fltBri=0;
  direct(&boardPX,0,100,"80",K_ENTER);CHK(boardPX==80&&boardPY==63);
  direct(&boardPY,0,100,"5",K_ENTER);CHK(boardPY==5);
  direct(&boardPX,0,100,"999",K_ENTER);CHK(boardPX==100);
  direct(&boardPY,0,100,"77",K_ESC);CHK(boardPY==5);
  direct(&gBright,25,150,"60",K_ENTER);CHK(gBright==60);
  direct(&gBright,25,150,"5",K_ENTER);CHK(gBright==25);
  direct(&gBright,25,150,"900",K_ENTER);CHK(gBright==150);
  {focus_id=0x1234;f_ptr=&fltBri;f_dptr=NULL;f_lo=-50;f_hi=50;f_fresh=1;snprintf(fbuf,sizeof fbuf,"0");type("-");type("20");key(K_ENTER);run(2);CHK(fltBri==-20);}
  // the board really moves when the position is typed
  gBright=100;menu=0;boardPX=0;run(3);{int x0=BX;boardPX=100;run(3);CHK(BX>x0);}
  printf("sliders test bad=%d\n",bad);return bad;}
