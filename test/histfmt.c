#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-hf/";} const char* plat_home_dir(void){return "/tmp/8tris-hf/";}
// History file format: migration from the old binary file, tolerance of unknown/missing keys and damaged lines, crash-safe writes.
static int bad=0;
#define CHK(c) do{if(!(c)){bad++;printf("FAIL line %d: %s\n",__LINE__,#c);}}while(0)
static int exists(const char*n){char p[300];snprintf(p,sizeof p,"/tmp/8tris-hf/%s",n);FILE*f=fopen(p,"rb");if(f){fclose(f);return 1;}return 0;}
int main(void){
  system("rm -rf /tmp/8tris-hf;mkdir -p /tmp/8tris-hf");
  // 1. an old binary history is converted, checked and removed
  {Hist h[3];memset(h,0,sizeof h);int i;for(i=0;i<3;i++){h[i].t=1767225600+i*60;h[i].score=1000*(i+1);h[i].lines=i;h[i].level=1;h[i].time=61+i;h[i].pieces=10+i;h[i].combo=i;
     h[i].s.W=10;h[i].s.H=20;h[i].s.slots=1;h[i].s.fall=0.55;h[i].s.weights[3]=1;int k;for(k=0;k<7;k++)h[i].s.tw[k]=1;h[i].s.theme[6]=TH_SOLID;h[i].s.themeOn=1;h[i].s.vol=50;h[i].s.keys[0][0]='a';h[i].s.keys[0][1]=K_LEFT;}
   FILE*f=fopen("/tmp/8tris-hf/history.bin","wb");unsigned m=0x38544831u,sz=sizeof(Hist);int n=3;fwrite(&m,4,1,f);fwrite(&sz,4,1,f);fwrite(&n,4,1,f);fwrite(h,sizeof(Hist),3,f);fclose(f);
   hist_load();CHK(nhist==3);CHK(exists("history.txt"));CHK(!exists("history.bin"));CHK(!exists("history.txt.tmp"));
   CHK(hist[0].score==1000&&hist[2].score==3000&&hist[1].time==62&&hist[0].s.theme[6]==TH_SOLID&&hist[0].s.keys[0][1]==K_LEFT&&fabs(hist[0].s.fall-.55)<1e-9&&hist[2].t==1767225720);
   nhist=0;hist_load();CHK(nhist==3);}   // and it reads back from text
  // 2. a binary file this version cannot read is left alone
  system("rm -f /tmp/8tris-hf/history.txt");
  {FILE*f=fopen("/tmp/8tris-hf/history.bin","wb");unsigned m=0x38544831u,sz=12345;int n=1;fwrite(&m,4,1,f);fwrite(&sz,4,1,f);fwrite(&n,4,1,f);fclose(f);
   hist_load();CHK(nhist==0);CHK(exists("history.bin"));system("rm -f /tmp/8tris-hf/history.bin");}
  // 3. unknown keys, missing keys, comments, garbage and an overlong line
  {FILE*f=fopen("/tmp/8tris-hf/history.txt","w");fprintf(f,"# comment\nt=100 score=5 future_key=7,8,9 lines=2\n\ngarbage line without equals\nscore=9\n");
   int i;fprintf(f,"t=200 score=1 ");for(i=0;i<5000;i++)fputc('x',f);fprintf(f,"\nt=300 score=77 theme=1,2,3 keys=1,2\n");fclose(f);
   hist_load();CHK(nhist==2);CHK(hist[0].score==5&&hist[0].lines==2&&hist[0].t==100);CHK(hist[1].score==77&&hist[1].s.theme[2]==3&&hist[1].s.keys[0][1]==2);
   char L[60][400];entry_lines(&hist[1],L,60);}   // must not crash on sparse entries
  // 4. finished runs are added and saved, newest first; nothing temporary is left behind
  {app_init();app_resize(800,600);int before=nhist;score=4321;pieces=5;over=1;saved=0;clearOn=1;endRun();CHK(nhist==before+1&&hist[0].score==4321);
   nhist=0;hist_load();CHK(nhist==before+1&&hist[0].score==4321);CHK(!exists("history.txt.tmp"));
   save_settings();CHK(exists("settings.txt")&&!exists("settings.txt.tmp"));}
  // 5. clearing history really empties the file
  {nhist=0;hist_save();nhist=9;hist_load();CHK(nhist==0);}
  printf("history format test bad=%d\n",bad);return bad;}
