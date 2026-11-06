// ===== PREFERENCES SCREEN, POPUPS AND EFFECT PREVIEW =====
// An immediate-mode interface: every frame the panel is walked top to bottom, each widget draws itself and reports clicks.
// Widgets: ui_head (collapsible section), ui_para / ui_warn (text), ui_check, ui_num, ui_slider, ui_select, ui_btn.
static int UZ=1;
static double UF=1;
static const double MWF[3]={1,1.3,1.6},LSF[3]={1.45,1.7,2.0};   // menu width and paragraph line spacing choices
#define LH (LSF[lsSel])      // extra zoom used only while the preferences screen is open, so its text and controls are larger
#define U(x) ((int)((x)*UZ*UF))
static int mx=-9999,my=-9999,mdown=0,mclicked=0,clx=0,cly=0,ui_changed=0;
static double wheel_acc=0;
static unsigned active_id=0;
typedef struct{u32 bg,fg,ln,sep;int wild,swirl,light;}Pal;
static Pal P;
static void pal_update(void){
  P.swirl=0;P.light=0;
  if(T(C_MENU)==TH_SWIRL){P.wild=0;P.swirl=1;P.bg=HEX(0x14081c);P.fg=HEX(0xfff4fb);P.ln=sw_col(.3+tm(NOW)*.00006);P.sep=sw_col(.8+tm(NOW)*.00006);}
  else if(T(C_MENU)==TH_WILD){P.wild=1;P.bg=HEX(0x14002a);P.fg=HEX(0xffffff);P.ln=HEX(0x00e5ff);P.sep=HEX(0xd500f9);}
  else if(flat(C_MENU)){const Flat*f=flat(C_MENU);P.wild=0;P.light=f->light;P.bg=HEX(f->mbg);P.fg=HEX(f->mfg);P.ln=HEX(f->mln);P.sep=HEX(f->msep);}
  else if(T(C_MENU)==TH_GRAPHITE){P.wild=0;P.light=1;P.bg=HEX(0xffffff);P.fg=HEX(0x2e2e2e);P.ln=HEX(0x9a9a9a);P.sep=HEX(0xdcdcdc);}   // light grey lines, softer than 8tris Light
  else if(T(C_MENU)==TH_LIGHT){P.wild=0;P.light=1;P.bg=HEX(0xffffff);P.fg=HEX(0x101010);P.ln=HEX(0x101010);P.sep=HEX(0xc4c4c4);}
  else{P.wild=0;P.bg=HEX(0x000000);P.fg=HEX(0xe0e0e0);P.ln=HEX(0xe0e0e0);P.sep=HEX(0x444444);}
  if(hiCon){if(P.light){P.bg=HEX(0xffffff);P.fg=HEX(0);P.ln=HEX(0);P.sep=HEX(0x505050);}else{P.wild=0;P.swirl=0;P.bg=HEX(0);P.fg=HEX(0xffffff);P.ln=HEX(0xffffff);P.sep=HEX(0x9a9a9a);}}   // high contrast menu: pure black and white, whatever the menu theme
}
// view being laid out (screen coordinates)
static int PX,PW,PVY,PVH,VX0,VX1,cy;
static int CWd(void){return U(168);}
static int vis(int y,int h){return y+h>PVY&&y<PVY+PVH;}
static int uhit(int x,int y,int w,int h){return mclicked&&clx>=x&&clx<x+w&&cly>=y&&cly<y+h&&cly>=PVY&&cly<PVY+PVH;}
static int sel_open=0;
static int uhov(int x,int y,int w,int h){return !sel_open&&mx>=x&&mx<x+w&&my>=y&&my<y+h&&my>=PVY&&my<PVY+PVH;}
static unsigned hid_salt=0;   // set inside a collapsible group, so the same short label ("Size") in two groups is two controls
static unsigned hid(const char*s,int k){unsigned h=2166136261u^(hid_salt*2654435761u);while(*s)h=(h^(unsigned char)*s++)*16777619u;return h^((unsigned)k*40503u);}

// ---- text ----
static void label(int y,const char*s){double sz=U(14),mw=PW-CWd()-U(8);while(sz>8&&text_w(0,sz,s)>mw)sz--;text_mid(0,sz,PX,y,U(30),P.fg,1,s);}
// Wraps s into width w and (if draw) paints it; returns the height used.
static int para(int font,int sz,int x,int y,int w,double lh,u32 col,double al,const char*s,int draw){
  Line L[256];int n=wrap_lines(font,sz,w,s,L,256),i;int step=(int)(sz*lh);char tmp[600];
  if(draw)for(i=0;i<n;i++){int yy=y+i*step;if(yy+step<PVY||yy>PVY+PVH)continue;int len=imin(L[i].n,599);memcpy(tmp,L[i].p,len);tmp[len]=0;text_mid(font,sz,x,yy,step,col,al,tmp);}
  return n*step;
}
static void ui_keys(const char*s){cy+=U(6);cy+=para(0,U(12),PX,cy,PW,LH,P.fg,hiCon?1:.6,s,1);cy+=U(6);}
static void ui_warn(const char*s){
  int h=para(0,U(14),0,0,PW-U(16),LH,P.fg,1,s,0)+U(12),y=cy+U(8);
  if(vis(y,h)){border_in(PX,y,PW,h,1,P.fg,1);para(0,U(14),PX+U(8),y+U(6),PW-U(16),LH,P.fg,1,s,1);}
  cy+=h+U(16);
}
static void ui_sub(const char*s){cy+=U(12);if(vis(cy,U(20)))text_mid(0,U(14),PX,cy,U(20),P.fg,.8,s);cy+=U(24);}
static int sec_open[8]={1,0,0,0,0,0,0,0},want_top=0;
static int ui_head(const char*s,int*open,double now){
  // In the preferences screen the tabs at the top pick the section, so only the open section shows its heading.
  if(open>=sec_open&&open<sec_open+8&&!*open){int k,any=0;for(k=0;k<8;k++)any|=sec_open[k];if(any)return 0;}
  int h=U(34),y=cy;cy+=h;
  if(vis(y,h)){
    if(uhov(PX,y,PW,h))fill_rect(PX,y,PW,h,P.fg,.09);
    u32 c=P.swirl?sw_col(y/400.0+tm(now)*.0003):P.wild?hsl(now/10+y/3.0,1,.65):P.fg;text_mid(1,U(14),PX,y,h,c,1,s);
    text_right(0,U(16),PX+PW,y+U(9)+U(12),P.fg,.6,*open?"\xE2\x88\x92":"+");
    fill_rect(PX,y+h-1,PW,1,P.sep,1);
    if(uhit(PX,y,PW,h)){if(open>=sec_open&&open<sec_open+8){if(!*open){*open=1;int k;for(k=0;k<8;k++)if(sec_open+k!=open)sec_open[k]=0;want_top=1;}}else *open=!*open;}   // settings sections work like an accordion: opening one closes the rest
  }
  if(*open)cy+=U(4);
  return *open;
}
// ---- controls ----
static void box(int x,int y,int w,int h,int hot){fill_rect(x,y,w,h,hot?P.fg:P.bg,1);border_in(x,y,w,h,1,P.ln,1);}
static int btn_at(int x,int y,int w,const char*lab){
  int h=U(30),hot=uhov(x,y,w,h);box(x,y,w,h,hot);
  double sz=U(14);while(sz>8&&text_w(0,sz,lab)>w-U(8))sz--;
  text_center(0,sz,x+w/2.0,y+(h-0)/2.0+sz*.36-sz*.0,hot?P.bg:P.fg,1,lab);
  return uhit(x,y,w,h);
}
// One button shape everywhere: the same height and padding, as wide as its label needs. Buttons that open a popup end in "...".
static int btn_w(const char*lab){return imin(PW,imax(U(80),(int)text_w(0,U(14),lab)+U(32)));}
// A row of buttons on its own line, left-aligned, wrapping onto more lines if they do not fit. Returns the one clicked, or -1.
static int ui_buttons(int n,const char*const*labs){
  int i,x=PX,hit=-1;cy+=U(2);
  for(i=0;i<n;i++){int w=btn_w(labs[i]);if(x>PX&&x+w>PX+PW){x=PX;cy+=U(38);}if(vis(cy,U(30))&&btn_at(x,cy,w,labs[i]))hit=i;x+=w+U(8);}
  cy+=U(42);return hit;}
static int ui_button(const char*lab){return ui_buttons(1,&lab)==0;}
// The same buttons in a dialog's fixed footer, placed left to right from *x.
static int foot_btn(int*x,int y,const char*lab){int w=btn_w(lab),r=btn_at(*x,y,w,lab);*x+=w+U(8);return r;}

// ---- collapsible groups: a row with an arrow (and an optional on/off box) that shows its options when opened ----
// Settings with several options sit in a group so the menu stays short. Ticking a group's box turns the feature on and opens it.
enum{G_TIMER,G_CLOCK,G_STACK,G_MUSIC,G_TRACK,G_BOARDPOS,G_MTEXT,G_MLAYOUT,G_PAUSE,G_N};
static int grp_open[G_N];static int grp_y0[4],grp_dep=0;static unsigned grp_salt[4];
static int ui_group(const char*lab,int*on,int g){
  int y=cy,h=U(30);cy+=U(36);
  if(vis(y,h)){
    if(uhov(PX,y,PW,h))fill_rect(PX,y,PW,h,P.fg,.07);
    text_mid(0,U(12),PX+U(4),y,h,P.fg,.8,grp_open[g]?"\xE2\x96\xBE":"\xE2\x96\xB8");
    double sz=U(14);while(sz>8&&text_w(0,sz,lab)>PW-U(64))sz--;text_mid(0,sz,PX+U(22),y,h,P.fg,1,lab);
    if(on){int bx=PX+PW-U(18),by=y+U(6);box(bx,by,U(18),U(18),0);if(*on)fill_rect(bx+U(4),by+U(4),U(10),U(10),P.fg,1);
      if(uhit(bx-U(10),y,U(28),h)){*on=!*on;ui_changed=1;if(*on)grp_open[g]=1;}
      else if(uhit(PX,y,PW-U(28),h))grp_open[g]=!grp_open[g];}
    else if(uhit(PX,y,PW,h))grp_open[g]=!grp_open[g];
  }
  if(!grp_open[g]||(on&&!*on)||grp_dep>=4)return 0;
  grp_salt[grp_dep]=hid_salt;grp_y0[grp_dep++]=cy;hid_salt=(unsigned)g+1;PX+=U(18);PW-=U(18);return 1;}
