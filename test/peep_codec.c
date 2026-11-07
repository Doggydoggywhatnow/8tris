#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/pc/";} const char* plat_home_dir(void){return "/tmp/pc/";}
// Liberated Peep as the WAV and FLAC decoder: every format it takes must come out as the same samples the dr_wav / dr_flac decoders give, the formats it does not take
// (float WAV, more than two channels in plain WAV) must still go to the fallback decoders, and seeking, looping and a long file must work.
// Needs ffmpeg for the test files (made in /tmp/pc).
static int bad=0;
#define CHK(c,...) do{if(!(c)){bad++;printf("FAIL line %d: %s | ",__LINE__,#c);printf(__VA_ARGS__);printf("\n");}}while(0)
typedef struct{const char*name,*args;int want;}Fix;   // want: the decoder that must open it
static const Fix FIX[]={
  {"s16.wav","-c:a pcm_s16le",ST_PEEP},{"mono16.wav","-ac 1 -c:a pcm_s16le",ST_PEEP},{"u8.wav","-c:a pcm_u8",ST_PEEP},
  {"s24.wav","-c:a pcm_s24le",ST_PEEP},{"s32.wav","-c:a pcm_s32le",ST_PEEP},
  {"f32.wav","-af volume=3 -c:a pcm_f32le",ST_WAV},                         // float stays on dr_wav so peaks above 1.0 reach the limiter
  {"six16.wav","-ac 6 -c:a pcm_s16le",ST_WAV},                              // plain 16-bit WAV with 6 channels: the fold-down needs the fallback
  {"alaw.wav","-c:a pcm_alaw",ST_PEEP},{"mulaw.wav","-c:a pcm_mulaw",ST_PEEP},{"ima.wav","-c:a adpcm_ima_wav",ST_PEEP},{"ms.wav","-c:a adpcm_ms",ST_PEEP},
  {"s16.flac","-c:a flac",ST_PEEP},{"mono.flac","-ac 1 -c:a flac",ST_PEEP},{"s24.flac","-c:a flac -sample_fmt s32",ST_PEEP},{"six.flac","-ac 6 -c:a flac",ST_PEEP},
};
// Reference: the same file through dr_wav / dr_flac as float, then the same stereo fold-down as st_read.
static float*ref_decode(const char*path,int*nf,int*rate){
  int ch=0;float*raw=NULL;drwav w;drflac*f;*nf=0;unsigned long long n=0;
  if(strstr(path,".flac")){if(!(f=drflac_open_file(path,NULL)))return NULL;ch=f->channels;*rate=f->sampleRate;n=f->totalPCMFrameCount;raw=malloc(n*ch*4);n=drflac_read_pcm_frames_f32(f,n,raw);drflac_close(f);}
  else{if(!drwav_init_file(&w,path,NULL))return NULL;ch=w.channels;*rate=w.sampleRate;n=w.totalPCMFrameCount;raw=malloc(n*ch*4);n=drwav_read_pcm_frames_f32(&w,n,raw);drwav_uninit(&w);}
  float*o=malloc((n+1)*8);unsigned long long i;
  for(i=0;i<n;i++){const float*q=raw+i*ch;float l,r;
    if(ch==1)l=r=q[0];else if(ch==2){l=q[0];r=q[1];}else{float c=ch>2?q[2]:0,bl=ch>4?q[4]:0,br=ch>5?q[5]:bl;l=(q[0]+.7071f*c+.7071f*bl)*.5858f;r=(q[1]+.7071f*c+.7071f*br)*.5858f;}
    if(!(l>-64&&l<64))l=0;if(!(r>-64&&r<64))r=0;o[2*i]=l;o[2*i+1]=r;}
  free(raw);*nf=(int)n;return o;
}
static long rss_kb(void){FILE*f=fopen("/proc/self/status","r");char l[200];long v=-1;if(!f)return -1;while(fgets(l,sizeof l,f))if(!strncmp(l,"VmRSS:",6)){v=atol(l+6);break;}fclose(f);return v;}
int main(void){
  system("mkdir -p /tmp/pc");
  { char cmd[900];int i;
    const char*ST2="aevalsrc=0.5*sin(2*PI*440*t)|0.4*sin(2*PI*660*t):d=2:s=44100",*SIX="aevalsrc=0.5*sin(2*PI*440*t)|0.4*sin(2*PI*660*t)|0.3*sin(2*PI*880*t)|0.2*sin(2*PI*110*t)|0.3*sin(2*PI*550*t)|0.25*sin(2*PI*770*t):c=5.1:d=2:s=44100";
    for(i=0;i<(int)(sizeof FIX/sizeof FIX[0]);i++){snprintf(cmd,sizeof cmd,"cd /tmp/pc; [ -f %s ] || ffmpeg -loglevel error -y -f lavfi -i '%s' %s %s",FIX[i].name,strstr(FIX[i].name,"six")?SIX:ST2,FIX[i].args,FIX[i].name);system(cmd);}
    system("cd /tmp/pc; [ -f s48.flac ] || ffmpeg -loglevel error -y -f lavfi -i 'aevalsrc=0.5*sin(2*PI*1000*t)|0.4*sin(2*PI*1500*t):d=2:s=48000' -c:a flac s48.flac;"
           "[ -f t.mp3 ] || ffmpeg -loglevel error -y -f lavfi -i 'sine=f=500:d=2' -c:a libmp3lame t.mp3;[ -f t.ogg ] || ffmpeg -loglevel error -y -f lavfi -i 'sine=f=500:d=2' -c:a libvorbis t.ogg;"
           "[ -f long.wav ] || ffmpeg -loglevel error -y -f lavfi -i 'aevalsrc=0.4*sin(2*PI*220*t)+0.3*sin(2*PI*331*t):d=330:s=44100' -ac 2 -c:a pcm_s16le long.wav");}
  // 1. every format: the right decoder opens it, and the samples match the reference decoders
  { int k;for(k=0;k<(int)(sizeof FIX/sizeof FIX[0]);k++){char p[200],err[120];snprintf(p,sizeof p,"/tmp/pc/%s",FIX[k].name);
      if(!st_open(p,err,sizeof err)){CHK(0,"%s: not opened (%s)",FIX[k].name,err);continue;}
      int kind=ST.kind;CHK(kind==FIX[k].want,"%s: decoder %d, wanted %d",FIX[k].name,kind,FIX[k].want);
      int rn,rate;float*ref=ref_decode(p,&rn,&rate);CHK(ref!=NULL,"%s: reference failed",FIX[k].name);if(!ref){st_close();continue;}
      static float out[(4096*2+64)*2];int total=0,o;double maxd=0;long mism=0;
      while((o=st_read(out,4096*2+64))>0){int i;for(i=0;i<o&&total+i<rn;i++){double d=fabs(out[2*i]-ref[2*(total+i)]),e=fabs(out[2*i+1]-ref[2*(total+i)+1]);if(d>maxd)maxd=d;if(e>maxd)maxd=e;if(d>1e-6||e>1e-6)mism++;}total+=o;}
      printf("%-10s %-5s %2d ch %6d Hz  frames %6d (ref %6d)  max diff %.2e  mismatches %ld\n",FIX[k].name,kind==ST_PEEP?"peep":"dr",ST.ch,rate,total,rn,maxd,mism);
      CHK(total==rn||(strstr(FIX[k].name,"ima")&&total<rn&&rn-total<1024),"%s: length %d, reference %d",FIX[k].name,total,rn);   /* ADPCM: Peep trims the last block to the length in the "fact" chunk, dr_wav plays the padding too */
      CHK(mism==0,"%s: %ld samples differ from the reference decoder (max %.2e)",FIX[k].name,mism,maxd);
      free(ref);st_close();}}
  // 2. a 48 kHz FLAC still goes through the resampler and opens with Peep
  {char err[120];CHK(st_open("/tmp/pc/s48.flac",err,sizeof err)&&ST.kind==ST_PEEP&&!ST.rs.pass,"48 kHz FLAC");static float o[(4096*2+64)*2];int n=st_read(o,4096*2+64);CHK(n>0,"48 kHz read");st_close();}
  // 3. rewinding (repeat one) gives the same samples again, for a codec (FLAC) and for the built-in WAV decoder
  { const char*fs[]={"/tmp/pc/s16.flac","/tmp/pc/s16.wav","/tmp/pc/ima.wav"};int k;
    for(k=0;k<3;k++){char err[120];static float a[(4096*2+64)*2],b[(4096*2+64)*2];CHK(st_open(fs[k],err,sizeof err),"open %s",fs[k]);
      int na=st_read(a,4096*2+64),n;while((n=st_read(b,4096*2+64))>0);st_rewind();int nb=st_read(b,4096*2+64);
      CHK(na==nb&&!memcmp(a,b,(size_t)na*8),"%s: after rewind the first block differs",fs[k]);st_close();}}
  // 4. files Peep cannot open are refused cleanly, a bad file is not played, an empty file is not mapped
  { char err[120];system("cd /tmp/pc; head -c 3000 /dev/urandom > junk.flac; head -c 3000 /dev/urandom > junk.wav; : > empty.wav; head -c 100 s16.flac > cut.flac");
    CHK(!st_open("/tmp/pc/junk.flac",err,sizeof err),"junk flac accepted");CHK(!st_open("/tmp/pc/junk.wav",err,sizeof err),"junk wav accepted");CHK(!st_open("/tmp/pc/empty.wav",err,sizeof err),"empty wav accepted");CHK(!st_open("/tmp/pc/nothere.wav",err,sizeof err),"missing wav accepted");
    if(st_open("/tmp/pc/cut.flac",err,sizeof err)){static float o[(4096*2+64)*2];int n,g=0;while((n=st_read(o,4096*2+64))>0&&g<100000)g+=n;st_close();}   /* a cut-off file must not crash */
    CHK(ST.kind==ST_NONE,"decoder left open");}
  // 5. mp3 and ogg are unchanged
  {char err[120];CHK(st_open("/tmp/pc/t.mp3",err,sizeof err)&&ST.kind==ST_MP3,"mp3 decoder");st_close();CHK(st_open("/tmp/pc/t.ogg",err,sizeof err)&&ST.kind==ST_OGG,"ogg decoder");st_close();}
  // 6. a long WAV (55 MB, past the 32 MB where played pages are handed back): same samples as dr_wav, and memory stays well below the file size
  { char err[120];static float o[(4096*2+64)*2];unsigned long long h1=1469598103934665603ULL,h2=h1;long n1=0,n2=0,peak=0;int n;
    CHK(st_open("/tmp/pc/long.wav",err,sizeof err)&&ST.kind==ST_PEEP,"long wav");size_t fsz=ST.mp.n;
    while((n=st_read(o,4096*2+64))>0){int i;for(i=0;i<2*n;i++){u32 b;memcpy(&b,&o[i],4);h1=(h1^b)*1099511628211ULL;}n1+=n;long r=rss_kb();if(r>peak)peak=r;}
    st_close();
    drwav w;CHK(drwav_init_file(&w,"/tmp/pc/long.wav",NULL),"dr_wav long");static float rb[4096*2];drwav_uint64 g;
    while((g=drwav_read_pcm_frames_f32(&w,4096,rb))>0){int i;for(i=0;i<(int)g;i++){u32 a,b;float l=rb[2*i],r=rb[2*i+1];memcpy(&a,&l,4);memcpy(&b,&r,4);h2=(h2^a)*1099511628211ULL;h2=(h2^b)*1099511628211ULL;}n2+=(long)g;}drwav_uninit(&w);
    printf("long.wav: %zu MB file, %ld frames each way, peak resident %ld MB while decoding, hashes %s\n",fsz>>20,n1,peak/1024,h1==h2?"equal":"DIFFER");
    CHK(n1==n2&&h1==h2,"long wav: stream differs from dr_wav (%ld vs %ld frames)",n1,n2);
    CHK(peak<(long)(fsz/1024)*6/10+30000,"long wav: resident memory %ld MB is not below the file size %zu MB",peak/1024,fsz>>20);}
  printf("peep_codec test bad=%d\n",bad);return bad;}
