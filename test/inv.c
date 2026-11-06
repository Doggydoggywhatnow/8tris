#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-inv/";} const char* plat_home_dir(void){return "/tmp/8tris-inv/";}
static u32*fb;static double t=1000;static int bad=0;
static void run(void){t+=16.7;app_frame(t,fb);}
#define CHK(c,...) do{if(!(c)){if(bad++<12){printf("FAIL %s: ",#c);printf(__VA_ARGS__);printf("\n");}}}while(0)
int main(void){fb=calloc(1000*720,4);system("rm -rf /tmp/8tris-inv;mkdir -p /tmp/8tris-inv");app_init();app_resize(1000,720);run();
 int g,f;long locks=0;
 for(g=0;g<300;g++){
  W=4+rand()%30;H=6+rand()%30;slots=1+rand()%10;
  int i;for(i=1;i<=11;i++)weights[i]=(rand()%3==0)?rand()%5:0;if(rand()%2)weights[4]=2;
  lvlSys=rand()%2;clearOn=rand()%4!=0;

  reset_game();paused=0;over=0;
  int lastPieces=0;
  for(f=0;f<3000&&!over;f++){
   int r=rand()%12;
   if(r==0)app_key('a',1,0);else if(r==1)app_key('d',1,0);else if(r==2)app_key('a',0,0);else if(r==3)app_key('d',0,0);
   else if(r==4)app_key(' ',1,0);else if(r==5)app_key(' ',0,0);else if(r==6){app_key('x',1,0);app_key('x',0,0);}
   else if(r==7){app_key('z',1,0);app_key('z',0,0);}else if(r==8){app_key('c',1,0);app_key('c',0,0);}
   run();
   if(!over&&!paused&&board){
     CHK(!hit(&p,p.x,p.y),"active piece overlaps g=%d f=%d W=%d H=%d",g,f,W,H);
     int n=0,k;for(k=0;k<W*H;k++){if(board[k])n++;}
     CHK(n<=W*H,"cells");
     CHK(score>=0&&score==score,"score %f",score);
     CHK(level>=1&&level<10000,"level %d",level);
     CHK(pieces>=lastPieces,"pieces went backwards");lastPieces=pieces;
     int cells=0;for(k=0;k<p.N*p.N;k++)cells+=((u8*)p.m)[0]*0; int a,b;for(a=0;a<p.N;a++)for(b=0;b<p.N;b++)cells+=p.m[a][b];
     CHK(cells==p.def->n,"piece cell count %d vs %d (rot %d)",cells,p.def->n,p.rot);
   }
  }
  locks+=pieces;
 }
 printf("games done, pieces locked %ld, bad=%d\n",locks,bad);return bad!=0;}