static void ui_group_end(void){PX-=U(18);PW+=U(18);int y0=grp_y0[--grp_dep];hid_salt=grp_salt[grp_dep];if(cy-y0>U(8))fill_rect(PX+U(8),y0,1,cy-y0-U(8),P.sep,1);cy+=U(4);}
static int ui_check(const char*lab,int*v){
  int y=cy;cy+=U(36);if(!vis(y,U(30)))return 0;
  label(y,lab);int bx=PX+PW-U(18),by=y+U(6);box(bx,by,U(18),U(18),0);if(*v)fill_rect(bx+U(4),by+U(4),U(10),U(10),P.fg,1);
  if(uhit(PX,y,PW,U(30))){*v=!*v;ui_changed=1;return 1;}
  return 0;
}
// A checkbox with its label on the right, placed at (x,y) inside a flowing grid.
static int check_at(int x,int y,int w,const char*lab,int*v){
  if(!vis(y,U(24)))return 0;
  box(x,y+U(3),U(18),U(18),0);if(*v)fill_rect(x+U(4),y+U(7),U(10),U(10),P.fg,1);text_mid(0,U(14),x+U(26),y,U(24),P.fg,1,lab);
  if(uhit(x,y,w,U(24))){*v=!*v;ui_changed=1;return 1;}return 0;
}
// Hex color entry state (the colors for the Solid Color theme). Typing a full 6 digits applies live; Enter or clicking away also accepts 3 digits.
static unsigned hx_id=0;static u32*hx_ptr=NULL;static char hx_buf[8];static int hx_fresh=0,hx_x,hx_y,hx_w,hx_h;
static void hx_commit(void){if(!hx_id)return;int n=(int)strlen(hx_buf);
  if(n==3){u32 v=(u32)strtoul(hx_buf,NULL,16),r=(v>>8)&15,g=(v>>4)&15,b=v&15;*hx_ptr=HEX(((r*17)<<16)|((g*17)<<8)|(b*17));}
  else if(n==6)*hx_ptr=HEX(strtoul(hx_buf,NULL,16));
  hx_id=0;ui_changed=1;}
static double*f_dptr=NULL;static int f_fresh=0;static unsigned focus_id=0;static char fbuf[12];static int*f_ptr,f_lo,f_hi,f_x,f_y,f_w,f_h;
static void focus_commit(void);
static void fall_typed(double rate);
static void focus_commit(void){hx_commit();if(!focus_id)return;
  if(f_dptr){double r=fbuf[0]?atof(fbuf):0;if(fbuf[0]&&r>0){r=dclamp(r,f_lo/1000.0,f_hi/1000.0);fall_typed(r);}f_dptr=NULL;}
  else{int v=fbuf[0]?atoi(fbuf):*f_ptr;*f_ptr=iclamp(v,f_lo,f_hi);}
  focus_id=0;ui_changed=1;}
static int ui_num(const char*lab,int*v,int lo,int hi){
  int y=cy,x=PX+PW-CWd(),w=CWd(),h=U(30);unsigned id=hid(lab,0);cy+=U(36);if(!vis(y,h))return 0;
  label(y,lab);
  if(uhit(x,y,w,h)&&focus_id!=id){focus_commit();focus_id=id;f_fresh=1;f_dptr=NULL;snprintf(fbuf,sizeof fbuf,"%d",*v);f_ptr=v;f_lo=lo;f_hi=hi;}
  box(x,y,w,h,0);char b[16];
  if(focus_id==id){f_x=x;f_y=y;f_w=w;f_h=h;f_ptr=v;snprintf(b,16,"%s",fbuf);border_in(x,y,w,h,2,P.fg,1);if(f_fresh)fill_rect(x+U(8),y+U(5),text_w(0,U(14),b),h-U(10),P.fg,.35);text_mid(0,U(14),x+U(8),y,h,P.fg,1,b);
    if(((int)(NOW/500))%2)fill_rect(x+U(8)+text_w(0,U(14),b)+1,y+U(6),1,h-U(12),P.fg,1);}
  else{snprintf(b,16,"%d",*v);text_mid(0,U(14),x+U(8),y,h,P.fg,1,b);}
  return 0;
}
// Number box for a decimal value (used where a rate can be typed exactly, like 0.75).
static int ui_numd(const char*lab,double*v,double lo,double hi,double*out){
  int y=cy,x=PX+PW-CWd(),w=CWd(),h=U(30);unsigned id=hid(lab,3);cy+=U(36);if(!vis(y,h))return 0;
  label(y,lab);
  if(uhit(x,y,w,h)&&focus_id!=id){focus_commit();focus_id=id;f_fresh=1;snprintf(fbuf,sizeof fbuf,"%.2f",*v);{int n=(int)strlen(fbuf);while(n>1&&fbuf[n-1]=='0'){fbuf[--n]=0;}if(n>0&&fbuf[n-1]=='.')fbuf[n-1]=0;}f_ptr=NULL;f_dptr=out;f_lo=(int)(lo*1000);f_hi=(int)(hi*1000);}
  box(x,y,w,h,0);char b[16];
  if(focus_id==id){f_x=x;f_y=y;f_w=w;f_h=h;snprintf(b,16,"%s",fbuf);border_in(x,y,w,h,2,P.fg,1);if(f_fresh)fill_rect(x+U(8),y+U(5),text_w(0,U(14),b),h-U(10),P.fg,.35);text_mid(0,U(14),x+U(8),y,h,P.fg,1,b);
    if(((int)(NOW/500))%2)fill_rect(x+U(8)+text_w(0,U(14),b)+1,y+U(6),1,h-U(12),P.fg,1);}
  else{snprintf(b,16,"%.2f",*v);text_mid(0,U(14),x+U(8),y,h,P.fg,1,b);}
  return 0;
}
static void fall_typed(double rate);
// Fall rate: drag the slider, or click the number and type an exact rate (rows per second; a typed rate may go past the slider's ends).
static void ui_fall(void){
  const char*lab="Fall rate (rows/s)";int y=cy,x=PX+PW-CWd(),w=CWd(),h=U(30);unsigned id=hid(lab,1),fid=hid(lab,3);cy+=U(36);if(!vis(y,h))return;
  label(y,lab);int bw=U(72),bx=x+w-bw,tx=x+U(6),tw=w-bw-U(18);
  if(uhit(tx-U(6),y,tw+U(12),h))active_id=id;
  if(active_id==id&&mdown){double nv=floor(dclamp((double)(mx-tx)/tw*100,0,100)+.5);if(nv!=fallV){fallV=nv;ui_changed=1;if(pieces>0&&!over)runDirty=1;}}
  double fr=dclamp(fallV/100,0,1);fill_rect(tx,y+h/2,tw,2,P.fg,.5);fill_rect(tx,y+h/2,tw*fr,2,P.fg,1);fill_rect(tx+tw*fr-U(5),y+h/2-U(8),U(10),U(17),P.fg,1);
  if(uhit(bx,y,bw,h)&&focus_id!=fid){focus_commit();focus_id=fid;f_fresh=1;snprintf(fbuf,sizeof fbuf,"%.2f",rateOf(fallV));{int n=(int)strlen(fbuf);while(n>1&&fbuf[n-1]=='0'){fbuf[--n]=0;}if(n>0&&fbuf[n-1]=='.')fbuf[n-1]=0;}f_ptr=NULL;f_dptr=&fallV;f_lo=50;f_hi=1000000;}
  box(bx,y,bw,h,0);char b[16];
  if(focus_id==fid){f_x=bx;f_y=y;f_w=bw;f_h=h;snprintf(b,16,"%s",fbuf);border_in(bx,y,bw,h,2,P.fg,1);if(f_fresh)fill_rect(bx+U(8),y+U(5),text_w(0,U(14),b),h-U(10),P.fg,.35);text_mid(0,U(14),bx+U(8),y,h,P.fg,1,b);
    if(((int)(NOW/500))%2)fill_rect(bx+U(8)+text_w(0,U(14),b)+1,y+U(6),1,h-U(12),P.fg,1);}
  else{double r=rateOf(fallV);snprintf(b,16,r<10?"%.2f":r<100?"%.1f":"%.0f",r);text_mid(0,U(14),bx+U(8),y,h,P.fg,1,b);}
}
// Slider with its value shown in a box on the right: drag the slider, or click the box and type an exact number (Enter or a click elsewhere accepts it).
static int sl_notype=0;   // set around sliders whose value is better typed another way (the colour picker has a hex box)
static int ui_slider(const char*lab,int*v,int lo,int hi,const char*vt){
  int y=cy,x=PX+PW-CWd(),w=CWd(),h=U(30);unsigned id=hid(lab,1),fid=hid(lab,6);int rv=0;cy+=U(36);if(!vis(y,h))return 0;
  label(y,lab);int bw=U(72),bx=x+w-bw,tx=x+U(4),tw=w-bw-U(18);
  if(uhit(tx-U(6),y,tw+U(12),h))active_id=id;
  if(active_id==id&&mdown){int nv=lo+(int)floor((double)(mx-tx)/tw*(hi-lo)+.5);nv=iclamp(nv,lo,hi);if(nv!=*v){*v=nv;ui_changed=1;rv=1;}}
  fill_rect(tx,y+h/2,tw,2,P.fg,.5);double fr=hi>lo?(double)(*v-lo)/(hi-lo):0;
  fill_rect(tx,y+h/2,tw*fr,2,P.fg,1);fill_rect(tx+tw*fr-U(5),y+h/2-U(8),U(10),U(17),P.fg,1);
  if(sl_notype){text_right(0,U(12),x+w,y+h/2+U(4),P.fg,1,vt);return rv;}
  if(uhit(bx,y,bw,h)&&focus_id!=fid){focus_commit();focus_id=fid;f_fresh=1;f_dptr=NULL;snprintf(fbuf,sizeof fbuf,"%d",*v);f_ptr=v;f_lo=lo;f_hi=hi;}
  box(bx,y,bw,h,0);
  if(focus_id==fid){char b[16];f_x=bx;f_y=y;f_w=bw;f_h=h;f_ptr=v;snprintf(b,16,"%s",fbuf);border_in(bx,y,bw,h,2,P.fg,1);if(f_fresh)fill_rect(bx+U(8),y+U(5),text_w(0,U(14),b),h-U(10),P.fg,.35);text_mid(0,U(14),bx+U(8),y,h,P.fg,1,b);
    if(((int)(NOW/500))%2)fill_rect(bx+U(8)+text_w(0,U(14),b)+1,y+U(6),1,h-U(12),P.fg,1);}
  else{double sz=U(13);while(sz>8&&text_w(0,sz,vt)>bw-U(12))sz--;text_mid(0,sz,bx+U(8),y,h,P.fg,1,vt);}
  return rv;
}
// Dropdown. The list itself is drawn last (select_popup) so it sits above everything.
static unsigned sel_id;static int sel_x,sel_y,sel_w,sel_n,sel_mask,sel_base,sel_py,sel_vis,sel_off,*sel_ptr;static const char*const*sel_names;
static void ui_select(const char*lab,int*v,const char*const*names,int n,int mask,int base){
  int y=cy,x=PX+PW-CWd(),w=CWd(),h=U(30);unsigned id=hid(lab,2);cy+=U(36);if(!vis(y,h))return;
  label(y,lab);int hot=uhov(x,y,w,h);box(x,y,w,h,0);(void)hot;
  const char*cur=names[iclamp(*v+base,0,n-1)];
  double sz=U(14);while(sz>8&&text_w(0,sz,cur)>w-U(30))sz--;
  text_mid(0,sz,x+U(8),y,h,P.fg,1,cur);text_right(0,U(10),x+w-U(8),y+U(19),P.fg,.7,"\xE2\x96\xBC");
  if(uhit(x,y,w,h)){sel_open=1;sel_id=id;sel_x=x;sel_y=y;sel_w=w;sel_n=n;sel_mask=mask;sel_base=base;sel_ptr=v;sel_names=names;
    sel_vis=imin(n,imax(3,(FH-U(8))/U(26)));sel_off=0;if(*v+base>=sel_vis)sel_off=imin(*v+base-sel_vis/2,n-sel_vis);if(sel_off<0)sel_off=0;   /* long lists scroll with the wheel */
    int ph=sel_vis*U(26);sel_py=(y+h+ph<=FH-U(4))?y+h:imax(U(4),y-ph);}
}
static void select_popup(void){
  if(!sel_open)return;int j,ih=U(26);
  if(wheel_acc!=0&&sel_n>sel_vis){sel_off=iclamp(sel_off-(int)(wheel_acc*3),0,sel_n-sel_vis);}
  fill_rect(sel_x,sel_py,sel_w,sel_vis*ih,P.bg,1);border_in(sel_x,sel_py,sel_w,sel_vis*ih,1,P.ln,1);
  for(j=0;j<sel_vis;j++){int i=sel_off+j,y=sel_py+j*ih,ok=(sel_mask>>i)&1,cur=(*sel_ptr+sel_base)==i,h=mx>=sel_x&&mx<sel_x+sel_w&&my>=y&&my<y+ih&&ok;
    if(h)fill_rect(sel_x+1,y,sel_w-2,ih,P.fg,1);else if(cur)fill_rect(sel_x+1,y,sel_w-2,ih,P.fg,.18);
    text_mid(0,U(14),sel_x+U(8),y,ih,h?P.bg:P.fg,ok?1:.35,sel_names[i]);}
}
static void select_click(void){ // called first thing each frame when a click happens while a dropdown is open
  if(!sel_open||!mclicked)return;int ih=U(26);
  if(clx>=sel_x&&clx<sel_x+sel_w&&cly>=sel_py&&cly<sel_py+sel_vis*ih){int i=(cly-sel_py)/ih+sel_off;if((sel_mask>>i)&1){*sel_ptr=i-sel_base;ui_changed=1;}}
  sel_open=0;mclicked=0;
}

