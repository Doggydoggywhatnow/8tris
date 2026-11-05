#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-rt/";} const char* plat_home_dir(void){return "/tmp/8tris-rt/";}
#define EQ(a) do{if((a)!=(b_##a)){printf("MISMATCH %s: %g vs %g\n",#a,(double)(a),(double)(b_##a));bad++;}}while(0)
int main(void){int bad=0,i;system("rm -rf /tmp/8tris-rt;mkdir -p /tmp/8tris-rt");u32*fb=calloc(1000*720,4);app_init();app_resize(1000,720);
 W=13;H=21;slots=3;fallV=47.5;lvlSys=0;clearOn=0;sdRate=77;snd_vol=.33;themeOn=1;calm=1;fxMaster=2;solidP=0xff123456;solidG=0xffabcdef;solidSep=1;
 uiFam=3;uiBold=1;uiItal=1;uiPt=19;hiCon=1;mwSel=2;lsSel=1;ghostStr=55;for(i=0;i<4;i++)ADV[i]=i-1;resSet=3;fpsSet=5;showFps=1;
 for(i=1;i<=11;i++)weights[i]=i%4;for(i=0;i<7;i++)tw[i]=i+1;TC[C_PIECES]=TH_SOLID;TC[C_PAGE]=TH_GRAPHITE;binds[2][0]=1234;binds[3][1]=-7;
 int b_W=W,b_H=H,b_slots=slots,b_sdRate=sdRate,b_themeOn=themeOn,b_calm=calm,b_fxMaster=fxMaster,b_uiFam=uiFam,b_uiBold=uiBold,b_uiItal=uiItal,b_uiPt=uiPt,b_hiCon=hiCon,b_mwSel=mwSel,b_lsSel=lsSel,b_ghostStr=ghostStr,b_resSet=resSet,b_fpsSet=fpsSet,b_showFps=showFps,b_lvlSys=lvlSys,b_clearOn=clearOn,b_solidSep=solidSep;
 double b_fallV=fallV,b_snd_vol=snd_vol;u32 b_solidP=solidP,b_solidG=solidG;int b_w[12],b_tw[7],b_TC[N_COMPS],b_ADV[4],b_bind[N_ACTS][2];
 memcpy(b_w,weights,sizeof b_w);memcpy(b_tw,tw,sizeof b_tw);memcpy(b_TC,TC,sizeof b_TC);memcpy(b_ADV,ADV,sizeof b_ADV);memcpy(b_bind,binds,sizeof b_bind);
 save_settings();
 W=5;H=5;slots=1;fallV=1;lvlSys=1;clearOn=1;sdRate=5;snd_vol=1;themeOn=0;calm=0;fxMaster=0;solidP=solidG=0;solidSep=0;uiFam=0;uiBold=uiItal=0;uiPt=14;hiCon=0;mwSel=lsSel=0;ghostStr=100;memset(ADV,0,sizeof ADV);resSet=fpsSet=showFps=0;memset(weights,0,sizeof weights);memset(tw,0,sizeof tw);memset(TC,0,sizeof TC);memset(binds,0,sizeof binds);
 load_settings();
 EQ(W);EQ(H);EQ(slots);EQ(sdRate);EQ(themeOn);EQ(calm);EQ(fxMaster);EQ(uiFam);EQ(uiBold);EQ(uiItal);EQ(uiPt);EQ(hiCon);EQ(mwSel);EQ(lsSel);EQ(ghostStr);EQ(resSet);EQ(fpsSet);EQ(showFps);EQ(lvlSys);EQ(clearOn);EQ(solidSep);EQ(fallV);EQ(solidP);EQ(solidG);
 if(fabs(snd_vol-b_snd_vol)>.011){printf("vol\n");bad++;}
 for(i=1;i<=11;i++)if(weights[i]!=b_w[i]){printf("weights %d\n",i);bad++;}
 for(i=0;i<7;i++)if(tw[i]!=b_tw[i]){printf("tw %d\n",i);bad++;}
 for(i=0;i<N_COMPS;i++)if(TC[i]!=b_TC[i]){printf("TC %d %d vs %d\n",i,TC[i],b_TC[i]);bad++;}
 for(i=0;i<4;i++)if(ADV[i]!=b_ADV[i]){printf("ADV %d\n",i);bad++;}
 if(memcmp(binds,b_bind,sizeof binds)){printf("binds\n");bad++;}
 printf("roundtrip bad=%d\n",bad);return bad;}
