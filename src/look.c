// ===== THEMES, BOARD DRAWING AND EFFECTS =====
// Theming: every visual part is its own component and each component can use any theme that defines it.
enum{C_PAGE,C_PANEL,C_MENU,C_FRAME,C_BOARD,C_GRID,C_PIECES,C_GHOST,C_EFFECTS,C_SOUND,N_COMPS};
enum{TH_MIN,TH_CLASSIC,TH_WILD,TH_SWIRL,TH_SOLID,TH_LIGHT,TH_GRAPHITE,N_THEMES};
static const char*COMP_NAME[N_COMPS]={"Page background","Side panel","Settings menu","Board frame","Board background","Grid lines","Pieces","Landing outline","Effect colors","Sound voice"};
static const char*THEME_NAME[N_THEMES]={"8tris","Classic","Wild","Swirl","Solid Color","8tris Light","Graphite"};
// The order themes are listed in the menus. Theme numbers never change (they are saved in settings and pick sound voices), only this list does.
static const int THEME_ORDER[N_THEMES]={TH_MIN,TH_LIGHT,TH_GRAPHITE,TH_CLASSIC,TH_SOLID,TH_SWIRL,TH_WILD};
// Which themes define each component (bit 0 = 8tris, bit 1 = Classic, bit 2 = Wild, bit 3 = Swirl, bit 4 = Solid Color, bit 5 = 8tris Light, bit 6 = Graphite). Add a theme to a component by setting its bit.
static const int AVAIL[N_COMPS]={109,109,109,109,109,109,127,125,111,111};   // Graphite (bit 6) is available for every component
// Solid Color theme: one hex color for the pieces and one for the landing outline. They stay equal unless solidSep is on.
static u32 solidP=0xFF4F9DFFu,solidG=0xFF4F9DFFu;static int solidSep=0;
// Appearance (settings menu only, plus the landing outline strength): font family, bold, italic, text size in pt, high contrast, menu width, line spacing.
static int uiFam=0,uiBold=0,uiItal=0,uiPt=14,hiCon=0,mwSel=0,lsSel=0,ghostStr=50;
static u32 ghost_solid(void){return solidSep?solidG:solidP;}
static int themeOn=0,TC[N_COMPS];static int calm=0;      // calm = reduce motion (freezes animation, softer effects, effects capped at Moderate)
static int T(int c){return themeOn?TC[c]:TH_MIN;}
static int LT(int c){return T(c)==TH_LIGHT;}                  // 8tris Light: the 8tris look with black and white swapped
static int LG(int c){return T(c)==TH_GRAPHITE;}               // Graphite: 8tris with light grey pieces on a white board
static int pale(int c){return LT(c)||LG(c);}                    // the component is drawn on a white/pale background
static int snd_theme(void){int t=T(C_SOUND);return(t==TH_LIGHT||t==TH_GRAPHITE)?TH_MIN:t;}   // 8tris Light has no voice of its own, it uses the 8tris one
static const char* theme_label(int comp,int t){if(comp==C_SOUND&&t==TH_CLASSIC)return "Classic chiptune";return (t>=0&&t<N_THEMES)?THEME_NAME[t]:"?";}
static double tm(double now){return calm?0:now;}

// Layout (set each frame by app_frame)
static int S=28,BX=0,BY=0,SHX=0,SHY=0;   // cell size, board origin, current screen-shake offset