// ---- color fields and the color picker (Solid Color theme) ----
static int pk_open=0;static u32*pk_t=NULL;static u32 pk_rgb=0,pk_orig=0;static double pk_h=0,pk_s=1,pk_v=1;static char pk_name[48];
static void hsv2rgb(double h,double sa,double v,int*r,int*g,int*b){
  h=fmod(h,360);if(h<0)h+=360;double c=v*sa,hp=h/60,x=c*(1-fabs(fmod(hp,2)-1)),m=v-c,R=0,G=0,B=0;
  if(hp<1){R=c;G=x;}else if(hp<2){R=x;G=c;}else if(hp<3){G=c;B=x;}else if(hp<4){G=x;B=c;}else if(hp<5){R=x;B=c;}else{R=c;B=x;}
  *r=(int)floor((R+m)*255+.5);*g=(int)floor((G+m)*255+.5);*b=(int)floor((B+m)*255+.5);}
static void rgb2hsv(int r,int g,int b,double*h,double*sa,double*v){
  double R=r/255.0,G=g/255.0,B=b/255.0,mx_=dmax(R,dmax(G,B)),mn=dmin(R,dmin(G,B)),d=mx_-mn;
  *v=mx_;*sa=mx_>0?d/mx_:0;
  if(d<=0){*h=0;return;}
  if(mx_==R)*h=60*fmod((G-B)/d,6);else if(mx_==G)*h=60*((B-R)/d+2);else *h=60*((R-G)/d+4);
  if(*h<0)*h+=360;}
// One row: a swatch (click it for the picker when pick is set) and a hex box (type a code like #3d9bff).
static void ui_color(const char*lab,u32*c,const char*pname,int pick){
  int y=cy,w=CWd(),x=PX+PW-w,h=U(30),sw=U(34),hx=x+sw+U(6),hw=w-sw-U(6);unsigned id=hid(lab,4);cy+=U(36);if(!vis(y,h))return;
  label(y,lab);
  fill_rect(x,y,sw,h,*c,1);border_in(x,y,sw,h,1,P.fg,.8);
  if(pick&&uhit(x,y,sw,h)){focus_commit();pk_open=1;pk_t=c;pk_orig=*c;pk_rgb=0;snprintf(pk_name,sizeof pk_name,"%s",pname);}
  if(uhit(hx,y,hw,h)&&hx_id!=id){focus_commit();hx_id=id;hx_ptr=c;snprintf(hx_buf,sizeof hx_buf,"%06x",(unsigned)(*c&0xFFFFFF));hx_fresh=1;}
  box(hx,y,hw,h,0);char b[16];
  if(hx_id==id){hx_x=hx;hx_y=y;hx_w=hw;hx_h=h;snprintf(b,16,"#%s",hx_buf);border_in(hx,y,hw,h,2,P.fg,1);
    if(hx_fresh)fill_rect(hx+U(8),y+U(5),text_w(0,U(14),b),h-U(10),P.fg,.35);text_mid(0,U(14),hx+U(8),y,h,P.fg,1,b);
    if(((int)(NOW/500))%2)fill_rect(hx+U(8)+text_w(0,U(14),b)+1,y+U(6),1,h-U(12),P.fg,1);}
  else{snprintf(b,16,"#%06x",(unsigned)(*c&0xFFFFFF));text_mid(0,U(14),hx+U(8),y,h,P.fg,1,b);}
}

// ---- dialogs: a frame with a scrolling middle, a fixed top and a fixed bottom ----
typedef struct{int scroll,ch;}Scr;
static Scr scr_main,scr_freq,scr_about,scr_adv,scr_pk,scr_parts,scr_flt;
static int DX,DY,DWW,DHH,DPAD;
static void dlg_begin(Scr*s,int dw,int top,int bot,double now){
  DPAD=U(14);int maxh=FH-2*U(16),want=DPAD*2+top+s->ch+bot,dh=s->ch?imin(want,maxh):maxh;
  DWW=imin(dw,FW-2*U(16));DX=(FW-DWW)/2;DY=(FH-dh)/2;DHH=dh;
  fill_rect(0,0,FW,FH,P.wild?HEX(0x2a0040):P.swirl?HEX(0x1c0630):P.light?HEX(0xf2f2f2):HEX(0),.85);
  if(P.swirl){fill_rect(DX,DY,DWW,dh,P.bg,1);swirl_fill(3,DX,DY,DWW,dh,6,tm(now)+2000,.2,50);swirl_ribbon(DX,DY,DWW,dh,0,4,22,tm(now),2,.95);}
  else if(P.wild){int y;for(y=0;y<dh;y++){double k=(double)y/dh;fill_rect(DX,DY+y,DWW,1,RGB((int)(26*(1-k)),(int)(k*26),(int)(51*(1-k)+51*k)),1);}
    border_in(DX,DY,DWW,dh,3,hsl(tm(now)/10,1,.6),1);glow_rect(DX,DY,DWW,dh,18,0,HEX(0xd500f9),.6);}
  else{fill_rect(DX,DY,DWW,dh,P.bg,1);border_in(DX,DY,DWW,dh,1,P.fg,1);}
  PX=DX+DPAD;PW=DWW-2*DPAD;VX0=DX;VX1=DX+DWW;PVY=DY+DPAD+top;PVH=dh-2*DPAD-top-bot;cy=PVY-s->scroll;
  clip(DX+1,PVY,DX+DWW-1,PVY+PVH);
}
static void dlg_end(Scr*s){
  s->ch=cy-(PVY-s->scroll);unclip();int maxs=imax(0,s->ch-PVH);
  if(wheel_acc!=0&&mx>=VX0&&mx<VX1&&my>=DY&&my<DY+DHH&&!sel_open)s->scroll-=(int)(wheel_acc*U(40));
  s->scroll=iclamp(s->scroll,0,maxs);
  if(maxs>0){int th=imax(U(20),PVH*PVH/s->ch),ty=PVY+(PVH-th)*s->scroll/maxs;fill_rect(DX+DWW-U(5),ty,U(3),th,P.fg,.4);}
  PVY=0;PVH=FH; // the fixed footer buttons that follow are outside the scrolling view, so let them take clicks anywhere on screen
}

