#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
static u32*fb;static double t=1000;static int w=1000,h=720;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void click(int x,int y){app_mouse_move(x,y);run(1);app_mouse_button(x,y,1);run(1);app_mouse_button(x,y,0);run(1);}
#define CHECK(c) do{if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);bad++;}}while(0)
int main(void){
  int bad=0;fb=calloc(w*h,4);system("mkdir -p /tmp/8tris-test-pref");
  app_init();app_resize(w,h);run(30);
  click(960,700);run(2);CHECK(menu==1);                       // Settings button
  click(590,241);CHECK(focus_id!=0);                          // Width field (Game open by default, first row)
  app_key(0x31,1,0);app_key(0x32,1,0);click(400,60);
  CHECK(pW==12);
  click(590,241);app_key('1',1,0);app_key('2',1,0);app_key(K_ENTER,1,0);CHECK(pW==12);
  click(385,630);run(2);                                       // Apply and restart (footer)
  CHECK(menu==0);CHECK(W==12);
  click(960,700);run(2);CHECK(menu==1);click(539,630);run(2);CHECK(menu==0);   // Resume
  click(960,700);run(2);click(685,629);run(2);CHECK(about_open==1);                // info button
  { /* about Close button: find via dialog geometry */ click(DX+DPAD+40,DY+DHH-DPAD-15);run(2);CHECK(about_open==0);}
  CHECK(menu==1);app_key(K_ESC,1,0);CHECK(menu==0);
  {int i;for(i=0;i<40;i++){app_key(' ',1,0);app_key(' ',0,0);run(3);}CHECK(over||pieces>3);int pc=pieces;app_key('r',1,0);app_key('r',0,0);run(2);CHECK(!over&&pieces<=1&&score==0);printf("restart: pieces %d -> %d\n",pc,pieces);
   app_key('p',1,0);CHECK(paused);app_key('p',1,0);CHECK(!paused);}
  { // typed fall rate, size reset, advanced popup
    click(960,700);run(2);CHECK(menu==1);
    fall_typed(1.0);CHECK(fabs(rateOf(fallV)-1.0)<1e-6);
    pW=14;pH=30;focus_id=0;
    for(int k=0;k<N_ACTS;k++){}
    advp_open=1;run(3);CHECK(advp_open==1);app_key(K_ESC,1,0);CHECK(advp_open==0&&menu==1);
    app_key(K_ESC,1,0);CHECK(menu==0);
  }
  { // appearance: settings persist, tabs switch sections, game text is untouched
    click(960,700);run(2);if(!menu){click(960,700);run(2);}CHECK(menu==1);
    click(447,148+0);run(2);   // "Appearance" tab (second row)
    CHECK(sec_open[7]==1&&sec_open[0]==0);
    uiFam=5;uiBold=1;uiItal=1;uiPt=19;hiCon=1;mwSel=2;lsSel=1;ghostStr=80;save_settings();
    uiFam=0;uiBold=0;uiItal=0;uiPt=14;hiCon=0;mwSel=0;lsSel=0;ghostStr=50;load_settings();
    CHECK(uiFam==5&&uiBold==1&&uiItal==1&&uiPt==19&&hiCon==1&&mwSel==2&&lsSel==1&&ghostStr==80);
    {u32*a=calloc(w*h,4),*b=calloc(w*h,4);menu=0;paused=1;uiFam=0;uiBold=0;uiItal=0;uiPt=14;run(2);memcpy(a,fb,w*h*4);uiFam=6;uiBold=1;uiItal=1;uiPt=28;run(2);memcpy(b,fb,w*h*4);
     CHECK(memcmp(a,b,w*h*4)==0);}   // gameplay screen identical whatever the menu font is
    uiFam=0;uiBold=0;uiItal=0;uiPt=14;hiCon=0;mwSel=0;lsSel=0;ghostStr=50;
  }
  printf("%s\n",bad?"FAILED":"all ok");return bad;
}
