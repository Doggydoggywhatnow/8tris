#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/mt/";} const char* plat_home_dir(void){return "/tmp/mt/";}
// Music player: every format plays at the right length and level, bad files are refused, and the output is clean:
// no resampling artifacts, no clipping when music and effects add up, no stale samples for odd buffer sizes.
static int bad=0;
#define CHK(c,...) do{if(!(c)){bad++;printf("FAIL line %d: %s | ",__LINE__,#c);printf(__VA_ARGS__);printf("\n");}}while(0)
static u32*fb;static double t=1000;
static void frame(void){t+=16;app_frame(t,fb);}
static float peakOf(int frames){static float o[2048];float pk=0;int i,done=0;while(done<frames){app_audio(o,1024);for(i=0;i<2048;i++)if(fabsf(o[i])>pk)pk=fabsf(o[i]);done+=1024;frame();}return pk;}
static int pick_file(const char*n){int k;for(k=0;k<nmfiles;k++)if(!strcmp(mfiles[k],n))return k;return -1;}
static void play_one(const char*n){musMode=1;musSel=pick_file(n);snprintf(musFile,sizeof musFile,"%s",n);frame();frame();}
static i64 file_len(const char*n){char p[800],e[120];snprintf(p,sizeof p,"%s/%s",music_dir(),n);if(!st_open(p,e,sizeof e))return -1;static float b[(4096*2+64)*2];i64 L=0;int o;while((o=st_read(b,4096*2+64))>0)L+=o;st_close();return L;}
// residual after removing the best-fitting sine at f: how far below the tone everything else is, in dB
static double resid_db(const float*x,int n,double f){double sc=0,ss=0,cc=0,cs=0,s2=0;int i;
  for(i=0;i<n;i++){double a=2*M_PI*f*i/44100;double c=cos(a),s=sin(a);sc+=x[i]*c;ss+=x[i]*s;cc+=c*c;cs+=s*s;}
  double A=sc/cc,B=ss/cs,e=0,p=0;for(i=0;i<n;i++){double a=2*M_PI*f*i/44100,y=A*cos(a)+B*sin(a);e+=(x[i]-y)*(x[i]-y);p+=y*y;}(void)s2;return 10*log10(e/p+1e-30);}
