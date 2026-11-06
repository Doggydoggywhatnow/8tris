// ===== COMBO SONG =====
// With "Combo song" on (Sound tab), a combo chain plays a song instead of the usual combo sounds. It starts at combo 2 and keeps
// going while the chain lasts, adding a layer as the combo grows, so a long chain turns into a full track:
//   2 arpeggio and a soft kick   4 bass            7 hi-hats          10 full drums (the first big jump)
//  15 pads                      20 lead melody    30 counter-melody, octave bass, 16th hats
//  40 sparkles                  50 key change     60 choir           70-79 "lucky sevens": the song turns major
//  80 double-time drums         100 final form: key change, lead doubled an octave up, crashes all over
// Every combo also rings a note that climbs the scale, plus the same short two-note blip each time the combo goes up. Every 10th combo hits a crash and a chord stab, numbers like 11, 22 and
// 77 play a fanfare (77 and 777 hit a jackpot of bells), and every 100th gets a drum roll. When the chain breaks the song ends
// on a final chord. Notes are scheduled a fraction of a second ahead on a 16th-note grid, straight onto the audio clock.
// The song plays at every Effects level and replaces combo sounds only; everything else still sounds as usual.

static const int SC_MINOR[7]={0,2,3,5,7,8,10},SC_MAJOR[7]={0,2,4,5,7,9,11};
// Chord progressions as scale-degree roots, one chord per bar, four bars per phrase.
static const int PROG[4][4]={{0,5,2,6},{0,6,5,6},{5,6,0,0},{0,3,5,4}},PROG_MAJ[4]={0,4,5,3};
// Rhythms for the lead, one bit per 16th note (the top bit is the first 16th of the bar).
static const unsigned RHY[6]={0x8888u,0xAAAAu,0xA8AAu,0x8A8Au,0xAA88u,0xB6B6u};

// song_resume: the song was stopped by a pause or the menu while the chain was still alive
static int song_playing=0,song_seen=0,song_resume=0;static i64 song_next=0;static long song_step=0;static double song_len=0;
typedef struct{int major,key;const int*sc;}SongMode;

static int song_repdigit(int k){if(k<11)return 0;int d=k%10;while(k){if(k%10!=d)return 0;k/=10;}return 1;}
static SongMode song_mode(int c){SongMode m;m.major=(c>=70&&c<80)||(c>=770&&c<780);m.sc=m.major?SC_MAJOR:SC_MINOR;
  int k=2*(c/50);if(c>=100)k+=1;m.key=k%12;if(m.key>6)m.key-=12;return m;}
// Frequency of a scale degree (any integer; 7 = one octave up) in the song's key. Degree 0 at octave 0 is A2 (110 Hz).
static double song_f(const SongMode*m,int deg,int oct){int o=(int)floor(deg/7.0);int d=deg-o*7;return 110.0*pow(2,(m->sc[d]+12*(o+oct)+m->key)/12.0);}
static int song_chord(int c,long bar){if((c>=70&&c<80)||(c>=770&&c<780))return PROG_MAJ[bar%4];int sec=c<30?0:(int)((c/10+bar/8)%4);return PROG[sec][bar%4];}

// Stereo position for the next notes: -1 left, 0 middle, 1 right (equal-power pan).
static double song_pan=0;
static void song_bus(Voice*v){double a=(dclamp(song_pan,-1,1)+1)*.7853981633974483;v->bus=1;v->pl=(float)(cos(a)*1.41421356);v->pr=(float)(sin(a)*1.41421356);}
// One note straight onto the audio clock. shape 1 holds the note; to = glide target; nosc 2 adds a detuned twin.
static void sn(i64 at,double f,double dur,int wave,double g,double wet,double to,int shape,int nosc,double det){
  if(!(f>0))return;Voice*v=vox_at(at,dur);if(!v)return;
  vox_wave(v,wave);v->f0=f;v->f1=to>0?to:f;v->shape=shape;v->nosc=nosc;v->det=det;v->gain=dmax(.0002,g);v->wet=wet;song_bus(v);vox_go(v);
}
static void snoise(i64 at,double dur,double g,double cut,double cut2,double wet){
  Voice*v=vox_at(at,dur);if(!v)return;v->noise=1;v->gain=g;v->cut0=cut;v->cut1=cut2>0?cut2:cut;v->wet=wet;song_bus(v);vox_go(v);
}
static void s_kick(i64 at,double g){song_pan=0;sn(at,150,.28,W_SINE,.36*g,0,42,0,1,0);snoise(at,.02,.12*g,4000,0,0);}
static void s_snare(i64 at,double g){song_pan=.05;snoise(at,.2,.32*g,5000,1800,.25);sn(at,190,.08,W_TRI,.14*g,.1,160,0,1,0);}
static void s_hat(i64 at,double g){song_pan=.35;snoise(at,.035,.07*g,11000,0,0);}
static void s_crash(i64 at,double g){song_pan=-.25;snoise(at,1.8,.26*g,9000,2500,.5);}

