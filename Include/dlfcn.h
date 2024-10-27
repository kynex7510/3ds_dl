#ifndef _CTRDL_DLFCN_H
#define _CTRDL_DLFCN_H

#include <3ds.h>
#include <sys/types.h>

#define RTLD_LOCAL 0x0000
#define RTLD_NOW 0x0002
#define RTLD_NOLOAD 0x0004
#define RTLD_GLOBAL 0x0100

typedef void*(*CTRDLSymResolver)(const char* sym, void* userData);

typedef struct {
    const char* dli_fname; // Object path.
    void* dli_fbase;       // Object base address.
    const char* dli_sname; // Symbol which overlaps the address.
    void* dli_saddr;       // Actual address for the symbol.
} Dl_info;

typedef struct {
    const char* path; // ObjectPath.
    u32 base;         // Object base address.
    size_t size;      // Object size.
} CTRDLExtInfo;

#if defined(__cplusplus)
extern "C" {
#endif

void* dlopen(const char* path, int flags);
int dlclose(void* handle);
void* dlsym(void* handle, const char* symbol);
int dladdr(const void* addr, Dl_info* info);
const char* dlerror(void);

void* ctrdlOpen(const char* path, int flags, CTRDLSymResolver resolver, void* userData);
bool ctrdlExtInfo(void* handle, CTRDLExtInfo* info);

#if defined(__cplusplus)
}
#endif

#endif /* _CTRDL_DLFCN_H */