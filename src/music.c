// ===== MUSIC FROM LOCAL FILES =====
// Plays .mp3, .ogg, .wav and .flac files the user puts in the "music" folder next to the saved preferences. Files are read only from
// this computer, streamed from the file by a background thread (any length; a few seconds are held at a time), and never copied, uploaded or tracked.
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
// How much of the start of a file holds its tags, so only that much is read (a multi-hour file is never pulled into memory).
static int meta_need(const char*path,const char*x){
  FILE*f=fopen(path,"rb");if(!f)return 0;unsigned char h[10];long need=0;
  if(!strcmp(x,"mp3")){if(fread(h,1,10,f)==10&&!memcmp(h,"ID3",3))need=10+(long)((h[6]&127)<<21|(h[7]&127)<<14|(h[8]&127)<<7|(h[9]&127))+((h[5]&0x10)?10:0);}
  else if(!strcmp(x,"flac")){if(fread(h,1,4,f)==4&&!memcmp(h,"fLaC",4)){long pos=4;int k;for(k=0;k<128;k++){if(fread(h,1,4,f)!=4)break;long len=(long)h[1]<<16|h[2]<<8|h[3];pos+=4+len;if(h[0]&128)break;if(fseek(f,len,SEEK_CUR))break;}need=pos;}}
  else if(!strcmp(x,"ogg"))need=4*1024*1024;
  fclose(f);return need>24*1024*1024?24*1024*1024:(int)need;}
// WAV tags sit in chunks that may come after hours of audio: walk the chunk headers and read only the tag chunks.
static void meta_wav(const char*path,Meta*m){
  FILE*f=fopen(path,"rb");if(!f)return;unsigned char h[12];if(fread(h,1,12,f)!=12||memcmp(h,"RIFF",4)){fclose(f);return;}
  int k;for(k=0;k<1000;k++){if(fread(h,1,8,f)!=8)break;unsigned cl=rd_le32(h+4);
    int tag=!memcmp(h,"LIST",4)||!memcmp(h,"id3 ",4)||!memcmp(h,"ID3 ",4);
    if(tag&&cl<=24u*1024*1024){unsigned char*cd=(unsigned char*)malloc(cl+1);if(!cd)break;if(fread(cd,1,cl,f)!=cl){free(cd);break;}
      if(!memcmp(h,"LIST",4)&&cl>=4&&!memcmp(cd,"INFO",4)){unsigned q=4;while(q+8<=cl){unsigned sl=rd_le32(cd+q+4);if(sl>cl-q-8)break;
          if(!memcmp(cd+q,"INAM",4)&&!m->title[0])meta_set(m->title,sizeof m->title,(const char*)cd+q+8,(int)sl);
          else if(!memcmp(cd+q,"IART",4)&&!m->artist[0])meta_set(m->artist,sizeof m->artist,(const char*)cd+q+8,(int)sl);
          else if(!memcmp(cd+q,"IPRD",4)&&!m->album[0])meta_set(m->album,sizeof m->album,(const char*)cd+q+8,(int)sl);
          q+=8+sl+(sl&1);}}
      else if(memcmp(h,"LIST",4))meta_id3v2(m,cd,(int)cl);
      free(cd);if(cl&1)fseek(f,1,SEEK_CUR);}
    else if(fseek(f,(long)cl+(long)(cl&1),SEEK_CUR))break;}
  fclose(f);}
