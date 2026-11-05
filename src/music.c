// ===== MUSIC FROM LOCAL FILES =====
// Plays .mp3, .ogg, .wav and .flac files the user puts in the "music" folder next to the saved preferences. Files are read only from
// this computer, decoded in a background thread into memory (stereo, 44.1 kHz, 16 bit), and never copied, uploaded or tracked.
// Decoders: dr_mp3, dr_flac, dr_wav (David Reid) and stb_vorbis (Sean Barrett), all public domain or MIT-0.
#include <stdint.h>
#include <ctype.h>
#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif
#define DR_MP3_IMPLEMENTATION
#define DR_FLAC_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#define DR_FLAC_NO_OGG
#define DRFLAC_NO_OGG
#include "dr_mp3.h"
#include "dr_flac.h"
#include "dr_wav.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 8192
#include "stb_image.h"
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"
#undef L
#undef R
#undef C
#ifdef _WIN32
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

// ---- Track information: title, artist, album and cover art, read from the places each format keeps them ----
typedef struct{char title[160],artist[120],album[120];u32*art;int aw,ah;}Meta;
static void meta_free(Meta*m){free(m->art);memset(m,0,sizeof*m);}
static void meta_set(char*dst,int n,const char*s,int len){   // copies at most n-1 bytes of UTF-8, trimming spaces and line breaks
  while(len>0&&(s[len-1]==' '||s[len-1]=='\0'||s[len-1]=='\r'||s[len-1]=='\n'))len--;
  while(len>0&&(*s==' '))s++,len--;
  if(len>n-1){len=n-1;while(len>0&&((unsigned char)s[len]&0xC0)==0x80)len--;}   // too long: cut on a character boundary
  {int i;for(i=0;i<len;i++){unsigned char c=(unsigned char)s[i];dst[i]=(c<32)?' ':(char)c;}dst[len]=0;}
}
// Any encoding of an ID3 text frame to UTF-8.
static void id3_text(char*dst,int n,const unsigned char*p,int len){
  if(len<1){dst[0]=0;return;}int enc=p[0];p++;len--;char tmp[700];int o=0;
  if(enc==0){int i;for(i=0;i<len&&o<(int)sizeof tmp-4;i++){unsigned c=p[i];if(!c)break;if(c<128)tmp[o++]=(char)c;else{tmp[o++]=(char)(0xC0|(c>>6));tmp[o++]=(char)(0x80|(c&63));}}}
  else if(enc==3){int i;for(i=0;i<len&&o<(int)sizeof tmp-1;i++){if(!p[i])break;tmp[o++]=(char)p[i];}}
  else{int be=enc==2,i=0;if(len>=2&&enc==1){if(p[0]==0xFE&&p[1]==0xFF){be=1;i=2;}else if(p[0]==0xFF&&p[1]==0xFE){be=0;i=2;}}
    for(;i+1<len&&o<(int)sizeof tmp-6;i+=2){unsigned c=be?(p[i]<<8|p[i+1]):(p[i+1]<<8|p[i]);if(!c)break;
      if(c>=0xD800&&c<0xDC00&&i+3<len){unsigned d=be?(p[i+2]<<8|p[i+3]):(p[i+3]<<8|p[i+2]);if(d>=0xDC00&&d<0xE000){c=0x10000+((c-0xD800)<<10)+(d-0xDC00);i+=2;}}
      if(c<0x80)tmp[o++]=(char)c;else if(c<0x800){tmp[o++]=(char)(0xC0|(c>>6));tmp[o++]=(char)(0x80|(c&63));}
      else if(c<0x10000){tmp[o++]=(char)(0xE0|(c>>12));tmp[o++]=(char)(0x80|((c>>6)&63));tmp[o++]=(char)(0x80|(c&63));}
      else{tmp[o++]=(char)(0xF0|(c>>18));tmp[o++]=(char)(0x80|((c>>12)&63));tmp[o++]=(char)(0x80|((c>>6)&63));tmp[o++]=(char)(0x80|(c&63));}}}
  meta_set(dst,n,tmp,o);
}
// JPEG or PNG bytes to a small ARGB picture (at most 256 px on the long side).
static void meta_art(Meta*m,const unsigned char*d,int len){
  if(m->art||len<16||len>40*1024*1024)return;int w,h,c;int ok=stbi_info_from_memory(d,len,&w,&h,&c);if(!ok||w<1||h<1||(i64)w*h>40000000)return;
  unsigned char*px=stbi_load_from_memory(d,len,&w,&h,&c,4);if(!px)return;
  int mx=w>h?w:h,tw=w,th=h;if(mx>256){tw=(int)((i64)w*256/mx);th=(int)((i64)h*256/mx);if(tw<1)tw=1;if(th<1)th=1;}
  u32*out=(u32*)malloc((size_t)tw*th*4);if(!out){stbi_image_free(px);return;}
  {int x,y;for(y=0;y<th;y++){int y0=(int)((i64)y*h/th),y1=(int)((i64)(y+1)*h/th);if(y1<=y0)y1=y0+1;
    for(x=0;x<tw;x++){int x0=(int)((i64)x*w/tw),x1=(int)((i64)(x+1)*w/tw);if(x1<=x0)x1=x0+1;long r=0,g=0,b=0,a=0,n=0;int yy,xx;
      for(yy=y0;yy<y1&&yy<h;yy++)for(xx=x0;xx<x1&&xx<w;xx++){const unsigned char*q=px+((size_t)yy*w+xx)*4;r+=q[0];g+=q[1];b+=q[2];a+=q[3];n++;}
      if(!n)n=1;r/=n;g/=n;b/=n;a/=n;out[y*tw+x]=0xFF000000u|((u32)((r*a+0*(255-a))/255)<<16)|((u32)((g*a)/255)<<8)|(u32)((b*a)/255);}}}
  stbi_image_free(px);m->art=out;m->aw=tw;m->ah=th;
}
static unsigned rd_be32(const unsigned char*p){return (unsigned)p[0]<<24|p[1]<<16|p[2]<<8|p[3];}
static unsigned rd_le32(const unsigned char*p){return (unsigned)p[3]<<24|p[2]<<16|p[1]<<8|p[0];}
// Vorbis comments (used by FLAC and Ogg): vendor string, then KEY=value entries.
static void vorbis_comments(Meta*m,const unsigned char*p,int len,void(*pic)(Meta*,const unsigned char*,int)){
  if(len<8)return;unsigned vl=rd_le32(p);if(vl>(unsigned)len-8)return;p+=4;len-=4;p+=vl;len-=(int)vl;
  if(len<4)return;unsigned cnt=rd_le32(p);p+=4;len-=4;unsigned i;
  for(i=0;i<cnt&&len>=4;i++){unsigned cl=rd_le32(p);p+=4;len-=4;if(cl>(unsigned)len)break;const char*s=(const char*)p;const char*eq=memchr(s,'=',cl);
    if(eq){int kl=(int)(eq-s);const char*v=eq+1;int vlen=(int)cl-kl-1;
      if(kl==5&&!strncasecmp(s,"TITLE",5)&&!m->title[0])meta_set(m->title,sizeof m->title,v,vlen);
      else if(kl==6&&!strncasecmp(s,"ARTIST",6)&&!m->artist[0])meta_set(m->artist,sizeof m->artist,v,vlen);
      else if(kl==5&&!strncasecmp(s,"ALBUM",5)&&!m->album[0])meta_set(m->album,sizeof m->album,v,vlen);
      else if(kl==22&&!strncasecmp(s,"METADATA_BLOCK_PICTURE",22)&&!m->art&&pic){   // base64 of a FLAC picture block
        unsigned char*bin=(unsigned char*)malloc((size_t)vlen);if(bin){int o=0,acc=0,bits=0,k;for(k=0;k<vlen;k++){char c=v[k];int val=c>='A'&&c<='Z'?c-'A':c>='a'&&c<='z'?c-'a'+26:c>='0'&&c<='9'?c-'0'+52:c=='+'?62:c=='/'?63:-1;if(val<0)continue;acc=acc<<6|val;bits+=6;if(bits>=8){bits-=8;bin[o++]=(unsigned char)(acc>>bits);acc&=(1<<bits)-1;}}
          pic(m,bin,o);free(bin);}}}
    p+=cl;len-=(int)cl;}
}
// A FLAC "picture" block: type, mime, description, size, colors, data.
static void flac_picture(Meta*m,const unsigned char*p,int len){
  if(len<32)return;unsigned ml=rd_be32(p+4);if(ml>(unsigned)len-32)return;int o=8+(int)ml;if(o+4>len)return;unsigned dl=rd_be32(p+o);o+=4;if(dl>(unsigned)len-(unsigned)o-20)return;o+=(int)dl+16;if(o+4>len)return;unsigned n=rd_be32(p+o);o+=4;if(n>(unsigned)(len-o))return;meta_art(m,p+o,(int)n);
}
static unsigned char* slurp(const char*path,int maxb,int*n){FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long sz=ftell(f);fseek(f,0,SEEK_SET);if(sz<0)sz=0;if(sz>maxb)sz=maxb;unsigned char*b=(unsigned char*)malloc((size_t)sz+1);if(!b){fclose(f);return NULL;}*n=(int)fread(b,1,(size_t)sz,f);fclose(f);return b;}
static void meta_id3v2(Meta*m,const unsigned char*b,int n){
  if(n<10||memcmp(b,"ID3",3))return;int ver=b[3];if(ver<2||ver>4)return;int flags=b[5];unsigned size=(b[6]&127)<<21|(b[7]&127)<<14|(b[8]&127)<<7|(b[9]&127);
  int pos=10,end=10+(int)size;if(end>n)end=n;if(flags&0x40){if(ver>=3&&pos+4<=end){unsigned es=rd_be32(b+pos);if(ver==4)es=(b[pos]&127)<<21|(b[pos+1]&127)<<14|(b[pos+2]&127)<<7|(b[pos+3]&127);pos+=(ver==4)?(int)es:(int)es+4;}}
  int hl=ver==2?6:10,bestType=-1;
  while(pos+hl<=end){const unsigned char*f=b+pos;if(!f[0])break;char id[5]={0};memcpy(id,f,ver==2?3:4);unsigned fs;
    if(ver==2)fs=(unsigned)f[3]<<16|f[4]<<8|f[5];else if(ver==3)fs=rd_be32(f+4);else fs=(f[4]&127)<<21|(f[5]&127)<<14|(f[6]&127)<<7|(f[7]&127);
    if(fs>(unsigned)(end-pos-hl))break;const unsigned char*d=f+hl;int dl=(int)fs;
    #define IS(a,b2) (!strcmp(id,a)||!strcmp(id,b2))
    if(IS("TIT2","TT2")&&!m->title[0])id3_text(m->title,sizeof m->title,d,dl);
    else if(IS("TPE1","TP1")&&!m->artist[0])id3_text(m->artist,sizeof m->artist,d,dl);
    else if(IS("TALB","TAL")&&!m->album[0])id3_text(m->album,sizeof m->album,d,dl);
    else if(IS("APIC","PIC")&&dl>8){   // encoding, mime (or 3-letter format in v2.2), picture type, description, data
      int enc=d[0],q=1,type;
      if(ver==2)q=4;else{while(q<dl&&d[q])q++;q++;}
      if(q<dl){type=d[q];q++;
        if(enc==1||enc==2){while(q+1<dl&&(d[q]||d[q+1]))q+=2;q+=2;}else{while(q<dl&&d[q])q++;q++;}
        if(q<dl&&(type==3||bestType<0)&&bestType!=3){Meta t;memset(&t,0,sizeof t);meta_art(&t,d+q,dl-q);if(t.art){free(m->art);m->art=t.art;m->aw=t.aw;m->ah=t.ah;bestType=type;}}}}
    #undef IS
    pos+=hl+(int)fs;}
}
static void meta_read(const char*path,Meta*m){
  memset(m,0,sizeof*m);const char*e=strrchr(path,'.');char x[8]={0};int i;if(e)for(i=0;e[i+1]&&i<6;i++)x[i]=(char)tolower((unsigned char)e[i+1]);
  int n=0;unsigned char*b=slurp(path,24*1024*1024,&n);if(!b)return;
  if(!strcmp(x,"mp3")){
    meta_id3v2(m,b,n);
    if((!m->title[0])&&n>=128&&!memcmp(b+n-128,"TAG",3)){const unsigned char*t=b+n-128;meta_set(m->title,sizeof m->title,(const char*)t+3,30);if(!m->artist[0])meta_set(m->artist,sizeof m->artist,(const char*)t+33,30);if(!m->album[0])meta_set(m->album,sizeof m->album,(const char*)t+63,30);}   // old-style ID3v1 tag at the very end
  }else if(!strcmp(x,"flac")&&n>8&&!memcmp(b,"fLaC",4)){
    int pos=4;for(;;){if(pos+4>n)break;int last=b[pos]&128,type=b[pos]&127;unsigned len=(unsigned)b[pos+1]<<16|b[pos+2]<<8|b[pos+3];pos+=4;if(len>(unsigned)(n-pos))break;
      if(type==4)vorbis_comments(m,b+pos,(int)len,NULL);else if(type==6&&!m->art)flac_picture(m,b+pos,(int)len);
      pos+=(int)len;if(last)break;}
  }else if(!strcmp(x,"ogg")&&n>27&&!memcmp(b,"OggS",4)){
    // Walk the Ogg pages and join the second packet, which is the Vorbis comment header.
    unsigned char*pk=NULL;int plen=0,pcap=0,npk=0,pos=0,done=0;
    while(!done&&pos+27<=n&&!memcmp(b+pos,"OggS",4)){int ns=b[pos+26];if(pos+27+ns>n)break;int dsz=0,s;for(s=0;s<ns;s++)dsz+=b[pos+27+s];const unsigned char*dp=b+pos+27+ns;if(pos+27+ns+dsz>n)break;
      int off=0;for(s=0;s<ns&&!done;s++){int sl=b[pos+27+s];
        if(npk==1){if(plen+sl>pcap){pcap=(plen+sl)*2+4096;unsigned char*np=(unsigned char*)realloc(pk,(size_t)pcap);if(!np){done=1;break;}pk=np;}memcpy(pk+plen,dp+off,(size_t)sl);plen+=sl;}
        off+=sl;if(sl<255){npk++;if(npk==2){done=1;break;}}}
      pos+=27+ns+dsz;if(npk==0&&pos>1<<20)break;}
    if(pk&&plen>7&&pk[0]==3&&!memcmp(pk+1,"vorbis",6))vorbis_comments(m,pk+7,plen-7,flac_picture);
    free(pk);
  }else if(!strcmp(x,"wav")&&n>12&&!memcmp(b,"RIFF",4)){
    int pos=12;while(pos+8<=n){unsigned cl=rd_le32(b+pos+4);const unsigned char*cd=b+pos+8;if(cl>(unsigned)(n-pos-8))cl=(unsigned)(n-pos-8);
      if(!memcmp(b+pos,"LIST",4)&&cl>=4&&!memcmp(cd,"INFO",4)){unsigned q=4;while(q+8<=cl){unsigned sl=rd_le32(cd+q+4);if(sl>cl-q-8)break;
          if(!memcmp(cd+q,"INAM",4)&&!m->title[0])meta_set(m->title,sizeof m->title,(const char*)cd+q+8,(int)sl);
          else if(!memcmp(cd+q,"IART",4)&&!m->artist[0])meta_set(m->artist,sizeof m->artist,(const char*)cd+q+8,(int)sl);
          else if(!memcmp(cd+q,"IPRD",4)&&!m->album[0])meta_set(m->album,sizeof m->album,(const char*)cd+q+8,(int)sl);
          q+=8+sl+(sl&1);}}
      else if(!memcmp(b+pos,"id3 ",4)||!memcmp(b+pos,"ID3 ",4))meta_id3v2(m,cd,(int)cl);
      pos+=8+(int)cl+(int)(cl&1);}
  }
  free(b);
  if(!m->title[0]){const char*s=strrchr(path,'/');s=s?s+1:path;const char*bs=strrchr(s,'\\');if(bs)s=bs+1;int len=(int)strlen(s);const char*dot=strrchr(s,'.');if(dot)len=(int)(dot-s);meta_set(m->title,sizeof m->title,s,len);}   // no title tag: the file name
}