// Everything that plays on one 16th-note step.
static void song_step_play(long step,i64 at,int c){
  SongMode m=song_mode(c);long bar=step/16;int s=(int)(step%16),root=song_chord(c,bar);
  int ch[3]={root,root+2,root+4};double L=song_len;
  // drums
  int dbl=c>=80;
  if(c>=10){if(s%4==0||(dbl&&(s==14||s==7)))s_kick(at,1);}else if(s==0||s==8)s_kick(at,.7);
  if(c>=10&&(s==4||s==12))s_snare(at,1);
  if(c>=20&&bar%4==3&&s>=12)s_snare(at,.35+.12*(s-12));                       // a fill at the end of every phrase
  if(c>=30){s_hat(at,s%2?1:.55);}else if(c>=7&&s%4==2)s_hat(at,1);
  if(c>=60&&s%4==0){song_pan=.5;}if(c>=60&&s%4==0)sn(at,song_f(&m,ch[0],4),.25,W_SINE,.025,.4,0,0,1,0);     // ride bell
  if(s==0&&((c>=20&&bar%8==0)||(c>=100&&bar%2==0)))s_crash(at,c>=100?.8:.6);
  // bass: 8ths, then octave jumps, then a gallop
  {int on=c>=80?1:(s%2==0);if(c>=4&&on){int oct=(c>=30&&((c>=80?s%4==2:s%4==2)))?1:0;
     song_pan=0;sn(at,song_f(&m,ch[0],-1+oct),L*(c>=80?.9:1.8),W_SAW,.16,.05,0,0,1,0);}}
  // arpeggio: 16ths over the chord across two octaves (up, or up and down past 40)
  {static const int UP[8]={0,1,2,3,4,5,4,3};int k=c>=40?UP[s%8]:s%6,ti=k%3,oo=k/3;
   song_pan=(s%2?.4:-.4);sn(at,song_f(&m,ch[ti],2+oo),L*.9,c>=50?W_PULSE25:W_SQUARE,c>=10?.045:.06,.22,0,0,1,0);}
  if(c>=40&&s%2==0)song_pan=(s%4?.75:-.75);if(c>=40&&s%2==0)sn(at,song_f(&m,ch[(s/2)%3],4),.18,W_SINE,.03,.55,0,0,1,0);    // sparkle
  // pads and choir: held chords, one per bar
  if(s==0){int i;
    if(c>=15)for(i=0;i<3;i++)if((song_pan=(i-1)*.45),1)sn(at,song_f(&m,ch[i],1),L*16*1.02,W_SAW,.028,.5,0,1,2,9);
    if(c>=60)for(i=0;i<3;i++)if((song_pan=(1-i)*.7),1)sn(at,song_f(&m,ch[i],2),L*16*1.02,W_TRI,.03,.6,0,1,2,18);}
  // lead melody: a four-bar phrase that repeats until the section changes, so it sticks in your head
  if(c>=20){
    unsigned sec=(unsigned)(c/10),seed=sec*2654435761u+(unsigned)(bar%4)*40503u+7u;unsigned rh=RHY[(seed>>7)%6];
    if(c>=50&&bar%4==3)rh=0xFFFFu&0xAAAAu;
    if(rh&(0x8000u>>s)){
      int nx=s+1;while(nx<16&&!(rh&(0x8000u>>nx)))nx++;double dur=L*(nx-s)*.95;
      unsigned h=seed^(unsigned)(s*97);h^=h>>13;h*=0x5bd1e995u;h^=h>>15;
      int deg=(s%4==0)?ch[h%3]:ch[h%3]+(int)(h>>8)%3-1;
      int oct=c>=100?3:2;
      song_pan=.08;sn(at,song_f(&m,deg,oct),dur,c>=50?W_SAW:W_PULSE25,.075,.3,0,1,c>=50?2:1,7);
      if(c>=100)sn(at,song_f(&m,deg,oct+1),dur,W_PULSE12,.035,.35,0,1,1,0);   // final form: doubled an octave up
      if(c>=30&&(song_pan=-.35,1))sn(at,song_f(&m,deg-2,oct-1),dur,W_TRI,.05,.3,0,1,1,0);         // counter-melody a third below
    }
  }
}
// The extra flourishes for reaching combo k, played from time 'at'.
static void song_milestone(int k,i64 at){
  SongMode m=song_mode(k);long bar=song_step/16;int root=song_chord(k,bar),i;double L=song_len;
  // every combo: a bell that climbs the scale (wrapping every three octaves)
  {int d=(k-2)%21;song_pan=(k%2?.3:-.3);sn(at,song_f(&m,root+d,3),.35,W_SINE,.09,.35,0,0,1,0);sn(at,song_f(&m,root+d,4),.2,W_TRI,.03,.35,0,0,1,0);}
  // and the same short two-note blip every time the combo goes up (tonic, then the fifth), so each step is easy to hear
  {song_pan=0;sn(at,song_f(&m,0,3),.11,W_PULSE25,.11,.15,0,0,1,0);sn(at+(i64)(.065*SR),song_f(&m,4,3),.16,W_PULSE25,.11,.2,0,0,1,0);}
  if(k%10==0){s_crash(at,1);s_kick(at,1.2);
    for(i=0;i<3;i++)if((song_pan=(i-1)*.5),1)sn(at,song_f(&m,root+2*i,2),.6,W_SAW,.07,.4,0,1,2,12);
    sn(at,110*pow(2,m.key/12.0),1.1,W_SINE,.5,0,32,0,1,0);}                     // sub drop
  if(song_repdigit(k)||k==77){
    for(i=0;i<12;i++)sn(at+(i64)(i*L*SR/2),song_f(&m,root+2*(i%3)+7*(i/3),2),.25,W_PULSE25,.07,.35,0,0,1,0);
    s_crash(at+(i64)(6*L*SR),1);
    if(k%111==0||k==77||k==777){for(i=0;i<16;i++)sn(at+(i64)((6+i)*L*SR/2),song_f(&m,root+21-i,3),.5,W_SINE,.05,.6,0,0,1,0);}   // jackpot bells
  }
  if(k%100==0){for(i=0;i<16;i++)s_snare(at+(i64)(i*L*SR/2),.3+.045*i);s_crash(at+(i64)(8*L*SR),1.2);}
}
static void song_end(i64 at,int flourish){
  if(flourish){SongMode m=song_mode(song_seen);int root=song_chord(song_seen,song_step/16),i;
    s_crash(at,1);s_kick(at,1);
    for(i=0;i<3;i++)sn(at,song_f(&m,root+2*i,1),1.8,W_SAW,.05,.6,0,1,2,10);
    sn(at,song_f(&m,root,3),1.4,W_SINE,.08,.6,0,0,1,0);}
  song_playing=0;
}
static double song_step_len(int c){double bpm=124+dmin(c,100)*.3+(c>=100?6:0);return 60.0/bpm/4;}
// Called every frame by the game. Starts, feeds and ends the song from the current combo.
static void song_tick(int active){
  int c=combo;
  if(!comboSong||snd_vol<=0){if(song_playing)song_end(0,0);song_seen=c;return;}
  i64 now=audio_clock(),ahead=(i64)(.12*SR);
  if(song_playing&&(c<2||!active)){song_end(song_next>now?song_next:now,c<2&&active);song_resume=c>=2&&!active;song_seen=c;return;}
  if(c<2)song_resume=0;
  if(!song_playing){if(c>=2&&active&&(c>song_seen||song_resume)){song_resume=0;song_playing=1;song_step=0;song_len=song_step_len(c);song_next=now+(i64)(.03*SR);}else{song_seen=c;return;}}
  if(song_next<now)song_next=now+(i64)(.01*SR);   // fell behind (a long frame): pick up from here rather than rush
  if(c>song_seen){   // new combos: their flourishes land on the next 16th
    i64 at=song_next,step=(i64)(song_len*SR);while(at-step>=now+(i64)(.02*SR))at-=step;
    int k;for(k=song_seen+1;k<=c&&k-song_seen<=8;k++)if(k>=2)song_milestone(k,at);
  }
  song_seen=c;
  while(song_next<now+ahead){
    if(song_step%16==0)song_len=song_step_len(c);   // tempo changes only at the start of a bar
    song_step_play(song_step,song_next,c);song_next+=(i64)(song_len*SR);song_step++;
  }
}
