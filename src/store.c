// ===== KEY BINDINGS, SCORE HISTORY AND SAVED SETTINGS =====
// The optional play timer (CLK[0]) and real-time clock (CLK[1]), set up in the Theming tab: on, place (0-5 corners/edges, 6 side panel), size pt, opacity %, font family, bold.
typedef struct{int on,pos,sz,op,fam,bold;}Clk;
static Clk CLK[2]={{0,2,28,80,0,0},{0,0,28,80,0,0}};
static Clk NP={0,3,16,85,0,0};static int npArt=1,npArtSz=72,npArtist=1;   // "now playing" text on screen: same style options as the clocks, plus artist/album line and album art
static int clkTzH=0,clkTzM=0,clkDst=0,clk24=1,clkSecs=0,stackV=1,stackFirst=0;   // clock: UTC offset typed by the user (hours, minutes), daylight saving +1h, 24-hour, seconds; stacking of the two when they share a place
// Filters over the whole game picture (not the menu): brightness -50..50, contrast %, saturation %, hue degrees, color split px, tint strength %, opacity %, invert, tint color.
static int fltBri=0,fltCon=100,fltSat=100,fltHue=0,fltSplit=0,fltTint=0,fltOp=100,fltInv=0;static u32 fltTintC=0xFFFF9A3Cu;
static int flt_active(void){return fltBri||fltCon!=100||fltSat!=100||fltHue||fltSplit||fltTint||fltOp!=100||fltInv;}
static int boardPX=50,boardPY=50;   // where the board sits in the play area: 0 = left/top, 50 = centered, 100 = right/bottom
static int prefBtn=6;   // where the Preferences button sits: 0-5 corners and edges of the play area, 6 = bottom of the side panel (default)
static int resSet=0,fpsSet=0,showFps=0,fullscreen=0;   // display settings: render resolution choice, refresh-rate choice, frame-rate counter, full screen
static const char*ACT_NAME[N_ACTS]={"Move left","Move right","Soft drop","Hard drop","Rotate clockwise","Rotate counterclockwise","Hold","Retrieve held","Cycle held","Pause","Restart"};
static const int ACT_DEF[N_ACTS][2]={{'a',K_LEFT},{'d',K_RIGHT},{'s',K_DOWN},{' ',0},{'j',K_UP},{'k','z'},{'l',0},{'e',0},{'q',0},{'p',0},{'r',0}};
static int binds[N_ACTS][2];
static void binds_default(void){int a,s;for(a=0;a<N_ACTS;a++)for(s=0;s<2;s++)binds[a][s]=ACT_DEF[a][s];}
static int bind_key(int a,int s){return binds[a][s];}
static int key_action(int k){int a,s;if(!k)return -1;for(a=0;a<N_ACTS;a++)for(s=0;s<2;s++)if(binds[a][s]==k)return a;return -1;}
static const char* key_label(int k){
  static char buf[4][16];static int bi=0;char*b=buf[bi=(bi+1)&3];
  if(!k)return "none";if(k==' ')return "Space";
  switch(k){case K_LEFT:return "\xE2\x86\x90";case K_RIGHT:return "\xE2\x86\x92";case K_UP:return "\xE2\x86\x91";case K_DOWN:return "\xE2\x86\x93";
    case K_ESC:return "Esc";case K_BKSP:return "Backspace";case K_DEL:return "Delete";case K_ENTER:return "Enter";case K_TAB:return "Tab";case K_F11:return "F11";
    case K_PGUP:return "PgUp";case K_PGDN:return "PgDn";case K_HOME:return "Home";case K_END:return "End";case K_SHIFT:return "Shift";case K_CTRL:return "Ctrl";case K_ALT:return "Alt";}
  if(k>=32&&k<127){b[0]=(char)((k>='a'&&k<='z')?k-32:k);b[1]=0;return b;}
  snprintf(b,16,"#%d",k);return b;
}

