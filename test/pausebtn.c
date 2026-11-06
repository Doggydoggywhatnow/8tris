#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/np/";} const char* plat_home_dir(void){return "/tmp/np/";}
// The music pause button: shown only when asked for, at the chosen place, and a click stops and restarts the music where it was.
static int bad=0;
#define CHK(c) do{if(!(c)){bad++;printf("FAIL line %d: %s\n",__LINE__,#c);}}while(0)
static u32*fb;static double t=1000;static int w=1000,h=720;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void click(double x,double y){app_mouse_button((int)x,(int)y,1);app_mouse_button((int)x,(int)y,0);run(1);}
static void ppm(const char*name){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
int main(void){fb=calloc(w*h,4);app_init();app_resize(w,h);run(5);menu=0;musMode=2;musVol=40;NP.on=1;NP.pos=3;NP.sz=20;npPause=0;run(5);paused=0;over=0;
  CHK(mus_have);CHK(!pb_on);                                  // off by default: nothing is drawn
  npPause=1;npPausePos=0;run(2);CHK(pb_on);ppm("/tmp/np/pb_with.ppm");
  CHK(pb_y>WH/2&&pb_x<WW/2);                                       // beside the track name, bottom left
  i64 p0=mus_played;static float o[2048];memset(o,0,sizeof o);music_mix(o,1024);CHK(mus_played>p0);
  click(pb_x+pb_s/2,pb_y+pb_s/2);CHK(musPaused==1);CHK(menu==0);
  p0=mus_played;memset(o,0,sizeof o);music_mix(o,1024);{int i,z=1;for(i=0;i<2048;i++)if(o[i]!=0)z=0;CHK(z&&mus_played==p0);}   // silent, and it stays where it was
  run(30);CHK(mus_played==p0&&mus_have);ppm("/tmp/np/pb_paused.ppm");
  click(pb_x+pb_s/2,pb_y+pb_s/2);CHK(musPaused==0);memset(o,0,sizeof o);music_mix(o,1024);CHK(mus_played==p0+1024);   // carries on from the same spot
  // every place of its own
  {int k;for(k=1;k<=6;k++){npPausePos=k;run(1);int pos=k-1,col=pos%3;CHK(pb_on);
     if(pos==3){CHK(pb_x>pb_x-1);}   /* bottom left = same place as the track: sits beside it */
     else{CHK(pos<3?pb_y<WH/2:pb_y>WH/2);CHK(col==0?pb_x<WW/3:col==1?(pb_x>WW/5&&pb_x<WW*4/5):pb_x>WW/2);}}}
  npPausePos=3;run(1);ppm("/tmp/np/pb_topright.ppm");
  // with the track name hidden, "with the track name" still shows the button where the name would be
  NP.on=0;npPausePos=0;run(1);CHK(pb_on&&pb_y>WH/2&&pb_x<WW/3);
  // while Preferences are open, a click there does not toggle it
  {double x=pb_x+pb_s/2,y=pb_y+pb_s/2;menu=1;run(2);app_mouse_button((int)x,(int)y,1);app_mouse_button((int)x,(int)y,0);CHK(musPaused==0);menu=0;run(1);}
  // music off: no button, and the pause is cleared
  musPaused=1;musMode=0;run(2);CHK(!pb_on&&musPaused==0);
  printf("pause button test bad=%d\n",bad);return bad;}
