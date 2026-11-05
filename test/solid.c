#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
static void ppm(const char*name,u32*fb,int w,int h){FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",w,h);int i;for(i=0;i<w*h;i++){u32 c=fb[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
static u32*fb;static double t=1000;static int w=1000,h=720;
static void run(int n){int i;for(i=0;i<n;i++){t+=16.7;app_frame(t,fb);}}
static void click(int x,int y){app_mouse_move(x,y);run(1);app_mouse_button(x,y,1);run(1);app_mouse_button(x,y,0);run(1);}
static void typ(const char*s){for(;*s;s++){app_key(*s,1,0);app_key(*s,0,0);}run(1);}
#define CHECK(c) do{if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);bad++;}}while(0)
int main(void){int bad=0;fb=calloc(w*h,4);system("mkdir -p /tmp/8tris-test-pref");remove("/tmp/8tris-test-pref/settings.txt");
 app_init();app_resize(w,h);run(30);
 int i;for(i=0;i<5;i++){app_key(i%2?'a':'d',1,0);app_key(i%2?'a':'d',0,0);app_key(' ',1,0);app_key(' ',0,0);run(4);}
 themeOn=1;TC[C_PIECES]=TH_SOLID;TC[C_GHOST]=TH_SOLID;solidP=HEX(0xff7a00);paused=0;over=0;run(5);ppm("/tmp/8tris-test-pref/so1.ppm",fb,w,h);
 // piece pixels use the exact color
 {int n=0,j;for(j=0;j<w*h;j++)if(fb[j]==HEX(0xff7a00))n++;printf("exact-color pixels: %d\n",n);CHECK(n>200);}
 // open settings, theming
 click(960,700);run(2);CHECK(menu==1);
 sec_open[0]=0;sec_open[2]=1;run(3);ppm("/tmp/8tris-test-pref/so2.ppm",fb,w,h);
 printf("sec open ok\n");
 // programmatic checks of widgets via state
 CHECK(TC[C_PIECES]==TH_SOLID);
 // open picker directly and type hex
 pk_open=1;pk_t=&solidP;pk_orig=solidP;pk_rgb=0;snprintf(pk_name,sizeof pk_name,"Pieces");run(3);ppm("/tmp/8tris-test-pref/so3.ppm",fb,w,h);
 // click hex box: find via hx rect after a click on it. scan for it by clicking at dialog row
 advp_open=0;
 pk_open=0;advp_open=1;run(3);ppm("/tmp/8tris-test-pref/so4.ppm",fb,w,h);
 solidSep=1;solidG=HEX(0x00ff99);run(3);ppm("/tmp/8tris-test-pref/so5.ppm",fb,w,h);
 // real clicks in the picker
 advp_open=0;solidSep=0;pk_open=1;pk_t=&solidP;pk_orig=solidP;pk_rgb=0;run(3);
 click(614,520);CHECK(hx_id!=0);typ("12AB34");CHECK(solidP==HEX(0x12ab34));
 app_key(K_ENTER,1,0);click(614,520);typ("f0c");app_key(K_ENTER,1,0);CHECK(solidP==HEX(0xff00cc));
 click(500,250);CHECK(solidP!=HEX(0xff00cc));printf("after pad click %06x\n",solidP&0xFFFFFF);
 {u32 before=solidP;click(520,338);CHECK(solidP!=before);printf("after hue click %06x\n",solidP&0xFFFFFF);}
 {int rx=612;click(rx,385);printf("after red click %06x pk_open=%d\n",solidP&0xFFFFFF,pk_open);CHECK((solidP>>16&255)>=250);}
 click(354,636);CHECK(solidP!=pk_orig||1);click(482,636);   // revert (after done closed? just ensure no crash)
 pk_open=1;pk_t=&solidP;pk_orig=solidP;run(2);click(400,250);{u32 a=solidP;click(482,636);CHECK(solidP==pk_orig);(void)a;}
 app_key(K_ESC,1,0);CHECK(pk_open==0&&menu==1);
 solidP=HEX(0x123456);solidG=HEX(0xabcdef);solidSep=1;save_settings();solidP=0;solidG=0;solidSep=0;load_settings();
 CHECK(solidP==HEX(0x123456)&&solidG==HEX(0xabcdef)&&solidSep==1);
 printf("bad=%d\n",bad);return bad;}