// ---- History: every finished run is saved with every setting that was in use ----
typedef struct{int W,H,slots;double fall;int lvl,clear;int weights[11];int tw[7];int themeOn;int theme[N_COMPS];int fx,pf,sc,sp,sd,vol;int keys[N_ACTS][2];}Snap;
typedef struct{i64 t;int why,score,lines,level,time,pieces,combo,dirty;Snap s;}Hist;
#define MAXHIST 200
static Hist hist[MAXHIST];static int nhist=0;
static void snapshot(Snap*s){
  int i;memset(s,0,sizeof*s);s->W=W;s->H=H;s->slots=slots;s->fall=floor(rateOf(fallV)*100+.5)/100;s->lvl=lvlSys;s->clear=clearOn;
  for(i=0;i<11;i++)s->weights[i]=weights[i+1];for(i=0;i<7;i++)s->tw[i]=tw[i];s->themeOn=themeOn;for(i=0;i<N_COMPS;i++)s->theme[i]=TC[i];
  s->fx=fxLevel;s->pf=pcLevel;s->sc=sndCombo;s->sp=sndPf;s->sd=sdRate;s->vol=(int)floor(snd_vol*100+.5);memcpy(s->keys,binds,sizeof binds);
}
static const char* get_pref_path(const char*name){static char b[600];snprintf(b,sizeof b,"%s%s",plat_pref_dir(),name);return b;}

