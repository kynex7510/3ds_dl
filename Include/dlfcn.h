#ifndef _3DS_DLFCN_H
#define _3DS_DLFCN_H

#include <sys/types.h>

// Flags

#define RTLD_LOCAL 0x0000
#define RTLD_NOW 0x0002
#define RTLD_NOLOAD 0x0004
#define RTLD_GLOBAL 0x0100

// Types

typedef struct {
  // Object path name
  const char *dli_fname;
  // Object base address
  void *dli_fbase;
  // Symbol that overlap the address
  const char *dli_sname;
  // Actual address of the symbol
  void *dli_saddr;
  // Extension: object entrypoint
  void* dli_fep;
} Dl_info;

// Standard API

#if defined(__cplusplus)
extern "C" {
#endif

/*
    dlopen() loads the specified module and returns an opaque
    handle referencing it. Parameter filename must be a non-null,
    absolute path. Dependencies are searched in the same folder,
    unless already loaded.
    dlopen() may be called many times, the return value will be
    the same and the reference counter will be incremented.
    Returns the handle on success, NULL on failure.
*/
void *dlopen(const char *filename, int flags);

/*
    Decrement the reference count of a previously loaded library.
    When it reaches 0, the library is unloaded.
    Returns 0 on success, nonzero on failure.
*/
int dlclose(void *handle);

/*
    Lookup the given symbol in the given library, and return its
    address if found. Return NULL if not found. The value of the
    symbol could effectively be NULL, so call dlerror() to check
    whether the function failed.
*/
void *dlsym(void *handle, const char *symbol);

/*
    If the specified address is part of a library, returns various
    informations about the library. On failure, returns 0.
*/
int dladdr(const void *addr, Dl_info *info);

/*
    When an error has occurred, dlerror() returns an human-readable
    string describing the last error. The error is then cleared.
    When no error, return NULL.
*/
const char *dlerror(void);

// Extensions

/*
    This is like dlopen(), but it takes a callback as an additional
    parameter, which can be used to resolve references. Unresolved
    dependencies won't cause an error, but the callback must provide
    a valid (ie. non-null) address for each unresolved reference.
*/
void *dlopen_ex(const char *filename, void *(*cb)(char const *symbol),
                int flags);

/*
    This is like dlopen(), but a buffer is specified instead of the
    library path.
*/
void *dlmap(const unsigned char *buffer, const size_t size, int flags);

/*
    This is a combination of dlopen_ex() and dlmap().
*/
void *dlmap_ex(const unsigned char *buffer, const size_t size,
               void *(*cb)(char const *symbol), int flags);

#if defined(__cplusplus)
}
#endif

#endif /* _3DS_DLFCN_H */