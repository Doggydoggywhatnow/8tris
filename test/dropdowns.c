#include "../src/8tris.c"
// Every dropdown whose value is worked out each frame (window size, UTC and date offset minutes, music track) must apply the
// choice the player clicks. (They used to hand the dropdown a value that no longer existed by the time it was written.)
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){}
static int lastW=0,lastH=0;void plat_window_size(int w,int h){lastW=w;lastH=h;}
int plat_thread(void(*fn)(void*),void*a){fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/mt/";} const char* plat_home_dir(void){return "/tmp/mt/";}
static u32*fb;static double t=1000;static int bad=0;
#define CHK(c) do{if(!(c)){bad++;printf("FAIL line %d: %s\n",__LINE__,#c);}}while(0)
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void click(int x,int y){app_mouse_move(x,y);run(1);app_mouse_button(x,y,1);run(1);app_mouse_button(x,y,0);run(1);}
// Finds the dropdown whose second choice is name1 by clicking down the control column, then picks choice opt.
static int pick(const char*name1,int n,int opt){
  int y,top=0;{int k;for(k=0;k<8;k++)if(sec_open[k])top=1;}(void)top;
  for(y=DY+U(112)+45;y<DY+DHH-U(66)-3;y+=5){if(!menu)return 0;int s0[8];memcpy(s0,sec_open,sizeof s0);click(PX+PW-CWd()/2,y);
    if(sel_open&&getenv("DDV"))printf("y=%d open n=%d [1]=%s\n",y,sel_n,sel_names[1]);
    if(sel_open&&sel_n==n&&!strcmp(sel_names[1],name1)){int ih=(int)(26*UZ*(1.25*uiPt/14.0));   /* the menu draws at its own zoom */{int*sp=sel_ptr;click(sel_x+10,sel_py+opt*ih+ih/2);}return 1;}
    if(sel_open)click(sel_x+5,sel_py-200>0?5:5);if(memcmp(s0,sec_open,sizeof s0)){memcpy(sec_open,s0,sizeof s0);run(2);}}
  return 0;}
int main(void){fb=calloc(1000*900,4);app_init();app_resize(1000,900);run(5);menu=1;
  int k;for(k=0;k<8;k++)sec_open[k]=0;sec_open[4]=1;run(3);
  CHK(pick("800 \xC3\x97 600",6,2));CHK(lastW==1024&&lastH==768);
  for(k=0;k<8;k++)sec_open[k]=0;sec_open[6]=1;run(3);clkTzM=0;
  CHK(pick("15",4,2));CHK(clkTzM==30);
  for(k=0;k<8;k++)sec_open[k]=0;sec_open[2]=1;themeOn=1;CLK[1].on=1;run(3);app_wheel(500);run(3);
  {int y,found=0;for(y=0;y<20&&!found;y++){if(pick("15",4,3)){found=1;break;}app_wheel(-4);run(2);}CHK(found);CHK(clkTzM==45);}
  for(k=0;k<8;k++)sec_open[k]=0;sec_open[3]=1;musMode=1;music_scan();run(3);
  musSel=0;snprintf(musFile,sizeof musFile,"%s",mfiles[0]);run(2);
  {int before=musSel;CHK(nmfiles>=3);CHK(pick(mfiles[1],nmfiles,2));CHK(musSel==2&&musSel!=before&&!strcmp(musFile,mfiles[2]));}
  printf("dropdowns test bad=%d\n",bad);return bad;}