#define MUS_MAXMIN 10        // longest track accepted, in minutes (it is held in memory: about 10 MB per minute)
#define MUS_MAXFILES 30
static int musMode=0,musVol=50;                 // 0 off, 1 repeat one track, 2 all in order, 3 all shuffled; volume %
static char musFile[256];                       // the track chosen for "repeat one" (saved by file name)
static char mfiles[MUS_MAXFILES][256];static int nmfiles=0,musSel=0,musCur=-1;
static char mus_status[200]="";static int mus_failed=0,mus_tries=0,mus_prevMode=0,mus_prevSel=-1;
// Audio thread side: the loaded track. Changed only under plat_lock.
static int16_t*mus_buf=NULL;static i64 mus_len=0,mus_pos=0;static int mus_ended=0;
// Loader side: a worker thread fills job_* and then sets mus_job to 2.
static volatile int mus_job=0;static int16_t*job_buf=NULL;static i64 job_len=0;static char job_path[700],job_err[120],job_name[256];static int job_idx=-1;static Meta job_meta,cur_meta;


static const char* music_dir(void){static char d[700];snprintf(d,sizeof d,"%smusic",plat_pref_dir());return d;}
static int music_ext_ok(const char*n){const char*e=strrchr(n,'.');if(!e)return 0;char x[8];int i;for(i=0;e[i+1]&&i<6;i++)x[i]=(char)tolower((unsigned char)e[i+1]);x[i]=0;if(e[i+1])return 0;return !strcmp(x,"mp3")||!strcmp(x,"ogg")||!strcmp(x,"wav")||!strcmp(x,"flac");}
static int music_cmp(const void*a,const void*b){const char*x=(const char*)a,*y=(const char*)b;for(;*x&&*y;x++,y++){int d=tolower((unsigned char)*x)-tolower((unsigned char)*y);if(d)return d;}return tolower((unsigned char)*x)-tolower((unsigned char)*y);}
// Lists the music folder (creating it if needed) and keeps the chosen track selected by name.
static void music_scan(void){
  nmfiles=0;const char*dir=music_dir();
#ifdef _WIN32
  CreateDirectoryA(dir,NULL);
  {char pat[720];snprintf(pat,sizeof pat,"%s\\*",dir);WIN32_FIND_DATAA fd;HANDLE h=FindFirstFileA(pat,&fd);
   if(h!=INVALID_HANDLE_VALUE){do{if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&fd.cFileName[0]!='.'&&strlen(fd.cFileName)<255&&music_ext_ok(fd.cFileName)&&nmfiles<MUS_MAXFILES)snprintf(mfiles[nmfiles++],256,"%s",fd.cFileName);}while(FindNextFileA(h,&fd));FindClose(h);}}
