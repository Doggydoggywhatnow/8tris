#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-fz/";} const char* plat_home_dir(void){return "/tmp/8tris-fz/";}
static u32*fb;static double t=1000;
static unsigned long long rs=88172645463325252ULL;static unsigned frnd(void){rs^=rs<<13;rs^=rs>>7;rs^=rs<<17;return (unsigned)(rs>>11);}
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7+(frnd()%3==0?frnd()%400:0);app_frame(t,fb);}}
static const char*KEYS[]={"grid","slots","fall","lvl","clear","sd","vol","themeOn","timer","calm","fxm","solid","ui","adv","res","fps","showfps","weights","tw","theme","binds","bogus","clock","stack","filters","prefbtn","boardpos","next","music","musicfile","nowplay"};
static const char*VALS[]={"1,2,3,4,5,6,7,8,9,10,11","-1,-1,-1,-1,-1,-1,-1,-1,ffffff","../../etc/passwd","a/b.mp3","nan","inf","-1","0","99999999999","-99999999999","","abc","1,2,3","ffffff,zz,1","500,500","4,4","9,9,9,9,9,9,9,9,9,9,9,9","-5,-5","1e308","0x7fffffff","2147483648,2147483648"};
int main(int argc,char**argv){int seed=argc>1?atoi(argv[1]):1,iters=argc>2?atoi(argv[2]):20,k;rs^=(unsigned long long)seed*0x9E3779B97F4A7C15ULL;fb=calloc(4096*2200,4);
 for(k=0;k<iters;k++){
  system("rm -rf /tmp/8tris-fz;mkdir -p /tmp/8tris-fz");
  FILE*f=fopen("/tmp/8tris-fz/settings.txt","w");int n=frnd()%12,i;for(i=0;i<n;i++){const char*kk=KEYS[frnd()%31];if(frnd()%2)fprintf(f,"%s=%s\n",kk,VALS[frnd()%21]);else fprintf(f,"%s=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",kk,(int)frnd()%600-50,(int)frnd()%12-1,(int)frnd()%12,(int)frnd()%8,(int)frnd()%30,frnd()%3,frnd()%3,frnd()%101,frnd()%5,frnd()%7,frnd()%9);}fclose(f);
  f=fopen("/tmp/8tris-fz/history.bin","wb");if(f&&frnd()%2){for(i=0;i<(int)(frnd()%3000);i++)fputc(frnd(),f);}if(f)fclose(f);
  f=fopen("/tmp/8tris-fz/history.txt","w");if(f&&frnd()%2){int L=frnd()%40;for(i=0;i<L;i++){static const char*K[]={"t","why","score","W","H","fall","weights","tw","theme","keys","themeOn","fx","vol","x"};int m=frnd()%8,j;for(j=0;j<m;j++)fprintf(f,"%s=%s ",K[frnd()%14],VALS[frnd()%21]);if(frnd()%10==0)for(j=0;j<300;j++)fputc(33+frnd()%90,f);fputc('\n',f);}}if(f)fclose(f);
  app_init();int ws[]={160,320,800,1000,1920,3840,121},hs[]={120,200,600,720,1080,2160,90};
  app_resize(ws[frnd()%7],hs[frnd()%7]);run(5);
  int ev;for(ev=0;ev<400;ev++){
   int r=frnd()%10,x=frnd()%2000,y=frnd()%1200;
   if(r<3){static const int ks[]={'a','d','s','w',' ','z','x','c',27,13,8,9,37,38,39,40,'p','r','q',127,300,5000,-1,'1','#'};app_key(ks[frnd()%25],frnd()%2,frnd()%2);}
   else if(r<6){app_mouse_move(x,y);app_mouse_button(x,y,frnd()%2);}
   else if(r==6)app_wheel((int)frnd()%40-20);
   else if(r==7)app_resize(ws[frnd()%7]+(int)(frnd()%50),hs[frnd()%7]+(int)(frnd()%50));
   else if(r==8&&frnd()%20==0)app_focus_lost();
   else {float a[512];app_audio(a,128);}
   run(1+frnd()%3);
  }
  app_shutdown();
 }
 puts("fuzz done");return 0;}
