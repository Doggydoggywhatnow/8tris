#include "../src/common.h"
void plat_lock(void){} void plat_unlock(void){}
#include "../src/audio.c"
static void music_mix(float*o,int n){(void)o;(void)n;}   /* music is tested in music.c */
static float buf[2*SR*6];
static void reset(void){memset(VX,0,sizeof VX);samples_done=0;memset(lim_dly,0,sizeof lim_dly);for(int i=0;i<64;i++)lim_need[i]=1;lim_gain=1;comp_env=0;
 for(int i=0;i<8;i++){if(cb[i])memset(cb[i],0,CT[i]*4);cs[i]=0;}for(int i=0;i<4;i++)if(ab[i])memset(ab[i],0,AT[i]*4);}
static void render(double*pk,double*rms,double*dur){
  int total=SR*6,done=0;while(done<total){int n=imin(1024,total-done);audio_mix(buf+2*done,n);done+=n;}
  double p=0,best=0;int i,last=0;for(i=0;i<total;i++){double v=fabs(buf[2*i]);if(v>p)p=v;if(v>1e-4)last=i;}
  int W=13230;for(int s=0;s+W<total;s+=2205){double q=0;for(i=s;i<s+W;i++)q+=(double)buf[2*i]*buf[2*i];if(q>best)best=q;}
  *pk=p;*rms=sqrt(best/W);*dur=last/(double)SR;
}
static double db(double x){return 20*log10(x+1e-9);}
int main(int argc,char**argv){
  snd_vol=1;double pk,rms,dur;
  printf("base sounds (peak/rms dBFS)\n");
  for(int v=0;v<NVOICES;v++){snd_voice=v;
    int kinds[4]={S_MOVE,S_CLEAR,S_CLEAR,S_HARD},ns[4]={0,2,4,0};const char*nm[4]={"move","clear2","tetris","hard"};
    printf("%-18s",VOICES[v].name);
    for(int k=0;k<4;k++){reset();sfx(kinds[k],ns[k],0);render(&pk,&rms,&dur);printf(" %s %6.1f/%6.1f",nm[k],db(pk),db(rms));}
    printf("\n");}
  printf("fanfare: voice ev level c  peak rms dur\n");double maxpk=-99;
  for(int v=0;v<NVOICES;v++)for(int ev=0;ev<2;ev++)for(int lv=0;lv<=4;lv++){int cs[3]={2,6,14};for(int ci=0;ci<(ev==0?3:1);ci++){
    int c=ev==0?cs[ci]:1;snd_voice=v;reset();sfx(S_CLEAR,2,0);fx_sound(ev,lv,c);render(&pk,&rms,&dur);
    if(db(pk)>maxpk)maxpk=db(pk);
    if(argc>1||(c==6||ev==1))printf("%d %s L%d c%-2d  %6.1f %6.1f %.2f\n",v,ev?"perfect":"combo ",lv,c,db(pk),db(rms),dur);}}
  printf("max peak %.1f dBFS\n",maxpk);
}
