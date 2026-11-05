// ===== APPLICATION GLUE: layout, page and side panel, input routing =====
static int WW=900,WH=700,focused=1,winW=900,winH=700,last_res=-1;static double prev_t=-1;   // WW,WH: the size the game draws at; winW,winH: the window
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
  for(by=0;by<AH;by++)for(bx=0;bx<AW;bx++){
    int d=(int)ANG[by*AW+bx];int ia=((d+ra)%360+360)%360,ib=((360-d+rb)%360+360)%360;u32 a=la[ia],b=lb[ib];
    u32 c=RGB((((a>>16)&255)+((b>>16)&255))/2,(((a>>8)&255)+((b>>8)&255))/2,((a&255)+(b&255))/2);
    int x0=bx*4,y0=by*4;for(j=0;j<4&&y0+j<WH;j++)for(i=0;i<4&&x0+i<WW;i++)FB[(y0+j)*FW+x0+i]=c;}
}
// ---- Side strip: held pieces, next pieces and the Settings button ----
static int stripX(void){return WW-80;}
static void strip_button_rect(int*x,int*y,int*w,int*h){*x=stripX()+6;*w=68;*h=30;*y=WH-8-30;}
static void draw_strip(double now){
  int sx=stripX(),i;
  if(T(C_PANEL)==TH_SWIRL){swirl_fill(1,sx,0,80,WH,6,tm(now)+4000,.32,50);
    {int yy;for(yy=0;yy<WH;yy++){double dx=sin(yy/26.0+tm(now)*.0014)*3,dx2=-dx;disc(sx+4+dx,yy,1.8,sw_col(yy/260.0+tm(now)*.00012),.95);disc(sx+4+dx2,yy,1.8,sw_col(yy/260.0+.5+tm(now)*.00012),.95);}}
  }else if(T(C_PANEL)==TH_WILD){
    for(i=0;i<WH;i++){double k=(double)i/WH;fill_rect(sx,i,80,1,RGB((int)(30*(1-k)),(int)(25*k),(int)(60)),.92);}
    fill_rect(sx,0,3,WH,hsl(tm(now)/10,1,.6),1);
  }else if(pale(C_PANEL)){fill_rect(sx,0,80,WH,HEX(0xffffff),1);fill_rect(sx,0,1,WH,LG(C_PANEL)?HEX(0xd6d6d6):HEX(0),1);}
  else fill_rect(sx,0,80,WH,HEX(0),1);
  int y=8;
  for(i=0;i<slots;i++){
    if(y+64>WH-60)break;
    if(slots>1&&i==sel)border_in(sx,y,80,64,2,LG(C_PANEL)?HEX(0x8f8f8f):LT(C_PANEL)?HEX(0):HEX(0xe0e0e0),.8);
    if(holds[i])draw_small(holds[i],sx,y,canHold?1:.4,now);y+=64;
  }
  if(T(C_PANEL)==TH_SWIRL){int xx;for(xx=8;xx<76;xx++)aa_px(sx+xx,y+sin(xx/7.0+tm(now)*.002)*2,sw_col(xx/80.0+tm(now)*.0001),.9);}else fill_rect(sx,y,80,1,T(C_PANEL)==TH_WILD?HEX(0x00e5ff):LG(C_PANEL)?HEX(0xd0d0d0):LT(C_PANEL)?HEX(0x999999):HEX(0x777777),1);y+=16;
  for(i=0;i<NQ&&i<nq;i++){if(y+64>WH-48)break;draw_small(queue[i],sx,y,1,now);y+=64;}
  int bx,by,bw,bh;strip_button_rect(&bx,&by,&bw,&bh);int hot=!menu&&mx>=bx&&mx<bx+bw&&my>=by&&my<by+bh;
  u32 bc=T(C_PANEL)==TH_SWIRL?sw_col(.15+tm(now)*.00005):T(C_PANEL)==TH_WILD?HEX(0xffee00):LG(C_PANEL)?HEX(0x5a5a5a):LT(C_PANEL)?HEX(0):HEX(0xe0e0e0);
  u32 bk=pale(C_PANEL)?HEX(0xffffff):HEX(0);
  fill_rect(bx,by,bw,bh,hot?bc:bk,1);border_in(bx,by,bw,bh,1,bc,1);text_center(0,14,bx+bw/2.0,by+20,hot?bk:(T(C_PANEL)==TH_SWIRL?HEX(0xffffff):bc),1,"Settings");
}