// ---- state edited by the preferences screen but applied only on "Apply and restart" ----
static int pW=10,pH=20,pSlots=1,pClear=1,pWeights[12],pTw[7];
static void pend_load(void){pW=W;pH=H;pSlots=slots;pClear=clearOn;memcpy(pWeights,weights,sizeof weights);memcpy(pTw,tw,sizeof tw);}
static void apply_pending(void){
  endRun();int i,any=0;
  W=iclamp(pW,4,500);H=iclamp(pH,4,500);slots=iclamp(pSlots,1,10);clearOn=pClear;memcpy(weights,pWeights,sizeof weights);memcpy(tw,pTw,sizeof tw);
  for(i=1;i<=11;i++)if(weights[i]>0)any=1;if(!any){weights[4]=1;}
  any=0;for(i=0;i<7;i++)if(tw[i]>0)any=1;if(!any)for(i=0;i<7;i++)tw[i]=1;
  pend_load();reset_game();ui_changed=1;
}
static int freq_open=0,about_open=0,bind_listen=-1;
static int partsp_open=0,fltp_open=0;   // the "Theme each part" and "Filters" popups (from the Theming tab)
static int advp_open=0,hist_open[MAXHIST],clear_arm=0;
static char hmsg[800]="";
// ---- effect preview: plays a sample combo and perfect clear (with sound) over the live board ----
static int pv_on=0,pv_stage=0,pv_combo=0;static double pv_t0=0;
static void pv_start(void){pv_combo=combo;fxReset();pv_on=1;pv_stage=0;pv_t0=NOW;clearHeld();}
static void pv_stop(void){if(!pv_on)return;fxReset();combo=pv_combo;pv_on=0;}
static void pv_tick(void){
  double e=NOW-pv_t0;int rows[3]={H-1,H-2,H-3};
  if(pv_stage==0&&e>=250){combo=3;fxClear(rows,2);fx_sound(EV_COMBO,sndCombo,3);pv_stage=1;}
  if(pv_stage==1&&e>=1250){combo=7;fxClear(rows,3);fx_sound(EV_COMBO,sndCombo,7);pv_stage=2;}
  if(pv_stage==2&&e>=2600){fx_break();combo=0;pv_stage=3;}
  if(pv_stage==3&&e>=3100){pfStart();fx_sound(EV_PERFECT,sndPf,1);pv_stage=4;}
  if(pv_stage==4&&e>=3100+PFD[pcLevel]+400)pv_stop();
}

static void fall_typed(double rate){fallV=100*log(rate/0.2)/log(100);if(pieces>0&&!over)runDirty=1;}
// Shared choice lists: the seven font families, and the places a timer, clock or track display can go (the last is the side panel).
static const char*FONT_NAMES[7]={"DejaVu Sans (default)","Liberation Sans (Arial-like)","Carlito (Calibri-like)","Liberation Serif (Times-like)","Liberation Mono (monospace)","Atkinson Hyperlegible","OpenDyslexic"};
static const char*PLACE_NAMES[7]={"Top left","Top center","Top right","Bottom left","Bottom center","Bottom right","Side panel"};
static const char*SETNAME[11]={"1 block","2 blocks","3 blocks","4 blocks","5 blocks","6 blocks","7 blocks","8 blocks","9 blocks","10 blocks","Letters A\xE2\x80\x93Z"};
static const char*WINN[6]={"Choose\xE2\x80\xA6","800 \xC3\x97 600","1024 \xC3\x97 768","1280 \xC3\x97 720","1600 \xC3\x97 900","1920 \xC3\x97 1080"};
static const int WINW[6]={0,800,1024,1280,1600,1920},WINH[6]={0,600,768,720,900,1080};
static const char*RESN[7]={"Native","1440p","1080p","900p","720p","540p","360p"};
static const int RES_H[7]={0,1440,1080,900,720,540,360};
static const char*FPSN[10]={"Match display (vsync)","30","60","75","90","120","144","165","240","Unlimited"};
static const int FPS_V[10]={0,30,60,75,90,120,144,165,240,-1};
static const char*ADVN[6]={"Match","Off","A bit","Moderate","High","Intense"};
static const char*ADVROW[4]={"Combo visuals","Perfect clear visuals","Combo sound","Perfect clear sound"};