// ---- Crash-safe saving: write a temporary file, flush it to the disk, then swap it in with one rename. ----
// A crash or power cut mid-save leaves either the old file or the new one, never half of one.
static FILE* safe_open(const char*path,char*tmp,int tn){snprintf(tmp,(size_t)tn,"%s.tmp",path);return fopen(tmp,"w");}
static int safe_close(FILE*f,const char*tmp,const char*path){
  int ok=!ferror(f)&&fflush(f)==0;
#ifdef _WIN32
  if(ok&&_commit(_fileno(f))!=0)ok=0;
#else
  if(ok&&fsync(fileno(f))!=0)ok=0;
#endif
  if(fclose(f)!=0)ok=0;
#ifdef _WIN32
  if(ok)ok=MoveFileExA(tmp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
  if(ok)ok=rename(tmp,path)==0;
#endif
  if(!ok)remove(tmp);
  return ok;
}

// ---- History file: plain text, one run per line, newest first, as "key=value" pairs separated by spaces. ----
// Unknown keys are ignored and missing ones read as 0, so older and newer versions of 8tris can read each other's history.
#define HIST_FILE "history.txt"
static void ints_out(FILE*f,const char*k,const int*v,int n){int i;fprintf(f," %s=",k);for(i=0;i<n;i++)fprintf(f,"%d%s",v[i],i<n-1?",":"");}
static int store_off=0;   // set by the self-test so it never writes over the player's files
static int hist_save(void){
  if(store_off)return 1;
  char tmp[640];const char*path=get_pref_path(HIST_FILE);char dst[600];snprintf(dst,sizeof dst,"%s",path);
  FILE*f=safe_open(dst,tmp,sizeof tmp);if(!f)return 0;int i;
  fprintf(f,"# 8tris history, format 1. One finished run per line, newest first.\n");
  for(i=0;i<nhist;i++){const Hist*h=&hist[i];const Snap*q=&h->s;
    fprintf(f,"t=%lld why=%d score=%d lines=%d level=%d time=%d pieces=%d combo=%d dirty=%d",(long long)h->t,h->why,h->score,h->lines,h->level,h->time,h->pieces,h->combo,h->dirty);
    fprintf(f," W=%d H=%d slots=%d fall=%g lvl=%d clear=%d",q->W,q->H,q->slots,q->fall,q->lvl,q->clear);
    ints_out(f,"weights",q->weights,11);ints_out(f,"tw",q->tw,7);fprintf(f," themeOn=%d",q->themeOn);ints_out(f,"theme",q->theme,N_COMPS);
    fprintf(f," fx=%d pf=%d sc=%d sp=%d sd=%d vol=%d",q->fx,q->pf,q->sc,q->sp,q->sd,q->vol);ints_out(f,"keys",&q->keys[0][0],N_ACTS*2);fprintf(f,"\n");}
  return safe_close(f,tmp,dst);
}
static int parse_ints(const char*s,int*out,int n);
// Splits off the next space-separated word (in place).
static char* next_tok(char**p){char*s=*p;while(*s==' '||*s=='\t'||*s=='\r'||*s=='\n')s++;if(!*s)return NULL;char*e=s;while(*e&&*e!=' '&&*e!='\t'&&*e!='\r'&&*e!='\n')e++;if(*e)*e++=0;*p=e;return s;}
// Reads one history line into h. Returns 1 if it held a run.
static int hist_parse(char*line,Hist*h){
  memset(h,0,sizeof*h);int got=0;char*p=line,*tok;Snap*q=&h->s;
  while((tok=next_tok(&p))){
    char*eq=strchr(tok,'=');if(!eq)continue;*eq=0;const char*k=tok,*v=eq+1;int iv=atoi(v);
    if(!strcmp(k,"t")){h->t=(i64)strtoll(v,NULL,10);got=1;}
    else if(!strcmp(k,"why"))h->why=iv?1:0;else if(!strcmp(k,"score"))h->score=iv;else if(!strcmp(k,"lines"))h->lines=iv;
    else if(!strcmp(k,"level"))h->level=iv;else if(!strcmp(k,"time"))h->time=iv;else if(!strcmp(k,"pieces"))h->pieces=iv;
    else if(!strcmp(k,"combo"))h->combo=iv;else if(!strcmp(k,"dirty"))h->dirty=iv?1:0;
    else if(!strcmp(k,"W"))q->W=iv;else if(!strcmp(k,"H"))q->H=iv;else if(!strcmp(k,"slots"))q->slots=iv;
    else if(!strcmp(k,"fall")){double d=atof(v);q->fall=d==d?d:0;}else if(!strcmp(k,"lvl"))q->lvl=iv;else if(!strcmp(k,"clear"))q->clear=iv;
    else if(!strcmp(k,"weights"))parse_ints(v,q->weights,11);else if(!strcmp(k,"tw"))parse_ints(v,q->tw,7);
    else if(!strcmp(k,"themeOn"))q->themeOn=iv;else if(!strcmp(k,"theme"))parse_ints(v,q->theme,N_COMPS);
    else if(!strcmp(k,"fx"))q->fx=iv;else if(!strcmp(k,"pf"))q->pf=iv;else if(!strcmp(k,"sc"))q->sc=iv;else if(!strcmp(k,"sp"))q->sp=iv;
    else if(!strcmp(k,"sd"))q->sd=iv;else if(!strcmp(k,"vol"))q->vol=iv;else if(!strcmp(k,"keys"))parse_ints(v,&q->keys[0][0],N_ACTS*2);
  }
  return got;
}
static int hist_read_text(Hist*out,int cap){
  FILE*f=fopen(get_pref_path(HIST_FILE),"r");if(!f)return -1;static char line[4096];int n=0;
  while(n<cap&&fgets(line,sizeof line,f)){
    size_t L=strlen(line);if(L==sizeof line-1&&line[L-1]!='\n'){int c;while((c=fgetc(f))!=EOF&&c!='\n');continue;}   // an overlong line is skipped, not misread
    if(line[0]=='#')continue;if(hist_parse(line,&out[n]))n++;}
  fclose(f);return n;
}
// Older versions (0.7.2 and before) kept history as a raw binary dump. It is converted once, checked, and then removed,
// so that "Clear history" really clears everything and no stale copy is left behind.
static void hist_migrate_bin(void){
  char bin[600];snprintf(bin,sizeof bin,"%s",get_pref_path("history.bin"));
  FILE*f=fopen(bin,"rb");if(!f)return;unsigned magic=0,sz=0;int n=0,ok=0;
  if(fread(&magic,4,1,f)==1&&fread(&sz,4,1,f)==1&&fread(&n,4,1,f)==1&&magic==0x38544831u&&sz==sizeof(Hist)&&n>=0&&n<=MAXHIST){nhist=(int)fread(hist,sizeof(Hist),(size_t)n,f);ok=1;}
  fclose(f);
  if(!ok){nhist=0;return;}   // a file this version cannot read is left alone
  if(hist_save()){static Hist chk[MAXHIST];if(hist_read_text(chk,MAXHIST)==nhist)remove(bin);}
}
static void hist_load(void){
  if(store_off){nhist=0;return;}
  int n=hist_read_text(hist,MAXHIST);
  if(n>=0){nhist=n;return;}
  nhist=0;hist_migrate_bin();
}
static void endRun(void){
  if(!board||saved)return;saved=1;if(!clearOn||pieces<1)return;int why=over?0:1;if(why==1&&score<=0)return;
  memmove(hist+1,hist,sizeof(Hist)*(nhist<MAXHIST?nhist:MAXHIST-1));if(nhist<MAXHIST)nhist++;
  Hist*h=&hist[0];memset(h,0,sizeof*h);h->t=(i64)time(NULL);h->why=why;h->score=(int)floor(score);h->lines=lines;h->level=level;h->time=(int)floor(playMs/1000+.5);h->pieces=pieces;h->combo=maxCombo;h->dirty=runDirty;snapshot(&h->s);
  hist_save();
}
static const char* FXN[5]={"none","a bit","moderate","high","intense"};
// Dates are shown in UTC plus the offset the user typed in (the same one the clock uses). The device's own timezone is never read.
static void fmt_date(i64 t,char*out,int n){
  int off=(clkTzH<0?-1:1)*(abs(clkTzH)*60+clkTzM)+(clkDst?60:0);time_t tt=(time_t)(t+(i64)off*60);struct tm*m=gmtime(&tt);
  if(!m){snprintf(out,n,"?");return;}snprintf(out,n,"%04d-%02d-%02d %02d:%02d",m->tm_year+1900,m->tm_mon+1,m->tm_mday,m->tm_hour,m->tm_min);
}
static void fmt_time(int sec,char*out,int n){if(sec<0)sec=0;if(sec>=3600)snprintf(out,n,"%d:%02d:%02d",sec/3600,sec/60%60,sec%60);else snprintf(out,n,"%d:%02d",sec/60,sec%60);}
// Every setting of a run as plain-text lines. Returns the number of lines written into out (each at most 400 chars).
static int setting_lines(const Snap*s,char out[][400],int cap){
  int n=0,i,any=0;char b[400];
  #define LINE(...) do{if(n<cap){snprintf(out[n],400,__VA_ARGS__);n++;}}while(0)
  LINE("Grid: %d x %d | Hold slots: %d",s->W,s->H,s->slots);
  LINE("Fall rate: %g rows/s | Soft drop: %d rows/s | Level system: %s | Line clearing: %s",s->fall,s->sd,s->lvl?"on":"off",s->clear?"on":"off");
  {int multi=0;for(i=0;i<11;i++)if(s->weights[i]>1)multi=1;b[0]=0;
   for(i=0;i<11;i++)if(s->weights[i]>0){char t[40];if(i<10)snprintf(t,40,"%d-block",i+1);else snprintf(t,40,"letters");if(multi)snprintf(t+strlen(t),40-strlen(t)," (%d)",s->weights[i]);if(any)strncat(b,", ",sizeof b-strlen(b)-1);strncat(b,t,sizeof b-strlen(b)-1);any=1;}
   LINE("Piece sets: %s",any?b:"none");}
  {int custom=0;for(i=1;i<7;i++)if(s->tw[i]!=s->tw[0])custom=1;if(custom){b[0]=0;for(i=0;i<7;i++)snprintf(b+strlen(b),sizeof b-strlen(b),"%s%c %d",i?", ":"",TN[i],s->tw[i]);LINE("Standard piece frequencies: %s",b);}}
  if(s->themeOn){b[0]=0;for(i=0;i<N_COMPS;i++)snprintf(b+strlen(b),sizeof b-strlen(b),"%s%s %s",i?", ":"",COMP_NAME[i],theme_label(i,s->theme[i]));LINE("Theming: on (%s)",b);}else LINE("Theming: off (8tris look)");
  LINE("Visual effects: combo %s, perfect clear %s",FXN[iclamp(s->fx,0,4)],FXN[iclamp(s->pf,0,4)]);
  if(s->vol>0)snprintf(b,sizeof b,"%d%%",s->vol);else snprintf(b,sizeof b,"off");
  LINE("Sound: %s | combo sound %s, perfect clear sound %s",b,FXN[iclamp(s->sc,0,4)],FXN[iclamp(s->sp,0,4)]);
  b[0]=0;for(i=0;i<N_ACTS;i++){char t[120];const char*k0=key_label(s->keys[i][0]),*k1=key_label(s->keys[i][1]);
    if(s->keys[i][0]&&s->keys[i][1])snprintf(t,120,"%s %s or %s",ACT_NAME[i],k0,k1);else if(s->keys[i][0])snprintf(t,120,"%s %s",ACT_NAME[i],k0);else if(s->keys[i][1])snprintf(t,120,"%s %s",ACT_NAME[i],k1);else snprintf(t,120,"%s none",ACT_NAME[i]);
    snprintf(b+strlen(b),sizeof b-strlen(b),"%s%s",i?"; ":"",t);}
  LINE("Controls: %s",b);
  #undef LINE
  return n;
}
static int entry_lines(const Hist*e,char out[][400],int cap){
  char d[32],tm_[16];fmt_date(e->t,d,32);fmt_time(e->time,tm_,16);int n=0;
  snprintf(out[n++],400,"%s  Score %d  Lines %d  Level %d  Time %s  Pieces %d  Best combo %d  (%s)",d,e->score,e->lines,e->level,tm_,e->pieces,e->combo,e->why?"restarted":"game over");
  if(e->dirty&&n<cap)snprintf(out[n++],400,"Note: fall rate or level system was changed during this run.");
  static char tmp[40][400];int m=setting_lines(&e->s,tmp,40),i;for(i=0;i<m&&n<cap;i++)snprintf(out[n++],400,"  %s",tmp[i]);
  return n;
}
// Writes the full history to a text file in the user's home folder. Returns the path (or NULL on failure).
static const char* export_history(void){
  static char path[700];char d[32];fmt_date((i64)time(NULL),d,32);d[10]=0;
  snprintf(path,sizeof path,"%s8tris-history-%s.txt",plat_home_dir(),d);char tmp[720];FILE*f=safe_open(path,tmp,sizeof tmp);if(!f)return NULL;
  char dd[32];fmt_date((i64)time(NULL),dd,32);fprintf(f,"8tris score history\nExported %s, %d run%s\n\n",dd,nhist,nhist==1?"":"s");
  static char L[60][400];int i,j;for(i=0;i<nhist;i++){int n=entry_lines(&hist[i],L,60);fprintf(f,"#%d  %s\n",nhist-i,L[0]);for(j=1;j<n;j++)fprintf(f,"%s\n",L[j]);fprintf(f,"\n");}
  return safe_close(f,tmp,path)?path:NULL;
}

// ---- Saved settings: a small key=value text file ----
static void save_settings(void){
  if(store_off)return;
  char tmp[640],dst[600];snprintf(dst,sizeof dst,"%s",get_pref_path("settings.txt"));FILE*f=safe_open(dst,tmp,sizeof tmp);if(!f)return;int i;
  fprintf(f,"grid=%d,%d\nslots=%d\nfall=%g\nlvl=%d\nclear=%d\nsd=%d\nvol=%d\nthemeOn=%d\ncalm=%d\nfxm=%d\nadv=%d,%d,%d,%d\nsolid=%06x,%06x,%d\nui=%d,%d,%d,%d,%d,%d,%d,%d\n",W,H,slots,fallV,lvlSys,clearOn,sdRate,(int)floor(snd_vol*100+.5),themeOn,calm,fxMaster,ADV[0],ADV[1],ADV[2],ADV[3],(unsigned)(solidP&0xFFFFFF),(unsigned)(solidG&0xFFFFFF),solidSep,uiFam,uiBold,uiItal,uiPt,hiCon,mwSel,lsSel,ghostStr);
  fprintf(f,"res=%d\nfps=%d\nshowfps=%d\nnowplay=%d,%d,%d,%d,%d,%d,%d,%d,%d\nmusic=%d,%d\nmusicfile=%s\nnext=%d\nprefbtn=%d\nboardpos=%d,%d\ntimer=%d,%d,%d,%d,%d,%d\nclock=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\nstack=%d,%d\nfilters=%d,%d,%d,%d,%d,%d,%d,%d,%06x\n",resSet,fpsSet,showFps,NP.on,NP.pos,NP.sz,NP.op,NP.fam,NP.bold,npArt,npArtSz,npArtist,musMode,musVol,musFile,NQ,prefBtn,boardPX,boardPY,
    CLK[0].on,CLK[0].pos,CLK[0].sz,CLK[0].op,CLK[0].fam,CLK[0].bold,CLK[1].on,CLK[1].pos,CLK[1].sz,CLK[1].op,CLK[1].fam,CLK[1].bold,clkTzH,clkTzM,clkDst,clk24,clkSecs,stackV,stackFirst,
    fltBri,fltCon,fltSat,fltHue,fltSplit,fltTint,fltOp,fltInv,(unsigned)(fltTintC&0xFFFFFF));
  fprintf(f,"weights=");for(i=1;i<=11;i++)fprintf(f,"%d%s",weights[i],i<11?",":"\n");
  fprintf(f,"tw=");for(i=0;i<7;i++)fprintf(f,"%d%s",tw[i],i<6?",":"\n");
  fprintf(f,"theme=");for(i=0;i<N_COMPS;i++)fprintf(f,"%d%s",TC[i],i<N_COMPS-1?",":"\n");
  fprintf(f,"binds=");for(i=0;i<N_ACTS*2;i++)fprintf(f,"%d%s",binds[i/2][i%2],i<N_ACTS*2-1?",":"\n");
  safe_close(f,tmp,dst);
}
static void clk_set(int i,const int*v){CLK[i].on=v[0]?1:0;CLK[i].pos=iclamp(v[1],0,6);CLK[i].sz=iclamp(v[2],12,120);CLK[i].op=iclamp(v[3],10,100);CLK[i].fam=iclamp(v[4],0,6);CLK[i].bold=v[5]?1:0;}
static int parse_ints(const char*s,int*out,int n){int i=0;while(*s&&i<n){out[i++]=atoi(s);while(*s&&*s!=',')s++;if(*s==',')s++;}return i;}
static void load_settings(void){
  if(store_off)return;   // the self-test always runs with the defaults
  FILE*f=fopen(get_pref_path("settings.txt"),"r");if(!f)return;char line[400];int v[40],i,n;
  while(fgets(line,sizeof line,f)){
    char*eq=strchr(line,'=');if(!eq)continue;*eq=0;const char*k=line,*val=eq+1;
    if(!strcmp(k,"grid")){if(parse_ints(val,v,2)==2){W=iclamp(v[0],4,500);H=iclamp(v[1],4,500);}}
    else if(!strcmp(k,"slots"))slots=iclamp(atoi(val),1,10);
    else if(!strcmp(k,"fall"))fallV=dclamp(atof(val),-60,130);
    else if(!strcmp(k,"lvl"))lvlSys=atoi(val)!=0;
    else if(!strcmp(k,"clear"))clearOn=atoi(val)!=0;
    else if(!strcmp(k,"sd"))sdRate=iclamp(atoi(val),1,500);
    else if(!strcmp(k,"vol"))snd_vol=iclamp(atoi(val),0,100)/100.0;
    else if(!strcmp(k,"themeOn"))themeOn=atoi(val)!=0;
    else if(!strcmp(k,"calm"))calm=atoi(val)!=0;
    else if(!strcmp(k,"fxm"))fxMaster=iclamp(atoi(val),0,4);
    else if(!strcmp(k,"solid")){unsigned a,b;int c;if(sscanf(val,"%x,%x,%d",&a,&b,&c)==3){solidP=0xFF000000u|(a&0xFFFFFF);solidG=0xFF000000u|(b&0xFFFFFF);solidSep=c?1:0;}}
    else if(!strcmp(k,"ui")){if(parse_ints(val,v,8)==8){uiFam=iclamp(v[0],0,6);uiBold=v[1]?1:0;uiItal=v[2]?1:0;uiPt=iclamp(v[3],8,28);hiCon=v[4]?1:0;mwSel=iclamp(v[5],0,2);lsSel=iclamp(v[6],0,2);ghostStr=iclamp(v[7],20,100);}}
    else if(!strcmp(k,"adv")){if(parse_ints(val,v,4)==4)for(i=0;i<4;i++)ADV[i]=iclamp(v[i],-1,4);}
    else if(!strcmp(k,"res"))resSet=iclamp(atoi(val),0,6);
    else if(!strcmp(k,"fps"))fpsSet=iclamp(atoi(val),0,9);
    else if(!strcmp(k,"showfps"))showFps=atoi(val)!=0;
    else if(!strcmp(k,"boardpos")){if(parse_ints(val,v,2)==2){boardPX=iclamp(v[0],0,100);boardPY=iclamp(v[1],0,100);}}
    else if(!strcmp(k,"nowplay")){if(parse_ints(val,v,9)==9){NP.on=v[0]?1:0;NP.pos=iclamp(v[1],0,5);NP.sz=iclamp(v[2],10,48);NP.op=iclamp(v[3],10,100);NP.fam=iclamp(v[4],0,6);NP.bold=v[5]?1:0;npArt=v[6]?1:0;npArtSz=iclamp(v[7],32,256);npArtist=v[8]?1:0;}}
    else if(!strcmp(k,"music")){if(parse_ints(val,v,2)==2){musMode=iclamp(v[0],0,3);musVol=iclamp(v[1],0,100);}}
    else if(!strcmp(k,"musicfile")){size_t L_=strlen(val);while(L_&&(val[L_-1]=='\n'||val[L_-1]=='\r'))L_--;if(L_<sizeof musFile&&!memchr(val,'/',L_)&&!memchr(val,'\\',L_)){memcpy(musFile,val,L_);musFile[L_]=0;}}
    else if(!strcmp(k,"next"))NQ=iclamp(atoi(val),0,10);
    else if(!strcmp(k,"prefbtn"))prefBtn=iclamp(atoi(val),0,6);
    else if(!strcmp(k,"timer")){if(parse_ints(val,v,6)==6)clk_set(0,v);}
    else if(!strcmp(k,"clock")){if(parse_ints(val,v,11)==11){clk_set(1,v);clkTzH=iclamp(v[6],-12,14);clkTzM=(v[7]==15||v[7]==30||v[7]==45)?v[7]:0;clkDst=v[8]?1:0;clk24=v[9]?1:0;clkSecs=v[10]?1:0;}}
    else if(!strcmp(k,"stack")){if(parse_ints(val,v,2)==2){stackV=v[0]?1:0;stackFirst=v[1]?1:0;}}
    else if(!strcmp(k,"filters")){int q[8];unsigned tc;if(sscanf(val,"%d,%d,%d,%d,%d,%d,%d,%d,%x",&q[0],&q[1],&q[2],&q[3],&q[4],&q[5],&q[6],&q[7],&tc)==9){fltBri=iclamp(q[0],-50,50);fltCon=iclamp(q[1],50,150);fltSat=iclamp(q[2],0,200);fltHue=iclamp(q[3],0,359);fltSplit=iclamp(q[4],0,12);fltTint=iclamp(q[5],0,100);fltOp=iclamp(q[6],10,100);fltInv=q[7]?1:0;fltTintC=0xFF000000u|(tc&0xFFFFFF);}}
    else if(!strcmp(k,"weights")){if(parse_ints(val,v,11)==11)for(i=0;i<11;i++)weights[i+1]=iclamp(v[i],0,10);}
    else if(!strcmp(k,"tw")){if(parse_ints(val,v,7)==7)for(i=0;i<7;i++)tw[i]=iclamp(v[i],0,10);}
    else if(!strcmp(k,"theme")){n=parse_ints(val,v,N_COMPS);for(i=0;i<n;i++)if(v[i]>=0&&v[i]<N_THEMES&&(AVAIL[i]>>v[i]&1))TC[i]=v[i];}
    else if(!strcmp(k,"binds")){if(parse_ints(val,v,N_ACTS*2)==N_ACTS*2)for(i=0;i<N_ACTS*2;i++)binds[i/2][i%2]=v[i];}
  }
  fclose(f);
  {int any=0;for(i=1;i<=11;i++)if(weights[i]>0)any=1;if(!any)weights[4]=1;}
  {int any=0;for(i=0;i<7;i++)if(tw[i]>0)any=1;if(!any)for(i=0;i<7;i++)tw[i]=1;}
}
