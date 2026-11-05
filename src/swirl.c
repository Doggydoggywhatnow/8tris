// ===== SWIRL THEME PRIMITIVES =====
// The Swirl theme avoids straight lines and squares: blocks are blobs with spinning spirals, grid lines waver, frames are braided
// ribbons, and backgrounds are flowing marbled fields. Its colors are an odd palette picked at random every time the game starts.
// Every Swirl component (page, panel, menu, frame, board, grid, pieces, landing outline, effects, sound) is built from the helpers
// here, so a component can be mixed with any other theme without depending on the rest.
//
// Speed, in order of importance:
//   1. Backgrounds are rendered into cached images by a background worker thread into a spare buffer, and swapped in when complete.
//      The frame itself only copies the cached image. (If a thread cannot be started the image is built on the spot instead.)
//   2. Each blob block is drawn once per (color, size, spin angle) into a small sprite, so drawing a block is a copy.
//   3. Sine waves come from a lookup table, per-line work (colors) is done once per line, and discs skip the edge math for interior pixels.
//
// Contents: palette, small shapes, the background field, cached backgrounds, braided ribbon, blob blocks, drifting loops.

// ---- Palette ----
#define SWN 1024                                  // palette entries
static u32 SWL[SWN];static int sw_ready=0;
static float SINT[1024];                          // sine table: fsin(x) = sin(x), x in radians
static inline float fsin(double x){return SINT[((int)(x*162.97466))&1023];}

static u32 mixc(u32 a,u32 b,double k){int r=(int)(((a>>16)&255)*(1-k)+((b>>16)&255)*k),g=(int)(((a>>8)&255)*(1-k)+((b>>8)&255)*k),bl=(int)((a&255)*(1-k)+(b&255)*k);return RGB(r,g,bl);}
static u32 darken(u32 c,double k){return RGB((int)(((c>>16)&255)*k),(int)(((c>>8)&255)*k),(int)((c&255)*k));}
// Picks 8 odd colors and blends them into a looping palette.
static void swirl_init(void){
  u32 st[8];int i;double h0=rnd()*360;
  for(i=0;i<8;i++){double h=h0+i*45+(rnd()-.5)*150;if(i%3==2)h+=180;st[i]=hsl(h,.72+.28*rnd(),.42+.22*rnd());}
  for(i=0;i<SWN;i++){double pos=(double)i/SWN*8;int a=(int)pos;double f=pos-a;f=f*f*(3-2*f);SWL[i]=mixc(st[a%8],st[(a+1)%8],f);}
  for(i=0;i<1024;i++)SINT[i]=(float)sin(i*6.2831853/1024);
  sw_ready=1;
}
// A color from the palette. t wraps around, so any real number is fine.
static u32 sw_col(double t){if(!sw_ready)swirl_init();t-=floor(t);return SWL[((int)(t*SWN))&(SWN-1)];}

// ---- Shapes (all anti-aliased) ----
static void disc(double cx,double cy,double R,u32 c,double alpha){
  int x0=imax((int)floor(cx-R-1),CX0),x1=imin((int)ceil(cx+R+1),CX1),y0=imax((int)floor(cy-R-1),CY0),y1=imin((int)ceil(cy+R+1),CY1),x,y;
  double inner=R-.5,in2=inner>0?inner*inner:-1,out=R+.5,out2=out*out;int a255=A255(alpha);
  for(y=y0;y<y1;y++){double dy=y+.5-cy;for(x=x0;x<x1;x++){double dx=x+.5-cx,d2=dx*dx+dy*dy;
    if(d2<=in2){putpx(x,y,c,a255);continue;}                       // fully inside
    if(d2>=out2)continue;                                           // fully outside
    putpx(x,y,c,A255(alpha*(R-sqrt(d2)+.5)));}}                     // on the edge
}
// A ring. Only the pixels near the ring itself are visited, so a big ring is cheap.
static void ring_aa(double cx,double cy,double R,double lw,u32 c,double alpha){
  double Ro=R+lw/2+1,Ri=dmax(0,R-lw/2-1);int y0=imax((int)floor(cy-Ro),CY0),y1=imin((int)ceil(cy+Ro),CY1),y;
  for(y=y0;y<y1;y++){double dy=y+.5-cy,o=Ro*Ro-dy*dy;if(o<=0)continue;double xo=sqrt(o),i2=Ri*Ri-dy*dy,xi=i2>0?sqrt(i2):0;int pass,x;
    for(pass=0;pass<2;pass++){int xa,xb;if(xi<=0){if(pass)break;xa=(int)floor(cx-xo);xb=(int)ceil(cx+xo);}else if(!pass){xa=(int)floor(cx-xo);xb=(int)ceil(cx-xi);}else{xa=(int)floor(cx+xi);xb=(int)ceil(cx+xo);}
      xa=imax(xa,CX0);xb=imin(xb,CX1);
      for(x=xa;x<xb;x++){double dx=x+.5-cx,cv=lw/2-fabs(sqrt(dx*dx+dy*dy)-R)+.5;if(cv<=0)continue;putpx(x,y,c,A255(alpha*(cv>1?1:cv)));}}}
}
// A point at a fractional x, split over the two pixels it overlaps.
static void aa_px(double x,double y,u32 c,double a){int ix=(int)floor(x);double f=x-ix;putpx(ix,(int)floor(y),c,A255(a*(1-f)));putpx(ix+1,(int)floor(y),c,A255(a*f));}

