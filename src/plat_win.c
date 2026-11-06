// Windows window, input and audio through plain Win32 (no runtime besides the system DLLs).
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include "8tris.c"
static CRITICAL_SECTION pcs;static int pcs_ready=0;
void plat_lock(void){if(pcs_ready)EnterCriticalSection(&pcs);}
void plat_unlock(void){if(pcs_ready)LeaveCriticalSection(&pcs);}
static HWND hwnd;static int running=1,is_full=0;static WINDOWPLACEMENT wp_saved;static LONG style_saved;
void plat_quit(void){running=0;}
void plat_fullscreen(int on){
  if(on&&!is_full){MONITORINFO mi={sizeof mi};style_saved=GetWindowLong(hwnd,GWL_STYLE);wp_saved.length=sizeof wp_saved;GetWindowPlacement(hwnd,&wp_saved);
    GetMonitorInfo(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&mi);SetWindowLong(hwnd,GWL_STYLE,style_saved&~WS_OVERLAPPEDWINDOW);
    SetWindowPos(hwnd,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_NOOWNERZORDER|SWP_FRAMECHANGED);is_full=1;}
  else if(!on&&is_full){SetWindowLong(hwnd,GWL_STYLE,style_saved);SetWindowPlacement(hwnd,&wp_saved);SetWindowPos(hwnd,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);is_full=0;}
}
typedef struct{void(*fn)(void*);void*arg;}ThArg;
static DWORD WINAPI thread_tramp(LPVOID p){ThArg a=*(ThArg*)p;free(p);a.fn(a.arg);return 0;}
int plat_thread(void(*fn)(void*),void*arg){ThArg*a=(ThArg*)malloc(sizeof*a);if(!a)return 0;a->fn=fn;a->arg=arg;HANDLE h=CreateThread(NULL,0,thread_tramp,a,0,NULL);if(!h){free(a);return 0;}CloseHandle(h);return 1;}
void plat_sleep(int ms){Sleep((DWORD)ms);}
void plat_window_size(int w,int h){if(is_full||!hwnd)return;RECT r={0,0,w,h};AdjustWindowRect(&r,(DWORD)GetWindowLong(hwnd,GWL_STYLE),FALSE);SetWindowPos(hwnd,NULL,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);}
static char prefdir[600],homedir[600];
// Preferences, history and the music folder live in %LOCALAPPDATA%\8tris: the local (not roaming) profile folder, so on work or
// school networks they are not copied to a server. Versions up to 0.7.2 used %APPDATA%\8tris; that folder is moved here once.
static void move_old_folder(const char*newdir){
  const char*ra=getenv("APPDATA");if(!ra||!*ra)return;char old[600];snprintf(old,sizeof old,"%s\\8tris",ra);
  DWORD oa=GetFileAttributesA(old);if(oa==INVALID_FILE_ATTRIBUTES||!(oa&FILE_ATTRIBUTE_DIRECTORY))return;
  char nd[600];snprintf(nd,sizeof nd,"%s",newdir);size_t L=strlen(nd);if(L&&nd[L-1]=='\\')nd[L-1]=0;
  if(GetFileAttributesA(nd)!=INVALID_FILE_ATTRIBUTES)return;              // already have a local folder: leave both alone
  if(MoveFileExA(old,nd,0))return;                                      // same drive: the whole folder moves at once
  // Different drives: copy the files one by one, then remove the originals that copied cleanly.
  if(!CreateDirectoryA(nd,NULL))return;
  {static const char*F[]={"settings.txt","history.txt","history.bin"};int i;for(i=0;i<3;i++){char a[700],b[700];snprintf(a,sizeof a,"%s\\%s",old,F[i]);snprintf(b,sizeof b,"%s\\%s",nd,F[i]);if(CopyFileA(a,b,TRUE))DeleteFileA(a);}}
  {char om[700],nm[700];snprintf(om,sizeof om,"%s\\music",old);snprintf(nm,sizeof nm,"%s\\music",nd);
   if(CreateDirectoryA(nm,NULL)){char pat[720];snprintf(pat,sizeof pat,"%s\\*",om);WIN32_FIND_DATAA fd;HANDLE h=FindFirstFileA(pat,&fd);
     if(h!=INVALID_HANDLE_VALUE){do{if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){char a[1000],b[1000];snprintf(a,sizeof a,"%s\\%s",om,fd.cFileName);snprintf(b,sizeof b,"%s\\%s",nm,fd.cFileName);if(CopyFileA(a,b,TRUE))DeleteFileA(a);}}while(FindNextFileA(h,&fd));FindClose(h);}
     RemoveDirectoryA(om);}}
  RemoveDirectoryA(old);   // only succeeds if everything was moved
}
const char* plat_pref_dir(void){
  if(!prefdir[0]){const char*a=getenv("LOCALAPPDATA");if(!a||!*a)a=getenv("APPDATA");char d[560];snprintf(d,sizeof d,"%s",a&&*a?a:".");CreateDirectoryA(d,NULL);
    snprintf(prefdir,sizeof prefdir,"%s\\8tris\\",d);move_old_folder(prefdir);CreateDirectoryA(prefdir,NULL);}
  return prefdir;
}
const char* plat_home_dir(void){if(!homedir[0]){const char*h=getenv("USERPROFILE");snprintf(homedir,sizeof homedir,"%s\\",h&&*h?h:".");}return homedir;}

