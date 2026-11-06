// ===== SOUND =====
// Everything is synthesized in software (no audio files). Four small tables describe all of it:
//   VOICES  how things sound: wave shapes, pitch, reverb, scale. Each voice is one choice in Theming ("Sound voice").
//   UI      one-shot gameplay blips: {pitch Hz, seconds, wave role, gain, glide-to Hz}.
//   LAYERS  the building blocks of a combo or perfect clear sound. Each layer plays a few notes through the current voice.
//   LADDER  which layers each Effects level adds. Every level keeps the layers of the levels below it.
// To add a voice: add an entry to VOICES and map its theme to it in voice_of() in look.c.
// To add a layer: write a function like L_melody, give it a weight in SHARE, and list it in LADDER at the level where it should arrive.
// To add a sound event: add it to EV and call fx_sound(event, level, combo).
// Loudness is capped three ways: SHARE and BUDGET keep the total weight of the active layers bounded, the master volume sets
// the ceiling, and a hard limiter (LIMIT_DB) sits last in the chain.
#define SR 44100
enum{W_SINE,W_SQUARE,W_TRI,W_SAW,W_PULSE25,W_PULSE12};
enum{R_LEAD,R_SOFT,R_SMOOTH,R_ROUGH,R_BODY};

typedef struct{
  const char*name;int wave[5];                 // oscillator for each role (lead, soft, smooth, rough, body)
  double pitch,len,det,wet,gain,root;          // blip pitch multiplier, note length multiplier, detune cents, base reverb, loudness trim, fanfare root Hz
  int nscale;double scale[8];                  // semitones a combo climbs through
  double tick[4];                              // semitones of the plain clear blips
  int nchord;double chord[5];                  // chord ratios used by the stab and tail layers
  int narp;double arp[8];                      // semitones of the perfect clear arpeggio
  int nspark;double spark[8];                  // ratios used by the sparkle layer
  double bend;                                 // semitones every note scoops up from (0 = none); gives a voice a sliding, swirling character
}SVoice;
static const SVoice VOICES[]={
 {"8tris",{W_SQUARE,W_TRI,W_SINE,W_SAW,W_TRI},1,1,0,0,1,130.81,5,{0,2,4,7,9},{0,4,7,12},4,{1,1.5,2,2.52},6,{0,4,7,12,16,19},6,{1,1.26,1.5,2,2.52,3}},
 {"Classic chiptune",{W_PULSE25,W_TRI,W_TRI,W_SQUARE,W_PULSE12},1,.85,0,.03,.85,130.81,7,{0,2,4,5,7,9,11},{0,4,7,12},4,{1,1.25,1.5,2},7,{0,4,7,12,16,19,24},6,{1,1.25,1.5,2,2.5,3}},
 {"Wild",{W_SAW,W_TRI,W_SINE,W_SAW,W_SAW},1.26,1.3,14,.2,.7,130.81,7,{0,2,4,6,7,9,11},{0,4,7,11},4,{1,1.26,1.5,1.89},8,{0,4,7,11,14,18,19,23},8,{1,1.26,1.41,1.5,2,2.52,3,4}},
 // Swirl: glassy sine and triangle tones with a chorus, every note scooping upward, a dreamy whole-tone scale, augmented chords and lots of reverb.
 {"Swirl",{W_SINE,W_TRI,W_SINE,W_TRI,W_SINE},1.5,1.2,9,.4,.8,164.81,7,{0,2,4,6,8,10,12},{0,4,8,12},4,{1,1.26,1.587,2},7,{0,4,8,12,16,20,24},6,{1,1.26,1.587,2,2.52,3.17},3.5},
 // Light: bright and airy. Glassy triangle and sine tones, a little higher than 8tris, a major scale with an added ninth, short and clean.
 {"Light",{W_TRI,W_SINE,W_SINE,W_TRI,W_SINE},1.12,.9,3,.12,.95,196,7,{0,2,4,7,9,11,14},{0,4,7,14},5,{1,1.26,1.5,1.89,2.25},7,{0,4,7,11,14,16,19},6,{1,1.26,1.5,1.89,2.52,3}},
 // Graphite: dry and mechanical. Narrow pulse leads, tight and muted, no reverb, a minor pentatonic scale, a little lower than 8tris.
 {"Graphite",{W_PULSE25,W_TRI,W_TRI,W_SQUARE,W_SQUARE},.9,.75,0,0,.85,110,7,{0,3,5,7,10,12,15},{0,3,7,12},4,{1,1.2,1.5,2},7,{0,3,7,10,12,15,19},6,{1,1.2,1.5,2,2.4,3}},
 // Dark: deep and moody. Low sine and triangle tones with a saw underneath, long notes, slow detune and a darker minor scale with a flat second.
 {"Dark",{W_SINE,W_TRI,W_SINE,W_SAW,W_SINE},.75,1.45,6,.3,.9,98,7,{0,1,3,5,7,8,10},{0,3,7,10},4,{1,1.2,1.5,1.78},7,{0,3,7,10,12,15,19},6,{1,1.2,1.5,1.78,2,2.4}},
 // Chocolate: warm and mellow. Rounded triangle and sine tones, slightly low, a dorian scale and soft major-seventh chords with a gentle room.
 {"Chocolate",{W_TRI,W_SINE,W_TRI,W_TRI,W_SINE},.85,1.25,5,.22,1,123.47,7,{0,2,3,5,7,9,10},{0,4,7,11},4,{1,1.26,1.5,1.89},7,{0,4,7,11,14,16,19},6,{1,1.26,1.5,1.89,2.25,3}},
 // Milk: smooth and creamy. Only sines and triangles, long soft notes with a roomy tail, a lydian scale and rich major-ninth chords.
 {"Milk",{W_SINE,W_SINE,W_TRI,W_TRI,W_SINE},1,1.5,4,.35,1.05,174.61,7,{0,2,4,6,7,9,11},{0,4,7,11},5,{1,1.26,1.5,1.89,2.25},7,{0,4,7,11,14,18,23},6,{1,1.26,1.5,1.89,2.25,3}}};
