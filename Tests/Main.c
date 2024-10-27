
#include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*DoMathFn)(int, int);

static void stub() {}

static void *resolver(const char *sym, void* unused) {
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

    void* h = ctrdlOpen("sdmc:/Math.so", RTLD_NOW, resolver, NULL);
    if (!h)
        goto fail;

    DoMathFn doMath = (DoMathFn)dlsym(h, "doMath");
    if (!doMath)
        goto fail;

    printf("Address of doMath(): 0x%08x\n", doMath);  
    //doMath(rand() % 14492, rand() % 9572);

    if (dlclose(h))
        goto fail;

    goto end;

fail:
    printf("ERROR: %s.\n", dlerror());

end:
    while (aptMainLoop()) {
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;
    }

    gfxExit();
    return 0;
}