// ---- presenting the frame: OpenGL scales the game's pixel buffer to the window on the GPU (and does vsync); plain GDI is the fallback ----
static int pw_=900,ph_=700,use_gl=0,texW=0,texH=0;static HDC hdc;static HGLRC glrc;static GLuint gltex;
static int dbg_on=0;static void dbg(const char*m){if(dbg_on){fprintf(stderr,"%s\n",m);fflush(stderr);}}
typedef BOOL(WINAPI*SWAPFN)(int);static SWAPFN swapi;
#define GL_BGRA_ 0x80E1
static int gl_init(void){
  PIXELFORMATDESCRIPTOR pfd;memset(&pfd,0,sizeof pfd);pfd.nSize=sizeof pfd;pfd.nVersion=1;pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;pfd.iLayerType=PFD_MAIN_PLANE;
  hdc=GetDC(hwnd);int pf=ChoosePixelFormat(hdc,&pfd);if(!pf||!SetPixelFormat(hdc,pf,&pfd)){dbg("gl: no pixel format");return 0;}
  glrc=wglCreateContext(hdc);if(!glrc||!wglMakeCurrent(hdc,glrc)){dbg("gl: no context");return 0;}
  const char*ver=(const char*)glGetString(GL_VERSION);if(dbg_on)fprintf(stderr,"gl: version %s\n",ver?ver:"?");if(!ver||atoi(ver)<2)return 0;   // needs non-power-of-two textures and BGRA
  swapi=(SWAPFN)wglGetProcAddress("wglSwapIntervalEXT");
  glGenTextures(1,&gltex);glBindTexture(GL_TEXTURE_2D,gltex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);   // clamp to edge
  return 1;
}
static void present(u32*buf,int iw,int ih){
  if(use_gl){
    glBindTexture(GL_TEXTURE_2D,gltex);
    if(iw!=texW||ih!=texH){glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,iw,ih,0,GL_BGRA_,GL_UNSIGNED_BYTE,buf);texW=iw;texH=ih;}
    else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,iw,ih,GL_BGRA_,GL_UNSIGNED_BYTE,buf);
    {GLenum er=glGetError();if(er!=GL_NO_ERROR){if(dbg_on)fprintf(stderr,"gl: error 0x%x, falling back to GDI\n",er);use_gl=0;return;}}
    glViewport(0,0,pw_,ph_);glDisable(GL_DEPTH_TEST);glEnable(GL_TEXTURE_2D);glColor3f(1,1,1);
    glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(-1,1);glTexCoord2f(1,0);glVertex2f(1,1);glTexCoord2f(1,1);glVertex2f(1,-1);glTexCoord2f(0,1);glVertex2f(-1,-1);glEnd();
    SwapBuffers(hdc);
  }else{
    BITMAPINFO bi;memset(&bi,0,sizeof bi);bi.bmiHeader.biSize=sizeof bi.bmiHeader;bi.bmiHeader.biWidth=iw;bi.bmiHeader.biHeight=-ih;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
    HDC dc=GetDC(hwnd);SetStretchBltMode(dc,COLORONCOLOR);StretchDIBits(dc,0,0,pw_,ph_,0,0,iw,ih,buf,&bi,DIB_RGB_COLORS,SRCCOPY);ReleaseDC(hwnd,dc);
  }
}
static int map_vk(WPARAM vk){
  switch(vk){case VK_LEFT:return K_LEFT;case VK_RIGHT:return K_RIGHT;case VK_UP:return K_UP;case VK_DOWN:return K_DOWN;case VK_ESCAPE:return K_ESC;case VK_BACK:return K_BKSP;case VK_DELETE:return K_DEL;
    case VK_RETURN:return K_ENTER;case VK_TAB:return K_TAB;case VK_F11:return K_F11;case VK_PRIOR:return K_PGUP;case VK_NEXT:return K_PGDN;case VK_HOME:return K_HOME;case VK_END:return K_END;
    case VK_SHIFT:case VK_LSHIFT:case VK_RSHIFT:return K_SHIFT;case VK_CONTROL:case VK_LCONTROL:case VK_RCONTROL:return K_CTRL;case VK_MENU:case VK_LMENU:case VK_RMENU:return K_ALT;case VK_SPACE:return ' ';}
  if(vk>='A'&&vk<='Z')return (int)vk+32;if(vk>='0'&&vk<='9')return (int)vk;if(vk>=VK_NUMPAD0&&vk<=VK_NUMPAD9)return '0'+(int)(vk-VK_NUMPAD0);
  {UINT c=MapVirtualKeyA((UINT)vk,2)&0x7fffffff;if(c>32&&c<127){if(c>='A'&&c<='Z')c+=32;return (int)c;}}
  return 0;
}
static LRESULT CALLBACK wndproc(HWND h,UINT m,WPARAM w,LPARAM l){
  switch(m){
    case WM_CLOSE:running=0;return 0;
    case WM_SIZE:if(w!=SIZE_MINIMIZED&&LOWORD(l)>0&&HIWORD(l)>0){pw_=LOWORD(l);ph_=HIWORD(l);app_resize(pw_,ph_);}return 0;
    case WM_ERASEBKGND:return 1;
    case WM_KEYDOWN:case WM_SYSKEYDOWN:{if(m==WM_SYSKEYDOWN&&w==VK_F4)break;int k=map_vk(w);if(k)app_key(k,1,(l>>30)&1);return 0;}
    case WM_KEYUP:case WM_SYSKEYUP:{int k=map_vk(w);if(k)app_key(k,0,0);return 0;}
    case WM_MOUSEMOVE:app_mouse_move((short)LOWORD(l),(short)HIWORD(l));return 0;
    case WM_LBUTTONDOWN:SetCapture(h);app_mouse_button((short)LOWORD(l),(short)HIWORD(l),1);return 0;
    case WM_LBUTTONUP:ReleaseCapture();app_mouse_button((short)LOWORD(l),(short)HIWORD(l),0);return 0;
    case WM_MOUSEWHEEL:app_wheel((short)HIWORD(w)/WHEEL_DELTA);return 0;
    case WM_KILLFOCUS:app_focus_lost();return 0;
  }
  return DefWindowProcA(h,m,w,l);
}
// ---- audio: a thread keeps a few small waveOut buffers filled ----
#define ABUF 1024
static HWAVEOUT wo;static WAVEHDR wh[4];static short wbuf[4][ABUF*2];static HANDLE wev;
static DWORD WINAPI audio_thread(LPVOID p){
  (void)p;float f[ABUF*2];int i,k;
  SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);   // a busy frame must never starve the sound (that is heard as crackling)
  for(k=0;k<4;k++){memset(&wh[k],0,sizeof wh[k]);wh[k].lpData=(LPSTR)wbuf[k];wh[k].dwBufferLength=sizeof wbuf[k];waveOutPrepareHeader(wo,&wh[k],sizeof wh[k]);wh[k].dwFlags|=WHDR_DONE;}
  while(running){
    for(k=0;k<4;k++)if(wh[k].dwFlags&WHDR_DONE){app_audio(f,ABUF);for(i=0;i<ABUF*2;i++){float v=f[i];if(v>1)v=1;if(v<-1)v=-1;wbuf[k][i]=(short)(v*32767);}wh[k].dwFlags&=~WHDR_DONE;waveOutWrite(wo,&wh[k],sizeof wh[k]);}
    WaitForSingleObject(wev,20);
  }
  return 0;
}
int WINAPI WinMain(HINSTANCE hi,HINSTANCE hp,LPSTR cmd,int show){
  (void)hp;(void)show;int i;
  for(i=1;i<__argc;i++)if(!strcmp(__argv[i],"--selftest")){int r=app_selftest();fflush(stdout);return r;}
  (void)cmd;
  InitializeCriticalSection(&pcs);pcs_ready=1;
  {HMODULE u=LoadLibraryA("user32.dll");if(u){BOOL(WINAPI*f)(void)=(BOOL(WINAPI*)(void))GetProcAddress(u,"SetProcessDPIAware");if(f)f();}}
  timeBeginPeriod(1);
  WNDCLASSA wc;memset(&wc,0,sizeof wc);wc.lpfnWndProc=wndproc;wc.hInstance=hi;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.lpszClassName="8tris";wc.hIcon=LoadIcon(hi,MAKEINTRESOURCE(1));RegisterClassA(&wc);
  RECT r={0,0,900,700};AdjustWindowRect(&r,WS_OVERLAPPEDWINDOW,FALSE);
  hwnd=CreateWindowExA(0,"8tris","8tris",WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,NULL,NULL,hi,NULL);
  if(!hwnd)return 1;
  app_init();
  {RECT c;GetClientRect(hwnd,&c);pw_=c.right;ph_=c.bottom;app_resize(pw_,ph_);}
  {const char*d=getenv("EIGHTRIS_DEBUG");dbg_on=d&&*d;}
  use_gl=gl_init();
  for(i=1;i<__argc;i++)if(!strcmp(__argv[i],"--info")){int k,iw,ih;for(k=0;k<20;k++){u32*b=app_render(k*16.7,&iw,&ih);present(b,iw,ih);}printf("present path: %s, vsync control: %s\n",use_gl?"OpenGL":"GDI",swapi?"yes":"no");fflush(stdout);return 0;}
  wev=CreateEventA(NULL,FALSE,FALSE,NULL);
  {WAVEFORMATEX fx;memset(&fx,0,sizeof fx);fx.wFormatTag=WAVE_FORMAT_PCM;fx.nChannels=2;fx.nSamplesPerSec=44100;fx.wBitsPerSample=16;fx.nBlockAlign=4;fx.nAvgBytesPerSec=44100*4;
   if(waveOutOpen(&wo,WAVE_MAPPER,&fx,(DWORD_PTR)wev,0,CALLBACK_EVENT)==MMSYSERR_NOERROR)CreateThread(NULL,0,audio_thread,NULL,0,NULL);}
  LARGE_INTEGER fq,t0;QueryPerformanceFrequency(&fq);QueryPerformanceCounter(&t0);
  double next=0;int cur_fps=-99;
  while(running){
    MSG m;while(PeekMessageA(&m,NULL,0,0,PM_REMOVE)){if(m.message==WM_QUIT)running=0;TranslateMessage(&m);DispatchMessageA(&m);}
    if(!running)break;
    LARGE_INTEGER t;QueryPerformanceCounter(&t);double now=(double)(t.QuadPart-t0.QuadPart)*1000.0/fq.QuadPart;
    int iw,ih;u32*buf=app_render(now,&iw,&ih);
    int want=app_fps_target();
    if(want!=cur_fps){cur_fps=want;if(use_gl&&swapi)swapi(want==0?1:0);next=now;}
    present(buf,iw,ih);
    // With GL vsync the swap already waits. Otherwise sleep most of the gap and spin the rest.
    if(!(use_gl&&swapi&&want==0)&&want!=-1){
      double hz=want;if(want==0){DEVMODEA dm;memset(&dm,0,sizeof dm);dm.dmSize=sizeof dm;hz=(EnumDisplaySettingsA(NULL,ENUM_CURRENT_SETTINGS,&dm)&&dm.dmDisplayFrequency>1)?dm.dmDisplayFrequency:60;}
      next+=1000.0/hz;QueryPerformanceCounter(&t);double n=(double)(t.QuadPart-t0.QuadPart)*1000.0/fq.QuadPart;if(next<n-50)next=n;
      for(;;){QueryPerformanceCounter(&t);n=(double)(t.QuadPart-t0.QuadPart)*1000.0/fq.QuadPart;if(n>=next)break;if(next-n>2.5)Sleep((DWORD)(next-n-1.5));}
    }
  }
  app_shutdown();if(wo){waveOutReset(wo);}return 0;
}