static void meta_read(const char*path,Meta*m){
  memset(m,0,sizeof*m);const char*e=strrchr(path,'.');char x[8]={0};int i;if(e)for(i=0;e[i+1]&&i<6;i++)x[i]=(char)tolower((unsigned char)e[i+1]);
  int n=0;unsigned char*b=NULL;if(strcmp(x,"wav")){int need=meta_need(path,x);if(need>0)b=slurp(path,need,&n);}   /* only the tag area */
  if(!b){b=(unsigned char*)calloc(1,1);n=0;}
  if(!strcmp(x,"mp3")){
    meta_id3v2(m,b,n);
    unsigned char tl[128];int tn=0;{FILE*f=fopen(path,"rb");if(f){if(fseek(f,-128,SEEK_END)==0)tn=(int)fread(tl,1,128,f);fclose(f);}}   /* the old tag sits at the very end, which may be past what was read for a long file */
    if((!m->title[0])&&tn==128&&!memcmp(tl,"TAG",3)){const unsigned char*t=tl;meta_set(m->title,sizeof m->title,(const char*)t+3,30);if(!m->artist[0])meta_set(m->artist,sizeof m->artist,(const char*)t+33,30);if(!m->album[0])meta_set(m->album,sizeof m->album,(const char*)t+63,30);}   // old-style ID3v1 tag at the very end
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
  }else if(!strcmp(x,"wav"))meta_wav(path,m);
  free(b);
  if(!m->title[0]){const char*s=strrchr(path,'/');s=s?s+1:path;const char*bs=strrchr(s,'\\');if(bs)s=bs+1;int len=(int)strlen(s);const char*dot=strrchr(s,'.');if(dot)len=(int)(dot-s);meta_set(m->title,sizeof m->title,s,len);}   // no title tag: the file name
}


#define MUS_MAXFILES 100
static int musMode=0,musVol=50;                 // 0 off, 1 repeat one track, 2 all in order, 3 all shuffled; volume %
static int musPaused=0;                         // the on-screen pause button stopped the music where it was (not saved)
static char musFile[256];                       // the track chosen for "repeat one" (saved by file name)
static char musFold[256];                       // "" = every track; otherwise only tracks inside this folder (and its folders) are played
static char mfold[MUS_MAXFILES][256];static int nfold=0;
static char mfiles[MUS_MAXFILES][256];static int nmfiles=0,musSel=0,musCur=-1,mus_have=0;   // mus_have: a track is open and playing (main thread)
static char mus_status[200]="";static int mus_failed=0,mus_tries=0,mus_prevMode=0,mus_prevSel=-1;static char mus_prevFold[256];
// Tracks are streamed: a decoder thread keeps a few seconds of audio ready in a ring, so any length works (multi-hour audiobooks too)
// and memory use stays the same. The ring is written only by the decoder and read only by the audio thread.
#define MR_FRAMES (1<<18)                         // about 6 seconds, stereo float (2 MB)
static float mr_buf[MR_FRAMES*2];static i64 mr_w=0,mr_r=0;static int mr_live=0,mr_eof=0,mus_ended=0;static i64 mus_played=0;
// Hand-over between the main thread and the decoder, always under plat_lock: a command (1 open job_path, 2 close) and its result.
static int sc_cmd=0,sc_seq=0,sc_res=0,sc_res_seq=0;static char job_path[700],job_err[120],job_name[256];static int job_idx=-1,mus_job=0;static Meta job_meta,cur_meta;


static const char* music_dir(void){static char d[700];snprintf(d,sizeof d,"%smusic",plat_pref_dir());return d;}
static int music_ext_ok(const char*n){{const unsigned char*q=(const unsigned char*)n;for(;*q;q++)if(*q<32||*q==127)return 0;}   // no control characters in names
  const char*e=strrchr(n,'.');if(!e)return 0;char x[8];int i;for(i=0;e[i+1]&&i<6;i++)x[i]=(char)tolower((unsigned char)e[i+1]);x[i]=0;if(e[i+1])return 0;return !strcmp(x,"mp3")||!strcmp(x,"ogg")||!strcmp(x,"wav")||!strcmp(x,"flac");}
