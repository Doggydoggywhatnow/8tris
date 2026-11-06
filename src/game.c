// ===== RULES =====
// Pieces, board, input handling, scoring, lock delay, levels and the score history. No drawing happens here.
#define MAXN 16
typedef struct{int n;int r[MAXN],c[MAXN];int ci;int letter;}Def;
typedef struct{Def*def;int ci,N,rot,kt,x,y;u8 m[10][10];}Piece;

static double NOW=0;                        // current time in ms, set once per frame by app_frame
static const u32 COL[10]={0xFF2fb8c9,0xFFe8b92e,0xFF9a5fd1,0xFF4bb86a,0xFFe2554f,0xFF3f6fd8,0xFFee8a3c,0xFFd95fa8,0xFF8bc34a,0xFFa1887f};

// ---- Polyominoes (rotations are the same piece, mirror images are different), built on demand one block at a time ----
static Def*LVd[11];static int LVn[11];
static void norm_cells(int n,int*r,int*c){
  int mr=1000,mc=1000,i,j;
  for(i=0;i<n;i++){if(r[i]<mr)mr=r[i];if(c[i]<mc)mc=c[i];}
  for(i=0;i<n;i++){r[i]-=mr;c[i]-=mc;}
  for(i=1;i<n;i++){int rr=r[i],cc=c[i];j=i-1;while(j>=0&&(r[j]>rr||(r[j]==rr&&c[j]>cc))){r[j+1]=r[j];c[j+1]=c[j];j--;}r[j+1]=rr;c[j+1]=cc;}
}
// Canonical form: the smallest key over the four rotations. Returns the key and the normalized cells.
static void canon(int n,const int*r0,const int*c0,int*or_,int*oc_,u8*key){
  int r[MAXN],c[MAXN],k,i;u8 best[MAXN];int have=0;
  memcpy(r,r0,n*sizeof(int));memcpy(c,c0,n*sizeof(int));
  for(k=0;k<4;k++){
    int tr[MAXN],tc[MAXN];u8 kk[MAXN];memcpy(tr,r,n*sizeof(int));memcpy(tc,c,n*sizeof(int));norm_cells(n,tr,tc);
    for(i=0;i<n;i++)kk[i]=(u8)(tr[i]*16+tc[i]);
    if(!have||memcmp(kk,best,n)<0){memcpy(best,kk,n);memcpy(or_,tr,n*sizeof(int));memcpy(oc_,tc,n*sizeof(int));have=1;}
    for(i=0;i<n;i++){int t=r[i];r[i]=c[i];c[i]=-t;}
  }
  memcpy(key,best,n);
}
static void gen(int n){
  static int tab[1<<16];static u8 keys[12000][10];
  while(LVd[n]==NULL){
    int m=0,i;for(i=2;i<=10;i++)if(LVd[i]==NULL){m=i;break;}
    if(m==0)return;
    int prev=m-1,nout=0,cap=LVn[prev]*(prev*4)+8;Def*out=(Def*)calloc(cap<12000?cap:12000,sizeof(Def));
    memset(tab,0,sizeof tab);
    int di,ci_;
    for(di=0;di<LVn[prev];di++){
      Def*d=&LVd[prev][di];
      for(ci_=0;ci_<d->n;ci_++){
        static const int dd[4][2]={{1,0},{-1,0},{0,1},{0,-1}};int q;
        for(q=0;q<4;q++){
          int rr=d->r[ci_]+dd[q][0],cc=d->c[ci_]+dd[q][1],occ=0;
          for(i=0;i<d->n;i++)if(d->r[i]==rr&&d->c[i]==cc){occ=1;break;}
          if(occ)continue;
          int tr[MAXN],tc[MAXN],orr[MAXN],occ2[MAXN];u8 key[MAXN];
          memcpy(tr,d->r,d->n*sizeof(int));memcpy(tc,d->c,d->n*sizeof(int));tr[d->n]=rr;tc[d->n]=cc;
          canon(m,tr,tc,orr,occ2,key);
          unsigned h=2166136261u;for(i=0;i<m;i++)h=(h^key[i])*16777619u;
          unsigned slot=h&((1<<16)-1);int found=0;
          while(tab[slot]){if(memcmp(keys[tab[slot]-1],key,m)==0){found=1;break;}slot=(slot+1)&((1<<16)-1);}
          if(found)continue;
          if(nout>=12000)continue;
          memcpy(keys[nout],key,m);tab[slot]=nout+1;
          Def*o=&out[nout];o->n=m;memcpy(o->r,orr,m*sizeof(int));memcpy(o->c,occ2,m*sizeof(int));o->ci=nout%10;o->letter=0;nout++;
        }
      }
    }
    LVd[m]=out;LVn[m]=nout;
    if(m>=n)break;
  }
}
// ---- Letters A-Z drawn on a small pixel grid; every glyph is one connected piece ----
static const char*GL_[26]={"###/#.#/###/#.#/#.#","##./#.#/###/#.#/##.","###/#../#../#../###","###/#.#/#.#/#.#/##.","###/#../###/#../###","###/#../###/#../#..","###/#../#.#/#.#/###","#.#/#.#/###/#.#/#.#","###/.#./.#./.#./###","..#/..#/..#/#.#/###","#..#/#.##/###./#.##/#..#","#../#../#../#../###","#####/#.#.#/#.#.#/#...#/#...#","###/#.#/#.#/#.#/#.#","###/#.#/#.#/#.#/###","###/#.#/###/#../#..","###/#.#/#.#/###/..#","###/#.#/###/##./.##","###/#../###/..#/###","###/.#./.#./.#./.#.","#.#/#.#/#.#/#.#/###","#...#/#...#/##.##/.###./..#..","#...#/#...#/#.#.#/#####/.#.#.","#.#/###/.#./###/#.#","#.#/#.#/###/.#./.#.","###/..#/.##/##./###"};
static Def LET[26];
static void letters_init(void){
  int i;for(i=0;i<26;i++){Def*d=&LET[i];d->n=0;d->letter='A'+i;d->ci=i%10;int r=0,c=0;const char*s=GL_[i];
    for(;*s;s++){if(*s=='/'){r++;c=0;}else{if(*s=='#'){d->r[d->n]=r;d->c[d->n]=c;d->n++;}c++;}}}
}
// ---- Settings that shape a run ----
static int W=10,H=20,slots=1,NQ=5;
static int weights[12]={0,0,0,0,1,0,0,0,0,0,0,0};  // relative frequency of each piece set (1..10 blocks, 11 letters)
static int tw[7]={1,1,1,1,1,1,1};                   // relative frequency of I O T S Z J L inside the 4-block set
static const char TN[8]="IOTSZJL";
static int tkeys_ready=0;static u8 TKEY[7][4];
static int tname(Def*d){
  if(!tkeys_ready){
    static const int ref[7][8]={{0,0,0,1,0,2,0,3},{0,0,0,1,1,0,1,1},{0,0,0,1,0,2,1,1},{0,1,0,2,1,0,1,1},{0,0,0,1,1,1,1,2},{0,0,1,0,1,1,1,2},{0,2,1,0,1,1,1,2}};
    int k,i;for(k=0;k<7;k++){int r[4],c[4],orr[4],oc[4];for(i=0;i<4;i++){r[i]=ref[k][i*2];c[i]=ref[k][i*2+1];}canon(4,r,c,orr,oc,TKEY[k]);}
    tkeys_ready=1;
  }
  if(d->n!=4)return -1;
  int r[MAXN],c[MAXN],orr[4],oc[4];u8 key[MAXN];memcpy(r,d->r,16);memcpy(c,d->c,16);canon(4,r,c,orr,oc,key);
  int k;for(k=0;k<7;k++)if(memcmp(key,TKEY[k],4)==0)return k;return -1;
}
static int tCustom(void){int i;for(i=1;i<7;i++)if(tw[i]!=tw[0])return 1;return 0;}

