#include "../src/common.h"
void plat_lock(void){} void plat_unlock(void){}
#include "../src/audio.c"
int main(void){float b[2048];snd_vol=1;snd_voice=3;sfx(S_MOVE,0,0);audio_mix(b,1024);int i,nan=0,nz=0;for(i=0;i<2048;i++){if(b[i]!=b[i])nan++;else if(b[i]!=0)nz++;}printf("nan %d nonzero %d, rv_scale %f pitch %f\n",nan,nz,rv_scale,VOICES[3].pitch);
 for(i=0;i<N_(VOICES);i++)printf("%s len %.2f det %.1f wet %.2f gain %.2f root %.2f nsc %d\n",VOICES[i].name,VOICES[i].len,VOICES[i].det,VOICES[i].wet,VOICES[i].gain,VOICES[i].root,VOICES[i].nscale);return 0;}
