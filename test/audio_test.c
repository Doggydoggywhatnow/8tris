#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;}
int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-at/";} const char* plat_home_dir(void){return "/tmp/8tris-at/";}
// Sound voices: every voice must make every sound, stay finite and under full scale, and sit at a similar loudness to the 8tris voice.
//   ./audio_test        summary and checks
//   ./audio_test wav    also writes one clear sound per voice to /tmp/8tris-at/voice-N.wav
static float buf[2*1024];
static void flush(void){int i;memset(VX,0,sizeof VX);for(i=0;i<200;i++)app_audio(buf,1024);}   // let reverb tails die
static void render(double secs,double*pk,double*rms){long n=0;double p=0,q=0;int k,i,blocks=(int)(secs*44100/1024);
  for(k=0;k<blocks;k++){app_audio(buf,1024);for(i=0;i<2*1024;i++){double x=buf[i];if(x!=x||fabs(x)>1e6){*pk=1e9;*rms=0;return;}if(fabs(x)>p)p=fabs(x);q+=x*x;n++;}}
  *pk=p;*rms=sqrt(q/(n?n:1));}
static double db(double x){return 20*log10(x+1e-9);}
int main(int argc,char**argv){
  system("mkdir -p /tmp/8tris-at");store_off=1;snd_vol=1;int bad=0,v;double base_rms=0;
  printf("%-10s %14s %14s %14s %14s\n","voice","move","clear2","tetris","hard drop  (peak/rms dBFS)");
  for(v=0;v<NVOICES;v++){
    snd_voice=v;const int kinds[4]={S_MOVE,S_CLEAR,S_CLEAR,S_HARD},ns[4]={0,2,4,0};double rmsum=0;int k;
    printf("%-10s",VOICES[v].name);
    for(k=0;k<4;k++){double pk,rms;flush();sfx(kinds[k],ns[k],0);render(1.2,&pk,&rms);printf(" %6.1f/%6.1f",db(pk),db(rms));rmsum+=rms;
      if(!(pk>1e-4)||pk>1.0){bad++;printf("  FAIL");}}
    printf("\n");if(v==0)base_rms=rmsum;
    else if(db(rmsum)-db(base_rms)>5||db(rmsum)-db(base_rms)<-8){bad++;printf("  FAIL: %s is %.1f dB from the 8tris voice\n",VOICES[v].name,db(rmsum)-db(base_rms));}
    // combo fanfares and the perfect clear at the top effect level
    {int ev,lv,c;double mx=0;for(ev=0;ev<2;ev++)for(lv=1;lv<=4;lv+=1)for(c=2;c<=14;c+=6){double pk,rms;flush();fx_sound(ev,lv,ev?1:c);render(4,&pk,&rms);if(pk>mx)mx=pk;if(!(pk>1e-4)||pk>1.0){bad++;printf("  FAIL fanfare ev%d lv%d c%d peak %.3f\n",ev,lv,c,pk);}}
     printf("           fanfares max peak %.1f dBFS\n",db(mx));}
    if(argc>1){char p[80];snprintf(p,sizeof p,"/tmp/8tris-at/voice-%d.wav",v);FILE*f=fopen(p,"wb");flush();sfx(S_CLEAR,4,0);fx_sound(0,2,8);
      fwrite("RIFF\0\0\0\0WAVEfmt ",1,16,f);unsigned h[4]={16,0x20001,44100,44100*4};fwrite(&h[0],4,1,f);fwrite(&h[1],4,1,f);fwrite(&h[2],4,1,f);fwrite(&h[3],4,1,f);unsigned short a=4,b=16;fwrite(&a,2,1,f);fwrite(&b,2,1,f);fwrite("data\0\0\0\0",1,8,f);
      int k2;for(k2=0;k2<200;k2++){app_audio(buf,1024);int i;for(i=0;i<2048;i++){int s=(int)(buf[i]*32767);if(s>32767)s=32767;if(s<-32768)s=-32768;short w=(short)s;fwrite(&w,2,1,f);}}
      long sz=ftell(f);fseek(f,4,SEEK_SET);unsigned u=(unsigned)(sz-8);fwrite(&u,4,1,f);fseek(f,40,SEEK_SET);u=(unsigned)(sz-44);fwrite(&u,4,1,f);fclose(f);}
  }
  printf("audio test %s (%d voices)\n",bad?"FAILED":"ok",NVOICES);return bad;}