// ---- Board and run state ----
static u8*board=NULL;static Piece p;static Def*holds[10];static int sel=0,canHold=1;
static double score=0;static int lines=0,level=1,over=0,paused=0,menu=0;static double acc=0,last_t=0;
static int dir=0,hl=0,hr=0,sd=0;static double mt=0,st=0,lockT=0;static int resets=0,low=0,b2b=0;
static double fallV=22;   // fall rate position: rate = 0.2 * 100^(fallV/100) rows/s; the slider covers 0..100, a typed rate may lie outside
static int lvlSys=0,sdRate=22,ovPlayed=0,quiet=0,clearOn=1,cellsCleared=0,pieces=0,maxCombo=0,runDirty=0,saved=0;static double playMs=0;
static int usable[12],nusable=0;static Def**lists[12];static int nlists[12];static Def**bags[12];static int nbag[12],bagpos[12];
static int combo=0;                       // current combo count (lives here because lock() uses it; effects only read it)
#define QMAX 16
static Def*queue[QMAX];static int nq=0;

static void dims(Def*d,int*w,int*h){int i;*w=0;*h=0;for(i=0;i<d->n;i++){if(d->c[i]+1>*w)*w=d->c[i]+1;if(d->r[i]+1>*h)*h=d->r[i]+1;}}
static int fits(Def*d){int w,h;dims(d,&w,&h);return(w<=W&&h<=H)||(h<=W&&w<=H);}
static void setup(void){
  int n,i;nusable=0;
  for(n=1;n<=11;n++){
    free(lists[n]);lists[n]=NULL;nlists[n]=0;free(bags[n]);bags[n]=NULL;nbag[n]=0;bagpos[n]=0;
    if(weights[n]>0){
      Def*src;int cnt;
      if(n==11){src=LET;cnt=26;}else{gen(n);src=LVd[n];cnt=LVn[n];}
      lists[n]=(Def**)malloc(sizeof(Def*)*(cnt+1));bags[n]=(Def**)malloc(sizeof(Def*)*(cnt+1));
      for(i=0;i<cnt;i++)if(fits(&src[i]))lists[n][nlists[n]++]=&src[i];
      if(nlists[n])usable[nusable++]=n;
    }
  }
}
static void shuffle_(Def**a,int n){int i;for(i=n-1;i>0;i--){int j=(int)(rnd()*(i+1));Def*t=a[i];a[i]=a[j];a[j]=t;}}
static Def* pick4(void){
  int n=nlists[4],i,t;double wv[16],tot=0,r;
  for(i=0;i<n&&i<16;i++){t=tname(lists[4][i]);double v=t>=0?tw[t]:0;wv[i]=v>0?v:0;tot+=wv[i];}
  if(tot<=0)return lists[4][(int)(rnd()*n)];
  r=rnd()*tot;for(i=0;i<n;i++){r-=wv[i];if(r<0)return lists[4][i];}
  return lists[4][n-1];
}
static Def* next_def(void){
  if(!nusable)return &LVd[1][0];
  double tot=0,r;int i,n;
  for(i=0;i<nusable;i++)tot+=weights[usable[i]];
  r=rnd()*tot;n=usable[nusable-1];
  for(i=0;i<nusable;i++){r-=weights[usable[i]];if(r<0){n=usable[i];break;}}
  if(n==4&&tCustom())return pick4();
  if(bagpos[n]>=nbag[n]){memcpy(bags[n],lists[n],nlists[n]*sizeof(Def*));nbag[n]=nlists[n];shuffle_(bags[n],nbag[n]);bagpos[n]=0;}
  return bags[n][bagpos[n]++];
}
static int fdiv_(int a,int b){int q=a/b;if((a%b!=0)&&((a<0)!=(b<0)))q--;return q;}
// The orientation a piece spawns in: the one that lies flattest (and fits the board). Fills br/bc with its cells. The next and hold
// previews use this too, so what you see in the side panel is exactly how the piece will appear.
static void spawn_orient(Def*d,int*br,int*bc){
  int k,i,z;double bs=-1e18;
  for(k=0;k<4;k++){
    int r[MAXN],c[MAXN];memcpy(r,d->r,d->n*sizeof(int));memcpy(c,d->c,d->n*sizeof(int));
    for(z=0;z<k;z++)for(i=0;i<d->n;i++){int t=r[i];r[i]=c[i];c[i]=-t;}
    norm_cells(d->n,r,c);
    int w=0,h=0,bot=0;for(i=0;i<d->n;i++){if(c[i]+1>w)w=c[i]+1;if(r[i]+1>h)h=r[i]+1;}
    for(i=0;i<d->n;i++)if(r[i]==h-1)bot++;
    double sc=((w<=W&&h<=H)?1e6:0)+(w-h)*1000+bot*10;
    if(sc>bs){bs=sc;memcpy(br,r,d->n*sizeof(int));memcpy(bc,c,d->n*sizeof(int));}
  }
}
// Builds a piece in its spawn orientation, centered at the top.
static Piece mk(Def*d){
  Piece q;memset(&q,0,sizeof q);int w0,h0;dims(d,&w0,&h0);int N=imax(w0,h0),i;int br[MAXN],bc[MAXN];
  spawn_orient(d,br,bc);
  int sw=0,sh=0;for(i=0;i<d->n;i++){if(bc[i]+1>sw)sw=bc[i]+1;if(br[i]+1>sh)sh=br[i]+1;}
  int orow=(N-sh)/2,ocol=(N-sw)/2;
  for(i=0;i<d->n;i++)q.m[br[i]+orow][bc[i]+ocol]=1;
  int r0=99,r1=-1,c0=99,c1=-1,a,b;
  for(a=0;a<N;a++)for(b=0;b<N;b++)if(q.m[a][b]){if(a<r0)r0=a;if(a>r1)r1=a;if(b<c0)c0=b;if(b>c1)c1=b;}
  q.def=d;q.ci=d->ci;q.N=N;q.rot=0;q.kt=d->n==4?(N==4?'I':N==3?'J':0):0;
  q.x=fdiv_(W-(c1-c0+1),2)-c0;q.y=-r0;return q;
}
static int hit_m(const u8 m[10][10],int N,int x,int y){
  int i,j;for(i=0;i<N;i++)for(j=0;j<N;j++)if(m[i][j]){int xx=x+j,yy=y+i;if(xx<0||xx>=W||yy>=H)return 1;if(yy>=0&&board[yy*W+xx])return 1;}
  return 0;
}
static int hit(const Piece*q,int x,int y){return hit_m(q->m,q->N,x,y);}
static void rot_cw(const u8 m[10][10],int n,u8 out[10][10]){int i,j;for(i=0;i<n;i++)for(j=0;j<n;j++)out[i][j]=m[n-1-j][i];}
static void rot_ccw(const u8 m[10][10],int n,u8 out[10][10]){int i,j;for(i=0;i<n;i++)for(j=0;j<n;j++)out[i][j]=m[j][n-1-i];}

