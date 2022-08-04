#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dlfcn.h"

typedef void (*do_math_t)(int, int);

static const char *PATH = "sdmc:/math.so";

static void stub() {}

static void *resolver(const char *sym) {
  if (!strcmp(sym, "puts"))
    return puts;

  if (!strcmp(sym, "printf"))
    return printf;

  return stub;
}

int main(int argc, char *argv[]) {
  gfxInitDefault();
  consoleInit(GFX_TOP, NULL);

  srand(time(NULL));

  void *h = dlopen_ex(PATH, resolver, RTLD_NOW);

  if (!h)
    goto fail;

  do_math_t do_math = (do_math_t)dlsym(h, "do_math");

  if (!do_math)
    goto fail;

  do_math(rand() % 14492, rand() % 9572);

  if (dlclose(h))
    goto fail;

  goto end;

fail:
  printf("ERROR: %s.\n", dlerror());

end:
  // Main loop
  while (aptMainLoop()) {
    gspWaitForVBlank();
    gfxSwapBuffers();
    hidScanInput();

    // Your code goes here
    u32 kDown = hidKeysDown();
    if (kDown & KEY_START)
      break; // break in order to return to hbmenu
  }

  gfxExit();
  return 0;
}