// ---- The flowing field that colors the backgrounds ----
// A gently warped spiral. Low spatial frequency on purpose, so it stays smooth.
static double sw_field(double x,double y,double t){
  double u=x+.4*fsin(y*1.5+t*.0006),w=y+.4*fsin(u*1.3-t*.0005),r=sqrt(u*u+w*w),a=atan2(w,u);
  return a*(2/6.2831853)+r*.7-t*.0001+.3*fsin(r*2.8-t*.0007);
}
static const u8 BAYER[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
#define GMAX 4096                                  // most grid points across one row

// Renders cell rows [gy0,gy1) of the field into dst (w x h pixels). Safe to run on the worker thread (it only touches its own arguments). The field is sampled on a coarse grid (one sample per `cell` pixels)
// and blended smoothly in between in 8.8 fixed point, with a tiny ordered dither so slow gradients show no banding.
static void swirl_rows(u32*dst,int w,int h,int cell,double t,double dark,int gy0,int gy1){
  u32 GR[2][GMAX];                                  // two rows of field samples
  int gw=(w+cell-1)/cell+1,gx,gy,i,j,cur=0;double sc=dmax(w,h)/2.0,k=1.5/sc;
  for(gx=0;gx<gw;gx++)GR[0][gx]=darken(sw_col(sw_field((gx*cell-w/2.0)*k,(gy0*cell-h/2.0)*k,t)),dark);
  for(gy=gy0;gy<gy1;gy++){
    u32*top=GR[cur],*bot=GR[cur^1];
    for(gx=0;gx<gw;gx++)bot[gx]=darken(sw_col(sw_field((gx*cell-w/2.0)*k,((gy+1)*cell-h/2.0)*k,t)),dark);
    for(gx=0;gx<gw-1;gx++){
      u32 c00=top[gx],c10=top[gx+1],c01=bot[gx],c11=bot[gx+1];
      for(j=0;j<cell;j++){int py=gy*cell+j;if(py>=h)break;int fy=j*256/cell;
        int lr=(int)((c00>>16)&255)*256+((int)((c01>>16)&255)-(int)((c00>>16)&255))*fy,lg=(int)((c00>>8)&255)*256+((int)((c01>>8)&255)-(int)((c00>>8)&255))*fy,lb=(int)(c00&255)*256+((int)(c01&255)-(int)(c00&255))*fy;
        int rr=(int)((c10>>16)&255)*256+((int)((c11>>16)&255)-(int)((c10>>16)&255))*fy,rg=(int)((c10>>8)&255)*256+((int)((c11>>8)&255)-(int)((c10>>8)&255))*fy,rb=(int)(c10&255)*256+((int)(c11&255)-(int)(c10&255))*fy;
        int sr=(rr-lr)/cell,sg=(rg-lg)/cell,sb=(rb-lb)/cell;u32*row=&dst[py*w];const u8*bay=&BAYER[(py&3)*4];
        for(i=0;i<cell;i++){int px=gx*cell+i;if(px>=w)break;int dt=bay[px&3]*16-120;
          int r=(lr+sr*i+dt)>>8,g=(lg+sg*i+dt)>>8,b=(lb+sb*i+dt)>>8;
          r=r<0?0:r>255?255:r;g=g<0?0:g>255?255:g;b=b<0?0:b>255?255:b;
          row[px]=RGB(r,g,b);}}
    }
    cur^=1;
  }
}

// ---- Cached backgrounds ----
// Each slot keeps the image on screen (buf) and a spare (back) that the worker thread fills. A refresh starts when the image is older than
// `every` ms; when the worker is done the two are swapped. Job states: 0 idle, 1 queued, 2 running, 3 done (waiting to be swapped in).
// slot: 0 page, 1 side panel, 2 board, 3 menu.
typedef struct{u32*dst;int w,h,cell,state;double t,dark;}SwJob;
typedef struct{u32*buf,*back;int w,h,cell;double t,tgen,dark;}SwCache;
static SwCache SC[4];static SwJob JOB[4];static int sw_thread=0;   // sw_thread: 0 not started, 1 running, -1 could not start
static int job_state(int k){return __atomic_load_n(&JOB[k].state,__ATOMIC_ACQUIRE);}
static void job_set(int k,int st){__atomic_store_n(&JOB[k].state,st,__ATOMIC_RELEASE);}
static void swirl_gen_all(u32*dst,int w,int h,int cell,double t,double dark){swirl_rows(dst,w,h,cell,t,dark,0,(h+cell-1)/cell);}
static void sw_worker(void*arg){
  (void)arg;
  for(;;){int k,did=0;
    for(k=0;k<4;k++)if(job_state(k)==1){SwJob*j=&JOB[k];job_set(k,2);swirl_gen_all(j->dst,j->w,j->h,j->cell,j->t,j->dark);job_set(k,3);did=1;}
    if(!did)plat_sleep(2);
  }
}
static void swirl_fill(int slot,double rx,double ry,double rw,double rh,int cell,double t,double dark,double every){
  int x0=(int)floor(rx),y0=(int)floor(ry),w=(int)ceil(rw),h=(int)ceil(rh),j;if(w<1||h<1)return;
  if(!sw_ready)swirl_init();                                         // the worker reads the palette, so build it before it can start
  // Big areas use bigger cells: the field is smooth, so this is invisible but several times cheaper at high resolutions.
  cell=imax(cell,imin(w,h)/110);while((w+cell-1)/cell+1>GMAX)cell++;
  SwCache*c=&SC[slot];SwJob*jb=&JOB[slot];
  if(!c->buf||c->w!=w||c->h!=h||c->dark!=dark||c->cell!=cell){       // first image, or the area changed: build it right away
    while(job_state(slot)==1||job_state(slot)==2)plat_sleep(1);       // never touch the buffers while the worker is using them
    job_set(slot,0);
    if(!c->buf||c->w!=w||c->h!=h){free(c->buf);free(c->back);c->buf=(u32*)malloc((size_t)w*h*4);c->back=(u32*)malloc((size_t)w*h*4);c->w=w;c->h=h;}
    if(!c->buf||!c->back)return;
    swirl_gen_all(c->buf,w,h,cell,t,dark);c->t=t;c->dark=dark;c->cell=cell;
  }else{
    if(job_state(slot)==3){u32*tmp=c->buf;c->buf=c->back;c->back=tmp;c->t=c->tgen;job_set(slot,0);}   // the worker finished: show its image
    if(job_state(slot)==0&&fabs(t-c->t)>=every){
      if(!sw_thread)sw_thread=plat_thread(sw_worker,NULL)?1:-1;
      c->tgen=t;
      if(sw_thread>0){jb->dst=c->back;jb->w=w;jb->h=h;jb->cell=cell;jb->t=t;jb->dark=dark;job_set(slot,1);}
      else{swirl_gen_all(c->back,w,h,cell,t,dark);u32*tmp=c->buf;c->buf=c->back;c->back=tmp;c->t=t;}
    }
  }
  int ya=imax(y0,CY0),yb=imin(y0+h,CY1),xa=imax(x0,CX0),xb=imin(x0+w,CX1);if(xb<=xa)return;
  for(j=ya;j<yb;j++)memcpy(&FB[j*FW+xa],&c->buf[(j-y0)*w+(xa-x0)],(size_t)(xb-xa)*4);
}

// ---- Braided ribbon ----
// Two sine waves that cross each other run around a rectangle, drawn as a chain of round dots. The braid settles to a straight line at each
// corner so corners stay clean.
static void swirl_ribbon(double x,double y,double w,double h,double off,double amp,double wl,double t,double thick,double alpha){
  double Lp=2*(w+h),s;int k;
  for(k=0;k<2;k++)for(s=0;s<Lp;s+=2){
    double px,py,nx,ny,u,len;
    if(s<w){px=x+s;py=y;nx=0;ny=-1;u=s;len=w;}else if(s<w+h){px=x+w;py=y+(s-w);nx=1;ny=0;u=s-w;len=h;}else if(s<2*w+h){px=x+w-(s-w-h);py=y+h;nx=0;ny=1;u=s-w-h;len=w;}else{px=x;py=y+h-(s-2*w-h);nx=-1;ny=0;u=s-2*w-h;len=h;}
    double e=dmin(u,len-u)/(wl*1.5);e=e>1?1:e*e*(3-2*e);
    double d=off+(k?-1:1)*amp*e*fsin(s/wl+t*.0014);disc(px+nx*d,py+ny*d,thick,sw_col(s/260+t*.00012+k*.5),alpha);
  }
}

// ---- Blob blocks ----
// A dark rim, a body, a spinning spiral arm and a shine. Drawn once per (color, size, spin angle) into a premultiplied-alpha sprite.
#define SPIN_STEPS 48
typedef struct{int key;int sz;u32*px;}Spr;
#define NSPR 2048
static Spr SPR[NSPR];static long spr_bytes=0,spr_count=0;
static void swirl_block_raw(double cx,double cy,double s,int ci,double rot){
  double R=s*.56;u32 base=sw_col(ci*.137+.05),acc=sw_col(ci*.137+.5);
  disc(cx,cy,R,darken(base,.5),1);disc(cx,cy,R-dmax(1,s*.07),base,1);
  if(s<9)return;
  int n=(int)(s*2.4),i;
  for(i=0;i<n;i++){double f=(double)i/n,th=rot+f*5.4,rr=f*R*.7;disc(cx+cos(th)*rr,cy+sin(th)*rr,dmax(.9,s*.06*(1-f*.45)),acc,.92);}
  disc(cx-R*.32,cy-R*.36,R*.15,HEX(0xffffff),.65);
}
static void spr_clear(void){int i;for(i=0;i<NSPR;i++){free(SPR[i].px);SPR[i].px=NULL;SPR[i].key=0;}spr_bytes=0;spr_count=0;}
static Spr* spr_get(int ci,int s,int ph){
  int key=((ci&63)*8192+s)*64+ph+1;unsigned h=(unsigned)key*2654435761u;int i=(int)(h%NSPR),n;
  for(n=0;n<NSPR;n++,i=(i+1)%NSPR){if(SPR[i].key==key)return &SPR[i];if(!SPR[i].key)break;}
  if(spr_count>NSPR*3/4||spr_bytes>96L*1024*1024){spr_clear();i=(int)(h%NSPR);}
  int sz=(int)ceil(s*1.12)+3;u32*a=(u32*)calloc((size_t)sz*sz,4),*m=(u32*)calloc((size_t)sz*sz,4);if(!a||!m){free(a);free(m);return NULL;}
  // Render the color layer and a silhouette mask into scratch buffers, then combine them into premultiplied ARGB.
  u32*sFB=FB;int sFW=FW,sFH=FH,c0=CX0,c1=CX1,r0=CY0,r1=CY1;
  gfx_target(a,sz,sz);swirl_block_raw(sz/2.0,sz/2.0,s,ci,ph*6.2831853/SPIN_STEPS);
  gfx_target(m,sz,sz);disc(sz/2.0,sz/2.0,s*.56,HEX(0xffffff),1);
  FB=sFB;FW=sFW;FH=sFH;CX0=c0;CX1=c1;CY0=r0;CY1=r1;
  int k;for(k=0;k<sz*sz;k++)a[k]=((m[k]&255)<<24)|(a[k]&0xffffff);   // the colors were drawn over black, so they are already premultiplied
  free(m);Spr*sp=&SPR[i];sp->key=key;sp->sz=sz;sp->px=a;spr_bytes+=(long)sz*sz*4;spr_count++;return sp;
}
// gx, gy vary the spin from block to block; t (ms) drives the spin.
static void swirl_block(double x,double y,double s,int ci,double t,int gx,int gy){
  int is=imax(2,(int)floor(s+.5));double rot=t*.0012+(gx*13+gy*7)*.9;
  int ph=(int)floor(fmod(fmod(rot,6.2831853)+6.2831853,6.2831853)/6.2831853*SPIN_STEPS)%SPIN_STEPS;if(is<9)ph=0;
  Spr*sp=spr_get(ci,is,ph);if(!sp)return;
  int sz=sp->sz,ox=(int)floor(x+s/2-sz/2.0+.5),oy=(int)floor(y+s/2-sz/2.0+.5),i,j;
  for(j=0;j<sz;j++){int py=oy+j;if(py<CY0||py>=CY1)continue;
    for(i=0;i<sz;i++){int px=ox+i;if(px<CX0||px>=CX1)continue;u32 p=sp->px[j*sz+i];unsigned a=p>>24;if(!a)continue;u32*d=&FB[py*FW+px];
      if(a>=255)*d=p|0xFF000000u;                                    // premultiplied: result = sprite + destination * (1 - alpha)
      else{unsigned ia=256-(a+(a>>7)),dd=*d;*d=0xFF000000u|(((p&0xFF00FFu)+(((dd&0xFF00FFu)*ia>>8)&0xFF00FFu))&0xFF00FFu)|(((p&0xFF00u)+(((dd&0xFF00u)*ia>>8)&0xFF00u))&0xFF00u);}}}
}

// ---- Loops ----
// Big thin rings drifting over the page background.
static void swirl_loops(double w,double h,double t){
  int i;for(i=0;i<4;i++){double a=t*.00021*(1+i*.13)+i*1.9,cx=w/2+cos(a*1.3+i)*w*.38,cy=h/2+sin(a*.9+i*2)*h*.36,R=dmin(w,h)*(.07+.05*fabs(sin(i*2.3+t*.0002)));
    ring_aa(cx,cy,R,2.5,sw_col(i/4.0+t*.00005),.22);}
}
