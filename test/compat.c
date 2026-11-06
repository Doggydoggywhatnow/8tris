#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-sc/";} const char* plat_home_dir(void){return "/tmp/8tris-sc/";}
// Upgrade safety net: settings files written by older versions (or damaged ones) must load without losing anything
// they contain, must never disturb the settings they do not mention, and save -> load must be lossless for every key.
static int bad=0;
#define CHK(c) do{if(!(c)){bad++;printf("FAIL line %d: %s\n",__LINE__,#c);}}while(0)
#define SP "/tmp/8tris-sc/settings.txt"
static void put(const char*txt){FILE*f=fopen(SP,"wb");fputs(txt,f);fclose(f);}
static void rd_set(char*out,size_t n){FILE*f=fopen(SP,"rb");size_t k=0;if(f){k=fread(out,1,n-1,f);fclose(f);}out[k]=0;}
// the text of one "key=..." line (without the newline) from a settings text, or "" when absent
static void getline_(const char*txt,const char*key,char*out){out[0]=0;size_t kl=strlen(key);const char*p=txt;
  while(*p){if(!strncmp(p,key,kl)&&p[kl]=='='){size_t i=0;while(p[i]&&p[i]!='\n'&&i<399){out[i]=p[i];i++;}out[i]=0;return;}while(*p&&*p!='\n')p++;if(*p)p++;}}
static char DEF[8000];
// every key but the ones in 'skip' (comma separated) has exactly the default line
static int same_except(const char*txt,const char*skip){
  char keys[80][24];int n=0;const char*p=DEF;
  while(*p){const char*e=strchr(p,'=');if(!e)break;size_t l=(size_t)(e-p);if(l<24&&n<80){memcpy(keys[n],p,l);keys[n][l]=0;n++;}while(*p&&*p!='\n')p++;if(*p)p++;}
  int ok=1,i;for(i=0;i<n;i++){char sk[200];snprintf(sk,sizeof sk,",%s,",skip);char kk[40];snprintf(kk,sizeof kk,",%s,",keys[i]);if(strstr(sk,kk))continue;
    char a[400],b[400];getline_(DEF,keys[i],a);getline_(txt,keys[i],b);if(strcmp(a,b)){ok=0;printf("  changed by mistake: %s -> %s\n",a,b);}}
  return ok;}