#else
  mkdir(dir,0755);
  {DIR*d=opendir(dir);struct dirent*e;if(d){while((e=readdir(d))&&nmfiles<MUS_MAXFILES){if(e->d_name[0]=='.'||strlen(e->d_name)>=255||!music_ext_ok(e->d_name))continue;snprintf(mfiles[nmfiles++],256,"%s",e->d_name);}closedir(d);}}
#endif
  qsort(mfiles,nmfiles,256,music_cmp);
  {int i;musSel=0;for(i=0;i<nmfiles;i++)if(!strcmp(mfiles[i],musFile)){musSel=i;break;}if(nmfiles)snprintf(musFile,sizeof musFile,"%s",mfiles[musSel]);}
  mus_failed=0;mus_tries=0;
}

// ---- Decoding: any supported file becomes stereo 44.1 kHz 16-bit. Sample rates are converted by linear interpolation. ----
typedef struct{int16_t*buf;i64 n,cap,limit;double ratio,ph;float pl,pr;int have,over;}Rs;
static void rs_emit(Rs*r,float l,float rr){
  if(r->n>=r->limit){r->over=1;return;}
  if(r->n>=r->cap){i64 nc=r->cap?r->cap*2:1<<20;int16_t*nb=(int16_t*)realloc(r->buf,(size_t)nc*4);if(!nb){r->over=1;return;}r->buf=nb;r->cap=nc;}
  l=l>1?1:l<-1?-1:l;rr=rr>1?1:rr<-1?-1:rr;r->buf[r->n*2]=(int16_t)(l*32767);r->buf[r->n*2+1]=(int16_t)(rr*32767);r->n++;
}
static void rs_push(Rs*r,const float*in,int ch,int nf){
  int i;for(i=0;i<nf&&!r->over;i++){float l=in[i*ch],rr=ch>1?in[i*ch+1]:l;
    if(!r->have){r->pl=l;r->pr=rr;r->have=1;r->ph=0;}
    while(r->ph<1&&!r->over){float f=(float)r->ph;rs_emit(r,r->pl+(l-r->pl)*f,r->pr+(rr-r->pr)*f);r->ph+=r->ratio;}
    r->ph-=1;r->pl=l;r->pr=rr;}
}
// Returns the decoded track (caller frees) or NULL with a reason in err.
static int16_t* music_decode(const char*path,i64*frames,char*err,int errn){
  const char*e=strrchr(path,'.');char x[8]={0};int i;if(e)for(i=0;e[i+1]&&i<6;i++)x[i]=(char)tolower((unsigned char)e[i+1]);
  Rs r;memset(&r,0,sizeof r);r.limit=(i64)MUS_MAXMIN*60*44100;static float tmp[4096*8];int ch=0,rate=0,ok=0;
  if(!strcmp(x,"mp3")){drmp3 m;if(drmp3_init_file(&m,path,NULL)){ch=m.channels;rate=m.sampleRate;if(ch>0&&ch<=8&&rate>0){r.ratio=rate/44100.0;ok=1;
      for(;;){drmp3_uint64 n=drmp3_read_pcm_frames_f32(&m,4096,tmp);if(!n)break;rs_push(&r,tmp,ch,(int)n);if(r.over)break;}}drmp3_uninit(&m);}}
  else if(!strcmp(x,"wav")){drwav w;if(drwav_init_file(&w,path,NULL)){ch=w.channels;rate=w.sampleRate;if(ch>0&&ch<=8&&rate>0){r.ratio=rate/44100.0;ok=1;
      for(;;){drwav_uint64 n=drwav_read_pcm_frames_f32(&w,4096,tmp);if(!n)break;rs_push(&r,tmp,ch,(int)n);if(r.over)break;}}drwav_uninit(&w);}}
  else if(!strcmp(x,"flac")){drflac*f=drflac_open_file(path,NULL);if(f){ch=f->channels;rate=f->sampleRate;if(ch>0&&ch<=8&&rate>0){r.ratio=rate/44100.0;ok=1;
      for(;;){drflac_uint64 n=drflac_read_pcm_frames_f32(f,4096,tmp);if(!n)break;rs_push(&r,tmp,ch,(int)n);if(r.over)break;}}drflac_close(f);}}
  else if(!strcmp(x,"ogg")){int er=0;stb_vorbis*v=stb_vorbis_open_filename(path,&er,NULL);if(v){stb_vorbis_info in=stb_vorbis_get_info(v);ch=2;rate=(int)in.sample_rate;if(rate>0){r.ratio=rate/44100.0;ok=1;
      for(;;){int n=stb_vorbis_get_samples_float_interleaved(v,2,tmp,4096*2);if(n<=0)break;rs_push(&r,tmp,2,n);if(r.over)break;}}stb_vorbis_close(v);}}
  if(!ok){snprintf(err,errn,"Could not read this file. It may be damaged or in an unsupported variant.");free(r.buf);return NULL;}
  if(r.over){snprintf(err,errn,"This track is longer than %d minutes, or too large to hold in memory.",MUS_MAXMIN);free(r.buf);return NULL;}
  if(r.n<2205){snprintf(err,errn,"This file has no audio in it.");free(r.buf);return NULL;}
  *frames=r.n;return r.buf;
}
static void music_job_fn(void*arg){
  (void)arg;i64 n=0;job_err[0]=0;job_buf=music_decode(job_path,&n,job_err,sizeof job_err);job_len=job_buf?n:0;meta_free(&job_meta);if(job_buf)meta_read(job_path,&job_meta);
  __atomic_store_n(&mus_job,2,__ATOMIC_RELEASE);
}
static void music_start(int idx){
  mus_job=1;job_idx=idx;snprintf(job_name,sizeof job_name,"%s",mfiles[idx]);snprintf(job_path,sizeof job_path,"%s/%s",music_dir(),mfiles[idx]);
  snprintf(mus_status,sizeof mus_status,"Loading %s...",job_name);
  if(!plat_thread(music_job_fn,NULL))music_job_fn(NULL);   // no thread available: decode right here
}
static void music_stop(void){
  int16_t*old;plat_lock();old=mus_buf;mus_buf=NULL;mus_len=0;mus_pos=0;mus_ended=0;plat_unlock();free(old);meta_free(&cur_meta);musCur=-1;
}
// Mixes the playing track into the output (called from the audio thread, after the sound effects).
static void music_mix(float*out,int n){
  if(!musMode||musVol<=0)return;
  plat_lock();
  if(mus_buf&&mus_len>0){float g=(float)musVol/100.0f*0.8f/32768.0f;int i;
    for(i=0;i<n;i++){
      if(mus_pos>=mus_len){if(musMode==1)mus_pos=0;else{mus_ended=1;break;}}
      float l=out[2*i]+mus_buf[mus_pos*2]*g,r=out[2*i+1]+mus_buf[mus_pos*2+1]*g;
      out[2*i]=l>1?1:l<-1?-1:l;out[2*i+1]=r>1?1:r<-1?-1:r;mus_pos++;}}
  plat_unlock();
}
// Called once per frame from the main thread: installs finished loads and starts the next track when needed.
static void music_tick(void){
  if(__atomic_load_n(&mus_job,__ATOMIC_ACQUIRE)==2){
    if(job_buf){int16_t*old;plat_lock();old=mus_buf;mus_buf=job_buf;mus_len=job_len;mus_pos=0;mus_ended=0;plat_unlock();free(old);job_buf=NULL;meta_free(&cur_meta);cur_meta=job_meta;memset(&job_meta,0,sizeof job_meta);
      musCur=job_idx;mus_tries=0;snprintf(mus_status,sizeof mus_status,"Playing %s",job_name);}
    else{snprintf(mus_status,sizeof mus_status,"%s: %s",job_name,job_err);mus_tries++;if(musMode==1||mus_tries>=nmfiles)mus_failed=1;
      else musCur=job_idx;}                 /* in the "all" modes a bad file is skipped */
    mus_job=0;
  }
  if(mus_job==1)return;
  if(musMode!=mus_prevMode||(musMode==1&&musSel!=mus_prevSel)){   // the user changed the mode or track
    mus_prevMode=musMode;mus_prevSel=musSel;mus_failed=0;mus_tries=0;if(!musMode){music_stop();mus_status[0]=0;return;}
    if(musMode==1||mus_buf){music_stop();}
  }
  if(!musMode||mus_failed)return;
  if(!nmfiles){snprintf(mus_status,sizeof mus_status,"No music files found yet.");mus_failed=1;return;}
  {int need=!mus_buf||mus_ended;
   if(musMode==1&&mus_buf&&musCur!=musSel)need=1;
   if(!need)return;
   int idx;
   if(musMode==1)idx=musSel;
   else if(musMode==2)idx=(musCur+1)%nmfiles;
   else{idx=nmfiles>1?(int)(rnd()*nmfiles):0;if(nmfiles>1&&idx==musCur)idx=(idx+1)%nmfiles;}
   if(idx<0||idx>=nmfiles)idx=0;
   if(musMode!=1&&mus_ended)music_stop();
   music_start(idx);}
}