static int music_cmp(const void*a,const void*b){const char*x=(const char*)a,*y=(const char*)b;for(;*x&&*y;x++,y++){int d=tolower((unsigned char)*x)-tolower((unsigned char)*y);if(d)return d;}return tolower((unsigned char)*x)-tolower((unsigned char)*y);}
// Lists the music folder and the folders inside it (names are kept as "Folder/Track.mp3"; never follows links, so it cannot loop or leave the folder).
#define MUS_DEPTH 4
static void music_walk(const char*rel,int depth){
  char full[800],child[300];snprintf(full,sizeof full,"%s%s%s",music_dir(),rel[0]?"/":"",rel);
#ifdef _WIN32
  {char pat[820];snprintf(pat,sizeof pat,"%s\\*",full);WIN32_FIND_DATAA fd;HANDLE h=FindFirstFileA(pat,&fd);
   if(h!=INVALID_HANDLE_VALUE){do{const char*n=fd.cFileName;if(n[0]=='.'||nmfiles>=MUS_MAXFILES)continue;
     if(strlen(n)+strlen(rel)+2>=255)continue;snprintf(child,sizeof child,"%s%s%s",rel,rel[0]?"/":"",n);
     if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){if(depth<MUS_DEPTH&&!(fd.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))music_walk(child,depth+1);}
     else if(music_ext_ok(n))snprintf(mfiles[nmfiles++],256,"%s",child);}while(FindNextFileA(h,&fd));FindClose(h);}}
#else
  {DIR*d=opendir(full);struct dirent*e;if(d){while((e=readdir(d))&&nmfiles<MUS_MAXFILES){const char*n=e->d_name;if(n[0]=='.'||strlen(n)+strlen(rel)+2>=255)continue;
     snprintf(child,sizeof child,"%s%s%s",rel,rel[0]?"/":"",n);int isdir=0;
     if(e->d_type==DT_DIR)isdir=1;else if(e->d_type==DT_UNKNOWN){char fp[1100];struct stat st;snprintf(fp,sizeof fp,"%s/%s",full,n);if(lstat(fp,&st)==0&&S_ISDIR(st.st_mode))isdir=1;}
     if(isdir){if(depth<MUS_DEPTH)music_walk(child,depth+1);}
     else if(music_ext_ok(n))snprintf(mfiles[nmfiles++],256,"%s",child);}closedir(d);}}
#endif
}
// Lists the music folder (creating it if needed) and keeps the chosen track selected by name.
static void music_scan(void){
  nmfiles=0;
#ifdef _WIN32
  CreateDirectoryA(music_dir(),NULL);
#else
  mkdir(music_dir(),0700);
#endif
  music_walk("",0);
  qsort(mfiles,nmfiles,256,music_cmp);
  {int i,j;nfold=0;for(i=0;i<nmfiles;i++){char t[256];snprintf(t,sizeof t,"%s",mfiles[i]);char*sl;while((sl=strrchr(t,'/'))){*sl=0;int have=0;for(j=0;j<nfold;j++)if(!strcmp(mfold[j],t)){have=1;break;}if(!have&&nfold<MUS_MAXFILES)snprintf(mfold[nfold++],256,"%s",t);}}
   qsort(mfold,nfold,256,music_cmp);
   {int ok=!musFold[0];for(j=0;j<nfold;j++)if(!strcmp(mfold[j],musFold))ok=1;if(!ok)musFold[0]=0;}}   // the chosen folder is gone: back to all
  {int i;musSel=0;for(i=0;i<nmfiles;i++)if(!strcmp(mfiles[i],musFile)){musSel=i;break;}if(nmfiles)snprintf(musFile,sizeof musFile,"%s",mfiles[musSel]);}
  mus_failed=0;mus_tries=0;
}

