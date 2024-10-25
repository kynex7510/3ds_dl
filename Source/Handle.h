#ifndef _CTRDL_HANDLE_H
#define _CTRDL_HANDLE_H

#include <dlfcn.h>

#define CTRDL_MAX_HANDLES 16
#define CTRDL_MAX_DEPS 16
#define CTRDL_PATHBUF_SIZE 256

typedef struct {
    char path[CTRDL_PATHBUF_SIZE];     // Object path.
    u32 base;                          // Mirror address of mapped region.
    u32 origin;                        // Original address of mapped region.
    size_t size;                       // Size of mapped region.
    size_t refc;                       // Object refcount.
    size_t flags;                      // Object flags.
    CTRDLHandle* deps[CTRDL_MAX_DEPS]; // Object dependencies.
    u32 ep;                            // Object entrypoint.
    u32 fini;                          // Fini array address.
    size_t finiSize;                   // Fini array size.
} CTRDLHandle;

void ctrdl_acquireHandleMtx(void);
void ctrdl_releaseHandleMtx(void);

CTRDLHandle* ctrdl_createHandle(const char* path, size_t flags);
bool ctrdl_freeHandle(CTRDLHandle* handle);
CTRDLHandle* ctrdl_getHandleByIndex(size_t index);
CTRDLHandle* ctrdl_findHandleByName(const char* name);

#endif /* _CTRDL_HANDLE_H */