static void settings_panel(double now){
  char b[200];int i;
  dlg_begin(&scr_main,U(350*MWF[mwSel]),U(112),U(66),now);
  // ---- Game ----
  if(ui_head("Game",&sec_open[0],now)){
    ui_num("Width",&pW,4,500);ui_num("Height",&pH,4,500);
    if(ui_button("Reset to 10 \xC3\x97 20")){pW=10;pH=20;focus_commit();ui_changed=1;}
    ui_num("Hold slots",&pSlots,1,10);
    ui_fall();
    {int sdv=sdRate;ui_num("Soft drop (rows/s)",&sdRate,1,500);(void)sdv;}
    if(ui_check("Level system",&lvlSys)){if(pieces>0&&!over)runDirty=1;level=curLevel();}
    ui_keys("With the level system on, the fall rate starts at your setting and speeds up every 100 cleared cells (10 lines on a 10-wide board), following the standard modern Tetris curve.");
    ui_num("Next pieces shown",&NQ,0,10);
    ui_keys("How many upcoming pieces you can see in the side panel (0 to 10). This changes right away. A short window may only have room for fewer.");
    ui_check("Line clearing",&pClear);
    if(!pClear)ui_warn("Line clearing is off: completed lines stay on the board, so 8tris becomes pure stacking. Scoring is disabled in this mode and runs are not saved to history.");
    ui_keys("Width, height, hold slots, piece sets and line clearing apply when you press Apply and restart.");
  }
  // ---- Pieces ----
  if(ui_head("Pieces",&sec_open[1],now)){
    ui_keys("Tick the piece sets that can appear. Ticked sets are equally likely unless you set frequencies.");
    int colw=PW/3;
    for(i=0;i<11;i++){int on=pWeights[i+1]>0,x=PX+(i%3)*colw,y=cy+(i/3)*U(28);if(check_at(x,y,colw,SETNAME[i],&on))pWeights[i+1]=on?(pWeights[i+1]>0?pWeights[i+1]:1):0;}
    cy+=4*U(28)+U(4);
    if(ui_button("Set frequencies\xE2\x80\xA6"))freq_open=1;
  }
  // ---- Theming ----
  if(ui_head("Theming",&sec_open[2],now)){
    ui_warn("No guarantee is made that all themes will be safe to use for those who are epileptic.");
    ui_check("Enable theming",&themeOn);
    if(!themeOn)ui_keys("Off by default: you get the standard 8tris look. Turn it on to choose a theme for each part of the game separately.");
    else{
      ui_sub("Presets (set every part at once)");
      {const char*nm[N_THEMES];int k;for(k=0;k<N_THEMES;k++)nm[k]=THEME_NAME[THEME_ORDER[k]];
       k=ui_buttons(N_THEMES,nm);if(k>=0){int t=THEME_ORDER[k],c;for(c=0;c<N_COMPS;c++)if((AVAIL[c]>>t)&1)TC[c]=t;ui_changed=1;}}
      ui_keys("Or give each part its own theme: the page, side panel, menu, board frame, board, grid, pieces, landing outline, effect colors and sound voice.");
      if(ui_button("Theme each part\xE2\x80\xA6"))partsp_open=1;
      if(TC[C_PIECES]==TH_SOLID||TC[C_GHOST]==TH_SOLID){
        ui_sub("Solid Color");
        if(!solidSep){ui_color("Color",&solidP,"Pieces and landing outline",1);
          ui_keys("Used for Pieces and Landing outline wherever they are set to Solid Color. Type a hex code like #3d9bff, or click the swatch for a full color picker. Advanced effect options lets each have its own color.");}
        else{ui_keys("Pieces and Landing outline use separate colors, set in Advanced effect options below.");
          if(ui_button("Advanced effect options\xE2\x80\xA6"))advp_open=1;}
      }
    }
    ui_sub("Filters");
    ui_keys(flt_active()?"Filters are on. They change the whole game picture on top of whichever themes are chosen.":"Brightness, contrast, saturation, hue, color split, tint, opacity and invert, over any theme. All off right now.");
    if(ui_button("Filters\xE2\x80\xA6"))fltp_open=1;
    {
     
     static const char*MN[4]={"00","15","30","45"};int i;
     ui_sub("Timer and clock");
     for(i=0;i<2;i++){
       if(!ui_group(i?"Real-time clock":"Game timer",&CLK[i].on,i?G_CLOCK:G_TIMER))continue;
       if(i==1){
         ui_num("UTC offset, hours",&clkTzH,-12,14);
         static int mi=0,miv=0;if(mi!=miv)clkTzM=iclamp(mi,0,3)*15;mi=miv=clkTzM/15;ui_select("UTC offset, minutes",&mi,MN,4,15,0);   /* static: the dropdown writes here after this function returns */
         ui_check("Daylight saving",&clkDst);ui_check("24-hour clock",&clk24);ui_check("Show seconds",&clkSecs);
         ui_keys("Type your own offset from UTC, for example -5 for New York in winter, or 5 and 30 for India. Daylight saving adds one hour. 8tris never looks up your timezone or location: it only reads the device's clock and adds the offset you enter.");
       }
       ui_select("Position",&CLK[i].pos,PLACE_NAMES,7,127,0);
       snprintf(b,sizeof b,"%d pt",CLK[i].sz);ui_slider("Size",&CLK[i].sz,12,120,b);
       snprintf(b,sizeof b,"%d%%",CLK[i].op);ui_slider("Opacity",&CLK[i].op,10,100,b);
       ui_select("Font",&CLK[i].fam,FONT_NAMES,7,127,0);
       ui_check("Bold",&CLK[i].bold);
       if(i==0){ui_check("Show milliseconds",&timerMs);
         ui_keys("Counts play time only: it stops while paused or in this menu, and starts over with each game. In the side panel the size shrinks to fit.");}
       ui_group_end();
     }
     if(CLK[0].on&&CLK[1].on&&ui_group("Stacking",NULL,G_STACK)){
       static const char*SV[2]={"Side by side","One above the other"},*SF[2]={"Game timer first","Clock first"};
       ui_select("Stack direction",&stackV,SV,2,3,0);ui_select("Stack order",&stackFirst,SF,2,3,0);
       ui_keys("These apply when the timer and the clock are set to the same position. The side panel always stacks one above the other.");
       ui_group_end();
     }}
    ui_sub("Effects");
    ui_select("Effects",&fxMaster,LVN,5,31,0);
    {char d[400];int lv=capLv(fxMaster);snprintf(d,sizeof d,"%s%s",lv?"Adds ":"",EFF_ADD[lv]);if(lv)strncat(d," Each level keeps everything from the levels below it.",sizeof d-strlen(d)-1);ui_keys(d);}
    if(fxMaster>=3||ADV[0]>=3||ADV[1]>=3||ADV[2]>=3||ADV[3]>=3)ui_warn("High and Intense add bright flashes, screen shake and loud sounds. No guarantee is made that they are safe for people who are sensitive to flashing light. Try Preview first, and turn this down if anything is uncomfortable.");
    ui_check("Reduce motion",&calm);
    if(calm)ui_keys("Reduce motion is on, so effects stop at Moderate, with softer flashes, no screen shake and no animated backgrounds.");
    if(ui_button("Preview")){focus_commit();resolve_fx();pv_start();}
    ui_keys("Preview hides this panel and plays a sample combo and perfect clear, with sound. Press any key or click to stop it.");
    if(ui_button("Advanced effect options\xE2\x80\xA6"))advp_open=1;
  }
  // ---- Sound ----
  if(ui_head("Sound",&sec_open[3],now)){
    static int pv=-1,pvl=-1;   // static: a typed number is written into it between frames
    {int cur=(int)floor(snd_vol*100+.5);if(pvl>=0&&pv!=pvl){snd_vol=pv/100.0;cur=pv;}pv=pvl=cur;}
    snprintf(b,sizeof b,pv?"%d%%":"Off",pv);
    if(ui_slider("Sound effects",&pv,0,100,b))snd_vol=pv/100.0;
    ui_keys("The sound voice and which sounds play with effects are set in Theming.");
    ui_check("Combo song",&comboSong);
    ui_keys("Replaces the combo sounds with a song that builds for as long as your combo chain lasts. Keep the chain going and see where it goes.");
    {static const char*MM[4]={"Off","Repeat one track","Play all in order","Play all shuffled"};static int was=0;
     if(!was){music_scan();}was=1;
     if(ui_group("Music",NULL,G_MUSIC)){
       ui_select("Music",&musMode,MM,4,15,0);
       if(musMode){
         if(nfold>0){static const char*fn[MUS_MAXFILES+1];static int fsel=0,fselv=0;int k;fn[0]="All folders";for(k=0;k<nfold;k++)fn[k+1]=mfold[k];
           if(fsel!=fselv){if(fsel<=0)musFold[0]=0;else if(fsel<=nfold)snprintf(musFold,sizeof musFold,"%s",mfold[fsel-1]);}
           fsel=0;for(k=0;k<nfold;k++)if(!strcmp(mfold[k],musFold))fsel=k+1;fselv=fsel;ui_select("Folder",&fsel,fn,nfold+1,-1,0);}   /* static: the dropdown writes here after this function returns */
         if(musMode==1&&nmfiles){static const char*nm[MUS_MAXFILES];static int ix[MUS_MAXFILES];int k,m=0;for(k=0;k<nmfiles;k++)if(music_in_scope(k)){nm[m]=mfiles[k];ix[m++]=k;}
           static int tsel=0,tselv=0,tm=0;if(tsel!=tselv&&tsel>=0&&tsel<tm){musSel=ix[tsel];snprintf(musFile,sizeof musFile,"%s",mfiles[musSel]);}
           tsel=0;for(k=0;k<m;k++)if(ix[k]==musSel)tsel=k;tselv=tsel;tm=m;if(m)ui_select("Track",&tsel,nm,m,-1,0);}   /* static: the dropdown writes here after this function returns */
         snprintf(b,sizeof b,musVol?"%d%%":"Off",musVol);ui_slider("Volume",&musVol,0,100,b);
         if(mus_status[0])ui_keys(mus_status);
       }
       snprintf(b,sizeof b,"Music folder: %s",music_dir());ui_keys(b);
       ui_keys("Put .mp3, .ogg, .wav or .flac files in that folder, then press Rescan. Files are only read on this computer, never copied or sent anywhere. Any length works, even audiobooks many hours long: tracks play straight from the file.");
       if(ui_button("Rescan folder")){music_scan();ui_changed=1;}
       ui_group_end();
     }
     if(musMode&&ui_group("Pause button",&npPause,G_PAUSE)){
       static const char*PB[7]={"With the track name","Top left","Top center","Top right","Bottom left","Bottom center","Bottom right"};
       ui_select("Position",&npPausePos,PB,7,127,0);
       ui_keys("Click it during a game to stop and start your music. It takes the size and opacity of the track name.");
       ui_group_end();}
     if(musMode&&ui_group("Show track on screen",&NP.on,G_TRACK)){
       ui_select("Position",&NP.pos,PLACE_NAMES,6,63,0);
       snprintf(b,sizeof b,"%d pt",NP.sz);ui_slider("Size",&NP.sz,10,48,b);
       snprintf(b,sizeof b,"%d%%",NP.op);ui_slider("Opacity",&NP.op,10,100,b);
       ui_select("Font",&NP.fam,FONT_NAMES,7,127,0);ui_check("Bold",&NP.bold);
       ui_check("Artist and album",&npArtist);ui_check("Album art",&npArt);
       if(npArt){snprintf(b,sizeof b,"%d px",npArtSz);ui_slider("Album art size",&npArtSz,32,256,b);}
       ui_keys("The title, artist, album and cover picture are read from the music file's own tags. If a file has no title tag, its file name is shown.");
       ui_group_end();
     }}
  }
  // ---- Display ----
  if(ui_head("Display",&sec_open[4],now)){
    {static int ws=0;if(ws>0&&ws<6){plat_window_size(WINW[ws],WINH[ws]);}ws=0;ui_select("Window size",&ws,WINN,6,63,0);}   /* static: the dropdown writes here after this function returns */
    {int fs=fullscreen;if(ui_check("Full screen",&fs)){fullscreen=fs;plat_fullscreen(fs);}}
    ui_select("Render resolution",&resSet,RESN,7,127,0);
    ui_keys("The picture is drawn at this height and scaled up to fit the window. Lower is faster, which helps on slower computers, especially with the Swirl theme. Native draws at the window's full size.");
    ui_select("Refresh rate",&fpsSet,FPSN,10,1023,0);
    ui_keys("How often the picture is redrawn. Match display (vsync) is the smoothest and easiest on power. A fixed rate caps it; Unlimited draws as fast as it can and may tear.");
    ui_check("Show frame rate",&showFps);
    {static const char*BP[7]={"Top left","Top center","Top right","Bottom left","Bottom center","Bottom right","Side panel (default)"};ui_select("Preferences button",&prefBtn,BP,7,127,0);}
    ui_keys("Where the Preferences button sits. Esc always opens preferences too, wherever the button is.");
    if(ui_group("Board position",NULL,G_BOARDPOS)){
      snprintf(b,sizeof b,boardPX==0?"Left":boardPX==100?"Right":boardPX==50?"Center":"%d%%",boardPX);ui_slider("Left to right",&boardPX,0,100,b);
      snprintf(b,sizeof b,boardPY==0?"Top":boardPY==100?"Bottom":boardPY==50?"Center":"%d%%",boardPY);ui_slider("Top to bottom",&boardPY,0,100,b);
      if(boardPX!=50||boardPY!=50){if(ui_button("Center the board")){boardPX=50;boardPY=50;ui_changed=1;}}
      ui_group_end();
    }
  }
  // ---- Appearance ----
  if(ui_head("Appearance",&sec_open[7],now)){
    ui_keys("These change how the preferences menu looks and reads. Apart from brightness, they never change the board, the pieces or the text shown during play.");
    {char t[16];snprintf(t,sizeof t,"%d%%",gBright);ui_slider("Screen brightness",&gBright,25,150,t);}
    ui_keys("Brightness applies to everything on screen: the board, the side panel, the preferences menu and every overlay. 100% leaves the picture untouched.");
    if(ui_group("Menu text",NULL,G_MTEXT)){
      ui_select("Font",&uiFam,FONT_NAMES,7,127,0);
      ui_keys("Atkinson Hyperlegible is designed for low vision, and OpenDyslexic for readers with dyslexia.");
      ui_check("Bold",&uiBold);ui_check("Italic",&uiItal);
      ui_num("Text size (pt)",&uiPt,8,28);
      {static const char*SL[2]={"Smaller","Larger"};int k=ui_buttons(2,SL);if(k>=0){focus_commit();uiPt=iclamp(uiPt+(k?1:-1),8,28);ui_changed=1;}}
      ui_group_end();
    }
    if(ui_group("Menu layout",NULL,G_MLAYOUT)){
      static const char*MWN[3]={"Normal","Wide","Extra wide"},*LSN[3]={"Normal","Roomy","Very roomy"};
      ui_select("Menu width",&mwSel,MWN,3,7,0);ui_select("Line spacing",&lsSel,LSN,3,7,0);
      ui_check("High contrast menu",&hiCon);
      ui_keys("High contrast draws the menu in pure black and white with stronger lines and brighter small text.");
      ui_group_end();
    }
    ui_sub("Playing");
    {char t[16];snprintf(t,sizeof t,"%d%%",ghostStr);ui_slider("Landing outline",&ghostStr,20,100,t);}
    ui_check("Reduce motion",&calm);
    ui_keys("A stronger landing outline is easier to see on any board. Reduce motion stops animated backgrounds and screen shake, and caps effects at Moderate.");
    ui_sub("Preview");
    {int h=para(0,U(14),0,0,PW-U(16),LH,P.fg,1,"The quick brown fox jumps over the lazy dog. 0123456789",0)+U(12),y=cy+U(4);
     if(vis(y,h)){border_in(PX,y,PW,h,1,P.fg,1);para(0,U(14),PX+U(8),y+U(6),PW-U(16),LH,P.fg,1,"The quick brown fox jumps over the lazy dog. 0123456789",1);}cy+=h+U(12);}
    if(ui_button("Reset appearance to defaults")){uiFam=0;uiBold=0;uiItal=0;uiPt=14;hiCon=0;mwSel=0;lsSel=0;ghostStr=50;gBright=100;focus_commit();ui_changed=1;}
  }
  // ---- Controls ----
  if(ui_head("Controls",&sec_open[5],now)){
    int a,s;
    for(a=0;a<N_ACTS;a++){int y=cy;cy+=U(34);if(!vis(y,U(30)))continue;label(y,ACT_NAME[a]);
      int bw=(CWd()-U(6))/2;
      for(s=0;s<2;s++){int x=PX+PW-CWd()+s*(bw+U(6)),listening=bind_listen==a*2+s;
        const char*t=listening?"press key\xE2\x80\xA6":key_label(binds[a][s]);int hot=uhov(x,y,bw,U(30));
        box(x,y,bw,U(30),hot&&!listening);if(listening)border_in(x,y,bw,U(30),2,P.fg,1);
        double sz=U(12);while(sz>8&&text_w(0,sz,t)>bw-U(8))sz--;
        text_center(0,sz,x+bw/2.0,y+U(19),(hot&&!listening)?P.bg:P.fg,1,t);
        if(uhit(x,y,bw,U(30))){bind_listen=a*2+s;focus_commit();}}}
    ui_keys("Click a key, then press the new one. Backspace clears it, Esc cancels. Esc always opens preferences. F11 toggles full screen.");
    if(ui_button("Reset key bindings")){binds_default();ui_changed=1;bind_listen=-1;}
  }
  // ---- History ----
  if(ui_head("History",&sec_open[6],now)){
    ui_keys("Finished runs are saved on this computer with every setting used, including graphics, sound and controls.");
    ui_num("Date offset, hours",&clkTzH,-12,14);
    {static const char*MN2[4]={"00","15","30","45"};static int mi2=0,mi2v=0;if(mi2!=mi2v)clkTzM=iclamp(mi2,0,3)*15;mi2=mi2v=clkTzM/15;ui_select("Date offset, minutes",&mi2,MN2,4,15,0);}
    ui_keys("Dates are shown in UTC plus this offset (the same one the real-time clock uses, with its daylight saving switch). 8tris never reads your device's timezone.");
    int shown=imin(nhist,25);static char L[60][400];
    for(i=0;i<shown;i++){
      char d[32],tl[16],hb[120];fmt_date(hist[i].t,d,32);fmt_time(hist[i].time,tl,16);snprintf(hb,sizeof hb,"%s  \xC2\xB7  %s",d,tl);
      int open=hist_open[i],y=cy,inner=PW-U(16),h=U(28);
      int n=entry_lines(&hist[i],L,60),j,extra=0;
      if(open)for(j=1;j<n;j++)extra+=para(0,U(11),0,0,inner,LH,P.fg,.85,L[j],0);
      h+=open?extra+U(6):0;
      if(vis(y,h)){
        border_in(PX,y,PW,h,1,P.sep,1);
        text_mid(0,U(13),PX+U(8),y,U(28),P.fg,1,hb);snprintf(hb,sizeof hb,"Score %d",hist[i].score);text_right(0,U(13),PX+PW-U(8),y+U(19),P.fg,1,hb);
        if(open){int yy=y+U(30);for(j=1;j<n;j++)yy+=para(0,U(11),PX+U(8),yy,inner,LH,P.fg,.85,L[j],1);}
        if(uhit(PX,y,PW,U(28)))hist_open[i]=!hist_open[i];
      }
      cy+=h+U(6);
    }
    if(!nhist)ui_keys("No finished runs yet.");
    if(nhist>25){snprintf(b,sizeof b,"Showing the latest 25 of %d. The export includes all of them.",nhist);ui_keys(b);}
    cy+=U(4);
    {const char*HB[2]={"Export .txt",clear_arm?"Click again to clear":"Clear history"};int k=ui_buttons(2,HB);
     if(k==0){if(!nhist)snprintf(hmsg,sizeof hmsg,"Nothing to export yet.");else{const char*pth=export_history();if(pth)snprintf(hmsg,sizeof hmsg,"Saved to %s",pth);else snprintf(hmsg,sizeof hmsg,"Could not write the file in your home folder.");}}
     if(k==1){if(!clear_arm)clear_arm=1;else{nhist=0;hist_save();clear_arm=0;snprintf(hmsg,sizeof hmsg,"History cleared.");}}}
    if(hmsg[0])ui_keys(hmsg);
  }
  dlg_end(&scr_main);
  if(want_top){scr_main.scroll=0;want_top=0;}
  // fixed top: run statistics
  {int ty=DY+DPAD;snprintf(b,sizeof b,"Score %d",(int)floor(score));text_mid(0,U(14),DX+DPAD,ty,U(30),P.fg,1,b);
   snprintf(b,sizeof b,"Lines %d",lines);text_mid(0,U(14),DX+DPAD+PW/2.0-text_w(0,U(14),b)/2,ty,U(30),P.fg,1,b);
   snprintf(b,sizeof b,"Level %d",level);text_mid(0,U(14),DX+DWW-DPAD-text_w(0,U(14),b),ty,U(30),P.fg,1,b);fill_rect(DX+DPAD,ty+U(34),PW,1,P.sep,1);
   // section tabs: jump straight to a section (the sections work like an accordion)
   {static const char*tn[8]={"Game","Pieces","Theming","Sound","Display","Controls","History","Appearance"};static const int ord[8]={0,1,2,3,4,7,5,6};
    int kk,cw=PW/4,ch=U(30),cyy=ty+U(40);double csz=U(14);for(kk=0;kk<8;kk++)while(csz>8&&text_w(0,csz,tn[kk])>cw-U(6))csz-=.5;
    for(kk=0;kk<8;kk++){int k=ord[kk],x=DX+DPAD+(kk%4)*cw,yy=cyy+(kk/4)*(ch+U(4)),on=sec_open[k],hot=mx>=x&&mx<x+cw&&my>=yy&&my<yy+ch&&!sel_open;
      if(on||hot)fill_rect(x+1,yy,cw-2,ch,P.fg,on?1:.15);
      text_center(0,csz,x+cw/2.0,yy+ch/2.0+csz*.36,on?P.bg:P.fg,1,tn[k]);
      if(mclicked&&clx>=x&&clx<x+cw&&cly>=yy&&cly<yy+ch){int j;for(j=0;j<8;j++)sec_open[j]=0;sec_open[k]=1;scr_main.scroll=0;focus_commit();}}
    fill_rect(DX+DPAD,cyy+2*ch+U(7),PW,1,P.sep,1);}}
  // ---- fixed bottom: Apply / Resume / info, credit ----
  {int by=DY+DHH-DPAD-U(66)+U(8);fill_rect(DX+DPAD,by-U(8),PW,1,P.sep,1);
   int fx=DX+DPAD;
   if(foot_btn(&fx,by,"Apply and restart")){focus_commit();apply_pending();menu=0;clearHeld();}
   if(foot_btn(&fx,by,"Resume")){focus_commit();menu=0;clearHeld();}
   {int x=DX+DWW-DPAD-U(22),y=by+U(4),hot=mx>=x&&mx<x+U(22)&&my>=y&&my<y+U(22);
    fill_ring(x+U(11),y+U(11),U(10.5),1.5,hot?P.fg:P.light?HEX(0x555555):HEX(0x777777),1);text_center(0,U(13),x+U(11),y+U(16),hot?P.fg:P.light?HEX(0x444444):HEX(0xaaaaaa),1,"i");
    if(mclicked&&clx>=x&&clx<x+U(22)&&cly>=y&&cly<y+U(22))about_open=1;}
   text_right(0,U(11),DX+DWW-DPAD,by+U(30)+U(20),P.fg,.4,"Created by Claude");}
}