// ---- Scoring, speed and lock delay ----
static void addScore(double v){if(clearOn)score+=v;}
static int curLevel(void){return lvlSys?1+cellsCleared/100:1;}
static double rateOf(double v){return 0.2*pow(100,v/100);}
// Fall interval in ms. With the level system on, the standard modern curve (0.8-(L-1)*0.007)^(L-1) scales the base interval.
static double spdNow(void){double b=1000/rateOf(fallV);if(!lvlSys)return b;int L=level-1;return b*pow(dmax(0.8-L*0.007,0.01),L);}
static int lastRot=0;                       // the piece's last successful action was a rotation (needed for a T-spin)
static double tsT=-1e9;static int tsN=0;    // when and how big the last T-spin clear was (drawn as a banner by look.c)
static void fresh(void){lockT=0;resets=0;low=p.y;lastRot=0;}
static void touched(void){if(hit(&p,p.x,p.y+1)&&resets<15){lockT=0;resets++;}}

// ---- Rotation (standard SRS wall kicks for the 7 tetrominoes, simple kicks for everything else) ----
static const int KJ[8][5][2]={{{0,0},{-1,0},{-1,1},{0,-2},{-1,-2}},{{0,0},{1,0},{1,-1},{0,2},{1,2}},{{0,0},{1,0},{1,-1},{0,2},{1,2}},{{0,0},{-1,0},{-1,1},{0,-2},{-1,-2}},{{0,0},{1,0},{1,1},{0,-2},{1,-2}},{{0,0},{-1,0},{-1,-1},{0,2},{-1,2}},{{0,0},{-1,0},{-1,-1},{0,2},{-1,2}},{{0,0},{1,0},{1,1},{0,-2},{1,-2}}};
static const int KI[8][5][2]={{{0,0},{-2,0},{1,0},{-2,-1},{1,2}},{{0,0},{2,0},{-1,0},{2,1},{-1,-2}},{{0,0},{-1,0},{2,0},{-1,2},{2,-1}},{{0,0},{1,0},{-2,0},{1,-2},{-2,1}},{{0,0},{2,0},{-1,0},{2,1},{-1,-2}},{{0,0},{-2,0},{1,0},{-2,-1},{1,2}},{{0,0},{1,0},{-2,0},{1,-2},{-2,1}},{{0,0},{-1,0},{2,0},{-1,2},{2,-1}}};
static int kidx(int a,int b){static const int pr[8][2]={{0,1},{1,0},{1,2},{2,1},{2,3},{3,2},{3,0},{0,3}};int i;for(i=0;i<8;i++)if(pr[i][0]==a&&pr[i][1]==b)return i;return 0;}
static int kicks(int a,int b,int out[16][2]){
  int n=0,i,dy,dx;static const int dxs[7]={0,-1,1,-2,2,-3,3};
  if(p.kt){const int(*t)[5][2]=p.kt=='I'?KI:KJ;int k=kidx(a,b);for(i=0;i<5;i++){out[n][0]=t[k][i][0];out[n][1]=-t[k][i][1];n++;}return n;}
  for(dy=0;dy<2;dy++)for(dx=0;dx<7;dx++){out[n][0]=dxs[dx];out[n][1]=-dy;n++;}
  return n;
}