// ---- Entry points ----
void app_init(void){
  int i;rs_^=(u32)time(NULL)*2654435761u;if(!rs_)rs_=1;
  binds_default();for(i=0;i<N_COMPS;i++)TC[i]=TH_MIN;fxMaster=1;
  levels_init();load_settings();hist_load();resolve_fx();pend_load();snd_voice=snd_theme();
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

void app_frame(double now,u32*fb){
  double dt=prev_t<0?0:now-prev_t;prev_t=now;dt=dclamp(dt,0,100);NOW=now;
  gfx_target(fb,WW,WH);unclip();
  snd_voice=snd_theme();resolve_fx();
  game_step(dt);
  if(T(C_PAGE)==TH_SWIRL){swirl_fill(0,0,0,WW,WH,6,tm(now),.9,50);swirl_loops(WW,WH,tm(now));}
  else if(T(C_PAGE)==TH_WILD)wild_page(now);else fill_rect(0,0,WW,WH,pale(C_PAGE)?HEX(0xffffff):HEX(0),1);
  // board layout inside the stage (everything left of the strip)
  int stW=stripX();S=imax(1,imin((stW-16)/W,(WH-16)/H));
  fx_shake(now);
  double bw=W*S,bh=H*S;int ox=(stW-(int)bw)/2,oy=(WH-(int)bh)/2;
  BX=ox+SHX;BY=oy+SHY;
  frame_draw(bw,bh,now);draw_board(now);draw_cover(pv_on);
  BX=ox;BY=oy;
  draw_strip(now);
  ui_frame(now);
  {static double t0=0;static int n=0;static double fps=0;n++;if(now-t0>=500){fps=n*1000.0/(now-t0);n=0;t0=now;}
   if(showFps&&!pv_on){char b[64];snprintf(b,sizeof b,"%.0f fps  %d\xC3\x97%d",fps,WW,WH);double sz=12*UZ;int w=(int)text_w(0,sz,b)+12*UZ;fill_rect(6,6,w,22*UZ,HEX(0),.6);text_mid(0,sz,12,6,22*UZ,HEX(0xffffff),1,b);}}
}

// ---- Built-in smoke test: `8tris --selftest` plays a scripted game with the menu and effects, mixes audio, and checks the output. ----
int app_selftest(void){
  static float au[2*1024];u32*fb=NULL;int i,bad=0,fw=0,fh=0;double t=1000,peak=0;long lit=0;
  app_init();app_resize(800,600);resSet=0;
  for(i=0;i<400;i++){t+=16.7;if(i%9==0){app_key('d',1,0);app_key('d',0,0);}if(i%13==0){app_key('j',1,0);app_key('j',0,0);}if(i%31==0){app_key(' ',1,0);app_key(' ',0,0);}fb=app_render(t,&fw,&fh);app_audio(au,1024);}
  app_key(K_ESC,1,0);app_key(K_ESC,0,0);for(i=0;i<20;i++){t+=16.7;fb=app_render(t,&fw,&fh);}
  if(!menu){printf("FAIL: Esc did not open settings\n");bad++;}
  {int k;fxMaster=4;resolve_fx();pv_start();for(k=0;k<500;k++){t+=16.7;fb=app_render(t,&fw,&fh);app_audio(au,1024);int j;for(j=0;j<2048;j++){if(au[j]!=au[j]){bad++;break;}if(fabs(au[j])>peak)peak=fabs(au[j]);}}}
  for(i=0;i<fw*fh;i++)if(fb[i]&0xffffff)lit++;
  if(lit<1000){printf("FAIL: frame is blank\n");bad++;}
  if(peak>1.0){printf("FAIL: audio clips (peak %.3f)\n",peak);bad++;}
  printf("selftest: %s (frame pixels lit %ld, audio peak %.3f, score %d)\n",bad?"FAILED":"ok",lit,peak,(int)score);
  return bad?1:0;
}
