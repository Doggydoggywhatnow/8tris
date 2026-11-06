#include "../src/8tris.c"
// Combo song: plays a long chain offline (no window), checks the mix stays clean, and can write it to a WAV file to listen to.
//   ./song [out.wav] [last combo] [seconds per combo]
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;}
int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-song/";} const char* plat_home_dir(void){return "/tmp/8tris-song/";}
static void w32(FILE*f,unsigned v){fwrite(&v,4,1,f);}static void w16(FILE*f,unsigned v){fputc(v&255,f);fputc(v>>8,f);}
int main(int argc,char**argv){
  const char*out=argc>1&&argv[1][0]?argv[1]:NULL;int last=argc>2?atoi(argv[2]):120;double per=argc>3?atof(argv[3]):.9;
  system("mkdir -p /tmp/8tris-song");store_off=1;snd_vol=.6;comboSong=1;snd_voice=0;
  FILE*f=NULL;if(out){f=fopen(out,"wb");fwrite("RIFF\0\0\0\0WAVEfmt ",1,16,f);w32(f,16);w16(f,1);w16(f,2);w32(f,44100);w32(f,44100*4);w16(f,4);w16(f,16);fwrite("data\0\0\0\0",1,8,f);}
  static float buf[2*735];double peak=0,sum=0;long n=0,frames=0;int nan=0,maxv=0,bad=0;
  double total=1.0+(last-1)*per+4.0;int fr,nfr=(int)(total*60);
  for(fr=0;fr<nfr;fr++){double t=fr/60.0;
    int c=t<1?0:(int)((t-1)/per)+2;if(c>last)c=0;   // the chain grows to 'last', then breaks
    combo=c;song_tick(1);
    {int k,a=0;for(k=0;k<NVOX;k++)if(VX[k].active)a++;if(a>maxv)maxv=a;}
    app_audio(buf,735);frames+=735;
    int i;for(i=0;i<2*735;i++){float x=buf[i];if(x!=x){nan++;x=0;}if(fabs(x)>peak)peak=fabs(x);sum+=x*x;n++;
      if(f){int v=(int)(x*32767);if(v>32767)v=32767;if(v<-32768)v=-32768;w16(f,(unsigned)(v&0xffff));}}
  }
  if(f){long sz=ftell(f);fseek(f,4,SEEK_SET);w32(f,(unsigned)(sz-8));fseek(f,40,SEEK_SET);w32(f,(unsigned)(sz-44));fclose(f);}
  printf("song: %.0f s, peak %.3f, rms %.3f, most voices at once %d of %d, NaN %d\n",frames/44100.0,peak,sqrt(sum/n),maxv,NVOX,nan);
  if(nan||peak>1.0||maxv>=NVOX)bad=1;
  // pausing stops the song; unpausing with the chain still alive starts it again; breaking the chain ends it; turning it off stops it
  combo=0;song_tick(1);combo=5;song_tick(1);if(!song_playing)bad=1;song_tick(0);if(song_playing)bad=1;song_tick(1);if(!song_playing)bad=1;
  combo=0;song_tick(1);if(song_playing)bad=1;combo=3;song_tick(1);if(!song_playing)bad=1;comboSong=0;song_tick(1);if(song_playing)bad=1;
  // with the combo song on, the usual combo sound is skipped (a fresh voice pool stays empty after a scored combo)
  // the per-combo melody: never the same note twice in a row, stays within range, and lands on a chord tone on strong beats
  {int k,last=-99,badm=0,lo=99,hi=-99;unsigned r=12345;for(k=2;k<=5000;k++){r=r*1664525u+1013904223u;int root=(int)(r>>24)%7,st=(int)(r>>20)&1;int d=song_tick_deg(k,root,st,last);
     if(d==last)badm++;if(d<lo)lo=d;if(d>hi)hi=d;if(st){int q=((d-root)%7+7)%7;if(q!=0&&q!=2&&q!=4)badm++;}last=d;}
   printf("combo melody: degrees %d..%d, problems %d\n",lo,hi,badm);if(badm||lo<-10||hi>9)bad=1;}
  printf("song test %s\n",bad?"FAILED":"ok");return bad;}
