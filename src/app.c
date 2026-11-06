// ===== APPLICATION GLUE: layout, page and side panel, input routing =====
static int WW=900,WH=700,winW=900,winH=700,last_res=-1;static double prev_t=-1;   // WW,WH: the size the game draws at; winW,winH: the window
static u32*RB=NULL;
static float*ANG=NULL;static int AW=0,AH=0;

// The game draws at the window size, or at a lower "render resolution" that the platform layer then scales up to the window.
static void layout_internal(void){
  int cap=RES_H[resSet],rw=winW,rh=winH;
  if(cap>0&&winH>cap){rh=cap;rw=imax(160,(int)((double)winW*cap/winH+.5));}
  WW=imax(rw,160);WH=imax(rh,120);UZ=imax(1,imin(WW,WH)/900);last_res=resSet;
  AW=WW/4+1;AH=WH/4+1;free(ANG);ANG=(float*)malloc(sizeof(float)*AW*AH);
  int x,y;for(y=0;y<AH;y++)for(x=0;x<AW;x++)ANG[y*AW+x]=(float)(atan2(y*4+2-WH/2.0,x*4+2-WW/2.0)*180/3.14159265+180);
  free(RB);RB=(u32*)calloc((size_t)WW*WH,4);
}
void app_resize(int w,int h){winW=imax(w,160);winH=imax(h,120);layout_internal();}
int app_fps_target(void){return FPS_V[fpsSet];}
u32* app_render(double now,int*w,int*h){if(resSet!=last_res)layout_internal();app_frame(now,RB);*w=WW;*h=WH;return RB;}
// ---- Wild page background: two slowly turning rainbow cones blended together ----
static void cone_lut(u32*lut,const u32*stops,int n){
  int d;for(d=0;d<360;d++){double pos=d/360.0*(n-1);int i=(int)pos;if(i>=n-1)i=n-2;double k=pos-i;u32 a=stops[i],b=stops[i+1];
    int r=(int)(((a>>16)&255)*(1-k)+((b>>16)&255)*k),g=(int)(((a>>8)&255)*(1-k)+((b>>8)&255)*k),bl=(int)((a&255)*(1-k)+(b&255)*k);lut[d]=RGB(r,g,bl);}
}
static void wild_page(double now){
  static const u32 sa[8]={HEX(0xff0080),HEX(0xff8c00),HEX(0xffee00),HEX(0x00e676),HEX(0x00e5ff),HEX(0x2979ff),HEX(0xd500f9),HEX(0xff0080)};
  static const u32 sb[6]={HEX(0x00e5ff),HEX(0xd500f9),HEX(0xff0080),HEX(0xffee00),HEX(0x00e676),HEX(0x00e5ff)};
  static u32 la[360],lb[360];static int ready=0;if(!ready){cone_lut(la,sa,8);cone_lut(lb,sb,6);ready=1;}
  double t=tm(now);int ra=(int)fmod(t/22000.0*360,360),rb=(int)fmod(t/14000.0*360,360),bx,by,i,j;
  // One row of 4x4 blocks is colored once, then copied to its four pixel rows.
  (void)i;for(by=0;by<AH;by++){int y0=by*4;if(y0>=WH)break;u32*row=FB+(size_t)y0*FW;
    for(bx=0;bx<AW;bx++){int x0=bx*4;if(x0>=WW)break;
      int d=(int)ANG[by*AW+bx];int ia=((d+ra)%360+360)%360,ib=((360-d+rb)%360+360)%360;u32 a=la[ia],b=lb[ib];
      u32 c=RGB((((a>>16)&255)+((b>>16)&255))/2,(((a>>8)&255)+((b>>8)&255))/2,((a&255)+(b&255))/2);
      int e=imin(4,WW-x0);for(j=0;j<e;j++)row[x0+j]=c;}
    for(j=1;j<4&&y0+j<WH;j++)memcpy(FB+(size_t)(y0+j)*FW,row,sizeof(u32)*(size_t)WW);}
}
// ---- Side strip: held pieces, next pieces and the Preferences button ----
static int stripX(void){return WW-80;}
static void strip_button_rect(int*x,int*y,int*w,int*h){
  int stW=stripX(),m=8;*w=74;*h=30;
  if(prefBtn==6){*x=stW+3;*y=WH-8-30;return;}
  {int col=prefBtn%3;*x=col==0?m:col==1?(stW-74)/2:stW-m-74;*y=prefBtn<3?m:WH-m-30;}
}
static void draw_strip(double now){
  int sx=stripX(),i;
  if(T(C_PANEL)==TH_SWIRL){swirl_fill(1,sx,0,80,WH,6,tm(now)+4000,.32,50);
    {int yy;for(yy=0;yy<WH;yy++){double dx=sin(yy/26.0+tm(now)*.0014)*3,dx2=-dx;disc(sx+4+dx,yy,1.8,sw_col(yy/260.0+tm(now)*.00012),.95);disc(sx+4+dx2,yy,1.8,sw_col(yy/260.0+.5+tm(now)*.00012),.95);}}
  }else if(T(C_PANEL)==TH_WILD){
    for(i=0;i<WH;i++){double k=(double)i/WH;fill_rect(sx,i,80,1,RGB((int)(30*(1-k)),(int)(25*k),(int)(60)),.92);}
    fill_rect(sx,0,3,WH,hsl(tm(now)/10,1,.6),1);
  }else if(flat(C_PANEL)){fill_rect(sx,0,80,WH,HEX(flat(C_PANEL)->panel),1);fill_rect(sx,0,1,WH,HEX(flat(C_PANEL)->line),1);}
  else if(pale(C_PANEL)){fill_rect(sx,0,80,WH,HEX(0xffffff),1);fill_rect(sx,0,1,WH,LG(C_PANEL)?HEX(0xd6d6d6):HEX(0),1);}
  else fill_rect(sx,0,80,WH,HEX(0),1);
  int y=8;
  for(i=0;i<slots;i++){
    if(y+64>WH-60)break;
    if(slots>1&&i==sel)border_in(sx,y,80,64,2,flat(C_PANEL)?HEX(flat(C_PANEL)->btn):LG(C_PANEL)?HEX(0x8f8f8f):LT(C_PANEL)?HEX(0):HEX(0xe0e0e0),.8);
    if(holds[i])draw_small(holds[i],sx,y,canHold?1:.4,now);y+=64;
  }
  if(T(C_PANEL)==TH_SWIRL){int xx;for(xx=8;xx<76;xx++)aa_px(sx+xx,y+sin(xx/7.0+tm(now)*.002)*2,sw_col(xx/80.0+tm(now)*.0001),.9);}else fill_rect(sx,y,80,1,flat(C_PANEL)?HEX(flat(C_PANEL)->line):T(C_PANEL)==TH_WILD?HEX(0x00e5ff):LG(C_PANEL)?HEX(0xd0d0d0):LT(C_PANEL)?HEX(0x999999):HEX(0x777777),1);y+=16;
  for(i=0;i<NQ&&i<nq;i++){if(y+64>WH-48)break;draw_small(queue[i],sx,y,1,now);y+=64;}
  int bx,by,bw,bh;strip_button_rect(&bx,&by,&bw,&bh);int hot=!menu&&mx>=bx&&mx<bx+bw&&my>=by&&my<by+bh;
  u32 bc=flat(C_PANEL)?HEX(flat(C_PANEL)->btn):T(C_PANEL)==TH_SWIRL?sw_col(.15+tm(now)*.00005):T(C_PANEL)==TH_WILD?HEX(0xffee00):LG(C_PANEL)?HEX(0x5a5a5a):LT(C_PANEL)?HEX(0):HEX(0xe0e0e0);
  u32 bk=pale(C_PANEL)?HEX(0xffffff):flat(C_PANEL)?HEX(flat(C_PANEL)->panel):HEX(0);
  fill_rect(bx,by,bw,bh,hot?bc:bk,1);border_in(bx,by,bw,bh,1,bc,1);text_center(0,11,bx+bw/2.0,by+19,hot?bk:(T(C_PANEL)==TH_SWIRL?HEX(0xffffff):bc),1,"Preferences");
}