#define NVOICES N_(VOICES)
#define TRIM_GAIN 2.0   // +6 dB to match the loudness of the web version
static int snd_voice=0;static double snd_vol=0.5;
static int comboSong=0;   // "Combo song" (Sound tab): combos play a building song (song.c) instead of the usual combo sounds
static const double LIMIT_DB=-4,BUDGET=6;

// ---- The synth engine: a pool of scheduled voices mixed into one mono bus, then reverb, compressor and limiter ----
typedef struct{
  int active,noise,wave,nosc,primed;i64 start;  // active: 0 free, 1 playing, 2 reserved (being set up by the game thread, not mixed yet)
  double duty,f0,f1,dur,gain,wet,det,ph[2];     // oscillator voices
  double fc,fr,env,ea,ed,dmul;int run,decay;    // running pitch and envelope, stepped once per sample instead of recomputed with pow()
  int bus;float pl,pr;                          // bus 1: the combo song's own stereo mix (pl, pr = left and right gains)
  int shape;                                    // 0: pluck (fast attack, decays over the note); 1: held (20 ms attack, steady, fades over the last 30%)
  double cut0,cut1,b[5],z1,z2;                    // noise voices: low-pass sweep and filter state
}Voice;
#define NVOX 1024
static Voice VX[NVOX];static i64 samples_done=0;

