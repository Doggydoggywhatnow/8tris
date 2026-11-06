// Software renderer: everything is drawn into one ARGB pixel buffer, then the platform layer puts it on screen.
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"
#include "fontdata.h"
#include "fontdata2.h"

#ifdef __SSE2__
#include <emmintrin.h>
#endif
static u32 *FB=0;static int FW=0,FH=0,CX0=0,CY0=0,CX1=0,CY1=0;
static void gfx_target(u32*fb,int w,int h){FB=fb;FW=w;FH=h;CX0=0;CY0=0;CX1=w;CY1=h;}
static void clip(int x0,int y0,int x1,int y1){CX0=imax(0,x0);CY0=imax(0,y0);CX1=imin(FW,x1);CY1=imin(FH,y1);}
static void unclip(void){CX0=0;CY0=0;CX1=FW;CY1=FH;}

// Alpha blend of source s over destination d, a in 0..255. Red+blue and green are blended together in two multiplies (no per-channel divides).
static inline u32 blend(u32 d,u32 s,int a){
  a+=a>>7;unsigned ia=256-a;
  unsigned rb=(((d&0xFF00FFu)*ia+(s&0xFF00FFu)*a)>>8)&0xFF00FFu,g=(((d&0xFF00u)*ia+(s&0xFF00u)*a)>>8)&0xFF00u;
  return 0xFF000000u|rb|g;
}
static inline int A255(double a){return a<=0?0:a>=1?255:(int)(a*255+.5);}
static inline void putpx(int x,int y,u32 c,int a){if(x<CX0||y<CY0||x>=CX1||y>=CY1||a<=0)return;u32*p=&FB[y*FW+x];*p=a>=255?c:blend(*p,c,a);}

// Blends color c over n pixels at p. aa is the alpha in 0..256 and ia = 256-aa. Uses SSE2 (4 pixels at a time) where available.
static inline void blend_row(u32*p,int n,u32 c,unsigned aa,unsigned ia){
  int i=0;
#ifdef __SSE2__
  {__m128i z=_mm_setzero_si128(),vi=_mm_set1_epi16((short)ia),sm=_mm_mullo_epi16(_mm_unpacklo_epi8(_mm_set1_epi32((int)c),z),_mm_set1_epi16((short)aa)),ao=_mm_set1_epi32((int)0xFF000000u);
   for(;i+4<=n;i+=4){__m128i d=_mm_loadu_si128((const __m128i*)(p+i));
     __m128i lo=_mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(_mm_unpacklo_epi8(d,z),vi),sm),8),hi=_mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(_mm_unpackhi_epi8(d,z),vi),sm),8);
     _mm_storeu_si128((__m128i*)(p+i),_mm_or_si128(_mm_packus_epi16(lo,hi),ao));}}
#endif
  {unsigned srb=(c&0xFF00FFu)*aa,sg=(c&0xFF00u)*aa;
   for(;i<n;i++){u32 d=p[i];p[i]=0xFF000000u|((((d&0xFF00FFu)*ia+srb)>>8)&0xFF00FFu)|((((d&0xFF00u)*ia+sg)>>8)&0xFF00u);}}
}
static void fill_rect(double xf,double yf,double wf,double hf,u32 c,double alpha){
  int x=(int)floor(xf+.5),y=(int)floor(yf+.5),w=(int)floor(xf+wf+.5)-x,h=(int)floor(yf+hf+.5)-y;
  int a=A255(alpha),x0=imax(x,CX0),y0=imax(y,CY0),x1=imin(x+w,CX1),y1=imin(y+h,CY1),i,j;
  if(a<=0||x1<=x0||y1<=y0)return;
  if(a>=255){for(j=y0;j<y1;j++){u32*p=&FB[j*FW];for(i=x0;i<x1;i++)p[i]=c;}return;}
  {unsigned aa=a+(a>>7),ia=256-aa;
   for(j=y0;j<y1;j++)blend_row(&FB[j*FW+x0],x1-x0,c,aa,ia);}
}
// A border drawn inside the rectangle, t pixels thick.
static void border_in(double x,double y,double w,double h,double t,u32 c,double a){
  fill_rect(x,y,w,t,c,a);fill_rect(x,y+h-t,w,t,c,a);fill_rect(x,y+t,t,h-2*t,c,a);fill_rect(x+w-t,y+t,t,h-2*t,c,a);
}
// A border drawn outside the rectangle.
static void border_out(double x,double y,double w,double h,double t,u32 c,double a){border_in(x-t,y-t,w+2*t,h+2*t,t,c,a);}
// Soft glow around a rectangle, like a CSS box-shadow with a blur radius.
static void glow_rect(double x,double y,double w,double h,double blur,double spread,u32 c,double a){
  int n=(int)blur,i;if(n<1)return;
  for(i=0;i<n;i++){double k=1.0-(double)i/n;border_out(x-spread-i,y-spread-i,w+2*(spread+i),h+2*(spread+i),1,c,a*k*k*.6);}
}
static void fill_ring(double cx,double cy,double R,double lw,u32 c,double alpha){
  double Ro=R+lw/2,Ri=imax(0,R-lw/2);int a=A255(alpha),y,y0=(int)floor(cy-Ro),y1=(int)ceil(cy+Ro);
  for(y=imax(y0,CY0);y<imin(y1,CY1);y++){double dy=y+.5-cy,o=Ro*Ro-dy*dy;if(o<=0)continue;double xo=sqrt(o),xi=Ri*Ri-dy*dy>0?sqrt(Ri*Ri-dy*dy):0;
    int xa=(int)floor(cx-xo),xb=(int)ceil(cx-xi),xc=(int)floor(cx+xi),xd=(int)ceil(cx+xo),x;
    if(xi<=0){for(x=xa;x<xd;x++)putpx(x,y,c,a);}else{for(x=xa;x<xb;x++)putpx(x,y,c,a);for(x=xc;x<xd;x++)putpx(x,y,c,a);}}
}

