// AppRun for the 8tris AppImage: a tiny static C program (no shell needed).
// It prefers the system's own SDL2 (it knows this desktop best, and it lets 8tris run on older systems) and falls back to
// the copy bundled inside the AppImage when the system has none or it is too old.
// Build: gcc -O2 -static -o AppRun appimage/launcher.c
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  char self[PATH_MAX], bin[PATH_MAX + 16], lib[PATH_MAX + 16];
  ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
  if (n <= 0) { fprintf(stderr, "8tris: cannot locate itself\n"); return 1; }
  self[n] = 0;
  char *slash = strrchr(self, '/');
  if (slash) *slash = 0;
  snprintf(bin, sizeof bin, "%s/usr/bin/8tris", self);
  snprintf(lib, sizeof lib, "%s/usr/lib", self);

  // Ask the game whether it can start with the libraries this system has (it exits 0 straight away if so).
  pid_t p = fork();
  if (p == 0) {
    int nul = open("/dev/null", O_RDWR);
    if (nul >= 0) { dup2(nul, 1); dup2(nul, 2); }
    setenv("LD_BIND_NOW", "1", 1);
    execl(bin, bin, "--probe", (char *)0);
    _exit(127);
  }
  int st = 0, ok = 0;
  if (p > 0 && waitpid(p, &st, 0) == p) ok = WIFEXITED(st) && WEXITSTATUS(st) == 0;

  if (!ok) {   // use the bundled SDL2
    const char *old = getenv("LD_LIBRARY_PATH");
    char *path = malloc(strlen(lib) + (old ? strlen(old) : 0) + 2);
    if (!path) return 1;
    strcpy(path, lib);
    if (old && *old) { strcat(path, ":"); strcat(path, old); }
    setenv("LD_LIBRARY_PATH", path, 1);
    free(path);
  }
  argv[0] = bin;
  execv(bin, argv);
  perror("8tris");
  return 1;
}