static void vox_wave(Voice*v,int w){v->wave=w;v->duty=w==W_PULSE25?.25:w==W_PULSE12?.12:.5;}
static Voice* vox_new(double delay,double dur){
  int i;plat_lock();
  for(i=0;i<NVOX;i++)if(!VX[i].active){Voice*v=&VX[i];memset(v,0,sizeof*v);v->active=2;v->start=samples_done+(i64)(delay*SR);v->dur=dur;plat_unlock();return v;}
  plat_unlock();return NULL;
}
// A voice that starts at an exact sample (used by the combo song, which schedules its notes on a beat grid).
static Voice* vox_at(i64 start,double dur){
  int i;plat_lock();
  for(i=0;i<NVOX;i++)if(!VX[i].active){Voice*v=&VX[i];memset(v,0,sizeof*v);v->active=2;v->start=start;v->dur=dur;plat_unlock();return v;}
  plat_unlock();return NULL;
}
static i64 audio_clock(void){i64 t;plat_lock();t=samples_done;plat_unlock();return t;}
// Hands a fully set-up voice to the audio thread. Until then it is skipped, so it is never mixed half-initialized.
static void vox_go(Voice*v){plat_lock();v->active=1;plat_unlock();}
// One oscillator note. role picks the wave from the current voice; to = glide target (0 for none); wet = reverb send.
static void tone(double f,double d,int role,double g,double to,double delay,double wet){
  const SVoice*sv=&VOICES[snd_voice];if(snd_vol<=0)return;
  double dur=d*sv->len;Voice*v=vox_new(delay,dur);if(!v)return;
  vox_wave(v,sv->wave[role]);v->f0=f;v->f1=to>0?to:f;
  if(sv->bend>0&&to<=0&&role!=R_BODY)v->f0=f*pow(2,-sv->bend/12.0);
  v->nosc=sv->det>0?2:1;v->det=sv->det;
  v->gain=dmax(.0002,g*(v->nosc>1?.65:1));v->wet=dmin(1,wet+sv->wet);
  if(!(v->f0>0)||!(v->f1>0))v->f0=v->f1=1;   // never a zero or invalid pitch (it would turn into NaN and silence everything)
  vox_go(v);
}
// Filtered noise burst: cut = starting low-pass cutoff in Hz, cut2 = cutoff it sweeps to (0 for none).
static void noise_burst(double d,double g,double cut,double delay,double cut2){
  if(snd_vol<=0)return;Voice*v=vox_new(delay,d);if(!v)return;
  v->noise=1;v->gain=g;v->cut0=cut>0?cut:1500;v->cut1=cut2>0?cut2:v->cut0;
  vox_go(v);
}
static double blep(double t,double dt){if(t<dt){t/=dt;return t+t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}return 0;}
static inline double osc_sample(int w,double duty,double t,double dt){
  double s;
  switch(w){
    case W_SINE:return sin(6.283185307179586*t);
    case W_TRI:return 4*fabs(t-.5)-1;
    case W_SAW:s=2*t-1;s-=blep(t,dt);return s;
    default:{double d=duty;s=t<d?1:-1;s+=blep(t,dt);double t2=t-d;if(t2<0)t2+=1;s-=blep(t2,dt);return s;}
  }
}
static void biquad_lp(double fc,double q,double*b){
  double w=6.283185307179586*dmin(fc,SR*.45)/SR,cw=cos(w),al=sin(w)/(2*q),a0=1+al;
  b[0]=(1-cw)/2/a0;b[1]=(1-cw)/a0;b[2]=b[0];b[3]=-2*cw/a0;b[4]=(1-al)/a0;
}
// Render one voice into the dry and reverb-send buses for samples [base, base+n).
// Song voices (bus 1) go to their own stereo buses sl/sr and reverb send ss; everything else to the mono effect buses.
static void vox_render(Voice*v,float*dry,float*snd,float*sl,float*sr,float*ss,i64 base,int n){
  int i;i64 end=v->start+(i64)(v->dur*SR);
  for(i=0;i<n;i++){
    i64 gi=base+i;if(gi<v->start)continue;if(gi>=end){v->active=0;return;}
    double t=(double)(gi-v->start)/SR,x;
    if(v->noise){
      if(!v->primed||((gi-v->start)&31)==0){biquad_lp(v->cut0*pow(v->cut1/v->cut0,t/v->dur),.8,v->b);v->primed=1;}
      double in=(rnd()*2-1)*(1-t/v->dur);
      x=v->b[0]*in+v->z1;v->z1=v->b[1]*in-v->b[3]*x+v->z2;v->z2=v->b[2]*in-v->b[4]*x;x*=v->gain;
    }else{
      // Exponential pitch glide and envelope (6 ms attack, then a decay to -80 dB over the note), advanced by a fixed ratio each sample.
      double env,f;int k;
      if(!v->run){v->run=1;v->fc=v->f0;v->fr=pow(v->f1/v->f0,1.0/dmax(v->dur*SR,1));v->env=1e-4;v->ea=pow(v->gain/1e-4,1.0/(.006*SR));
        v->ed=pow(1e-4/v->gain,1.0/(dmax(v->dur-.006,1e-4)*SR));v->dmul=pow(2,v->det/1200);}
      f=v->fc;v->fc*=v->fr;
      if(v->shape==1){double r=t>v->dur*.7?(v->dur-t)/(v->dur*.3):1;env=v->gain*dmin(1,t/.02)*dmax(r,0);}
      else if(t<.006){env=v->env;v->env*=v->ea;}
      else{if(!v->decay){v->decay=1;v->env=v->gain*pow(1e-4/v->gain,dmin((t-.006)/dmax(v->dur-.006,1e-4),1));}env=v->env;v->env=dmax(v->env*v->ed,1e-4);}
      x=0;for(k=0;k<v->nosc;k++){double ff=k?f*v->dmul:f,d2=ff/SR;x+=osc_sample(v->wave,v->duty,v->ph[k],d2);v->ph[k]+=d2;if(v->ph[k]>=1)v->ph[k]-=1;}
      x*=env;
    }
    if(v->bus){sl[i]+=(float)x*v->pl;sr[i]+=(float)x*v->pr;ss[i]+=(float)(x*v->wet);}else{dry[i]+=(float)x;snd[i]+=(float)(x*v->wet);}
  }
  if(base+n>=end)v->active=0;
}