// ---- Entry points ----
void app_init(void){
  int i;rs_^=(u32)time(NULL)*2654435761u;if(!rs_)rs_=1;
  binds_default();for(i=0;i<N_COMPS;i++)TC[i]=TH_MIN;fxMaster=1;
  levels_init();load_settings();music_scan();hist_load();resolve_fx();pend_load();snd_voice=snd_theme();
  NOW=0;reset_game();paused=0;menu=0;
}
void app_key(int key,int down,int repeat){
  if(key==K_F11){if(down&&!repeat){fullscreen=!fullscreen;plat_fullscreen(fullscreen);}return;}
  if(ui_key(key,down))return;
  if(key==K_ESC){if(down&&!repeat){menu=1;clearHeld();}return;}
  int a=key_action(key);if(a<0)return;
  if(down){if(!repeat)game_press(a);}else game_release(a);
}
static int to_x(int x){return winW>0?(int)((double)x*WW/winW):x;}
static int to_y(int y){return winH>0?(int)((double)y*WH/winH):y;}
void app_mouse_move(int x,int y){mx=to_x(x);my=to_y(y);}
void app_mouse_button(int x,int y,int down){
  x=to_x(x);y=to_y(y);mx=x;my=y;
  if(down){
    if(pv_on){pv_stop();return;}
    mdown=1;mclicked=1;clx=x;cly=y;
    if(!menu){int bx,by,bw,bh;strip_button_rect(&bx,&by,&bw,&bh);if(x>=bx&&x<bx+bw&&y>=by&&y<by+bh){menu=1;clearHeld();mclicked=0;}}
  }else{mdown=0;active_id=0;}
}
void app_wheel(int dy){wheel_acc+=dy;}
void app_focus_lost(void){clearHeld();mdown=0;active_id=0;if(!over&&!menu&&pieces>0)paused=1;}
void app_audio(float*out,int frames){audio_mix(out,frames);}
void app_shutdown(void){focus_commit();endRun();save_settings();}