// ---- Pieces: paint a single block at pixel position (x, y), size s ----
static u32 piece_base(int ci,double now){int t=T(C_PIECES);if(t==TH_GRAPHITE)return HEX(0xb8b8b8);if(t==TH_LIGHT)return HEX(0x000000);if(t==TH_SOLID)return solidP;if(t==TH_SWIRL)return sw_col(ci*.137+.5);if(t==TH_CLASSIC)return COL[ci];if(t==TH_WILD)return hsl(fmod(ci*47+tm(now)/14,360),1,.60);return HEX(0xbdbdbd);}
static void piece_paint(double x,double y,double s,int ci,double now,int gx,int gy){
  int t=T(C_PIECES);
  if(t==TH_MIN){fill_rect(x,y,s,s,HEX(0xbdbdbd),1);return;}
  if(t==TH_SOLID){fill_rect(x,y,s,s,solidP,1);return;}
  if(t==TH_LIGHT){fill_rect(x,y,s,s,HEX(0x000000),1);return;}
  if(t==TH_GRAPHITE){fill_rect(x,y,s,s,HEX(0xb8b8b8),1);return;}
  if(t==TH_CLASSIC){fill_rect(x,y,s,s,COL[ci],1);return;}
  if(t==TH_SWIRL){swirl_block(x,y,s,ci,tm(now),gx,gy);return;}
  double h=fmod(ci*47+gx*9+gy*9+tm(now)/14,360);
  fill_rect(x,y,s,s,hsl(h,1,.52),1);if(s<6)return;
  double b=dmax(1,floor(s/7+.5)),ins=dmax(1,s-b*4);
  fill_rect(x+b*2,y+b*2,ins,ins,hsl(h+45,1,.72),.55);
  fill_rect(x,y,s,b,HEX(0xffffff),.6);fill_rect(x,y,b,s,HEX(0xffffff),.6);
  fill_rect(x,y+s-b,s,b,HEX(0),.35);fill_rect(x+s-b,y,b,s,HEX(0),.35);
  if(s>=10&&(((gx*7+gy*13+(int)floor(tm(now)/140))%13)+13)%13==0){double m=floor(s/2);fill_rect(x+m-1,y+b+1,2,s-b*2-2,HEX(0xffffff),1);fill_rect(x+b+1,y+m-1,s-b*2-2,2,HEX(0xffffff),1);}
}
// ---- Landing outline ----
static void ghost_paint(double x,double y,double s,int ci,double now,int gx,int gy){
  int t=T(C_GHOST);u32 c;double a;
  if(t==TH_SWIRL){double cx=x+s/2,cy=y+s/2;ring_aa(cx,cy,s*.42,s>8?2.8:1.4,sw_col(ci*.137+.5+(gx+gy)*.05+tm(now)*.0003),1);return;}
  if(t==TH_GRAPHITE){c=HEX(0x8f8f8f);a=1;}else if(t==TH_SOLID){c=ghost_solid();a=1;}else if(t==TH_WILD){c=hsl(tm(now)/6+gx*20+gy*20,1,.65);a=.95;}else{c=piece_base(ci,now);a=dmin(1,.5*ghostStr/50.0);}
  if(s>8)border_in(x,y,s,s,2,c,a);else border_in(x+1,y+1,s-2,s-2,1,c,a);
}
// ---- Board background and grid lines ----
static void board_bg(double w,double h,double now){
  if(T(C_BOARD)==TH_MIN){fill_rect(BX,BY,w,h,HEX(0),1);return;}
  if(pale(C_BOARD)){fill_rect(BX,BY,w,h,HEX(0xffffff),1);return;}
  if(T(C_BOARD)==TH_SWIRL){swirl_fill(2,BX,BY,w,h,6,tm(now)+9000,.2,50);return;}
  double t=tm(now)/30;u32 c0=hsl(t,.7,.13),c1=hsl(t+80,.7,.07),c2=hsl(t+160,.7,.14);
  int x,y,ix=(int)w,iy=(int)h;double den=w*w+h*h;
  for(y=0;y<iy;y++){int py=BY+y;if(py<CY0||py>=CY1)continue;
    for(x=0;x<ix;x++){int px=BX+x;if(px<CX0||px>=CX1)continue;
      double q=(x*w+y*h)/den;u32 a,b;double k;if(q<.5){a=c0;b=c1;k=q*2;}else{a=c1;b=c2;k=(q-.5)*2;}
      int ar=(a>>16)&255,ag=(a>>8)&255,ab=a&255,br=(b>>16)&255,bg=(b>>8)&255,bb=b&255;
      FB[py*FW+px]=RGB((int)(ar+(br-ar)*k),(int)(ag+(bg-ag)*k),(int)(ab+(bb-ab)*k));}}
  // twinkling stars on a fixed pattern
  unsigned sd=12345;int i;double z=dmax(1,floor(S/9.0+.5));
  for(i=0;i<40;i++){
    sd=(unsigned)(((unsigned long long)sd*16807ull)%2147483647ull);double r1=sd/2147483647.0;
    sd=(unsigned)(((unsigned long long)sd*16807ull)%2147483647ull);double r2=sd/2147483647.0;
    sd=(unsigned)(((unsigned long long)sd*16807ull)%2147483647ull);double r3=sd/2147483647.0*6.28;
    double al=.25+.75*fabs(sin(tm(now)/500+r3));
    fill_rect(BX+floor(r1*w),BY+floor(r2*h),z,z,hsl(fmod(r3*57,360),1,.8),al);
  }
}
static void grid_draw(double w,double h,double now){
  if(S<8)return;int X,Y;
  if(T(C_GRID)==TH_MIN||pale(C_GRID)){u32 c=LG(C_GRID)?HEX(0xe2e2e2):LT(C_GRID)?HEX(0xdcdcdc):HEX(0x2b2b2b);for(X=1;X<W;X++)fill_rect(BX+X*S,BY,1,h,c,1);for(Y=1;Y<H;Y++)fill_rect(BX,BY+Y*S,w,1,c,1);}
  else if(T(C_GRID)==TH_SWIRL){double t=tm(now);int yy,xx,ih=(int)h,iw=(int)w;   // lines that waver slightly
    for(X=1;X<W;X++){u32 c=sw_col(X/(double)W+t*.00008);double ph=X*.9+t*.0012,k=1/(S*1.1);
      for(yy=0;yy<ih;yy++)aa_px(BX+X*S+fsin(yy*k+ph)*S*.05,BY+yy,c,.2);}
    for(Y=1;Y<H;Y++){u32 c=sw_col(Y/(double)H+.5+t*.00008);double ph=Y*.9-t*.0012,k=1/(S*1.1);
      for(xx=0;xx<iw;xx++){double y=BY+Y*S+fsin(xx*k+ph)*S*.05;int iy=(int)floor(y);double f=y-iy;putpx(BX+xx,iy,c,A255(.2*(1-f)));putpx(BX+xx,iy+1,c,A255(.2*f));}}
  }
  else{double t=tm(now)/12;
    for(X=1;X<W;X++)fill_rect(BX+X*S,BY,1,h,hsl(X*360.0/W+t,1,.6),.4);
    for(Y=1;Y<H;Y++)fill_rect(BX,BY+Y*S,w,1,hsl(Y*360.0/H+t+180,1,.6),.4);}
}