// Freeverb-style reverb (8 combs + 4 all-passes), tuned for a tail of about 1.3 seconds. Sound effects and the combo song each have one.
static const int CT[8]={1116,1188,1277,1356,1422,1491,1557,1617},AT[4]={556,441,341,225};
typedef struct{float*cb[8],*ab[4];int cp[8],ap[4];float cs[8];}Rv;
static Rv rv_fx,rv_song;static int rv_ready=0;static double rv_scale=1;
static float reverb_tick(Rv*r,float in){
  float o=0;int i;float x=in*.015f;
  for(i=0;i<8;i++){float y=r->cb[i][r->cp[i]];r->cs[i]=y*.75f+r->cs[i]*.25f;r->cb[i][r->cp[i]]=x+r->cs[i]*.84f;if(++r->cp[i]>=CT[i])r->cp[i]=0;o+=y;}
  for(i=0;i<4;i++){float y=r->ab[i][r->ap[i]];float z=o-y;r->ab[i][r->ap[i]]=o+y*.5f;o=z;if(++r->ap[i]>=AT[i])r->ap[i]=0;}
  return o;
}
static void reverb_clear(Rv*r){int i;for(i=0;i<8;i++){memset(r->cb[i],0,CT[i]*4);r->cp[i]=0;r->cs[i]=0;}for(i=0;i<4;i++){memset(r->ab[i],0,AT[i]*4);r->ap[i]=0;}}
static void reverb_init(void){
  int i,k;if(rv_ready)return;
  for(k=0;k<2;k++){Rv*r=k?&rv_song:&rv_fx;for(i=0;i<8;i++)r->cb[i]=(float*)calloc(CT[i],4);for(i=0;i<4;i++)r->ab[i]=(float*)calloc(AT[i],4);}
  // Calibrate so the wet signal carries about 0.6 of the dry level (like the convolver reverb in the web version).
  double pin=0,pout=0;for(i=0;i<SR*2;i++){float x=(float)(rnd()*2-1);float y=reverb_tick(&rv_fx,x);pin+=x*x;pout+=y*y;}
  rv_scale=sqrt(.36*pin/pout);
  reverb_clear(&rv_fx);reverb_clear(&rv_song);rv_ready=1;
}
// Compressor (threshold -24 dB, ratio 12, soft knee) followed by a look-ahead limiter at LIMIT_DB.
static double comp_env=0,lim_gain=1;static float lim_dly[64];static int lim_pos=0;static float lim_need[64];
static float master_tick(float x){
  static double ca=0,cr=0,ceil_=0;if(ca==0){ca=exp(-1.0/(.003*SR));cr=exp(-1.0/(.25*SR));ceil_=pow(10,LIMIT_DB/20);}
  double a=fabs(x);
  comp_env=a>comp_env?ca*comp_env+(1-ca)*a:cr*comp_env+(1-cr)*a;
  double db=20*log10(comp_env+1e-9),th=-24,kn=30,over=db-th,out;
  if(2*over<-kn)out=db;else if(2*over>kn)out=th+over/12;else{double q=over+kn/2;out=db+(1/12.0-1)*q*q/(2*kn);}
  float y=(float)(x*pow(10,(out-db)/20));
  // look-ahead limiter
  lim_dly[lim_pos]=y;float need=fabsf(y)>ceil_?(float)(ceil_/fabsf(y)):1;lim_need[lim_pos]=need;
  float mn=1;int i;for(i=0;i<64;i++)if(lim_need[i]<mn)mn=lim_need[i];
  if(mn<lim_gain)lim_gain=mn;else lim_gain+=(1-lim_gain)*.002;
  int rp=(lim_pos+1)&63;float o=lim_dly[rp]*(float)lim_gain;lim_pos=(lim_pos+1)&63;return o;
}
// The combo song skips the effects compressor (which would flatten its build-up) and only meets this stereo look-ahead limiter.
#define SONG_GAIN 2.6
static float slim_gain=1,slim_dl[64],slim_dr[64],slim_need[64];static int slim_pos=0;static int song_bus_on=0;
static void song_limit(float*l,float*r){
  static float ceil_=0;if(ceil_==0)ceil_=(float)pow(10,-1.5/20);
  float a=fmaxf(fabsf(*l),fabsf(*r));slim_dl[slim_pos]=*l;slim_dr[slim_pos]=*r;slim_need[slim_pos]=a>ceil_?ceil_/a:1;
  float mn=1;int i;for(i=0;i<64;i++)if(slim_need[i]<mn)mn=slim_need[i];
  if(mn<slim_gain)slim_gain=mn;else slim_gain+=(1-slim_gain)*.0004f;
  int rp=(slim_pos+1)&63;*l=slim_dl[rp]*slim_gain;*r=slim_dr[rp]*slim_gain;slim_pos=rp;
}
static void music_mix(float*out,int n);
// The very last stage: a stereo look-ahead limiter over effects, song and music together, so their sum can never clip.
static float olim_g=1,olim_dl[64],olim_dr[64],olim_need[64];static int olim_pos=0;
static void out_limit(float*l,float*r){
  static float ceil_=0;if(ceil_==0)ceil_=(float)pow(10,-1.0/20);
  float a=fmaxf(fabsf(*l),fabsf(*r));if(!(a<1e6f)){*l=*r=0;a=0;}olim_dl[olim_pos]=*l;olim_dr[olim_pos]=*r;olim_need[olim_pos]=a>ceil_?ceil_/a:1;
  float mn=1;int i;for(i=0;i<64;i++)if(olim_need[i]<mn)mn=olim_need[i];
  if(mn<olim_g)olim_g+=(mn-olim_g)*.08f;else olim_g+=(1-olim_g)*.0003f;   // smooth attack inside the 64-sample look-ahead, slow release
  int rp=(olim_pos+1)&63;float x=olim_dl[rp]*olim_g,y=olim_dr[rp]*olim_g;olim_pos=rp;
  *l=x>1?1:x<-1?-1:x;*r=y>1?1:y<-1?-1:y;}