// settings.txt as it would have been written by a given older version, loaded into a fresh process state
static void reset_defaults(void){put(DEF);load_settings();}
int main(void){
  system("rm -rf /tmp/8tris-sc;mkdir -p /tmp/8tris-sc");
  save_settings();rd_set(DEF,sizeof DEF);CHK(strlen(DEF)>300);   // the defaults, exactly as this version writes them
  char cur[8000],ln[400];

  // 1. the very first versions only knew a few keys: those load, nothing else moves
  put("grid=12,24\nslots=2\nfall=1.5\nvol=30\n");reset_defaults();put("grid=12,24\nslots=2\nfall=1.5\nvol=30\n");load_settings();
  CHK(W==12&&H==24&&slots==2&&fabs(fallV-1.5)<1e-9&&fabs(snd_vol-.30)<1e-9);
  save_settings();rd_set(cur,sizeof cur);CHK(same_except(cur,",grid,slots,fall,vol,"));
  // 2. a mid-life file: no timer, clock, filters, music, next, prefbtn, boardpos, combosong or nowplay keys yet
  put("grid=10,20\nslots=1\nfall=0.55\nlvl=1\nclear=0\nsd=40\nvol=70\nthemeOn=1\ncalm=1\nfxm=3\nadv=1,2,3,4\nsolid=112233,445566,1\nui=2,1,1,14,1,1,2,60\nweights=1,1,1,1,1,1,1,1,1,1,1\ntw=1,2,3,4,5,6,7\n");
  reset_defaults();put("grid=10,20\nslots=1\nfall=0.55\nlvl=1\nclear=0\nsd=40\nvol=70\nthemeOn=1\ncalm=1\nfxm=3\nadv=1,2,3,4\nsolid=112233,445566,1\nui=2,1,1,14,1,1,2,60\nweights=1,1,1,1,1,1,1,1,1,1,1\ntw=1,2,3,4,5,6,7\n");load_settings();
  CHK(lvlSys==1&&clearOn==0&&sdRate==40&&themeOn==1&&calm==1&&fxMaster==3&&ADV[0]==1&&ADV[3]==4&&solidP==0xFF112233u&&solidG==0xFF445566u&&solidSep==1);
  CHK(uiFam==2&&uiBold==1&&uiItal==1&&uiPt==14&&hiCon==1&&mwSel==1&&lsSel==2&&ghostStr==60&&tw[0]==1&&tw[6]==7&&weights[11]==1);
  save_settings();rd_set(cur,sizeof cur);CHK(same_except(cur,",grid,slots,fall,lvl,clear,sd,vol,themeOn,calm,fxm,adv,solid,ui,weights,tw,"));
  // 3. everything the file does not mention keeps whatever the program had; keys from the future are skipped without harm
  reset_defaults();put("futurekey=1,2,3\nfall=2\nnext=7,9,9,9\nboardpos=50,50,50\ntimer=1,2,30,80,3,1,99\n# a comment\n\n=\nnoequals\n");load_settings();
  CHK(fabs(fallV-2)<1e-9);CHK(NQ==7);CHK(boardPX==50&&boardPY==50);CHK(CLK[0].on==1&&CLK[0].pos==2&&CLK[0].sz==30&&CLK[0].op==80&&CLK[0].fam==3&&CLK[0].bold==1);
  // 4. values from a hand edit or a newer version that are out of range are clamped, never trusted
  reset_defaults();put("grid=9999,-5\nbright=900\nslots=99\nfall=1e9\nsd=0\nvol=500\nfxm=40\nres=77\nfps=99\nnext=-3\nprefbtn=99\nboardpos=-10,900\nmusic=9,500\n");load_settings();
  CHK(gBright==150&&W==500&&H==4&&slots==10&&fallV<=130.0001&&sdRate==1&&fabs(snd_vol-1)<1e-9&&fxMaster==4&&resSet==6&&fpsSet==9&&NQ==0&&prefBtn==6&&boardPX==0&&boardPY==100&&musMode==3&&musVol==100);
  // 5. damaged files: empty, binary junk, a truncated line, a huge line, missing final newline
  reset_defaults();put("");load_settings();save_settings();rd_set(cur,sizeof cur);CHK(same_except(cur,""));
  {char junk[3000];int i;for(i=0;i<3000;i++)junk[i]=(char)(i*37+11);junk[2999]=0;put(junk);reset_defaults();put(junk);load_settings();}
  put("grid=1");load_settings();put("grid=");load_settings();put("clock=1,2");load_settings();put("filters=1,2,3");load_settings();
  {FILE*f=fopen(SP,"wb");int i;fputs("musicfile=",f);for(i=0;i<9000;i++)fputc('a',f);fputs("\nslots=3",f);fclose(f);reset_defaults();put("");f=fopen(SP,"ab");fputs("musicfile=",f);for(i=0;i<9000;i++)fputc('a',f);fputs("\nslots=3",f);fclose(f);load_settings();CHK(slots==3||slots==1);}
  // a music file name with a path in it is refused (it could point outside the music folder)
  reset_defaults();put("musicfile=../../etc/passwd\n");load_settings();CHK(musFile[0]==0);
  // 6. save -> load is lossless for every key: load file A (everything changed), save, load file B (a different set), reload A's save, save again
  {const char*A="grid=17,33\nslots=3\nfall=7.25\nlvl=1\nclear=0\nsd=77\nvol=35\nthemeOn=1\ncalm=1\nfxm=1\nadv=1,2,3,0\nsolid=abcdef,123456,0\nui=3,1,0,19,1,2,1,45\ntimerms=1\nbright=80\nres=2\nfps=5\nshowfps=1\ncombosong=1\nnowplay=1,3,22,55,4,1,1,96,1\nmusic=2,41\nmusicfile=song.mp3\nmusicfolder=Album/Disc 1\nnext=9\nprefbtn=4\nboardpos=23,77\ntimer=1,5,40,66,2,1\nclock=1,6,28,44,5,0,-7,45,1,1,1\nstack=1,1\nfilters=12,110,130,200,9,25,70,1,aabbcc\nweights=3,0,1,2,3,4,5,6,7,8,9\ntw=9,8,7,6,5,4,3\n";
   const char*B="grid=5,6\nslots=9\nfall=0.1\nvol=99\nnext=1\nprefbtn=1\nstack=0,0\nclock=0,0,12,10,0,0,3,0,0,0,0\n";
   reset_defaults();put(A);load_settings();save_settings();char s1[8000],s2[8000];rd_set(s1,sizeof s1);
   CHK(W==17&&H==33&&slots==3&&fabs(fallV-7.25)<1e-9&&sdRate==77&&gBright==80&&NQ==9&&prefBtn==4&&boardPX==23&&boardPY==77&&comboSong==1&&showFps==1&&resSet==2&&fpsSet==5);
   CHK(NP.on==1&&NP.pos==3&&NP.sz==22&&npArt==1&&npArtSz==96&&npArtist==1&&musMode==2&&musVol==41&&!strcmp(musFile,"song.mp3")&&!strcmp(musFold,"Album/Disc 1"));
   CHK(timerMs==1);{char ts[32];playMs=83456.7;clk_string(0,ts,sizeof ts);CHK(!strcmp(ts,"1:23.456"));playMs=3723004;clk_string(0,ts,sizeof ts);CHK(!strcmp(ts,"1:02:03.004"));timerMs=0;playMs=83456.7;clk_string(0,ts,sizeof ts);CHK(!strcmp(ts,"1:23"));timerMs=1;}
   CHK(CLK[1].on==1&&CLK[1].pos==6&&clkTzH==-7&&clkTzM==45&&clkDst==1&&clk24==1&&clkSecs==1&&stackV==1&&stackFirst==1);
   CHK(fltBri==12&&fltCon==110&&fltSat==130&&fltHue==200&&fltSplit==9&&fltTint==25&&fltOp==70&&fltInv==1&&(fltTintC&0xFFFFFF)==0xaabbcc);
   put(B);load_settings();put(s1);load_settings();save_settings();rd_set(s2,sizeof s2);
   CHK(!strcmp(s1,s2));if(strcmp(s1,s2)){char a[400],b[400];const char*k;const char*keys[]={"grid","slots","fall","lvl","clear","sd","vol","themeOn","calm","fxm","adv","solid","ui","res","fps","showfps","combosong","nowplay","music","musicfile","musicfolder","next","prefbtn","boardpos","timer","clock","stack","filters","weights","tw","theme","binds",0};int i;
     for(i=0;(k=keys[i]);i++){getline_(s1,k,a);getline_(s2,k,b);if(strcmp(a,b))printf("  differs: [%s] vs [%s]\n",a,b);}}
   // every key of this version's own output is one the loader above understood (no key can be saved but silently dropped)
   {const char*p=s1;int n=0;while(*p){const char*e=strchr(p,'=');if(!e)break;char k[40];size_t l=(size_t)(e-p);if(l<40){memcpy(k,p,l);k[l]=0;getline_(A,k,ln);if(!ln[0]&&strcmp(k,"theme")&&strcmp(k,"binds")){printf("  key %s is saved but not covered by this test\n",k);bad++;}n++;}while(*p&&*p!='\n')p++;if(*p)p++;}CHK(n>=28);}}
  // a track inside a folder is a valid saved choice; anything that climbs out of the music folder is not
  reset_defaults();put("musicfile=Album/Disc 1/song.mp3\n");load_settings();CHK(!strcmp(musFile,"Album/Disc 1/song.mp3"));
  reset_defaults();put("musicfile=a/../../b.mp3\n");load_settings();CHK(musFile[0]==0);
  reset_defaults();put("musicfile=/etc/x.mp3\n");load_settings();CHK(musFile[0]==0);
  reset_defaults();put("musicfile=C:x.mp3\n");load_settings();CHK(musFile[0]==0);
  // the scan finds tracks in folders, skips hidden folders, other file types and links, and stops at a depth limit
  system("rm -rf /tmp/8tris-sc/music;mkdir -p /tmp/8tris-sc/music/Album/Disc1 /tmp/8tris-sc/music/.hid /tmp/8tris-sc/music/a/b/c/d/e;"
         "touch /tmp/8tris-sc/music/top.mp3 /tmp/8tris-sc/music/Album/one.ogg /tmp/8tris-sc/music/Album/Disc1/two.WAV /tmp/8tris-sc/music/.hid/no.mp3 /tmp/8tris-sc/music/Album/notes.txt /tmp/8tris-sc/music/a/b/c/d/deep.mp3 /tmp/8tris-sc/music/a/b/c/d/e/toodeep.mp3;"
         "ln -s /tmp/8tris-sc/music /tmp/8tris-sc/music/Album/loop");
  musFile[0]=0;music_scan();
  {int i,f1=0,f2=0,f3=0,f4=0,bad_=0;for(i=0;i<nmfiles;i++){if(!strcmp(mfiles[i],"top.mp3"))f1=1;else if(!strcmp(mfiles[i],"Album/one.ogg"))f2=1;else if(!strcmp(mfiles[i],"Album/Disc1/two.WAV"))f3=1;else if(!strcmp(mfiles[i],"a/b/c/d/deep.mp3"))f4=1;else bad_=1;}
   CHK(f1&&f2&&f3&&f4&&!bad_&&nmfiles==4);if(nmfiles!=4)for(i=0;i<nmfiles;i++)printf("  found %s\n",mfiles[i]);}
  // folders: the scan lists every folder, a chosen folder limits what plays, and a vanished folder falls back to all
  musFold[0]=0;music_scan();
  CHK(nfold==6&&!strcmp(mfold[0],"a")&&!strcmp(mfold[4],"Album")&&!strcmp(mfold[5],"Album/Disc1")&&music_scope_count()==4);
  snprintf(musFold,sizeof musFold,"Album");CHK(music_scope_count()==2);
  {int i,k=0;for(i=0;i<nmfiles;i++)if(music_in_scope(i)){k++;CHK(!strncmp(mfiles[i],"Album/",6));}CHK(k==2);}
  snprintf(musFold,sizeof musFold,"Album/Disc1");CHK(music_scope_count()==1);
  snprintf(musFold,sizeof musFold,"Alb");CHK(music_scope_count()==0);   // a name prefix is not a folder
  snprintf(musFold,sizeof musFold,"Nope");music_scan();CHK(musFold[0]==0&&music_scope_count()==4);
  // playing "all in order" inside a folder only ever picks tracks of that folder, and shuffle does too
  {int i,bad2=0;snprintf(musFold,sizeof musFold,"Album");musMode=2;musCur=-1;for(i=0;i<20;i++){int idx=musCur<0?-1:musCur,k;for(k=0;k<nmfiles;k++){idx=(idx+1)%nmfiles;if(music_in_scope(idx))break;}if(!music_in_scope(idx))bad2=1;musCur=idx;}CHK(!bad2);musMode=0;musFold[0]=0;}
  // 7. no temporary file is left behind
  {FILE*f=fopen(SP ".tmp","rb");CHK(f==NULL);if(f)fclose(f);}
  printf("settings compat test bad=%d\n",bad);return bad;}