// ===== EFFECTS =====
// Combo and perfect clear effects share one ladder: 0 Off, 1 A bit, 2 Moderate, 3 High, 4 Intense. Each level keeps everything from the levels below it.
// resolve_fx() turns the master Effects level plus the Advanced overrides into the four working levels (combo/perfect clear, visuals/sound).
// Colors come from the Effects look (look()), so a new look re-colors every effect. Shake, glow and flashes are applied only in this section,
// and every full-screen flash goes through flashOK() so flashing stays well under 3 per second.
static int fxMaster=1,ADV[4]={-1,-1,-1,-1};            // ADV: combo visuals, perfect clear visuals, combo sound, perfect clear sound (-1 = match)
static int fxLevel=1,pcLevel=1;                         // sndCombo and sndPf live in game.c
static const char*LVN[5]={"Off","A bit","Moderate","High","Intense"};
static const char*EFF_ADD[5]={
 "Nothing extra: no combo or perfect clear effects, and clears play a plain blip.",
 "a combo counter with a chime that climbs each step, and a banner with a short arpeggio on perfect clears.",
 "a pop on the counter, a flash across cleared rows, a soft chord, and a brief screen flash on perfect clears.",
 "sparks, screen shake, rank words like NICE and GREAT, a long reverb tail, and a particle burst on perfect clears.",
 "a glowing frame, bigger text, expanding rings, a deep bass hit, a rising sweep and sparkle."};