// ---- Optional play timer and real-time clock: placed and styled from the Theming tab; stacked when they share a place ----
typedef struct{char s[24];double sz,w,h,asc;}ClkBox;
static void clk_ctx(int i){fctx_on=1;fctx_fam=CLK[i].fam;fctx_bold=CLK[i].bold;fctx_ital=0;}
// Digits all get the width of a "0" so the text does not jitter as the numbers change.
static double clk_width(double sz,const char*s){double dw=text_w(0,sz,"0"),w=0;for(;*s;s++){char c[2]={*s,0};w+=(*s>='0'&&*s<='9')?dw:text_w(0,sz,c);}return w;}
static void clk_string(int i,char*s,int n){
  if(i==0){int sec=(int)floor(playMs/1000);if(timerMs){int ms=(int)floor(playMs)%1000;if(sec>=3600)snprintf(s,n,"%d:%02d:%02d.%03d",sec/3600,sec/60%60,sec%60,ms);else snprintf(s,n,"%d:%02d.%03d",sec/60,sec%60,ms);return;}if(sec>=3600)snprintf(s,n,"%d:%02d:%02d",sec/3600,sec/60%60,sec%60);else snprintf(s,n,"%d:%02d",sec/60,sec%60);return;}
  // Real time: the device's UTC clock plus the offset typed in the preferences. The device's own timezone is never read.
  int off=(clkTzH<0?-1:1)*(abs(clkTzH)*60+clkTzM)+(clkDst?60:0);
  time_t t=time(NULL)+(time_t)off*60;struct tm*g=gmtime(&t);if(!g){snprintf(s,n,"--:--");return;}
  if(clk24)snprintf(s,n,clkSecs?"%02d:%02d:%02d":"%02d:%02d",g->tm_hour,g->tm_min,g->tm_sec);
  else{int h=g->tm_hour%12;if(!h)h=12;if(clkSecs)snprintf(s,n,"%d:%02d:%02d %s",h,g->tm_min,g->tm_sec,g->tm_hour<12?"AM":"PM");else snprintf(s,n,"%d:%02d %s",h,g->tm_min,g->tm_hour<12?"AM":"PM");}
}
static void clk_measure(int i,ClkBox*b){
  clk_string(i,b->s,sizeof b->s);clk_ctx(i);b->sz=CLK[i].sz*UZ;b->w=clk_width(b->sz,b->s);
  if(CLK[i].pos==6&&b->w>74){b->sz=b->sz*74/b->w;b->w=clk_width(b->sz,b->s);}   // in the side panel the text shrinks to fit
  double d;text_ascent(0,b->sz,&b->asc,&d);b->h=b->asc-d;fctx_on=0;
}
static void clk_draw(int i,const ClkBox*b,double x,double y){
  // Dark text on light pictures and light text on dark ones, judged from the pixels actually behind the text (so themes, tints and inversion all work).
  int lum=0,cnt=0,gx,gy;for(gy=0;gy<3;gy++)for(gx=0;gx<5;gx++){int px=(int)(x+b->w*(gx+.5)/5),py=(int)(y+b->h*(gy+.5)/3);if(px>=0&&py>=0&&px<FW&&py<FH){u32 c=FB[(size_t)py*FW+px];lum+=(int)((((c>>16)&255)*54+((c>>8)&255)*183+(c&255)*19)>>8);cnt++;}}
  int light=cnt&&lum/cnt>140;u32 fg=light?HEX(0):HEX(0xffffff),sh=light?HEX(0xffffff):HEX(0);double al=CLK[i].op/100.0,dw;
  clk_ctx(i);dw=text_w(0,b->sz,"0");
  int pass;for(pass=0;pass<2;pass++){
    double px=x+(pass==0?UZ:0),py=y+b->asc+(pass==0?UZ:0);const char*p;
    for(p=b->s;*p;p++){char c[2]={*p,0};int dg=*p>='0'&&*p<='9';double cw=text_w(0,b->sz,c);
      text_at(0,b->sz,dg?px+(dw-cw)/2:px,py,pass==0?sh:fg,pass==0?al*.5:al,c);px+=dg?dw:cw;}
  }
  fctx_on=0;
}
// ---- "Now playing": title (and artist/album) of the current music file, with its album art, from the file's own tags ----
static double np_fit(const char*s,double sz,double maxw,char*out,int n){   // shortens with an ellipsis until it fits maxw
  snprintf(out,n,"%s",s);double w=text_w(0,sz,out);if(w<=maxw)return w;
  int len=(int)strlen(out);while(len>1){len--;while(len>0&&((unsigned char)out[len]&0xC0)==0x80)len--;char t[260];memcpy(t,out,(size_t)len);t[len]=0;snprintf(out,n,"%s\xE2\x80\xA6",t);out[len+3]=0;if(text_w(0,sz,out)<=maxw){out[n-1]=0;return text_w(0,sz,out);}out[len]=0;}
  return text_w(0,sz,out);
}
static void draw_nowplaying(void){
  if(!NP.on||!musMode||!mus_buf)return;
  int stW=stripX();double m=12*UZ,sz=NP.sz*UZ,maxw=stW*.45,gap=10*UZ;
  char l1[300],l2[300],det[300];const Meta*mt=&cur_meta;
  fctx_on=1;fctx_fam=NP.fam;fctx_bold=NP.bold;fctx_ital=0;
  det[0]=0;if(npArtist){if(mt->artist[0]&&mt->album[0])snprintf(det,sizeof det,"%s \xC2\xB7 %s",mt->artist,mt->album);else if(mt->artist[0])snprintf(det,sizeof det,"%s",mt->artist);else if(mt->album[0])snprintf(det,sizeof det,"%s",mt->album);}
  double asc,desc;text_ascent(0,sz,&asc,&desc);double th=asc-desc,sz2=sz*.78,asc2,desc2;text_ascent(0,sz2,&asc2,&desc2);double th2=asc2-desc2;
  double w1=np_fit(mt->title,sz,maxw,l1,sizeof l1),w2=det[0]?np_fit(det,sz2,maxw,l2,sizeof l2):0;
  double tw=w1>w2?w1:w2,tH=th+(det[0]?th2+3*UZ:0);
  int hasArt=npArt&&mt->art;double S=npArtSz*UZ,aw=0,ah=0;
  if(hasArt){double sc=S/(mt->aw>mt->ah?mt->aw:mt->ah);aw=mt->aw*sc;ah=mt->ah*sc;}
  double W_=(hasArt?S+gap:0)+tw,H_=hasArt?(S>tH?S:tH):tH;
  int pos=NP.pos>5?5:NP.pos,col=pos%3;double x0=col==0?m:col==1?(stW-W_)/2:stW-m-W_,y0=pos<3?m:WH-m-H_;
  double tx=x0+(hasArt?S+gap:0),ty=y0+(H_-tH)/2;
  int lum=0,cnt=0,gx,gy;for(gy=0;gy<3;gy++)for(gx=0;gx<5;gx++){int px=(int)(x0+W_*(gx+.5)/5),py=(int)(y0+H_*(gy+.5)/3);if(px>=0&&py>=0&&px<FW&&py<FH){u32 c=FB[(size_t)py*FW+px];lum+=(int)((((c>>16)&255)*54+((c>>8)&255)*183+(c&255)*19)>>8);cnt++;}}
  int light=cnt&&lum/cnt>140;u32 fg=light?HEX(0):HEX(0xffffff),sh=light?HEX(0xffffff):HEX(0);double al=NP.op/100.0;
  if(hasArt){   // the picture, centered in its square, scaled with nearest-neighbour sampling
    int ix=(int)(x0+(S-aw)/2),iy=(int)(y0+(H_-ah)/2),iw=(int)aw,ih=(int)ah,x,y,a255=(int)(al*255+.5);
    for(y=0;y<ih;y++)for(x=0;x<iw;x++){int sx=x*mt->aw/iw,sy=y*mt->ah/ih;putpx(ix+x,iy+y,mt->art[sy*mt->aw+sx],a255);}
    border_in(ix-1,iy-1,iw+2,ih+2,1,fg,al*.35);
  }
  {int pass;for(pass=0;pass<2;pass++){double o=pass==0?UZ:0;u32 c=pass==0?sh:fg;double a=pass==0?al*.5:al;
    text_at(0,sz,tx+o,ty+asc+o,c,a,l1);if(det[0])text_at(0,sz2,tx+o,ty+th+3*UZ+asc2+o,c,a*.85,l2);}}
  fctx_on=0;
}
static void draw_clocks(void){
  ClkBox b[2];int i,pos,stW=stripX();double m=12*UZ,gap_v=4*UZ,gap_h=16*UZ;
  for(i=0;i<2;i++)if(CLK[i].on)clk_measure(i,&b[i]);
  for(pos=0;pos<7;pos++){
    int mem[2],k=0;int first=stackFirst?1:0;
    for(i=0;i<2;i++){int j=i?1-first:first;if(CLK[j].on&&CLK[j].pos==pos)mem[k++]=j;}
    if(!k)continue;
    int col=pos<6?pos%3:1,top=pos<3,vert=pos==6||stackV||k==1;
    if(vert){
      double H=0,y0;for(i=0;i<k;i++)H+=b[mem[i]].h+(i?gap_v:0);
      if(pos==6)y0=WH-8-30-10-H;else y0=top?m:WH-m-H;
      for(i=0;i<k;i++){ClkBox*c=&b[mem[i]];double x=pos==6?stW+(80-c->w)/2:col==0?m:col==1?(stW-c->w)/2:stW-m-c->w;clk_draw(mem[i],c,x,y0);y0+=c->h+gap_v;}
    }else{
      double G=gap_h,hm=0,x0,y0;for(i=0;i<k;i++){G+=b[mem[i]].w;if(b[mem[i]].h>hm)hm=b[mem[i]].h;}G-=gap_h*(k==1);
      x0=col==0?m:col==1?(stW-G)/2:stW-m-G;y0=top?m:WH-m-hm;
      for(i=0;i<k;i++){ClkBox*c=&b[mem[i]];clk_draw(mem[i],c,x0,y0+(hm-c->h)/2);x0+=c->w+gap_h;}
    }
  }
}
// ---- Brightness: one lookup-table pass over the finished picture, menu and overlays included. Skipped entirely at 100%. ----
static void bright_apply(void){
  static unsigned char lut[256];static int last=-1;int i;
  if(last!=gBright){for(i=0;i<256;i++){int v=(i*gBright+50)/100;lut[i]=(unsigned char)(v>255?255:v);}last=gBright;}
  u32*p=FB;size_t n=(size_t)FW*FH,k;
  for(k=0;k<n;k++){u32 c=p[k];p[k]=(c&0xFF000000u)|((u32)lut[(c>>16)&255]<<16)|((u32)lut[(c>>8)&255]<<8)|lut[c&255];}
}
// ---- Filters: one pass over the finished game picture (not the menu). Everything except the color split is a single 3x4 color matrix. ----
static void flt_mul(double M[3][4],const double A[3][4]){double n[3][4];int r,c,k;for(r=0;r<3;r++)for(c=0;c<4;c++){n[r][c]=(c==3)?A[r][3]:0;for(k=0;k<3;k++)n[r][c]+=A[r][k]*M[k][c];}memcpy(M,n,sizeof n);}
static void flt_apply(void){
  double M[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}},A[3][4];int r,c;
  if(fltSat!=100){double s=fltSat/100.0,w[3]={.2126,.7152,.0722};for(r=0;r<3;r++)for(c=0;c<4;c++)A[r][c]=c==3?0:(1-s)*w[c]+(r==c?s:0);flt_mul(M,A);}
  if(fltHue){double a=fltHue*3.14159265358979/180,co=cos(a),si=sin(a);
    double H[3][4]={{.213+co*.787-si*.213,.715-co*.715-si*.715,.072-co*.072+si*.928,0},{.213-co*.213+si*.143,.715+co*.285+si*.140,.072-co*.072-si*.283,0},{.213-co*.213-si*.787,.715-co*.715+si*.715,.072+co*.928+si*.072,0}};flt_mul(M,H);}
  if(fltTint){double t=fltTint/100.0,tc[3]={((fltTintC>>16)&255)/255.0,((fltTintC>>8)&255)/255.0,(fltTintC&255)/255.0};memset(A,0,sizeof A);for(r=0;r<3;r++)A[r][r]=1-t+t*tc[r];flt_mul(M,A);}
  if(fltCon!=100){double k=fltCon/100.0;memset(A,0,sizeof A);for(r=0;r<3;r++){A[r][r]=k;A[r][3]=.5*(1-k);}flt_mul(M,A);}
  if(fltBri){memset(A,0,sizeof A);for(r=0;r<3;r++){A[r][r]=1;A[r][3]=fltBri/100.0;}flt_mul(M,A);}
  if(fltInv){memset(A,0,sizeof A);for(r=0;r<3;r++){A[r][r]=-1;A[r][3]=1;}flt_mul(M,A);}
  if(fltOp!=100){double o=fltOp/100.0,base=pale(C_PAGE)?1:0;memset(A,0,sizeof A);for(r=0;r<3;r++){A[r][r]=o;A[r][3]=base*(1-o);}flt_mul(M,A);}
  int mi[3][3],bi[3];for(r=0;r<3;r++){for(c=0;c<3;c++)mi[r][c]=(int)floor(M[r][c]*256+.5);bi[r]=(int)floor(M[r][3]*255*256+.5);}
  int d=fltSplit?imax(1,fltSplit*FH/720):0,y,x;static u32*row=NULL;static int rowN=0;
  if(d&&rowN<FW){free(row);row=(u32*)malloc(sizeof(u32)*(size_t)FW);rowN=row?FW:0;if(!row)d=0;}
  // Without hue or saturation changes every channel is independent, so three lookup tables do the whole job.
  int cross=0;for(r=0;r<3;r++)for(c=0;c<3;c++)if(r!=c&&mi[r][c])cross=1;
  static unsigned char lut[3][256];if(!cross){for(r=0;r<3;r++)for(c=0;c<256;c++){int v=(mi[r][r]*c+bi[r])>>8;lut[r][c]=(unsigned char)(v<0?0:v>255?255:v);}}
  for(y=0;y<FH;y++){u32*p=FB+(size_t)y*FW;const u32*src=p;
    if(d){memcpy(row,p,sizeof(u32)*(size_t)FW);src=row;}
    u32 memo_in=0,memo_out=0;int memo=0;   // runs of identical pixels (a black page, a flat panel) are converted once
    for(x=0;x<FW;x++){
      u32 px=src[x];
      if(d){u32 lo=src[x-d<0?0:x-d],hi=src[x+d>=FW?FW-1:x+d];px=(px&0xFF00FF00u)|(lo&0x00FF0000u)|(hi&0x000000FFu);}
      if(memo&&px==memo_in){p[x]=memo_out;continue;}
      int R=(px>>16)&255,G=(px>>8)&255,B=px&255,o0,o1,o2;
      if(cross){
        o0=(mi[0][0]*R+mi[0][1]*G+mi[0][2]*B+bi[0])>>8;o1=(mi[1][0]*R+mi[1][1]*G+mi[1][2]*B+bi[1])>>8;o2=(mi[2][0]*R+mi[2][1]*G+mi[2][2]*B+bi[2])>>8;
        o0=o0<0?0:o0>255?255:o0;o1=o1<0?0:o1>255?255:o1;o2=o2<0?0:o2>255?255:o2;
      }else{o0=lut[0][R];o1=lut[1][G];o2=lut[2][B];}
      u32 out=0xFF000000u|((u32)o0<<16)|((u32)o1<<8)|(u32)o2;p[x]=out;memo_in=px;memo_out=out;memo=1;
    }
  }
}
void app_frame(double now,u32*fb){
  double dt=prev_t<0?0:now-prev_t;prev_t=now;dt=dclamp(dt,0,100);NOW=now;
  gfx_target(fb,WW,WH);unclip();
  snd_voice=snd_theme();resolve_fx();
  game_step(dt);
  if(board)refill();   // the "next pieces shown" count can change at any time
  if(T(C_PAGE)==TH_SWIRL){swirl_fill(0,0,0,WW,WH,6,tm(now),.9,50);swirl_loops(WW,WH,tm(now));}
  else if(T(C_PAGE)==TH_WILD)wild_page(now);else fill_rect(0,0,WW,WH,pale(C_PAGE)?HEX(0xffffff):flat(C_PAGE)?HEX(flat(C_PAGE)->page):HEX(0),1);
  // board layout inside the stage (everything left of the strip)
  int stW=stripX();S=imax(1,imin((stW-16)/W,(WH-16)/H));
  fx_shake(now);
  double bw=W*S,bh=H*S;int ox,oy;
  {int lx=8,hx=stW-(int)bw-8,ly=8,hy=WH-(int)bh-8;   // the board slides between the edges of the play area, keeping an 8 px margin when there is room
   ox=hx>lx?lx+(int)((hx-lx)*boardPX/100.0+.5):(stW-(int)bw)/2;oy=hy>ly?ly+(int)((hy-ly)*boardPY/100.0+.5):(WH-(int)bh)/2;}
  BX=ox+SHX;BY=oy+SHY;
  frame_draw(bw,bh,now);draw_board(now);draw_cover(pv_on);
  BX=ox;BY=oy;
  draw_strip(now);
  music_tick();
  song_tick((!paused&&!over&&!menu)||pv_on);
  if(flt_active())flt_apply();
  if(!pv_on){draw_clocks();draw_nowplaying();}
  ui_frame(now);
  {static double t0=0;static int n=0;static double fps=0;n++;if(now-t0>=500){fps=n*1000.0/(now-t0);n=0;t0=now;}
   if(showFps&&!pv_on){char b[64];snprintf(b,sizeof b,"%.0f fps  %d\xC3\x97%d",fps,WW,WH);double sz=12*UZ;int w=(int)text_w(0,sz,b)+12*UZ;fill_rect(6,6,w,22*UZ,HEX(0),.6);text_mid(0,sz,12,6,22*UZ,HEX(0xffffff),1,b);}}
  if(gBright!=100)bright_apply();   // last of all, so it covers the menu and every overlay too
}