// ---- Decoding: any supported file is streamed as stereo 44.1 kHz float. Other sample rates go through a windowed-sinc resampler
// (32 taps, Kaiser window, low-passed below the lower Nyquist) so nothing aliases; 44.1 kHz files pass straight through. Samples are
// kept as floats with no clipping; the one limiter at the very end of the mix keeps the output clean. ----
#define RS_HALF 16
#define RS_TAPS (2*RS_HALF)
#define RS_PH 1024
#define RS_IN 8192
typedef struct{double ratio,t;int pass,n;float fl[RS_IN+RS_TAPS],fr[RS_IN+RS_TAPS];float*h;}Rs;
static double rs_bessel0(double x){double s=1,t=1;int k;for(k=1;k<30;k++){t*=(x/(2*k))*(x/(2*k));s+=t;}return s;}
static void rs_init(Rs*r,int rate){
  r->ratio=rate/44100.0;r->pass=rate==44100;r->t=RS_HALF-1;r->n=0;memset(r->fl,0,sizeof r->fl);memset(r->fr,0,sizeof r->fr);
  r->n=RS_HALF-1;   /* leading silence so the first output lines up with the first input frame */
  if(r->pass)return;
  if(!r->h)r->h=(float*)malloc(sizeof(float)*(RS_PH+1)*RS_TAPS);if(!r->h){r->pass=1;return;}
  double fc=(r->ratio>1?1/r->ratio:1)*.94,beta=8.0,ib=1/rs_bessel0(beta);int p,k;
  for(p=0;p<=RS_PH;p++){double fr=(double)p/RS_PH,sum=0;float*row=r->h+p*RS_TAPS;
    for(k=0;k<RS_TAPS;k++){double x=k-(RS_HALF-1)-fr,sx=fc*x,v=fabs(sx)<1e-9?fc:fc*sin(M_PI*sx)/(M_PI*sx),q=x/RS_HALF,w=fabs(q)>=1?0:rs_bessel0(beta*sqrt(1-q*q))*ib;row[k]=(float)(v*w);sum+=v*w;}
    for(k=0;k<RS_TAPS;k++)row[k]=(float)(row[k]/sum);}   // each phase sums to exactly 1: no level change, no DC ripple
}
// Feeds stereo frames in; writes resampled frames to out (up to cap), returns how many were written.
static int rs_run(Rs*r,const float*in,int nf,float*out,int cap){
  int o=0,i;
  if(r->pass){for(i=0;i<nf&&o<cap;i++){out[2*o]=in[2*i];out[2*o+1]=in[2*i+1];o++;}return o;}
  for(i=0;i<nf;i++){r->fl[r->n]=in[2*i];r->fr[r->n]=in[2*i+1];r->n++;
    while(o<cap){int b=(int)r->t;if(b+RS_HALF+1>r->n)break;double fr=(r->t-b)*RS_PH;int ph=(int)fr;if(ph>=RS_PH)ph=RS_PH-1;float pa=(float)(fr-ph);const float*h0=r->h+ph*RS_TAPS,*h1=h0+RS_TAPS;const float*xl=r->fl+b-(RS_HALF-1),*xr=r->fr+b-(RS_HALF-1);
      float sl=0,sr=0;int k;for(k=0;k<RS_TAPS;k++){float c=h0[k]+(h1[k]-h0[k])*pa;sl+=c*xl[k];sr+=c*xr[k];}   /* between two phase rows: no phase steps */out[2*o]=sl;out[2*o+1]=sr;o++;r->t+=r->ratio;}
    if(r->n>=RS_IN){int d=(int)r->t-(RS_HALF-1);if(d>0){memmove(r->fl,r->fl+d,sizeof(float)*(r->n-d));memmove(r->fr,r->fr+d,sizeof(float)*(r->n-d));r->n-=d;r->t-=d;}}}
  return o;
}
// The open track (decoder thread only).
enum{ST_NONE,ST_MP3,ST_WAV,ST_FLAC,ST_OGG};
static struct{int kind,ch;drmp3 m;drwav w;drflac*f;stb_vorbis*v;Rs rs;int tail;}ST;
static void st_close(void){
  if(ST.kind==ST_MP3)drmp3_uninit(&ST.m);else if(ST.kind==ST_WAV)drwav_uninit(&ST.w);else if(ST.kind==ST_FLAC)drflac_close(ST.f);else if(ST.kind==ST_OGG)stb_vorbis_close(ST.v);
  ST.kind=ST_NONE;ST.f=NULL;ST.v=NULL;}