static void freq_popup(double now){
  int i;char b[40];
  dlg_begin(&scr_freq,U(340*MWF[mwSel]),0,U(46),now);
  if(ui_head("Piece set frequency",&(int){1},now)){}
  ui_keys("Relative chance of each set appearing. 0 turns a set off. Applies when you press Apply and restart.");
  for(i=0;i<11;i++){snprintf(b,sizeof b,"%d",pWeights[i+1]);ui_slider(SETNAME[i],&pWeights[i+1],0,10,b);}
  {int one=1;ui_head("Standard pieces (4 blocks)",&one,now);}
  ui_keys("Relative chance of each piece inside the 4-block set. For example, lots of T, a few I and 0 for the rest. Only affects the 4-block set.");
  for(i=0;i<7;i++){char nm[8];snprintf(nm,sizeof nm,"%c",TN[i]);snprintf(b,sizeof b,"%d",pTw[i]);ui_slider(nm,&pTw[i],0,10,b);}
  dlg_end(&scr_freq);
  {int by=DY+DHH-DPAD-U(30);fill_rect(DX+DPAD,by-U(8),PW,1,P.sep,1);
   int fx=DX+DPAD;if(foot_btn(&fx,by,"Done"))freq_open=0;
   if(foot_btn(&fx,by,"Reset all to equal")){for(i=1;i<=11;i++)if(pWeights[i]>0)pWeights[i]=1;for(i=0;i<7;i++)pTw[i]=1;ui_changed=1;}}
}

