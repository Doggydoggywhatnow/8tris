#include "../src/8tris.c"
#include <pthread.h>
#include <sys/resource.h>
// Real threads: the decoder runs on its own thread like in the game. Checks a gapless, sample-exact stream (no drops, no repeats),
// pause, and a three-hour audiobook that opens at once and plays with flat memory.
static pthread_mutex_t mu=PTHREAD_MUTEX_INITIALIZER;
void plat_lock(void){pthread_mutex_lock(&mu);} void plat_unlock(void){pthread_mutex_unlock(&mu);} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;}
static void*tr(void*p){void**q=(void**)p;((void(*)(void*))q[0])(q[1]);return 0;}
int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker)return 0;static void*p[2];p[0]=(void*)fn;p[1]=a;pthread_t t;return pthread_create(&t,0,tr,p)==0;}
void plat_sleep(int ms){usleep(ms*1000);}
const char* plat_pref_dir(void){return "/tmp/ml/";} const char* plat_home_dir(void){return "/tmp/ml/";}
static int bad=0;
#define CHK(c,...) do{if(!(c)){bad++;printf("FAIL line %d: %s | ",__LINE__,#c);printf(__VA_ARGS__);printf("\n");}}while(0)
static long rss_kb(void){FILE*f=fopen("/proc/self/statm","r");long a=0,b=0;if(f){if(fscanf(f,"%ld %ld",&a,&b)!=2)b=0;fclose(f);}return b*4;}
static void wait_ok(void){int k;for(k=0;k<500&&!mus_have&&!mus_failed;k++){music_tick();usleep(2000);}}
int main(void){
  system("mkdir -p /tmp/ml/music; cd /tmp/ml/music;"
    "[ -f sine.wav ] || ffmpeg -loglevel error -y -f lavfi -i 'sine=f=441:r=44100:d=20' -c:a pcm_f32le sine.wav;"
    "[ -f book.mp3 ] || ffmpeg -loglevel error -y -f lavfi -i 'sine=f=300:r=16000:d=10800' -ac 1 -b:a 16k book.mp3");
  musVol=100;music_scan();CHK(nmfiles==2,"%d",nmfiles);
  // 1. 20 s of a 441 Hz sine (ffmpeg makes it 1/8 scale, times the 0.8 music gain), pulled in real-time-sized blocks while the decoder thread fills behind: must match the formula exactly
  musMode=2;musFold[0]=0;{int k;for(k=0;k<nmfiles;k++)if(!strcmp(mfiles[k],"sine.wav"))musSel=k;}
  musCur=-1;mus_prevMode=2;mus_prevSel=musSel;snprintf(mus_prevFold,sizeof mus_prevFold,"%s",musFold);music_start(musSel);wait_ok();CHK(mus_have,"%s",mus_status);
  {static float o[1024*2];i64 n=0;double err=0;int waits=0,i;
   while(n<44100*19){memset(o,0,sizeof o);i64 before=mus_played;music_mix(o,1024);int got=(int)(mus_played-before);if(got<1024)waits++;
     for(i=0;i<got;i++){double want=.1*sin(2*M_PI*441.0*(double)(n+i)/44100);double e=fabs(o[2*i]-want);if(e>err){if(e>1e-3&&err<=1e-3)printf("  first error at frame %lld\n",(long long)(n+i));err=e;}}n+=got;if(got==0)usleep(1000);if(waits>2000)break;}
   printf("threaded stream: %lld frames, max error %.2e, short reads %d\n",(long long)n,err,waits);CHK(err<1e-4,"%e",err);}
  // 2. pause holds the place exactly
  {static float o[2048];i64 p=mus_played;musPaused=1;int k;for(k=0;k<50;k++){memset(o,0,sizeof o);music_mix(o,1024);}CHK(mus_played==p&&o[0]==0,"moved while paused");musPaused=0;}
  // 3. a three-hour audiobook: opens quickly, plays, memory stays flat
  {int k,bk=-1;for(k=0;k<nmfiles;k++)if(!strcmp(mfiles[k],"book.mp3"))bk=k;long r0=rss_kb();struct timespec a,b;clock_gettime(CLOCK_MONOTONIC,&a);
   music_stop();musCur=-1;music_start(bk);wait_ok();clock_gettime(CLOCK_MONOTONIC,&b);double ms=(b.tv_sec-a.tv_sec)*1e3+(b.tv_nsec-a.tv_nsec)/1e6;
   CHK(mus_have,"%s",mus_status);printf("3-hour file: opened in %.0f ms\n",ms);CHK(ms<1500,"%.0f ms",ms);
   static float o[4096*2];i64 n=0;int i;float pk=0;struct timespec c;clock_gettime(CLOCK_MONOTONIC,&a);
   long rmid=0;for(;;){music_mix(memset(o,0,sizeof o),4096);if(!rmid&&mus_played>44100*60)rmid=rss_kb();for(i=0;i<8192;i++)if(fabsf(o[i])>pk)pk=fabsf(o[i]);n=mus_played;clock_gettime(CLOCK_MONOTONIC,&c);if((c.tv_sec-a.tv_sec)>4)break;usleep(500);}
   long r1=rss_kb();printf("played %.0f s of it in 4 s, peak %.2f, memory %ld KB before, %ld after a minute, %ld at the end\n",n/44100.0,pk,r0,rmid,r1);
   CHK(n>44100*30,"decoder too slow: %lld",(long long)n);CHK(pk>.05f,"silent");CHK(r1-rmid<1000,"memory keeps growing: %ld KB",r1-rmid);}
  printf("music long test bad=%d\n",bad);return bad;}