// ---- Effects and history hooks implemented later ----
static void fxClear(const int*rows,int nrows);
static void fx_break(void);
static void pfStart(void);
static void fxReset(void);
static void endRun(void);
static int sndCombo=1,sndPf=1;

static void move_p(int d){if(!hit(&p,p.x+d,p.y)){p.x+=d;lastRot=0;sfx(S_MOVE,0,NOW);touched();}}
static void turn(int cw){
  u8 m[10][10];if(cw)rot_cw(p.m,p.N,m);else rot_ccw(p.m,p.N,m);
  int nr=(p.rot+(cw?1:3))%4,k[16][2],n=kicks(p.rot,nr,k),i;
  for(i=0;i<n;i++){
    if(!hit_m(m,p.N,p.x+k[i][0],p.y+k[i][1])){memcpy(p.m,m,sizeof m);p.x+=k[i][0];p.y+=k[i][1];p.rot=nr;if(p.y>low){low=p.y;resets=0;}lastRot=1;sfx(S_ROT,0,NOW);touched();return;}
  }
}
static void fall(void){p.y++;lastRot=0;lockT=0;if(p.y>low){low=p.y;resets=0;}}
static void grav(void){if(!hit(&p,p.x,p.y+1))fall();}
static void soft(void){if(!hit(&p,p.x,p.y+1)){fall();addScore(p.def->n/4.0);sfx(S_SOFT,0,NOW);}}
static void spawn(void);
static void lock_piece(void);
static void drop(void){int n=0;while(!hit(&p,p.x,p.y+1)){p.y++;n++;}if(n>0)lastRot=0;addScore(n*2*p.def->n/4.0);sfx(S_HARD,0,NOW);quiet=1;lock_piece();quiet=0;}
static void refill(void){while(nq<=NQ)queue[nq++]=next_def();}
static Def* qpop(void){Def*d=queue[0];memmove(queue,queue+1,(nq-1)*sizeof(Def*));nq--;return d;}
static void hold_p(void){
  if(!canHold)return;Def*d=p.def;int i,fi=-1;for(i=0;i<slots;i++)if(!holds[i]){fi=i;break;}
  if(fi<0){Def*t=holds[sel];holds[sel]=d;p=mk(t);}else{holds[fi]=d;refill();p=mk(qpop());}
  canHold=0;acc=0;fresh();sfx(S_HOLD,0,NOW);if(hit(&p,p.x,p.y))over=1;
}
static void retrieve_p(void){
  if(!canHold||!holds[sel])return;Def*d=p.def,*t=holds[sel];holds[sel]=d;p=mk(t);canHold=0;acc=0;fresh();sfx(S_HOLD,0,NOW);if(hit(&p,p.x,p.y))over=1;
}
static void cycle_p(void){sel=(sel+1)%slots;sfx(S_CYC,0,NOW);}
static void spawn(void){canHold=1;refill();p=mk(qpop());fresh();if(hit(&p,p.x,p.y))over=1;}
static void lock_piece(void){
  int i,j,vis=0,n=0,rows[512],nrows=0,y,tspin=0;
  // T-spin: a T piece whose last move was a rotation and whose bounding box has at least 3 of its 4 corners blocked (walls and floor count).
  if(clearOn&&lastRot&&p.N==3&&tname(p.def)==2){int cn=0,cx,cy;for(cy=0;cy<3;cy+=2)for(cx=0;cx<3;cx+=2){int bx=p.x+cx,by=p.y+cy;if(bx<0||bx>=W||by>=H||(by>=0&&board[by*W+bx]))cn++;}tspin=cn>=3;}
  if(!quiet)sfx(S_LOCK,0,NOW);
  for(i=0;i<p.N;i++)for(j=0;j<p.N;j++)if(p.m[i][j]){int yy=p.y+i;if(yy>=0){vis=1;board[yy*W+p.x+j]=(u8)(p.ci+1);}}
  if(!vis)over=1;
  pieces++;
  if(clearOn){
    for(y=H-1;y>=0;){
      int full=1;for(j=0;j<W;j++)if(!board[y*W+j]){full=0;break;}
      if(full){if(nrows<512)rows[nrows++]=y-n;memmove(board+W,board,(size_t)y*W);memset(board,0,W);n++;}else y--;
    }
  }
  if(n){
    int pc=0,lv0=level,lm=lvlSys?level:1;double wf=W/10.0;
    combo++;if(combo>maxCombo)maxCombo=combo;lines+=n;cellsCleared+=n*W;
    static const int LP[5]={0,100,300,500,800},PCB[5]={0,800,1200,1800,2000};
    // A T-spin that clears lines is worth as much as a 4-line clear and counts as a "difficult" clear for back-to-back, like a 4-line clear does.
    int big=n>=4||tspin;double pts=(LP[tspin?4:imin(n,4)]+(!tspin&&n>4?(n-4)*400:0))*wf;
    score+=floor(pts*((big&&b2b)?1.5:1))*lm;b2b=big;
    if(tspin){tsT=NOW;tsN=n;}
    {int empty=1;for(i=0;i<W*H;i++)if(board[i]){empty=0;break;}if(empty){score+=floor(PCB[imin(n,4)]*wf)*lm;pc=1;}}
    if(combo>1)score+=10*(combo-1)*lm;
    level=curLevel();fxClear(rows,nrows);sfx(S_CLEAR,tspin?4:n,NOW);
    if(combo>=2&&!comboSong)fx_sound(EV_COMBO,sndCombo,combo);   // with the combo song on, song.c plays the combo instead
    if(pc){pfStart();fx_sound(EV_PERFECT,sndPf,1);}
    if(level>lv0)sfx(S_LEVEL,0,NOW);
  }else{fx_break();combo=0;}
  spawn();
}
static void reset_game(void){
  int i;endRun();
  free(board);board=(u8*)calloc((size_t)W*H,1);setup();nq=0;for(i=0;i<10;i++)holds[i]=NULL;sel=0;
  tsT=-1e9;lastRot=0;score=0;lines=0;level=1;cellsCleared=0;pieces=0;maxCombo=0;playMs=0;runDirty=0;saved=0;fxReset();b2b=0;over=0;ovPlayed=0;paused=0;
  dir=0;hl=hr=sd=0;spawn();acc=0;last_t=NOW;
}
static void levels_init(void){
  LVd[1]=(Def*)calloc(1,sizeof(Def));LVn[1]=1;LVd[1][0].n=1;LVd[1][0].r[0]=0;LVd[1][0].c[0]=0;LVd[1][0].ci=0;letters_init();
}
// ---- Held keys: auto-shift (150 ms delay, 40 ms repeat) and soft drop at the chosen rate ----
static void clearHeld(void){hl=hr=sd=0;dir=0;}
enum{A_LEFT,A_RIGHT,A_SOFT,A_HARD,A_CW,A_CCW,A_HOLD,A_RETRIEVE,A_CYCLE,A_PAUSE,A_RESTART,N_ACTS};
static void game_press(int a){
  if(menu)return;
  if(a==A_RESTART){clearHeld();reset_game();return;}
  if(a==A_PAUSE){if(!over)paused=!paused;return;}
  if(paused||over)return;
  switch(a){
    case A_LEFT:hl=1;dir=-1;mt=-110;move_p(-1);break;
    case A_RIGHT:hr=1;dir=1;mt=-110;move_p(1);break;
    case A_CW:turn(1);break;case A_CCW:turn(0);break;
    case A_SOFT:sd=1;st=0;soft();break;
    case A_HARD:drop();break;
    case A_HOLD:hold_p();break;case A_RETRIEVE:retrieve_p();break;case A_CYCLE:cycle_p();break;
  }
}
static void game_release(int a){
  if(a==A_LEFT){hl=0;if(dir==-1){dir=hr?1:0;mt=-110;}}
  else if(a==A_RIGHT){hr=0;if(dir==1){dir=hl?-1:0;mt=-110;}}
  else if(a==A_SOFT)sd=0;
}
static void held(double dt){
  if(dir){mt+=dt;while(mt>=40&&!over){mt-=40;move_p(dir);}}
  if(sd){double iv=dmax(1,dmin(1000/sdRate,spdNow()));int g2=0;st+=dt;while(st>=iv&&!over&&g2++<60){st-=iv;soft();}if(st>iv)st=0;}
}
// One frame of game time.
static void game_step(double dt){
  if(over&&!ovPlayed){ovPlayed=1;sfx(S_OVER,0,NOW);endRun();}
  if(!paused&&!over&&!menu){
    playMs+=dmin(dt,250);held(dt);acc+=dt;
    double spd=spdNow();int g=0;
    if(spd<8){while(!over&&!hit(&p,p.x,p.y+1))fall();acc=0;}
    else{while(acc>spd&&g++<5){acc-=spd;if(!over)grav();}if(acc>spd)acc=0;}
    if(!over&&hit(&p,p.x,p.y+1)){lockT+=dt;if(lockT>=500)lock_piece();}else lockT=0;
  }
}
