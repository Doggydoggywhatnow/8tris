#include "../src/8tris.c"
void plat_lock(void){} void plat_unlock(void){} void plat_fullscreen(int on){(void)on;} void plat_quit(void){} void plat_window_size(int w,int h){(void)w;(void)h;} int plat_thread(void(*fn)(void*),void*a){if(fn==sw_worker||fn==mus_stream_fn)return 0;fn(a);return 1;} void plat_sleep(int ms){(void)ms;}
const char* plat_pref_dir(void){return "/tmp/8tris-mm/";} const char* plat_home_dir(void){return "/tmp/8tris-mm/";}
int main(int argc,char**argv){int k;for(k=1;k<argc;k++){Meta m;meta_read(argv[k],&m);printf("%-22s title='%s' artist='%s' album='%s' art=%dx%d\n",strrchr(argv[k],'/')+1,m.title,m.artist,m.album,m.aw,m.ah);
 if(m.art&&k<=6){char nm[64];snprintf(nm,64,"/tmp/mm/art%d.ppm",k);FILE*f=fopen(nm,"wb");fprintf(f,"P6\n%d %d\n255\n",m.aw,m.ah);int i;for(i=0;i<m.aw*m.ah;i++){u32 c=m.art[i];fputc(c>>16,f);fputc(c>>8,f);fputc(c,f);}fclose(f);}meta_free(&m);}return 0;}