// ---- Built-in smoke test: `8tris --selftest` plays a scripted game with the menu and effects, mixes audio, and checks the output. ----
int app_selftest(void){
  static float au[2*1024];u32*fb=NULL;int i,bad=0,fw=0,fh=0;double t=1000,peak=0;long lit=0;
  store_off=1;app_init();app_resize(800,600);resSet=0;
  for(i=0;i<400;i++){t+=16.7;if(i%9==0){app_key('d',1,0);app_key('d',0,0);}if(i%13==0){app_key('j',1,0);app_key('j',0,0);}if(i%31==0){app_key(' ',1,0);app_key(' ',0,0);}fb=app_render(t,&fw,&fh);app_audio(au,1024);}
  app_key(K_ESC,1,0);app_key(K_ESC,0,0);for(i=0;i<20;i++){t+=16.7;fb=app_render(t,&fw,&fh);}
  if(!menu){printf("FAIL: Esc did not open preferences\n");bad++;}
  {int k;fxMaster=4;resolve_fx();pv_start();for(k=0;k<500;k++){t+=16.7;fb=app_render(t,&fw,&fh);app_audio(au,1024);int j;for(j=0;j<2048;j++){if(au[j]!=au[j]){bad++;break;}if(fabs(au[j])>peak)peak=fabs(au[j]);}}}
  for(i=0;i<fw*fh;i++)if(fb[i]&0xffffff)lit++;
  if(lit<1000){printf("FAIL: frame is blank\n");bad++;}
  if(peak>1.0){printf("FAIL: audio clips (peak %.3f)\n",peak);bad++;}
  printf("selftest: %s (frame pixels lit %ld, audio peak %.3f, score %d)\n",bad?"FAILED":"ok",lit,peak,(int)score);
  return bad?1:0;
}