static void audio_mix_block(float*out,int n);
static void audio_mix(float*out,int n){   // any buffer size the system asks for, in blocks
  while(n>0){int k=n>4096?4096:n;audio_mix_block(out,k);int i;for(i=0;i<k;i++)out_limit(&out[2*i],&out[2*i+1]);out+=2*k;n-=k;}}
static void audio_mix_block(float*out,int n){
  static float dry[8192],snd[8192],sl[8192],sr[8192],ss[8192];int i,k;if(n>8192)n=8192;
  reverb_init();memset(dry,0,n*4);memset(snd,0,n*4);memset(sl,0,n*4);memset(sr,0,n*4);memset(ss,0,n*4);
  plat_lock();
  int song=0;for(k=0;k<NVOX;k++)if(VX[k].active==1){if(VX[k].bus)song=1;vox_render(&VX[k],dry,snd,sl,sr,ss,samples_done,n);}
  samples_done+=n;plat_unlock();
  if(song)song_bus_on=SR*3;   // keep the song's reverb and limiter running a little after the last note, for the tail
  for(i=0;i<n;i++){float d=dry[i],w=snd[i];if(!(d>-1e6f&&d<1e6f))d=0;if(!(w>-1e6f&&w<1e6f))w=0;   // a bad sample can never poison the reverb
    float x=(d+reverb_tick(&rv_fx,w)*(float)rv_scale)*(float)snd_vol*(float)TRIM_GAIN;float y=master_tick(x);out[2*i]=y;out[2*i+1]=y;}
  if(song_bus_on>0){song_bus_on-=n;float g=(float)(snd_vol*SONG_GAIN);
    for(i=0;i<n;i++){float l=sl[i],r=sr[i],w=ss[i];if(!(l>-1e6f&&l<1e6f))l=0;if(!(r>-1e6f&&r<1e6f))r=0;if(!(w>-1e6f&&w<1e6f))w=0;
      float rv=reverb_tick(&rv_song,w)*(float)rv_scale;l=(l+rv)*g;r=(r+rv)*g;song_limit(&l,&r);
      out[2*i]+=l;out[2*i+1]+=r;}}
  music_mix(out,n);
}