static int maxLv(void){return calm?2:4;}
static int capLv(int v){return v<maxLv()?v:maxLv();}
static void resolve_fx(void){
  int m=capLv(fxMaster);fxLevel=ADV[0]<0?m:capLv(ADV[0]);pcLevel=ADV[1]<0?m:capLv(ADV[1]);
  sndCombo=ADV[2]<0?fxLevel:capLv(ADV[2]);sndPf=ADV[3]<0?pcLevel:capLv(ADV[3]);
}
// Effects looks: look(kind, time, i). kind is one of the K_ values; i is the effect level for text,
// the particle color index for sparks, and the ring number for rings.
enum{K_TEXT,K_SPARK,K_GLOW,K_RING,K_TINT,K_FLASH};
static u32 look(int k,double now,int i){
  int t=T(C_EFFECTS);
  if(t==TH_GRAPHITE){if(k==K_FLASH||k==K_TINT)return HEX(0x404040);if(k==K_TEXT)return i>=4?(((int)(now/90))%2?HEX(0xffffff):HEX(0xd0d0d0)):HEX(0xffffff);return HEX(0x707070);}   // soft grey flashes and sparks on the white board
  if(t==TH_LIGHT){if(k==K_FLASH||k==K_TINT)return HEX(0);if(k==K_TEXT)return i>=4?(((int)(now/90))%2?HEX(0xffffff):HEX(0xcccccc)):HEX(0xffffff);return HEX(0x303030);}   // dark flashes and sparks on the white board, white text (it has a black outline)
  if(k==K_FLASH)return HEX(0xffffff);
  if(t==TH_MIN){if(k==K_TEXT)return i>=4?(((int)(now/90))%2?HEX(0xffffff):HEX(0xbdbdbd)):HEX(0xe0e0e0);return(k==K_SPARK||k==K_GLOW)?HEX(0xe0e0e0):HEX(0xffffff);}
  if(t==TH_SWIRL){double q=tm(now)*.0006;if(k==K_TEXT)return i>=4?sw_col(q):mixc(sw_col(q*.5),HEX(0xffffff),.6);if(k==K_SPARK)return sw_col(i*.11+q);if(k==K_RING)return sw_col(q+i*.23);return sw_col(q*.7+i*.1);}
  if(t==TH_CLASSIC){int n=k==K_SPARK?0:(int)(now/(k==K_GLOW||k==K_RING?250:k==K_TINT?600:120));if(k==K_TEXT)return i>=4?COL[n%10]:HEX(0xe0e0e0);return COL[(n+i*3)%10];}
  if(k==K_TEXT)return i>=4?hsl(now/3,1,.65):HEX(0xe0e0e0);
  if(k==K_SPARK)return hsl(i*36+now/3,1,.6);
  if(k==K_RING)return hsl(now/3+i*80,1,.65);
  return hsl(now/3,1,.6);
}
// Flash limiter: at most 2 full-screen flashes in any second. Softer flashes when reduced motion is on.
static double flashLog[4];static int nflash=0;
static int flashOK(double t){int i,n=0;double keep[4];for(i=0;i<nflash;i++)if(t-flashLog[i]<1000)keep[n++]=flashLog[i];if(n>=2){memcpy(flashLog,keep,n*sizeof(double));nflash=n;return 0;}keep[n++]=t;memcpy(flashLog,keep,n*sizeof(double));nflash=n;return 1;}
static double fAlpha(double x){return calm?x*.4:x;}
typedef struct{double x,y,vx,vy,t;int c;}Part;
#define NPT 1500
typedef struct{struct{int y;double t;}fl[600];int nfl;Part pt[NPT];int npt;double sa,sk,fa,pop,end;int n;}FXS;
typedef struct{double t,sk;Part pt[200];int npt;int fl;}PFS;
static FXS FX;static PFS PF;static const double PFD[5]={0,1800,2200,2600,3400};
static void fx_blank(FXS*f){memset(f,0,sizeof*f);f->sk=-1e9;f->fa=-1e9;f->pop=-1e9;}
static void pf_blank(PFS*f){memset(f,0,sizeof*f);f->t=-1e9;f->sk=-1e9;}
static void fxReset(void){combo=0;fx_blank(&FX);pf_blank(&PF);}
static void fxClear(const int*rows,int nrows){
  double t=NOW;int r,i;FX.n=combo;FX.end=0;if(combo<2||fxLevel<1)return;FX.pop=t;
  if(fxLevel>=2)for(r=0;r<nrows&&FX.nfl<600;r++){FX.fl[FX.nfl].y=rows[r];FX.fl[FX.nfl].t=t;FX.nfl++;}
  if(fxLevel>=3){
    FX.sk=t;FX.sa=dmin(2+combo*1.5,12)*(fxLevel>=4?1.6:1);int per=(int)(dmin(8+combo*2,40)*(fxLevel>=4?2:1));
    for(r=0;r<nrows;r++)for(i=0;i<per;i++){
      if(FX.npt>=NPT){memmove(FX.pt,FX.pt+1,(NPT-1)*sizeof(Part));FX.npt=NPT-1;}
      Part*q=&FX.pt[FX.npt++];q->x=rnd()*W*S;q->y=(rows[r]+.5)*S;q->vx=(rnd()-.5)*S*10;q->vy=-rnd()*S*8-S*2;q->t=t;q->c=(int)(rnd()*10);
    }
  }
  if(fxLevel>=4&&flashOK(t))FX.fa=t;
}
static void fx_break(void){if(combo>=2)FX.end=NOW;}
static int fxShown(double now){return fxLevel>=1&&FX.n>=2&&!(FX.end&&now-FX.end>600);}
static void pfStart(void){
  double t=NOW;int i;pf_blank(&PF);PF.t=t;if(pcLevel<1)return;PF.fl=pcLevel>=2&&flashOK(t);
  if(pcLevel>=3){PF.sk=t;int n=pcLevel>=4?160:80;for(i=0;i<n&&i<200;i++){double a=rnd()*6.283,sp=S*(4+rnd()*10);Part*q=&PF.pt[PF.npt++];q->x=W*S/2.0;q->y=H*S/2.0;q->vx=cos(a)*sp;q->vy=sin(a)*sp;q->c=(int)(rnd()*10);}}
}
// Screen shake for this frame (applied to the whole board, frame and glow).
static void fx_shake(double now){
  double sh=0;
  if(fxLevel>=3&&now-FX.sk<300)sh=FX.sa*(1-(now-FX.sk)/300);
  if(pcLevel>=3&&now-PF.sk<500)sh=dmax(sh,(pcLevel>=4?14:8)*(1-(now-PF.sk)/500));
  if(sh>0.2){SHX=(int)((rnd()-.5)*2*sh);SHY=(int)((rnd()-.5)*2*sh);}else{SHX=0;SHY=0;}
}
// Frame and glow around the board: the one place that styles the outside of the board for effects.
static void frame_draw(double w,double h,double now){
  int t=T(C_FRAME);
  if(t==TH_SWIRL){
    if(fxLevel>=4&&fxShown(now))glow_rect(BX,BY,w,h,dmin(10+FX.n*4,50),0,look(K_GLOW,now,0),.9);
    else if(pcLevel>=4&&now-PF.t<PFD[4])glow_rect(BX,BY,w,h,70,10,look(K_GLOW,now,0),.9);
    swirl_ribbon(BX,BY,w,h,5,3.5,dmax(14,S*1.2),tm(now),1.9,.95);return;
  }
  if(t==TH_WILD){
    double hh=tm(now)/10;
    int glowed=0;
    if(fxLevel>=4&&fxShown(now)){glow_rect(BX,BY,w,h,dmin(10+FX.n*4,50),0,look(K_GLOW,now,0),.9);glowed=1;}
    else if(pcLevel>=4&&now-PF.t<PFD[4]){glow_rect(BX,BY,w,h,70,10,look(K_GLOW,now,0),.9);glowed=1;}
    if(!glowed){glow_rect(BX,BY,w,h,56,0,hsl(hh+260,1,.55),.9);glow_rect(BX,BY,w,h,28,0,hsl(hh+160,1,.55),.9);glow_rect(BX,BY,w,h,12,0,hsl(hh+60,1,.6),.9);}
    border_out(BX,BY,w,h,3,hsl(hh,1,.6),1);
  }else{
    if(fxLevel>=4&&fxShown(now))glow_rect(BX,BY,w,h,dmin(10+FX.n*4,50),0,look(K_GLOW,now,0),.9);
    else if(pcLevel>=4&&now-PF.t<PFD[4])glow_rect(BX,BY,w,h,70,10,look(K_GLOW,now,0),.9);
    if(LG(C_FRAME))border_out(BX,BY,w,h,1,HEX(0xb4b4b4),1);else if(LT(C_FRAME))border_out(BX,BY,w,h,2,HEX(0),1);else border_out(BX,BY,w,h,1,HEX(0x555555),1);
  }
}
static void spark_px(double x,double y,double ps,u32 c,double al){if(T(C_EFFECTS)==TH_SWIRL)disc(x+ps/2,y+ps/2,ps*.75,c,al);else fill_rect(x,y,ps,ps,c,al);}
static const char* tier(int n){return n>=10?"UNSTOPPABLE":n>=7?"INCREDIBLE":n>=5?"GREAT":n>=3?"NICE":"";}
static void drawPf(double now){
  if(pcLevel<1)return;double a=now-PF.t,D=PFD[pcLevel];if(a<0||a>=D)return;double w=W*S,h=H*S;int i;
  if(PF.fl&&a<500)fill_rect(BX,BY,w,h,look(K_FLASH,now,0),fAlpha(.45)*(1-a/500));
  if(pcLevel>=4){
    fill_rect(BX,BY,w,h,look(K_TINT,a,0),.2*(1-a/D));
    double R=hypot(w,h)/2;
    for(i=0;i<3;i++){double ra=a-i*260;if(ra>0&&ra<1100)fill_ring(BX+w/2,BY+h/2,R*ra/1100,dmax(2,S/3.0),look(K_RING,a,i),.8*(1-ra/1100));}
  }
  if(pcLevel>=3){double sec=a/1000,ps=dmax(2,S/4.0),al=dmax(0,1-a/1500);
    for(i=0;i<PF.npt;i++){Part*q=&PF.pt[i];spark_px(BX+q->x+q->vx*sec,BY+q->y+q->vy*sec+S*10*sec*sec,ps,look(K_SPARK,a,q->c),al);}}
  double fade=a>D-500?(D-a)/500:1,pop=(pcLevel>=2&&a<300)?1+.7*(1-a/300):1,base=dmin(pcLevel>=4?50:pcLevel>=3?38:28,w/14),fs=dmax(10,base)*pop;
  text_outlined(1,fs,BX+w/2,BY+h*.4,fs/5,look(K_TEXT,a,pcLevel),fade,"PERFECT CLEAR");
}
static void drawFx(double now){
  if(fxLevel<1)return;double w=W*S,h=H*S;int i,k;
  if(fxLevel>=2){
    for(i=0,k=0;i<FX.nfl;i++)if(now-FX.fl[i].t<400)FX.fl[k++]=FX.fl[i];FX.nfl=k;
    for(i=0;i<FX.nfl;i++)fill_rect(BX,BY+FX.fl[i].y*S,w,S,look(K_FLASH,now,0),fAlpha(.7)*(1-(now-FX.fl[i].t)/400));
  }
  if(fxLevel>=3){
    for(i=0,k=0;i<FX.npt;i++)if(now-FX.pt[i].t<800)FX.pt[k++]=FX.pt[i];FX.npt=k;double ps=dmax(2,S/4.0);
    for(i=0;i<FX.npt;i++){Part*q=&FX.pt[i];double a=(now-q->t)/1000;spark_px(BX+q->x+q->vx*a,BY+q->y+q->vy*a+S*14*a*a,ps,look(K_SPARK,now,q->c),1-(now-q->t)/800);}
  }
  if(fxLevel>=4&&now-FX.fa<250)fill_rect(BX,BY,w,h,look(K_FLASH,now,0),.25*(1-(now-FX.fa)/250));
  if(!fxShown(now))return;
  double fade=FX.end?1-(now-FX.end)/600:1,pa=now-FX.pop,pop=(fxLevel>=2&&pa<250)?1+.6*(1-pa/250):1;
  double base=dmax(14,dmin(fxLevel>=4?56:fxLevel>=3?40:28,w/(fxLevel>=4?5:7))),fs=base*pop,y=base*1.4;char buf[64];
  snprintf(buf,sizeof buf,"Combo \xC3\x97%d",FX.n);
  text_outlined(1,fs,BX+w/2,BY+y,fs/5,look(K_TEXT,now,fxLevel),fade,buf);
  const char*tw=fxLevel>=3?tier(FX.n):"";
  if(*tw){double f2=base*.45;text_outlined(1,f2,BX+w/2,BY+y+base*.6,f2/4,look(K_TEXT,now,fxLevel),fade,tw);}
}
// ---- The whole board: background, grid, blocks, landing outline, effects and the Paused / Game over cover ----
static const char* key_label(int k);
static int bind_key(int act,int slot);
// "T-SPIN DOUBLE" banner: this is game information, so it shows whatever the effects level is.
static void drawTspin(double now){
  double a=now-tsT;if(a<0||a>1400||tsN<1)return;double w=W*S,fade=a>900?(1400-a)/500:1,fs=dmax(11,dmin(30,w/8))*(a<200?1+.5*(1-a/200):1);
  static const char*nm[4]={"","SINGLE","DOUBLE","TRIPLE"};char buf[40];snprintf(buf,sizeof buf,"T-SPIN %s",nm[imin(tsN,3)]);
  text_outlined(1,fs,BX+w/2,BY+H*S*.58,fs/5,look(K_TEXT,now,2),fade,buf);
}
static void draw_board(double now){
  double w=W*S,h=H*S;int x,y,i,j;
  clip(BX,BY,BX+(int)w,BY+(int)h);
  board_bg(w,h,now);grid_draw(w,h,now);
  for(y=0;y<H;y++)for(x=0;x<W;x++){int v=board[y*W+x];if(v)piece_paint(BX+x*S,BY+y*S,S,v-1,now,x,y);}
  if(!over){
    int gy=p.y;while(!hit(&p,p.x,gy+1))gy++;
    for(i=0;i<p.N;i++)for(j=0;j<p.N;j++)if(p.m[i][j]&&gy+i>=0)ghost_paint(BX+(p.x+j)*S,BY+(gy+i)*S,S,p.ci,now,p.x+j,gy+i);
    for(i=0;i<p.N;i++)for(j=0;j<p.N;j++)if(p.m[i][j]&&p.y+i>=0)piece_paint(BX+(p.x+j)*S,BY+(p.y+i)*S,S,p.ci,now,p.x+j,p.y+i);
  }
  drawFx(now);drawPf(now);drawTspin(now);
  unclip();
}
static void draw_cover(int preview){
  if(!(paused||over)||preview)return;double w=W*S,h=H*S;char buf[96];
  clip(BX,BY,BX+(int)w,BY+(int)h);fill_rect(BX,BY,w,h,pale(C_BOARD)?HEX(0xffffff):HEX(0),.8);
  double fs=dmax(12,dmin(32,w/7));
  text_center(0,fs,BX+w/2,BY+h/2,pale(C_BOARD)?HEX(0x101010):HEX(0xe0e0e0),1,over?"Game over":"Paused");
  if(over){if(clearOn)snprintf(buf,sizeof buf,"Score %d. Press %s.",(int)floor(score),key_label(bind_key(A_RESTART,0)));else snprintf(buf,sizeof buf,"Press %s to restart.",key_label(bind_key(A_RESTART,0)));}
  else snprintf(buf,sizeof buf,"Press %s to resume",key_label(bind_key(A_PAUSE,0)));
  text_center(0,fs*.55,BX+w/2,BY+h/2+fs,pale(C_BOARD)?HEX(0x101010):HEX(0xe0e0e0),1,buf);
  unclip();
}
// A piece drawn small inside an 80x64 cell of the side panel (used for next and held pieces).
static void draw_small(Def*d,double x0,double y0,double alpha,double now){
  int br[MAXN],bc[MAXN],w=0,h=0,i;(void)alpha;
  spawn_orient(d,br,bc);                                            // shown the way it will spawn (S and Z lie flat, I is horizontal, ...)
  for(i=0;i<d->n;i++){if(bc[i]+1>w)w=bc[i]+1;if(br[i]+1>h)h=br[i]+1;}
  int u=iclamp((int)floor(56.0/imax(w,h)),2,14);double ox=x0+(80-w*u)/2.0,oy=y0+(64-h*u)/2.0;
  for(i=0;i<d->n;i++)piece_paint(ox+bc[i]*u,oy+br[i]*u,u,d->ci,now,bc[i],br[i]);
}
