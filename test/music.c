#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/mt/";} const char* plat_home_dir(void){return "/tmp/mt/";}
static int bad=0;
#define CHK(c,...) do{if(!(c)){bad++;printf("FAIL line %d: %s | ",__LINE__,#c);printf(__VA_ARGS__);printf("\n");}}while(0)
static float peakOf(int frames){static float o[2048];float pk=0;int i,done=0;while(done<frames){app_audio(o,1024);for(i=0;i<2048;i++)if(fabsf(o[i])>pk)pk=fabsf(o[i]);done+=1024;}return pk;}
int main(void){
 system("mkdir -p /tmp/8tris-mp");u32*fb=calloc(1000*720,4);app_init();app_resize(1000,720);double t=1000;
 snd_vol=0;musVol=100;music_scan();printf("files:%d\n",nmfiles);int k;for(k=0;k<nmfiles;k++)printf("  %s\n",mfiles[k]);
 for(k=0;k<nmfiles;k++){
   musMode=1;musSel=k;snprintf(musFile,sizeof musFile,"%s",mfiles[k]);t+=16;app_frame(t,fb);t+=16;app_frame(t,fb);
   int good=!strstr(mfiles[k],"bad")&&!strstr(mfiles[k],"empty");
   float pk=peakOf(4096);
   printf("%-12s status='%s' len=%lld peak=%.3f\n",mfiles[k],mus_status,(long long)mus_len,pk);
   if(good){CHK(mus_len>130000&&mus_len<134500,"len %lld",(long long)mus_len);CHK(pk>.04f&&pk<.95f,"peak %f",pk);}
   else CHK(mus_buf==NULL&&pk==0,"bad file played");
 }
 // all-in-order skips bad files and moves on when a track ends
 musMode=2;musSel=0;music_stop();mus_prevMode=0;int seen[16]={0},n=0;
 for(k=0;k<40;k++){t+=16;app_frame(t,fb);if(mus_buf&&musCur>=0&&!seen[musCur]){seen[musCur]=1;n++;}mus_pos=mus_len>1000?mus_len-300:mus_pos;peakOf(2048);}
 printf("order mode played %d distinct tracks\n",n);CHK(n>=5,"n=%d",n);
 musMode=3;for(k=0;k<30;k++){t+=16;app_frame(t,fb);if(mus_buf)mus_pos=mus_len-300;peakOf(2048);}
 musMode=0;t+=16;app_frame(t,fb);CHK(mus_buf==NULL,"off should stop");
 printf("music test bad=%d\n",bad);return bad;}
