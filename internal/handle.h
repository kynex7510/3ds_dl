#ifndef _3DS_DL_HANDLE_H
#define _3DS_DL_HANDLE_H

#include <stdint.h>

#define MAX_PATH 351
#define MAX_DEPS 32

struct DLHandle {
  char path[MAX_PATH + 1];
  uintptr_t deps[MAX_DEPS];
  uintptr_t base;
  uintptr_t origin;
  size_t size;
  uintptr_t ep;
  size_t flags;
  size_t refc;
  uintptr_t fini;
  size_t finiSize;
};

// Initialize handle manager (thread-unsafe).
void ctrdl_initHandleManager();

// Find a handle from the name, and increases the refcount.
struct DLHandle *ctrdl_findHandle(const char *name);

// Create a new handle entry.
struct DLHandle *ctrdl_createHandle(const char *path, const uint32_t flags);
int ctrdl_freeHandle(struct DLHandle *handle);

#endif /* _3DS_DL_HANDLE_H */