// ---- Gameplay blips and effect sounds ----
static const double UI_[][5]={ // move, rot, soft, lock, hold, cyc: {Hz, seconds, role, gain, glide-to}
 {200,.03,R_LEAD,.04,0},{380,.05,R_LEAD,.05,520},{140,.025,R_SOFT,.05,0},{120,.06,R_SOFT,.12,80},{300,.07,R_SMOOTH,.1,600},{520,.04,R_SMOOTH,.06,0}};
enum{S_MOVE,S_ROT,S_SOFT,S_LOCK,S_HOLD,S_CYC,S_HARD,S_CLEAR,S_LEVEL,S_OVER};
static void ui_tone(double f,double d,int role,double g,double to,double delay){double p=VOICES[snd_voice].pitch;tone(f*p,d,role,g,to*p,delay,0);}
static double last_soft=-1e9;
static void sfx(int k,int n,double now_ms){
  const SVoice*sv=&VOICES[snd_voice];int i;if(snd_vol<=0)return;
  if(k<=S_CYC){if(k==S_SOFT){if(now_ms-last_soft<=70)return;last_soft=now_ms;}const double*u=UI_[k];ui_tone(u[0],u[1],(int)u[2],u[3],u[4],0);}
  else if(k==S_HARD){noise_burst(.12,.35,900,0,0);ui_tone(140,.12,R_SOFT,.25,45,0);}
  else if(k==S_CLEAR){int cnt=imin(n,4);for(i=0;i<cnt;i++)tone(sv->root*4*pow(2,sv->tick[i]/12),.15,R_LEAD,.07,0,i*.06,0);if(n>=4)noise_burst(.35,.3,3000,0,0);}
  else if(k==S_LEVEL){static const double f[3]={523,659,784};for(i=0;i<3;i++)ui_tone(f[i],.16,R_SOFT,.12,0,.25+i*.08);}
  else if(k==S_OVER){static const double f[4]={392,330,262,196};for(i=0;i<4;i++)ui_tone(f[i],.3,R_ROUGH,.1,0,i*.15);}
}
// A layer gets s: the voice, the combo step c, whether it is a perfect clear, a time stretch, the base pitch f,
// and tone()/noise() helpers that are already scaled to the loudness budget.
typedef struct{const SVoice*v;int c,pf;double span,f,k;}Snd;
static void s_tone(Snd*s,double f,double d,int role,double g,double to,double delay,double wet){tone(f,d,role,g*s->k,to,delay,wet);}
static void s_noise(Snd*s,double d,double g,double cut,double delay,double cut2){noise_burst(d,g*s->k,cut,delay,cut2);}
// Level 1: a combo gets one chime that climbs the scale; a perfect clear gets a rising arpeggio.
static void L_melody(Snd*s){int j;if(s->pf){for(j=0;j<s->v->narp;j++)s_tone(s,s->f*4*pow(2,s->v->arp[j]/12),.2,R_SOFT,.12,0,j*.07,.15);}else{s_tone(s,s->f*4,.32,R_SOFT,.13,0,.03,.2);s_tone(s,s->f*8,.2,R_SMOOTH,.05,0,.03,.2);}}
// Level 2: a soft chord.
static void L_stab(Snd*s){int j,n=(s->c>=3||s->pf)?s->v->nchord:imin(3,s->v->nchord);for(j=0;j<n;j++)s_tone(s,s->f*2*s->v->chord[j],.45*s->span,R_BODY,.08,0,0,.3);}
// Level 3: a long reverb tail.
static void L_tail(Snd*s){int j,n=imin(3,s->v->nchord);for(j=0;j<n;j++){double r=s->v->chord[j];s_tone(s,s->f*r,.9*s->span,R_ROUGH,.08,0,0,.7);s_tone(s,s->f*r*1.007,.9*s->span,R_ROUGH,.065,0,0,.7);if(s->c>=12)s_tone(s,s->f*r,1.6,R_ROUGH,.06,0,.3,.8);}}
// Level 4: a deep bass hit, a rising sweep and sparkle.
static void L_sub(Snd*s){s_tone(s,s->f/2,.9*s->span,R_SMOOTH,.6,s->f/4,0,.2);s_noise(s,.45*s->span,.5,700,0,60);if(s->c>=8){s_tone(s,s->f/2,.8,R_SMOOTH,.6,s->f/4,.14,.2);s_noise(s,.5,.45,5000,.14,400);}}
static void L_sweep(Snd*s){s_noise(s,.7*s->span,.28,300,0,9000);}
static void L_sparkle(Snd*s){int j;static const double hi[6]={2,2.52,3,4,5,6};for(j=0;j<s->v->nspark;j++)s_tone(s,s->f*2*s->v->spark[j],.2,R_LEAD,.07,0,.18+j*.07,.5);if(s->c>=8)for(j=0;j<6;j++)s_tone(s,s->f*hi[j],.16,R_SOFT,.09,0,.5+j*.05,.6);}
enum{LY_MELODY,LY_STAB,LY_TAIL,LY_SUB,LY_SWEEP,LY_SPARKLE,N_LAYERS};
static void (*const LAYER[N_LAYERS])(Snd*)={L_melody,L_stab,L_tail,L_sub,L_sweep,L_sparkle};
static const double SHARE[N_LAYERS]={1,1.5,1.5,2,1,1.5};
static const int LADDER[5][4]={{-1},{LY_MELODY,-1},{LY_STAB,-1},{LY_TAIL,-1},{LY_SUB,LY_SWEEP,LY_SPARKLE,-1}}; // -1 ends a level
static const double EV_SPAN[2]={1,1.5};enum{EV_COMBO,EV_PERFECT};
static int layers_for(int lv,int*out){int n=0,i,j;for(i=1;i<=lv&&i<5;i++)for(j=0;LADDER[i][j]>=0;j++)out[n++]=LADDER[i][j];return n;}
// Pitch of the fanfare: a combo climbs the voice's scale (two octaves at most); a perfect clear sits on the root.
static double climb(const SVoice*v,int c,int pf){if(pf)return v->root;int i=iclamp(c-2,0,v->nscale*2-1);return v->root*pow(2,(v->scale[i%v->nscale]+12*(i/v->nscale))/12);}
static void fx_sound(int ev,int lv,int c){
  if(snd_vol<=0||lv<1)return;int names[16],n=layers_for(lv,names),i;double tot=0;
  for(i=0;i<n;i++)tot+=SHARE[names[i]];
  const SVoice*v=&VOICES[snd_voice];Snd s={v,c,ev==EV_PERFECT,EV_SPAN[ev],climb(v,c,ev==EV_PERFECT),dmin(1,BUDGET/tot)*v->gain};
  for(i=0;i<n;i++)LAYER[names[i]](&s);
}