// ---- "Theme each part" popup: one dropdown per part of the game (the presets on the Theming tab set them all at once) ----
static void parts_popup(double now){
  dlg_begin(&scr_parts,U(340*MWF[mwSel]),0,U(46),now);
  {int one=1;ui_head("Theme each part",&one,now);}
  ui_keys("Each part of the game can use any theme that has a version for it. Changes show right away.");
      {int c;static const char*nm[N_COMPS][N_THEMES];static int pos[N_COMPS],prev[N_COMPS];   // pos: the theme's place in the list (a menu choice writes here)
       for(c=0;c<N_COMPS;c++){int k,mask=0;
         if(pos[c]!=prev[c])TC[c]=THEME_ORDER[pos[c]];                        // a choice was made last frame
         for(k=0;k<N_THEMES;k++){int t=THEME_ORDER[k];nm[c][k]=theme_label(c,t);if((AVAIL[c]>>t)&1)mask|=1<<k;if(t==TC[c])pos[c]=k;}
         prev[c]=pos[c];ui_select(COMP_NAME[c],&pos[c],nm[c],N_THEMES,mask,0);}}
  dlg_end(&scr_parts);
  {int by=DY+DHH-DPAD-U(30);fill_rect(DX+DPAD,by-U(8),PW,1,P.sep,1);int fx=DX+DPAD;if(foot_btn(&fx,by,"Done"))partsp_open=0;}
}
// ---- "Filters" popup ----
static void flt_popup(double now){
  char b[64];
  dlg_begin(&scr_flt,U(340*MWF[mwSel]),0,U(46),now);
  {int one=1;ui_head("Filters",&one,now);}
    ui_keys("Filters change the whole game picture on top of whichever themes are chosen. The menu and the timer and clock are not filtered.");
    snprintf(b,sizeof b,"%+d",fltBri);ui_slider("Brightness",&fltBri,-50,50,b);
    snprintf(b,sizeof b,"%d%%",fltCon);ui_slider("Contrast",&fltCon,50,150,b);
    snprintf(b,sizeof b,"%d%%",fltSat);ui_slider("Saturation",&fltSat,0,200,b);
    snprintf(b,sizeof b,"%d deg",fltHue);ui_slider("Hue shift",&fltHue,0,359,b);
    snprintf(b,sizeof b,"%d px",fltSplit);ui_slider("Color split",&fltSplit,0,12,b);
    snprintf(b,sizeof b,"%d%%",fltTint);ui_slider("Tint strength",&fltTint,0,100,b);
    if(fltTint)ui_color("Tint color",&fltTintC,"Tint color",1);
    snprintf(b,sizeof b,"%d%%",fltOp);ui_slider("Picture opacity",&fltOp,10,100,b);
    ui_check("Invert colors",&fltInv);
    if(ui_button("Reset filters")){fltBri=0;fltCon=100;fltSat=100;fltHue=0;fltSplit=0;fltTint=0;fltOp=100;fltInv=0;fltTintC=0xFFFF9A3Cu;ui_changed=1;}
  dlg_end(&scr_flt);
  {int by=DY+DHH-DPAD-U(30);fill_rect(DX+DPAD,by-U(8),PW,1,P.sep,1);int fx=DX+DPAD;if(foot_btn(&fx,by,"Done"))fltp_open=0;}
}
static void adv_popup(double now){
  int i;
  dlg_begin(&scr_adv,U(340*MWF[mwSel]),0,U(46),now);
  {int one=1;ui_head("Advanced effect options",&one,now);}
  ui_keys("Each row follows the master Effects level unless you pick a level here. Sound follows its own visuals by default, so you can have a loud combo with quiet visuals, or the other way round.");
  for(i=0;i<4;i++)ui_select(ADVROW[i],&ADV[i],ADVN,6,63,1);
  if(calm)ui_keys("Reduce motion is on, so every level here stops at Moderate.");
  ui_sub("Solid Color theme");
  {int sp=solidSep;if(ui_check("Different colors",&sp)){if(sp&&!solidSep)solidG=solidP;solidSep=sp;}}
  if(solidSep){
    ui_keys("Pieces and the landing outline each get their own color. Click a swatch for the picker, or type a hex code.");
    ui_color("Pieces color",&solidP,"Pieces",1);ui_color("Landing outline color",&solidG,"Landing outline",1);
  }else ui_keys("Pieces and the landing outline share one color, set under Theming. Turn this on to give each its own. These colors are used wherever Pieces or Landing outline is set to Solid Color.");
  dlg_end(&scr_adv);
  {int by=DY+DHH-DPAD-U(30);fill_rect(DX+DPAD,by-U(8),PW,1,P.sep,1);int fx=DX+DPAD;if(foot_btn(&fx,by,"Done"))advp_open=0;}
}

static void pk_popup(double now){
  if(!pk_t){pk_open=0;return;}
  dlg_begin(&scr_pk,U(340*MWF[mwSel]),0,U(46),now);
  {int one=1;char t[64];snprintf(t,sizeof t,"Pick a color: %s",pk_name);ui_head(t,&one,now);}
  if(*pk_t!=pk_rgb){int r=(*pk_t>>16)&255,g=(*pk_t>>8)&255,b=*pk_t&255;double h,sa,v;rgb2hsv(r,g,b,&h,&sa,&v);if(v>0){pk_s=sa;pk_v=v;if(sa>0)pk_h=h;}else pk_v=0;pk_rgb=*pk_t;}
  int changed=0,x0=PX,pw=PW,ph=U(160),y0=cy,xx,yy,r,g,b;
  // saturation / brightness square for the current hue
  {unsigned id=hid("pk-sv",5);cy+=ph+U(10);
   if(uhit(x0,y0,pw,ph))active_id=id;
   if(active_id==id&&mdown){pk_s=dclamp((mx-x0)/(double)(pw-1),0,1);pk_v=1-dclamp((my-y0)/(double)(ph-1),0,1);changed=1;}
   if(vis(y0,ph)){int hr,hg,hb;hsv2rgb(pk_h,1,1,&hr,&hg,&hb);
     for(yy=0;yy<ph;yy++){double v=1-yy/(double)(ph-1);for(xx=0;xx<pw;xx++){double sa=xx/(double)(pw-1);
       int R=(int)((v*((1-sa)+sa*hr/255.0))*255+.5),G=(int)((v*((1-sa)+sa*hg/255.0))*255+.5),B=(int)((v*((1-sa)+sa*hb/255.0))*255+.5);
       putpx(x0+xx,y0+yy,RGB(R,G,B),255);}}
     border_in(x0,y0,pw,ph,1,P.ln,1);
     double px=x0+pk_s*(pw-1),py=y0+(1-pk_v)*(ph-1);ring_aa(px,py,U(8),1.6,HEX(0),1);ring_aa(px,py,U(6),1.6,HEX(0xffffff),1);}}
  // hue strip
  {unsigned id=hid("pk-hue",5);int hh=U(22),yb=cy;cy+=hh+U(12);
   if(uhit(x0,yb,pw,hh))active_id=id;
   if(active_id==id&&mdown){pk_h=dclamp((mx-x0)/(double)(pw-1),0,1)*359.999;changed=1;}
   if(vis(yb,hh)){for(xx=0;xx<pw;xx++){hsv2rgb(xx/(double)(pw-1)*360,1,1,&r,&g,&b);fill_rect(x0+xx,yb,1,hh,RGB(r,g,b),1);}
     border_in(x0,yb,pw,hh,1,P.ln,1);double hx=x0+pk_h/360.0*(pw-1);fill_rect(hx-U(2),yb-U(2),U(4),hh+U(4),HEX(0xffffff),1);border_in(hx-U(2),yb-U(2),U(4),hh+U(4),1,HEX(0),1);}}
  if(changed){hsv2rgb(pk_h,pk_s,pk_v,&r,&g,&b);*pk_t=HEX(((u32)r<<16)|((u32)g<<8)|(u32)b);pk_rgb=*pk_t;ui_changed=1;}
  // exact red / green / blue, 256 steps each: 16.7 million colors
  {int R=(*pk_t>>16)&255,G=(*pk_t>>8)&255,B=*pk_t&255,ch=0;char t[8];sl_notype=1;
   snprintf(t,sizeof t,"%d",R);if(ui_slider("Red",&R,0,255,t))ch=1;
   snprintf(t,sizeof t,"%d",G);if(ui_slider("Green",&G,0,255,t))ch=1;
   snprintf(t,sizeof t,"%d",B);if(ui_slider("Blue",&B,0,255,t))ch=1;
   sl_notype=0;if(ch){*pk_t=HEX(((u32)R<<16)|((u32)G<<8)|(u32)B);}}
  ui_color("Hex code",pk_t,"",0);
  {int y=cy,h=U(30);cy+=U(40);if(vis(y,h)){label(y,"New / previous");int w=CWd(),x=PX+PW-w;
     fill_rect(x,y,w/2,h,*pk_t,1);fill_rect(x+w/2,y,w-w/2,h,pk_orig,1);border_in(x,y,w,h,1,P.fg,.8);}}
  dlg_end(&scr_pk);
  {int by=DY+DHH-DPAD-U(30);fill_rect(DX+DPAD,by-U(8),PW,1,P.sep,1);
   int fx=DX+DPAD;if(foot_btn(&fx,by,"Done")){hx_commit();pk_open=0;ui_changed=1;}
   if(foot_btn(&fx,by,"Revert")){hx_commit();*pk_t=pk_orig;ui_changed=1;}}
}
static const char* ABOUT_INTRO="8tris starts from how modern Tetris plays and asks what happens when you can change almost everything. The ideas behind it:";
static const char* IDEAS[5][2]={
 {"Minimal first.","You open to a black board and plain gray blocks. Themes, effects and extras are opt-in, so the first experience is just the game."},
 {"Open ended.","Any board size, pieces from 1 to 10 blocks or letters, several hold slots, your own keys and your own pace. Presets are starting points, not rules."},
 {"Modern by default.","Where it can, 8tris plays like current Tetris. Where it does not, the list below says so."},
 {"Honest scores.","Scoring adjusts for board width and piece size, and history records every setting, so runs can be compared fairly."},
 {"Yours alone.","Preferences and history stay in a folder on this computer. 8tris does not send them anywhere, and it runs as a plain native program with no browser inside."}};
