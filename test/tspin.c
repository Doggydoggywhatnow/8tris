#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
#define CHECK(c) do{if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);bad++;}}while(0)
static int bad=0;
static Def*tdef(void){int i;for(i=0;i<nlists[4];i++)if(tname(lists[4][i])==2)return lists[4][i];return NULL;}
// Puts a downward-pointing T at (3,17) over a slot, optionally with a third corner block, and locks it.
static double try_lock(int rot,int corner,int rows2){
  int x,y;reset_game();paused=0;score=0;b2b=0;combo=0;memset(board,0,W*H);
  for(x=0;x<W;x++){if(x!=4)board[19*W+x]=1;if(x<3||x>5)board[18*W+x]=1;}
  if(!rows2)for(x=0;x<W;x++)board[19*W+x]=(x==0?0:board[19*W+x]),board[19*W+4]=0;
  if(corner)board[17*W+3]=1;board[0]=1;
  p.def=tdef();p.N=3;p.ci=2;memset(p.m,0,sizeof p.m);p.m[1][0]=p.m[1][1]=p.m[1][2]=1;p.m[2][1]=1;p.x=3;p.y=17;lastRot=rot;
  double s0=score;lock_piece();return score-s0;
}
int main(void){
  app_init();app_resize(900,700);menu=0;W=10;H=20;lvlSys=1;
  double tsd=try_lock(1,1,1),plain=try_lock(0,1,1),nocorner=try_lock(1,0,1);
  printf("T-spin double %.0f | same placement without rotation %.0f | rotation but only 2 corners %.0f\n",tsd,plain,nocorner);
  CHECK(tsd==800);CHECK(plain==300);CHECK(nocorner==300);
  // back-to-back: a second T-spin right after is worth 1.5x (combo adds 10 per step)
  {double a=try_lock(1,1,1);b2b=1;memset(board,0,W*H);int x;for(x=0;x<W;x++){if(x!=4)board[19*W+x]=1;if(x<3||x>5)board[18*W+x]=1;}board[17*W+3]=1;board[0]=1;
   p.def=tdef();p.N=3;p.ci=2;memset(p.m,0,sizeof p.m);p.m[1][0]=p.m[1][1]=p.m[1][2]=1;p.m[2][1]=1;p.x=3;p.y=17;lastRot=1;combo=0;double s0=score;lock_piece();printf("B2B T-spin double %.0f (first %.0f)\n",score-s0,a);CHECK(score-s0==1200);}
  // a tetris (I piece, 4 rows) is worth the same as a T-spin double
  {reset_game();paused=0;score=0;b2b=0;memset(board,0,W*H);int x,y;for(y=16;y<20;y++)for(x=0;x<W;x++)if(x!=9)board[y*W+x]=1;board[0]=1;
   Def*d=NULL;int i;for(i=0;i<nlists[4];i++)if(lists[4][i]->n==4){Def*q=lists[4][i];int mr=0,mc=0,k;for(k=0;k<4;k++){mr=imax(mr,q->r[k]);mc=imax(mc,q->c[k]);}if(mr==3&&mc==0)d=q;if(mr==0&&mc==3&&!d)d=q;}
   p=mk(d);memset(p.m,0,sizeof p.m);for(i=0;i<4;i++)p.m[i][0]=1;p.N=4;p.x=9;p.y=16;lastRot=0;double s0=score;lock_piece();printf("tetris %.0f\n",score-s0);CHECK(score-s0==800);}
  // movement and falling cancel the T-spin flag
  {reset_game();paused=0;p=mk(tdef());lastRot=0;turn(1);int r1=lastRot;move_p(1);int r2=lastRot;turn(1);int r3=lastRot;fall();int r4=lastRot;printf("lastRot after rotate %d, move %d, rotate %d, fall %d\n",r1,r2,r3,r4);CHECK(r1==1&&r2==0&&r3==1&&r4==0);}
  // line clearing off: no T-spin points
  clearOn=1;
  printf("%s\n",bad?"FAILED":"all ok");return bad;
}
