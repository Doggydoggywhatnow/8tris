// Linux window, input and audio through SDL2. SDL is only used for the window surface, key/mouse events and the audio device.
#include <SDL2/SDL.h>
#include "8tris.c"
static SDL_mutex*mtx;
void plat_lock(void){if(mtx)SDL_LockMutex(mtx);}
void plat_unlock(void){if(mtx)SDL_UnlockMutex(mtx);}
static SDL_Window*win;
void plat_fullscreen(int on){SDL_SetWindowFullscreen(win,on?SDL_WINDOW_FULLSCREEN_DESKTOP:0);}
static int running=1;
void plat_quit(void){running=0;}
static char prefdir[600],homedir[600];
const char* plat_pref_dir(void){
  if(!prefdir[0]){const char*x=getenv("XDG_DATA_HOME"),*h=getenv("HOME");char d[560];
    if(x&&*x)snprintf(d,sizeof d,"%s",x);else snprintf(d,sizeof d,"%s/.local/share",h?h:"/tmp");
    char a[600];snprintf(a,sizeof a,"%s",d);mkdir(a,0755);snprintf(prefdir,sizeof prefdir,"%s/8tris/",d);mkdir(prefdir,0700);
    chmod(prefdir,0700);}   // private to this user: other accounts on the same computer cannot read preferences, history or the music list
  return prefdir;
}
const char* plat_home_dir(void){if(!homedir[0]){const char*h=getenv("HOME");snprintf(homedir,sizeof homedir,"%s/",h?h:"/tmp");}return homedir;}
static int map_key(SDL_Keycode k){
  switch(k){case SDLK_LEFT:return K_LEFT;case SDLK_RIGHT:return K_RIGHT;case SDLK_UP:return K_UP;case SDLK_DOWN:return K_DOWN;case SDLK_ESCAPE:return K_ESC;
    case SDLK_BACKSPACE:return K_BKSP;case SDLK_DELETE:return K_DEL;case SDLK_RETURN:case SDLK_KP_ENTER:return K_ENTER;case SDLK_TAB:return K_TAB;case SDLK_F11:return K_F11;
    case SDLK_PAGEUP:return K_PGUP;case SDLK_PAGEDOWN:return K_PGDN;case SDLK_HOME:return K_HOME;case SDLK_END:return K_END;
    case SDLK_LSHIFT:case SDLK_RSHIFT:return K_SHIFT;case SDLK_LCTRL:case SDLK_RCTRL:return K_CTRL;case SDLK_LALT:case SDLK_RALT:return K_ALT;
    case SDLK_KP_0:return '0';case SDLK_KP_1:return '1';case SDLK_KP_2:return '2';case SDLK_KP_3:return '3';case SDLK_KP_4:return '4';case SDLK_KP_5:return '5';case SDLK_KP_6:return '6';case SDLK_KP_7:return '7';case SDLK_KP_8:return '8';case SDLK_KP_9:return '9';}
  if(k>=32&&k<127)return (int)k;return 0;
}
static void audio_cb(void*ud,Uint8*stream,int len){(void)ud;app_audio((float*)stream,len/8);}
static int thread_tramp(void*p){void**a=(void**)p;void(*fn)(void*)=(void(*)(void*))a[0];void*arg=a[1];free(a);fn(arg);return 0;}
int plat_thread(void(*fn)(void*),void*arg){void**a=(void**)malloc(2*sizeof(void*));if(!a)return 0;a[0]=(void*)fn;a[1]=arg;SDL_Thread*th=SDL_CreateThread(thread_tramp,"8tris-bg",a);if(!th){free(a);return 0;}SDL_DetachThread(th);return 1;}
void plat_sleep(int ms){SDL_Delay((Uint32)ms);}
void plat_window_size(int w,int h){if(win&&!(SDL_GetWindowFlags(win)&SDL_WINDOW_FULLSCREEN_DESKTOP))SDL_SetWindowSize(win,w,h);}
static double now_s(void){return (double)SDL_GetPerformanceCounter()/(double)SDL_GetPerformanceFrequency();}
int main(int argc,char**argv){
  int i;for(i=1;i<argc;i++){if(!strcmp(argv[i],"--selftest"))return app_selftest();if(!strcmp(argv[i],"--probe"))return 0;}   // --probe: the AppImage launcher checks that this binary can load with the system's libraries
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS)<0){fprintf(stderr,"8tris: SDL could not start: %s\n",SDL_GetError());return 1;}
  mtx=SDL_CreateMutex();
  win=SDL_CreateWindow("8tris",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,900,700,SDL_WINDOW_RESIZABLE|SDL_WINDOW_SHOWN);
  if(!win){fprintf(stderr,"8tris: could not open a window: %s\n",SDL_GetError());return 1;}
  // The game draws into its own pixel buffer; SDL's renderer (the GPU when there is one) scales it to the window and handles vsync.
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"linear");
  SDL_Renderer*ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);
  if(!ren)ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE);
  if(!ren){fprintf(stderr,"8tris: could not create a renderer: %s\n",SDL_GetError());return 1;}
  app_init();
  SDL_AudioSpec want,have;SDL_zero(want);want.freq=44100;want.format=AUDIO_F32SYS;want.channels=2;want.samples=1024;want.callback=audio_cb;
  SDL_AudioDeviceID dev=SDL_OpenAudioDevice(NULL,0,&want,&have,0);
  if(dev)SDL_PauseAudioDevice(dev,0);else fprintf(stderr,"8tris: no audio device (%s); continuing without sound\n",SDL_GetError());
  int w=900,h=700;SDL_GetWindowSize(win,&w,&h);app_resize(w,h);
  SDL_Texture*tex=NULL;int tw=0,th=0,cur_fps=-99,vs_ok=0;double t0=now_s(),next=t0;
  while(running){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
      switch(e.type){
        case SDL_QUIT:running=0;break;
        case SDL_KEYDOWN:case SDL_KEYUP:{int k=map_key(e.key.keysym.sym);if(k)app_key(k,e.type==SDL_KEYDOWN,e.key.repeat);break;}
        case SDL_MOUSEMOTION:app_mouse_move(e.motion.x,e.motion.y);break;
        case SDL_MOUSEBUTTONDOWN:if(e.button.button==SDL_BUTTON_LEFT)app_mouse_button(e.button.x,e.button.y,1);break;
        case SDL_MOUSEBUTTONUP:if(e.button.button==SDL_BUTTON_LEFT)app_mouse_button(e.button.x,e.button.y,0);break;
        case SDL_MOUSEWHEEL:app_wheel(e.wheel.y);break;
        case SDL_WINDOWEVENT:
          if(e.window.event==SDL_WINDOWEVENT_FOCUS_LOST)app_focus_lost();
          if(e.window.event==SDL_WINDOWEVENT_SIZE_CHANGED||e.window.event==SDL_WINDOWEVENT_RESIZED){SDL_GetWindowSize(win,&w,&h);app_resize(w,h);}
          break;
      }
    }
    int iw,ih;u32*buf=app_render((now_s()-t0)*1000.0,&iw,&ih);
    if(!tex||iw!=tw||ih!=th){if(tex)SDL_DestroyTexture(tex);tex=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,iw,ih);tw=iw;th=ih;}
    int want_fps=app_fps_target();
    if(want_fps!=cur_fps){cur_fps=want_fps;vs_ok=0;if(want_fps==0){vs_ok=SDL_RenderSetVSync(ren,1)==0;if(vs_ok){SDL_RendererInfo ri;if(SDL_GetRendererInfo(ren,&ri)==0&&!(ri.flags&SDL_RENDERER_PRESENTVSYNC))vs_ok=0;}}else SDL_RenderSetVSync(ren,0);next=now_s();}
    if(tex){SDL_UpdateTexture(tex,NULL,buf,iw*4);SDL_RenderClear(ren);SDL_RenderCopy(ren,tex,NULL,NULL);SDL_RenderPresent(ren);}
    // Pacing: with working vsync the present call already waits. Otherwise wait here: a sleep for most of the gap, then a short spin for accuracy.
    if(!(want_fps==0&&vs_ok)&&want_fps!=-1){
      double hz=want_fps;if(want_fps==0){SDL_DisplayMode dm;hz=(SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(win),&dm)==0&&dm.refresh_rate>0)?dm.refresh_rate:60;}
      next+=1.0/hz;double n=now_s();if(next<n-.05)next=n;
      while((n=now_s())<next){double left=next-n;if(left>.002)SDL_Delay((Uint32)((left-.001)*1000));}
    }else if(want_fps==-1)SDL_Delay(0);
  }
  app_shutdown();if(tex)SDL_DestroyTexture(tex);SDL_DestroyRenderer(ren);if(dev)SDL_CloseAudioDevice(dev);SDL_DestroyWindow(win);SDL_Quit();return 0;
}