// ---- Text ----
typedef struct{int f,sz,cp,w,h,xo,yo;float adv;u8*bmp;int used;}Gl;
// Faces are numbered family*4 + (bold) + 2*(italic). The game always uses family 0 (DejaVu Sans): f=0 regular, f=1 bold.
// The settings menu can switch on fctx_on, which maps every text call to the chosen family, bold and italic instead.
#define NFACE 28
static stbtt_fontinfo FI[NFACE];static unsigned char face_ok[NFACE];
static int fctx_on=0,fctx_fam=0,fctx_bold=0,fctx_ital=0;
static int face_of(int f){if(!fctx_on)return f&1;return fctx_fam*4+((f||fctx_bold)?1:0)+(fctx_ital?2:0);}
#define GCN 4096
static Gl GCa[GCN];static int gc_count=0;
static void font_init(int f){
  if(face_ok[f])return;const unsigned char*d=FDATA[f/4][f%4];
  stbtt_InitFont(&FI[f],d,stbtt_GetFontOffsetForIndex(d,0));face_ok[f]=1;
}
static Gl* glyph(int f,int sz,int cp){
  unsigned h=((unsigned)(f*977+sz)*2654435761u)^((unsigned)cp*40503u);int i=h%GCN,n;
  for(n=0;n<GCN;n++,i=(i+1)%GCN){Gl*g=&GCa[i];if(!g->used)break;if(g->f==f&&g->sz==sz&&g->cp==cp)return g;}
  if(gc_count>GCN*3/4){for(i=0;i<GCN;i++){if(GCa[i].used)free(GCa[i].bmp);GCa[i].used=0;}gc_count=0;i=h%GCN;}
  Gl*g=&GCa[i];font_init(f);
  int ff=f;if(f>=4){font_init(f&1);if(!stbtt_FindGlyphIndex(&FI[f],cp))ff=f&1;}   // glyphs a chosen font lacks come from DejaVu Sans
  float sc=stbtt_ScaleForMappingEmToPixels(&FI[f],(float)sz);int adv,lsb,x0,y0,x1,y1;
  if(ff!=f)sc=stbtt_ScaleForMappingEmToPixels(&FI[ff],(float)sz);stbtt_GetCodepointHMetrics(&FI[ff],cp,&adv,&lsb);stbtt_GetCodepointBitmapBox(&FI[ff],cp,sc,sc,&x0,&y0,&x1,&y1);
  g->f=f;g->sz=sz;g->cp=cp;g->adv=adv*sc;g->xo=x0;g->yo=y0;g->w=x1-x0;g->h=y1-y0;g->used=1;
  g->bmp=NULL;if(g->w>0&&g->h>0){g->bmp=(u8*)malloc((size_t)g->w*g->h);stbtt_MakeCodepointBitmap(&FI[ff],g->bmp,g->w,g->h,g->w,sc,sc,cp);}
  gc_count++;return g;
}
static int utf8_next(const char**s){
  const unsigned char*p=(const unsigned char*)*s;int c=*p++;
  if(c<0x80){}else if((c&0xE0)==0xC0){c=((c&0x1F)<<6)|(p[0]&0x3F);p++;}else if((c&0xF0)==0xE0){c=((c&0x0F)<<12)|((p[0]&0x3F)<<6)|(p[1]&0x3F);p+=2;}else{c=((c&0x07)<<18)|((p[0]&0x3F)<<12)|((p[1]&0x3F)<<6)|(p[2]&0x3F);p+=3;}
  *s=(const char*)p;return c;
}
// f: 0 regular, 1 bold. sz in pixels.
static double text_w(int f,double sz,const char*s){
  f=face_of(f);int isz=imax(4,(int)floor(sz+.5));double w=0;
  while(*s){int cp=utf8_next(&s);w+=glyph(f,isz,cp)->adv;}return w;
}
static void text_ascent(int f,double sz,double*asc,double*desc){f=face_of(f);font_init(f);float sc=stbtt_ScaleForMappingEmToPixels(&FI[f],(float)imax(4,(int)floor(sz+.5)));int a,d,g;stbtt_GetFontVMetrics(&FI[f],&a,&d,&g);*asc=a*sc;*desc=d*sc;}
// Draws with the left end at x and the text baseline at y.
static void text_at(int f,double sz,double x,double y,u32 c,double alpha,const char*s){
  f=face_of(f);int isz=imax(4,(int)floor(sz+.5)),a0=A255(alpha),ix=(int)floor(x+.5),iy=(int)floor(y+.5);double pen=0;
  while(*s){int cp=utf8_next(&s);Gl*g=glyph(f,isz,cp);
    if(g->bmp){int gx=ix+(int)floor(pen+.5)+g->xo,gy=iy+g->yo,i,j;
      for(j=0;j<g->h;j++){int yy=gy+j;if(yy<CY0||yy>=CY1)continue;for(i=0;i<g->w;i++){int xx=gx+i;if(xx<CX0||xx>=CX1)continue;int a=g->bmp[j*g->w+i];if(a)putpx(xx,yy,c,a*a0/255);}}}
    pen+=g->adv;}
}
// Text vertically centered in a box of height h whose top is y.
static void text_mid(int f,double sz,double x,double y,double h,u32 c,double alpha,const char*s){
  double asc,desc;text_ascent(f,sz,&asc,&desc);text_at(f,sz,x,y+(h-(asc-desc))/2+asc,c,alpha,s);
}
static void text_center(int f,double sz,double cx,double y,u32 c,double alpha,const char*s){text_at(f,sz,cx-text_w(f,sz,s)/2,y,c,alpha,s);}
static void text_right(int f,double sz,double rx,double y,u32 c,double alpha,const char*s){text_at(f,sz,rx-text_w(f,sz,s),y,c,alpha,s);}
// Text with a dark outline (used on top of the board where it must stay readable).
static void text_outlined(int f,double sz,double cx,double y,double lw,u32 fill,double alpha,const char*s){
  double x=cx-text_w(f,sz,s)/2,r=lw/2;int i;
  for(i=0;i<12;i++){double an=i*6.2831853/12;text_at(f,sz,x+cos(an)*r,y+sin(an)*r,RGB(0,0,0),alpha,s);}
  text_at(f,sz,x,y,fill,alpha,s);
}
// Greedy word wrap into lines of at most maxw pixels. Returns the number of lines written to out (each a pointer+length).
typedef struct{const char*p;int n;}Line;
static int wrap_lines(int f,double sz,double maxw,const char*s,Line*out,int cap){
  int n=0;const char*ls=s,*wend=s,*p=s;double w=0;
  while(1){
    // advance one word
    const char*ws=p;while(*p&&*p!=' '&&*p!='\n')p++;
    char tmp[512];int len=(int)(p-ws);if(len>500)len=500;memcpy(tmp,ws,len);tmp[len]=0;
    double ww=text_w(f,sz,tmp),sp=text_w(f,sz," ");
    if(w>0&&w+sp+ww>maxw){if(n<cap){out[n].p=ls;out[n].n=(int)(wend-ls);n++;}ls=ws;w=ww;}else w=(w>0?w+sp:0)+ww;
    wend=p;
    if(*p=='\n'){if(n<cap){out[n].p=ls;out[n].n=(int)(wend-ls);n++;}ls=p+1;w=0;wend=ls;p++;continue;}
    if(!*p){if(n<cap){out[n].p=ls;out[n].n=(int)(wend-ls);n++;}break;}
    p++;
  }
  return n;
}