static int st_open(const char*path,char*err,int errn){
  const char*e=strrchr(path,'.');char x[8]={0};int i,rate=0;if(e)for(i=0;e[i+1]&&i<6;i++)x[i]=(char)tolower((unsigned char)e[i+1]);
  st_close();
  if(!strcmp(x,"mp3")){if(drmp3_init_file(&ST.m,path,NULL)){ST.kind=ST_MP3;ST.ch=ST.m.channels;rate=ST.m.sampleRate;}}
  else if(!strcmp(x,"wav")){if(drwav_init_file(&ST.w,path,NULL)){ST.kind=ST_WAV;ST.ch=ST.w.channels;rate=ST.w.sampleRate;}}
  else if(!strcmp(x,"flac")){if((ST.f=drflac_open_file(path,NULL))){ST.kind=ST_FLAC;ST.ch=ST.f->channels;rate=ST.f->sampleRate;}}
  else if(!strcmp(x,"ogg")){int er=0;if((ST.v=stb_vorbis_open_filename(path,&er,NULL))){stb_vorbis_info in=stb_vorbis_get_info(ST.v);ST.kind=ST_OGG;ST.ch=2;rate=(int)in.sample_rate;}}   /* stb_vorbis mixes any channel layout down to 2 */
  if(!ST.kind||ST.ch<1||ST.ch>8||rate<4000||rate>768000){st_close();snprintf(err,errn,"Could not read this file. It may be damaged or in an unsupported variant.");return 0;}
  rs_init(&ST.rs,rate);ST.tail=0;return 1;
}
static void st_rewind(void){
  if(ST.kind==ST_MP3)drmp3_seek_to_pcm_frame(&ST.m,0);else if(ST.kind==ST_WAV)drwav_seek_to_pcm_frame(&ST.w,0);else if(ST.kind==ST_FLAC)drflac_seek_to_pcm_frame(ST.f,0);else if(ST.kind==ST_OGG)stb_vorbis_seek_start(ST.v);
  ST.tail=0;}
