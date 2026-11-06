#include "../src/common.h"
void plat_lock(void){} void plat_unlock(void){}
#include "../src/audio.c"
static void music_mix(float*o,int n){(void)o;(void)n;}   /* music is tested in music.c */
#include "../src/game.c"
static int fxc=0,pfc=0,brk_=0;
static void fxClear(const int*r,int n){fxc++;} static void fx_break(void){brk_++;} static void pfStart(void){pfc++;} static void fxReset(void){combo=0;} static void endRun(void){}
int main(void){
  levels_init();int n;
  for(n=1;n<=10;n++){gen(n);printf("n=%d count=%d\n",n,LVn[n]);}
  // tetromino identity: all 7 recognised in the 4-block list
  int seen[8]={0};for(int i=0;i<LVn[4];i++){int t=tname(&LVd[4][i]);if(t>=0)seen[t]++;}
  printf("tetromino ids:");for(int i=0;i<7;i++)printf(" %c=%d",TN[i],seen[i]);printf("\n");
  // normal game: spawn & random play
  snd_vol=0;W=10;H=20;weights[4]=1;reset_game();
  printf("spawned piece n=%d N=%d kt=%c x=%d y=%d\n",p.def->n,p.N,p.kt?p.kt:'-',p.x,p.y);
  int steps=0;for(int k=0;k<20000&&!over;k++){int a=(int)(rnd()*4);if(a==0)turn(1);else if(a==1)move_p(-1);else if(a==2)move_p(1);else drop();steps++;}
  printf("random play: steps=%d over=%d pieces=%d lines=%d score=%.0f\n",steps,over,pieces,lines,score);
  // scripted tetris clear: fill bottom 4 rows except column 0, drop I piece vertical
  W=10;H=20;reset_game();for(int y=16;y<20;y++)for(int x=1;x<10;x++)board[y*W+x]=1;
  LVd[4][0];Def*I=NULL;for(int i=0;i<LVn[4];i++)if(tname(&LVd[4][i])==0)I=&LVd[4][i];
  p=mk(I);printf("I piece N=%d kt=%c\n",p.N,p.kt);
  turn(1);while(p.x>0-0&&!hit(&p,p.x-1,p.y)&&0)p.x--;
  // move to column 0 area
  for(int i=0;i<12;i++)move_p(-1);
  printf("I rotated at x=%d rot=%d\n",p.x,p.rot);
  double s0=score;drop();printf("after drop: lines=%d score delta=%.0f combo=%d fx=%d\n",lines,score-s0,combo,fxc);
  // wide board scoring factor
  W=20;H=20;reset_game();for(int x=0;x<20;x++)board[19*W+x]=(x==5?0:1);
  Def*one=&LVd[1][0];p=mk(one);p.x=5;s0=score;drop();printf("20-wide single line score=%.0f (expect 200 + drop pts)\n",score-s0);
  // letters + big sizes
  for(int i=1;i<=11;i++)weights[i]=1;W=30;H=30;reset_game();
  for(int k=0;k<3000&&!over;k++){int a=(int)(rnd()*4);if(a==0)turn(1);else if(a==1)move_p(-1);else if(a==2)move_p(1);else drop();}
  printf("all sets random play: pieces=%d over=%d\n",pieces,over);
  return 0;}
