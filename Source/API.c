#include "Handle.h"
#include "Error.h"
#include "Loader.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define RTLD_LAZY 0x0001
#define RTLD_DEEPBIND 0x0008
#define RTLD_NODELETE 0x1000

static bool ctrdl_checkFlags(int flags) {
    // Unsupported flags.
    if (flags & (RTLD_LAZY | RTLD_DEEPBIND | RTLD_NODELETE))
        return false;

    // Required flags.
    if (!(flags & RTLD_NOW))
        return false;

    return true;
}

void* dlopen(const char* path, int flags) { return ctrdlOpen(path, flags, NULL, NULL); }
int dlclose(void* handle) { return !ctrdl_freeHandle((CTRDLHandle*)handle); }
const char* dlerror(void) { return ctrdl_getErrorAsString(ctrdl_getLastError()); }

// We dont have to provide error values for this function.
int dladdr(const void* addr, Dl_info* info) {
    if (!info)
        return 0;

    ctrdl_acquireHandleMtx();

    // Look for object.
    const u32 address = (u32)addr;
    for (size_t i = 0; i < CTRDL_MAX_HANDLES; ++i) {
        CTRDLHandle* h = ctrdl_getHandleByIndex(i);
        if ((address >= h->base) && (address <= (h->base + h->size))) {
            info->dli_fname = h->path;
            info->dli_fbase = (void*)h->base;
            
            // TODO
            info->dli_sname = NULL;
            info->dli_saddr = NULL;

            break;
        }
    }

    ctrdl_releaseHandleMtx();
    return info->dli_fbase != NULL;
}

void* ctrdlOpen(const char* path, int flags, CTRDLSymResolver resolver, void* resolverUserData) {
    // We don't support the NULL pseudo handle.
    if (!path || !ctrdl_checkFlags(flags)) {
        ctrdl_setLastError(Err_InvalidParam);
        return NULL;
    }

    // Validate path length.
    if (strlen(path) >= CTRDL_PATHBUF_SIZE) {
        ctrdl_setLastError(Err_LargePath);
        return NULL;
    }

    // Avoid reading if already open.
    CTRDLHandle* handle = ctrdl_findHandleByName(path);
    if (handle) {
        // Update flags.
        handle->flags = flags;
        return (void*)handle;
    }

    if (flags & RTLD_NOLOAD) {
        ctrdl_setLastError(Err_NotFound);
        return NULL;
    }

    // Open file for reading.
    FILE* f = fopen(path, "rb");
    if (!f) {
        ctrdl_setLastError(Err_InvalidParam);
        return NULL;
    }

    CTRDLStream stream;
    ctrdl_makeFileStream(&stream, f);
    handle = ctrdl_loadObject(path, flags, &stream, resolver, resolverUserData);

    fclose(f);
    return handle;
}

bool ctrdlExtInfo(void* handle, CTRDLExtInfo* info) {
    if (!handle || !info) {
        ctrdl_setLastError(Err_InvalidParam);
        return false;
    }

    ctrdl_acquireHandleMtx();

    CTRDLHandle* h = (CTRDLHandle*)handle;
    info->path = h->path;
    info->base = h->base;
    info->size = h->size;

    ctrdl_releaseHandleMtx();
    return true;
}