static const char* DIFFS[10][2]={
 {"Board:","any size from 4 to 500 in each direction (default 10 by 20). There are no hidden rows above the board; pieces appear inside the top rows."},
 {"Pieces:","besides the 7 tetrominoes there are pieces of 1 to 10 blocks and letters A to Z, picked by size weight. Each size has its own shuffled bag, so with only size 4 on it behaves like a normal 7-bag."},
 {"Rotation:","standard SRS wall kicks for the 7 tetrominoes only. Other pieces use simpler sideways and upward kicks. No 180-degree rotation and no initial rotation or hold."},
 {"Gravity:","you set the base fall rate (default about 0.55 rows/s, where modern level 1 is 1 row/s). The optional level system uses the standard speed curve, but a level is 100 cleared cells instead of 10 lines."},
 {"Soft drop:","you choose its speed in rows per second (default 22). It is never slower than normal falling and never locks a piece."},
 {"Movement:","auto-shift is fixed at about 150 ms delay and 40 ms repeat. Modern games let you tune both."},
 {"Hold:","1 to 10 hold slots with retrieve and cycle keys. Holding and retrieving share one use per piece."},
 {"Scoring:","modern line-clear values, back-to-back 1.5 times for clears of 4 or more lines, and perfect clear bonuses. The combo bonus is small (10 per step). Line points scale with board width (10 wide is 1 times), drop points scale with piece size (4 blocks is 1 times), and the level multiplier only applies with the level system on. A T-spin (a T piece rotated into a spot where at least 3 of its 4 corners are blocked) that clears lines scores as much as a 4-line clear and counts for back-to-back. Mini T-spins are not told apart, and a T-spin that clears nothing scores nothing."},
 {"Game flow:","lines clear instantly with no clear or entry delay. The game is endless marathon style: no sprint, ultra, versus or garbage. It ends on block out or when a piece locks entirely above the board."},
 {"Stacking mode:","turning line clearing off removes clearing and scoring completely, which modern Tetris has no equivalent of."}};
static void about_popup(double now){
  int i;
  dlg_begin(&scr_about,U(380*MWF[mwSel]),0,U(46),now);
  {int one=1;ui_head("About 8tris",&one,now);}
  cy+=U(4);cy+=para(0,U(14),PX,cy,PW,1.5,P.fg,.9,ABOUT_INTRO,1)+U(6);
  for(i=0;i<5;i++){text_mid(1,U(13),PX+U(8),cy,U(20),P.fg,.95,IDEAS[i][0]);cy+=U(20);cy+=para(0,U(13),PX+U(8),cy,PW-U(8),LH,P.fg,.85,IDEAS[i][1],1)+U(6);}
  {int one=1;ui_head("How 8tris differs from modern Tetris",&one,now);}
  for(i=0;i<10;i++){text_mid(1,U(13),PX+U(8),cy,U(20),P.fg,.95,DIFFS[i][0]);cy+=U(20);cy+=para(0,U(13),PX+U(8),cy,PW-U(8),LH,P.fg,.85,DIFFS[i][1],1)+U(6);}
  cy+=para(0,U(11),PX,cy,PW,LH,P.fg,.5,"Game text is set in DejaVu Sans (free Bitstream Vera license). The preferences menu can also use Liberation, Carlito, Atkinson Hyperlegible and OpenDyslexic, all under the SIL Open Font License.",1)+U(6);
  cy+=para(0,U(11),PX,cy,PW,LH,P.fg,.5,"Music files are decoded with dr_mp3, dr_flac, dr_wav (David Reid), stb_vorbis and stb_image (Sean Barrett), all public domain or MIT-0.",1)+U(6);
  text_right(0,U(11),PX+PW,cy+U(12),P.fg,.4,"Created by Claude");cy+=U(24);
  dlg_end(&scr_about);
  {int by=DY+DHH-DPAD-U(30);fill_rect(DX+DPAD,by-U(8),PW,1,P.sep,1);int fx=DX+DPAD;if(foot_btn(&fx,by,"Close"))about_open=0;}
}

// ---- Per-frame entry points ----
static int menu_visible(void){return menu&&!pv_on;}
static void ui_frame(double now){
  UF=(menu&&!pv_on)?1.25*uiPt/14.0:1;fctx_on=(menu&&!pv_on);fctx_fam=uiFam;fctx_bold=uiBold;fctx_ital=uiItal;pal_update();
  if(hx_id&&mclicked&&!(clx>=hx_x&&clx<hx_x+hx_w&&cly>=hx_y&&cly<hx_y+hx_h))hx_commit();
  if(focus_id&&mclicked&&!(clx>=f_x&&clx<f_x+f_w&&cly>=f_y&&cly<f_y+f_h))focus_commit();
  select_click();
  if(pv_on){pv_tick();}
  else if(menu){
    int sx=mx,sy=my,sc=mclicked;double sw=wheel_acc;
    if(freq_open||about_open||advp_open||pk_open||partsp_open||fltp_open){mx=my=-9999;mclicked=0;wheel_acc=0;}
    settings_panel(now);
    mx=sx;my=sy;mclicked=sc;wheel_acc=sw;
    if(partsp_open){int s1=mx,s2=my,s3=mclicked;double s4=wheel_acc;if(about_open||pk_open){mx=my=-9999;mclicked=0;wheel_acc=0;}parts_popup(now);mx=s1;my=s2;mclicked=s3;wheel_acc=s4;}
    if(fltp_open){int s1=mx,s2=my,s3=mclicked;double s4=wheel_acc;if(about_open||pk_open){mx=my=-9999;mclicked=0;wheel_acc=0;}flt_popup(now);mx=s1;my=s2;mclicked=s3;wheel_acc=s4;}
    if(advp_open){int s1=mx,s2=my,s3=mclicked;double s4=wheel_acc;if(about_open||freq_open||pk_open){mx=my=-9999;mclicked=0;wheel_acc=0;}adv_popup(now);mx=s1;my=s2;mclicked=s3;wheel_acc=s4;}
    if(freq_open&&!pk_open){int s1=mx,s2=my,s3=mclicked;double s4=wheel_acc;if(about_open){mx=my=-9999;mclicked=0;wheel_acc=0;}freq_popup(now);mx=s1;my=s2;mclicked=s3;wheel_acc=s4;}
    if(about_open)about_popup(now);
    if(pk_open)pk_popup(now);
    select_popup();
  }
  mclicked=0;wheel_acc=0;UF=1;fctx_on=0;
  // Changes are written at most a few times a second (a dragged slider changes every frame), and right away when the menu closes.
  {static int dirty=0;static double last=-1e9;if(ui_changed){dirty=1;ui_changed=0;}
   if(dirty&&(now-last>=400||!menu||now<last)){save_settings();dirty=0;last=now;}}
}
// Returns 1 when the UI used the key.
static int ui_key(int key,int down){
  if(pv_on){if(down)pv_stop();return 1;}
  if(!menu)return 0;
  if(bind_listen>=0){
    if(!down)return 1;
    if(key==K_ESC){bind_listen=-1;return 1;}
    int a=bind_listen/2,s=bind_listen%2;
    if(key==K_BKSP){binds[a][s]=0;}
    else if(key!=K_F11&&key!=K_SHIFT&&key!=K_CTRL&&key!=K_ALT){int x,y;for(x=0;x<N_ACTS;x++)for(y=0;y<2;y++)if(binds[x][y]==key)binds[x][y]=0;binds[a][s]=key;}
    else return 1;
    bind_listen=-1;ui_changed=1;clearHeld();save_settings();return 1;
  }
  if(!down)return 1;
  if(!hx_id&&!focus_id&&!sel_open&&!advp_open&&!freq_open&&!about_open&&!pk_open&&!partsp_open&&!fltp_open){
    if(key==K_PGDN)scr_main.scroll+=U(320);else if(key==K_PGUP)scr_main.scroll-=U(320);else if(key==K_DOWN)scr_main.scroll+=U(56);else if(key==K_UP)scr_main.scroll-=U(56);
    else if(key==K_HOME)scr_main.scroll=0;else if(key==K_END)scr_main.scroll=1<<20;
    if(scr_main.scroll<0)scr_main.scroll=0;
  }
  if(hx_id){
    int n=(int)strlen(hx_buf),d=-1;
    if(key>='0'&&key<='9')d=key;else if(key>='a'&&key<='f')d=key;else if(key>='A'&&key<='F')d=key+32;
    if(d>=0){if(hx_fresh){hx_buf[0]=0;n=0;hx_fresh=0;}if(n<6){hx_buf[n]=(char)d;hx_buf[n+1]=0;n++;if(n==6){*hx_ptr=HEX(strtoul(hx_buf,NULL,16));ui_changed=1;}}}
    else if(key==K_BKSP){if(hx_fresh){hx_buf[0]=0;hx_fresh=0;}else if(n>0)hx_buf[n-1]=0;}
    else if(key==K_ENTER||key==K_TAB)hx_commit();
    else if(key==K_ESC)hx_id=0;
    return 1;
  }
  if(focus_id){
    int n=(int)strlen(fbuf);
    if(key>='0'&&key<='9'){if(f_fresh){fbuf[0]=0;n=0;f_fresh=0;}if(n<(f_dptr?8:6)){if(!f_dptr&&n==1&&fbuf[0]=='0'){fbuf[0]=(char)key;}else{fbuf[n]=(char)key;fbuf[n+1]=0;}}}
    else if(key=='-'&&f_lo<0&&!f_dptr){if(f_fresh){fbuf[0]=0;n=0;f_fresh=0;}if(n==0){fbuf[0]='-';fbuf[1]=0;}}
    else if(key=='.'&&f_dptr){if(f_fresh){fbuf[0]=0;n=0;f_fresh=0;}if(n<8&&!strchr(fbuf,'.')){if(n==0){fbuf[0]='0';n=1;}fbuf[n]='.';fbuf[n+1]=0;}}
    else if(key==K_BKSP){if(f_fresh){fbuf[0]=0;f_fresh=0;}else if(n>0)fbuf[n-1]=0;}
    else if(key==K_ENTER||key==K_TAB)focus_commit();
    else if(key==K_ESC)focus_id=0;
    return 1;
  }
  if(key==K_ESC){
    if(sel_open)sel_open=0;else if(pk_open)pk_open=0;else if(about_open)about_open=0;else if(freq_open)freq_open=0;else if(advp_open)advp_open=0;else if(fltp_open)fltp_open=0;else if(partsp_open)partsp_open=0;else{menu=0;clearHeld();}
    return 1;
  }
  return 1;
}