// Decodes one block and returns resampled stereo frames in out (up to cap; cap must be at least 4096*2+64), 0 at the end of the file.
static int st_read(float*out,int cap){
  static float raw[4096*8],st2[4096*2];int n=0,i,ch=ST.ch,nin=ST.rs.ratio>=1?4096:(int)(4096*ST.rs.ratio)-RS_TAPS;if(nin<64)nin=64;   /* upsampling: fewer frames in, so the output fits */
  for(;;){
    if(ST.kind==ST_MP3)n=(int)drmp3_read_pcm_frames_f32(&ST.m,nin,raw);
    else if(ST.kind==ST_WAV)n=(int)drwav_read_pcm_frames_f32(&ST.w,nin,raw);
    else if(ST.kind==ST_FLAC)n=(int)drflac_read_pcm_frames_f32(ST.f,nin,raw);
    else if(ST.kind==ST_OGG){n=stb_vorbis_get_samples_float_interleaved(ST.v,2,raw,nin*2);if(n<0)n=0;}
    if(n<=0){   // end of file: push the resampler's last half-window of silence through once, so the very end is not cut off
      if(ST.rs.pass||ST.tail)return 0;ST.tail=1;memset(st2,0,sizeof(float)*2*(RS_HALF+1));return rs_run(&ST.rs,st2,RS_HALF+1,out,cap);}
    for(i=0;i<n;i++){const float*q=raw+i*ch;float l,r;   // to stereo: mono is doubled; surround folds centre and rears in
      if(ch==1)l=r=q[0];else if(ch==2){l=q[0];r=q[1];}
      else{float c=ch>2?q[2]:0,bl=ch>4?q[4]:0,br=ch>5?q[5]:bl;l=(q[0]+.7071f*c+.7071f*bl)*.5858f;r=(q[1]+.7071f*c+.7071f*br)*.5858f;}
      if(!(l>-64&&l<64))l=0;if(!(r>-64&&r<64))r=0;st2[2*i]=l;st2[2*i+1]=r;}   /* a broken frame can never turn into a blast */
    {int o=rs_run(&ST.rs,st2,n,out,cap);if(o>0)return o;}   // downsampling may need more input before the first frame comes out
  }
}
// Decoder thread work: takes a command, or tops up the ring. Returns 1 when it did something (so the caller loops at once).
static int stream_service(void){
  static float blk[(4096*2+64)*2];int cmd,seq;
  plat_lock();cmd=sc_cmd;sc_cmd=0;seq=sc_seq;char path[700];snprintf(path,sizeof path,"%s",job_path);plat_unlock();
  if(cmd){
    st_close();plat_lock();mr_live=0;mr_w=mr_r=0;mr_eof=0;mus_ended=0;plat_unlock();
    if(cmd==1){char err[120]="";int ok=st_open(path,err,sizeof err),got=0;
      if(ok){meta_free(&job_meta);meta_read(path,&job_meta);
        while(got<2205){int o=st_read(blk,4096*2+64);if(o<=0)break;memcpy(mr_buf+2*got,blk,sizeof(float)*2*o);got+=o;}   /* the first block goes straight in (ring is empty) */
        if(got<2205){ok=0;st_close();snprintf(err,sizeof err,"This file has no audio in it.");}}
      plat_lock();
      if(ok&&sc_cmd==0&&sc_seq==seq){__atomic_store_n(&mr_w,(i64)got,__ATOMIC_RELEASE);mr_live=1;mus_played=0;}
      sc_res=ok?2:3;sc_res_seq=seq;snprintf(job_err,sizeof job_err,"%s",err);plat_unlock();}
    return 1;}
  if(!ST.kind||mr_eof)return 0;
  i64 w=mr_w,r=__atomic_load_n(&mr_r,__ATOMIC_ACQUIRE);if(MR_FRAMES-(w-r)<4096*2+64)return 0;   // the ring is full enough
  int o=st_read(blk,4096*2+64);
  if(o<=0){if(musMode==1){st_rewind();return 1;}plat_lock();mr_eof=1;plat_unlock();return 0;}   // repeat one loops without a gap
  int i;for(i=0;i<o;i++){size_t k=(size_t)((w+i)&(MR_FRAMES-1));mr_buf[2*k]=blk[2*i];mr_buf[2*k+1]=blk[2*i+1];}
  __atomic_store_n(&mr_w,w+o,__ATOMIC_RELEASE);return 1;
}
static void mus_stream_fn(void*arg){(void)arg;for(;;){if(!stream_service())plat_sleep(4);}}
static int st_started=0,st_threaded=0;
static void music_start(int idx){
  if(!st_started){st_started=1;st_threaded=plat_thread(mus_stream_fn,NULL);}
  mus_job=1;job_idx=idx;mus_have=0;snprintf(job_name,sizeof job_name,"%s",mfiles[idx]);
  plat_lock();mr_live=0;snprintf(job_path,sizeof job_path,"%s/%s",music_dir(),mfiles[idx]);sc_seq++;sc_cmd=1;plat_unlock();
  snprintf(mus_status,sizeof mus_status,"Loading %s...",job_name);
  if(!st_threaded)stream_service();   // no thread available: open it right here (the main loop keeps the ring topped up)
}
static void music_stop(void){
  plat_lock();mr_live=0;sc_seq++;sc_cmd=2;plat_unlock();if(st_started&&!st_threaded)stream_service();
  mus_have=0;mus_job=0;meta_free(&cur_meta);musCur=-1;
}
// Is track i inside the chosen folder (or are all folders chosen)?
static int music_in_scope(int i){if(!musFold[0])return 1;size_t L=strlen(musFold);return !strncmp(mfiles[i],musFold,L)&&mfiles[i][L]=='/';}
static int music_scope_count(void){int i,n=0;for(i=0;i<nmfiles;i++)n+=music_in_scope(i);return n;}
// Mixes the playing track into the output (called from the audio thread, after the sound effects). No clipping here: the final
// limiter in audio_mix handles the sum. If the decoder ever falls behind, the gap is silence, never old or broken samples.
static void music_mix(float*out,int n){
  if(!musMode||musVol<=0||musPaused)return;
  plat_lock();
  if(mr_live){float g=(float)musVol/100.0f*0.8f;i64 r=mr_r,av=__atomic_load_n(&mr_w,__ATOMIC_ACQUIRE)-r;int m=av<n?(int)av:n,i;
    for(i=0;i<m;i++){size_t k=(size_t)((r+i)&(MR_FRAMES-1));out[2*i]+=mr_buf[2*k]*g;out[2*i+1]+=mr_buf[2*k+1]*g;}
    __atomic_store_n(&mr_r,r+m,__ATOMIC_RELEASE);mus_played+=m;
    if(m<n&&mr_eof&&r+m==mr_w)mus_ended=1;}
  plat_unlock();
}
// Called once per frame from the main thread: takes finished opens and starts the next track when needed.
static void music_tick(void){
  if(st_started&&!st_threaded){int k;for(k=0;k<64&&stream_service();k++);}   // no decoder thread: top the ring up from here
  {int res=0;plat_lock();if(sc_res){if(sc_res_seq==sc_seq&&mus_job==1)res=sc_res;sc_res=0;}plat_unlock();
   if(res==2){meta_free(&cur_meta);cur_meta=job_meta;memset(&job_meta,0,sizeof job_meta);musCur=job_idx;mus_have=1;mus_tries=0;mus_job=0;snprintf(mus_status,sizeof mus_status,"Playing %s",job_name);}
   else if(res==3){snprintf(mus_status,sizeof mus_status,"%s: %s",job_name,job_err);mus_tries++;mus_job=0;if(musMode==1||mus_tries>=music_scope_count())mus_failed=1;
     else musCur=job_idx;}}                 /* in the "all" modes a bad file is skipped */
  if(mus_job==1)return;
  if(strcmp(musFold,mus_prevFold)){snprintf(mus_prevFold,sizeof mus_prevFold,"%s",musFold);   // another folder was chosen
    if(musMode==1&&nmfiles&&!music_in_scope(musSel)){int i;for(i=0;i<nmfiles;i++)if(music_in_scope(i)){musSel=i;snprintf(musFile,sizeof musFile,"%s",mfiles[i]);break;}}
    if(musMode>1&&musCur>=0&&!music_in_scope(musCur)){music_stop();mus_failed=0;mus_tries=0;}}
  if(musMode!=mus_prevMode||(musMode==1&&musSel!=mus_prevSel)){   // the user changed the mode or track
    mus_prevMode=musMode;mus_prevSel=musSel;mus_failed=0;mus_tries=0;if(!musMode){music_stop();mus_status[0]=0;musPaused=0;return;}
    if(musMode==1||mus_have){music_stop();}
  }
  if(!musMode||mus_failed)return;
  if(!nmfiles){snprintf(mus_status,sizeof mus_status,"No music files found yet.");mus_failed=1;return;}
  {int sc=music_scope_count();if(!sc){snprintf(mus_status,sizeof mus_status,"No music files in this folder.");mus_failed=1;return;}}
  {int ended;plat_lock();ended=mus_ended;plat_unlock();
   int need=!mus_have||ended;
   if(musMode==1&&mus_have&&musCur!=musSel)need=1;
   if(!need)return;
   int idx;
   if(musMode==1)idx=musSel;
   else if(musMode==2){idx=musCur<0?-1:musCur;int k;for(k=0;k<nmfiles;k++){idx=(idx+1)%nmfiles;if(music_in_scope(idx))break;}}
   else{int sc=music_scope_count(),pick=sc>1?(int)(rnd()*sc):0,k;if(sc>1&&musCur>=0&&music_in_scope(musCur)){int cpos=0;for(k=0;k<musCur;k++)cpos+=music_in_scope(k);if(pick==cpos)pick=(pick+1)%sc;}
     idx=0;for(k=0;k<nmfiles;k++)if(music_in_scope(k)){if(!pick--){idx=k;break;}}}
   if(idx<0||idx>=nmfiles)idx=0;
   music_start(idx);}
}