int main(void){
  system("mkdir -p /tmp/mt/music; cd /tmp/mt/music;"
   "[ -f s48.wav ] || ffmpeg -loglevel error -y -f lavfi -i 'sine=f=10000:r=48000:d=2' -af volume=0dB -c:a pcm_f32le s48.wav;"
   "rm -f loud.wav;[ -f loud.wav ] || ffmpeg -loglevel error -y -f lavfi -i 'sine=f=200:r=44100:d=2' -af volume=10 -c:a pcm_f32le loud.wav;"
   "[ -f up22.wav ] || ffmpeg -loglevel error -y -f lavfi -i 'sine=f=3000:r=22050:d=2' -c:a pcm_s16le up22.wav;"
   "[ -f surround.flac ] || ffmpeg -loglevel error -y -f lavfi -i 'sine=f=440:r=44100:d=2' -af 'pan=5.1|FC=c0' -c:a flac surround.flac");
  fb=calloc(1000*720,4);app_init();app_resize(1000,720);
  snd_vol=0;musVol=100;music_scan();printf("files:%d\n",nmfiles);int k;
  // 1. each test file: right length (3 s), sensible level; broken ones refused and silent
  for(k=0;k<nmfiles;k++){const char*n=mfiles[k];if(strncmp(n,"a.",2)&&strncmp(n,"b.",2)&&strncmp(n,"c.",2)&&strncmp(n,"d.",2)&&strncmp(n,"e ",2)&&strncmp(n,"bad",3)&&strncmp(n,"empty",5))continue;
    int good=!strstr(n,"bad")&&!strstr(n,"empty");play_one(n);{static float z[2048];app_audio(z,1024);}float pk=peakOf(4096);   /* first block: the limiter's few-sample delay may still hold the end of the last track */i64 L=good?file_len(n):0;
    printf("%-12s status='%s' len=%lld peak=%.3f\n",n,mus_status,(long long)L,pk);
    if(good){CHK(mus_have,"%s",n);CHK(L>130000&&L<134500,"len %lld",(long long)L);CHK(pk>.04f&&pk<.95f,"peak %f",pk);}
    else CHK(!mus_have&&pk==0,"bad file played");}
  // 2. 48 kHz -> 44.1 kHz on a 10 kHz tone: linear interpolation left products around -25 dB; the sinc resampler must be far below
  {char p[800],e[120];snprintf(p,sizeof p,"%s/s48.wav",music_dir());CHK(st_open(p,e,sizeof e),"%s",e);static float b[(4096*2+64)*2],x[60000];int n=0,o;
   while(n<60000&&(o=st_read(b,4096*2+64))>0){int i;for(i=0;i<o&&n<60000;i++)x[n++]=b[2*i];}st_close();
   double r=resid_db(x+4000,40000,10000);printf("48k->44.1k 10 kHz tone: everything else at %.1f dB\n",r);CHK(r<-70,"%.1f dB",r);
   double pk=0;int i;for(i=4000;i<44000;i++)if(fabs(x[i])>pk)pk=fabs(x[i]);CHK(pk>.122&&pk<.128,"level %f",pk);}   // ffmpeg's test tone is 1/8 scale: no level loss in the passband
  // 3. upsampling (22.05 kHz) and 5.1 fold-down also come out clean
  {char p[800],e[120];snprintf(p,sizeof p,"%s/up22.wav",music_dir());CHK(st_open(p,e,sizeof e),"%s",e);static float b[(4096*2+64)*2],x[60000];int n=0,o;
   while(n<60000&&(o=st_read(b,4096*2+64))>0){int i;for(i=0;i<o&&n<60000;i++)x[n++]=b[2*i];}st_close();double r=resid_db(x+4000,40000,3000);printf("22.05k->44.1k: %.1f dB\n",r);CHK(r<-70,"%.1f",r);
   CHK(file_len("up22.wav")>88000&&file_len("up22.wav")<88400,"len");}
  {play_one("surround.flac");float pk=peakOf(8192);printf("5.1 centre-only file peak %.3f\n",pk);CHK(pk>.035f,"centre channel lost: %f",pk);   /* 1/8-scale tone folded in at -4.6 dB, times the 0.8 music gain */}
  // 4. a track that goes past full scale (1.25, like a loud master's inter-sample peaks), at full music volume, plus loud effects: the output never hard-clips
  {play_one("loud.wav");snd_vol=1;static float o[2048];int i,j,clip=0;float pk=0;
   for(j=0;j<60;j++){if(j%3==0)sfx(S_HARD,0,t);sfx(S_CLEAR,4,t);app_audio(o,1024);for(i=0;i<2048;i++){if(fabsf(o[i])>pk)pk=fabsf(o[i]);if(fabsf(o[i])>=.9999f)clip++;}frame();}
   printf("loud music + effects: peak %.3f, samples at the rail %d\n",pk,clip);CHK(clip==0&&pk<.93f,"peak %f clip %d",pk,clip);snd_vol=0;}
  // 5. any buffer size the system asks for is filled completely (it used to stop at 8192 frames)
  {play_one("d.wav");static float o[20000*2];int i,nan_=0;for(i=0;i<40000;i++)o[i]=NAN;app_audio(o,20000);for(i=0;i<40000;i++)if(o[i]!=o[i])nan_++;CHK(nan_==0,"%d unwritten",nan_);}
  // 6. all-in-order skips bad files and moves on when a track ends; shuffle keeps going; off stops
  musMode=2;music_stop();mus_prevMode=0;{int seen[64]={0},n=0;
   for(k=0;k<400&&n<9;k++){frame();if(mus_have&&musCur>=0&&!seen[musCur]){seen[musCur]=1;n++;}peakOf(16384);}
   printf("order mode played %d distinct tracks\n",n);CHK(n>=9,"n=%d",n);}
  musMode=3;for(k=0;k<60;k++)peakOf(16384);CHK(mus_have||mus_job,"shuffle stalled: %s",mus_status);
  musMode=0;frame();CHK(!mus_have&&!mr_live,"off should stop");
  // 7. tags are found in every format while only the tag area is read (ID3v2, the old ID3v1 at the end, FLAC, Ogg and WAV INFO)
  system("mkdir -p /tmp/mt/tags; cd /tmp/mt/tags; S='sine=f=300:d=1';"
   "ffmpeg -loglevel error -y -f lavfi -i $S -metadata title=T2 -metadata artist=A2 -metadata album=B2 v2.mp3;"
   "ffmpeg -loglevel error -y -f lavfi -i $S -id3v2_version 0 -map_metadata -1 v1.mp3;"
   "python3 -c \"import sys;f=open('v1.mp3','ab');f.write(b'TAG'+b'T1'.ljust(30,b'\\0')+b'A1'.ljust(30,b'\\0')+b'\\0'*30+b'2026'+b'\\0'*30+b'\\xff')\";"
   "ffmpeg -loglevel error -y -f lavfi -i $S -metadata title=TF -metadata artist=AF f.flac;"
   "ffmpeg -loglevel error -y -f lavfi -i $S -metadata title=TO -metadata artist=AO o.ogg;"
   "ffmpeg -loglevel error -y -f lavfi -i $S -metadata title=TW -metadata artist=AW w.wav");
  {const char*F[5][3]={{"v2.mp3","T2","A2"},{"v1.mp3","T1","A1"},{"f.flac","TF","AF"},{"o.ogg","TO","AO"},{"w.wav","TW","AW"}};int i;
   for(i=0;i<5;i++){char p[200];snprintf(p,sizeof p,"/tmp/mt/tags/%s",F[i][0]);Meta m;meta_read(p,&m);printf("tags %-7s '%s' '%s'\n",F[i][0],m.title,m.artist);
     CHK(!strcmp(m.title,F[i][1])&&!strcmp(m.artist,F[i][2]),"%s: '%s' '%s'",F[i][0],m.title,m.artist);meta_free(&m);}
   Meta m;meta_read("/tmp/mt/music/d.wav",&m);CHK(!strcmp(m.title,"d"),"untagged: file name, got '%s'",m.title);meta_free(&m);}
  printf("music test bad=%d\n",bad);return bad;}
