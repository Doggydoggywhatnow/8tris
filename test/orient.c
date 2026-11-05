#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){(void)fn;(void)a;return 0;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-test-pref/";} const char* plat_home_dir(void){return "/tmp/8tris-test-pref/";}
#define CHECK(c) do{if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);bad++;}}while(0)
static Def*find(int tn){int i;for(i=0;i<nlists[4];i++)if(tname(lists[4][i])==tn)return lists[4][i];return NULL;}
// returns the shape as a 2x3 / 4x1 picture string
static void shape(Def*d,char*out){int br[MAXN],bc[MAXN],i,w=0,h=0;spawn_orient(d,br,bc);for(i=0;i<d->n;i++){if(bc[i]+1>w)w=bc[i]+1;if(br[i]+1>h)h=br[i]+1;}
  int y,x,k=0;for(y=0;y<h;y++){for(x=0;x<w;x++){int on=0;for(i=0;i<d->n;i++)if(br[i]==y&&bc[i]==x)on=1;out[k++]=on?'#':'.';}out[k++]='/';}out[k]=0;}
int main(void){int bad=0;char b[64];app_init();app_resize(800,600);
  const char*want[7]={"####/","##/##/","." "#./###/","","","",""};(void)want;
  struct{int t;const char*s;}ex[]={{0,"####/"},{1,"##/##/"},{2,".#./###/"},{3,".##/##./"},{4,"##./.##/"},{5,"#../###/"},{6,"..#/###/"}};
  int i;for(i=0;i<7;i++){Def*d=find(ex[i].t);CHECK(d!=NULL);if(!d)continue;shape(d,b);printf("%c: %s\n",TN[ex[i].t],b);CHECK(strcmp(b,ex[i].s)==0);
    // the spawned piece has the same shape
    Piece q=mk(d);int n=0,x,y;for(y=0;y<q.N;y++)for(x=0;x<q.N;x++)n+=q.m[y][x];CHECK(n==4);}
  printf("%s\n",bad?"FAILED":"all ok");return bad;}
