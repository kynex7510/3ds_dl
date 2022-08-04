#include <3ds/synchronization.h>

#include "dlfcn.h"
#include "error.h"
#include "handle.h"
#include "init.h"
#include "loader.h"

#include <limits.h>
#include <string.h>

#define MAX_HANDLES 16

// Globals

static struct DLHandle g_Handles[MAX_HANDLES];
static LightLock g_Mtx;

// Helpers

static int incrementRefCount(struct DLHandle *handle) {
  if (handle->refc < UINT_MAX) {
    ++handle->refc;
    return 1;
  }

  return 0;
}

static int decrementRefCount(struct DLHandle *handle) {
  if (handle->refc)
    --handle->refc;

  return !handle->refc;
}

// Handle

void ctrdl_initHandleManager() {
  memset(&g_Handles, 0x00, sizeof(struct DLHandle) * MAX_HANDLES);
  LightLock_Init(&g_Mtx);
}

struct DLHandle *ctrdl_findHandle(const char *name) {
  struct DLHandle *found = NULL;

  if (!name) {
    ctrdl_setLastError(ERR_INVALID_PARAM);
    return NULL;
  }

  LightLock_Lock(&g_Mtx);

  // Look for handle.
  for (size_t i = 0; i < MAX_HANDLES; ++i) {
    struct DLHandle *h = &g_Handles[i];

    if (h->base && h->path[0] != 0x00 && strstr(h->path, name)) {
      found = h;
      break;
    }
  }

  // Increment the refcount.
  if (found) {
    if (!incrementRefCount(found)) {
      ctrdl_setLastError(ERR_REF_LIMIT);
      found = NULL;
    }
  } else {
    ctrdl_setLastError(ERR_NOT_FOUND);
  }

  LightLock_Unlock(&g_Mtx);
  return found;
}

struct DLHandle *ctrdl_createHandle(const char *path, const uint32_t flags) {
  struct DLHandle *handle = NULL;

  LightLock_Lock(&g_Mtx);

  // Look for free handle.
  for (size_t i = 0; i < MAX_HANDLES; ++i) {
    struct DLHandle *h = &g_Handles[i];

    if (!h->base) {
      handle = h;
      break;
    }
  }

  // If we have a handle, initialize it.
  if (handle) {
    if (path)
      memcpy(handle->path, path, strlen(path));
    handle->flags = flags;
    handle->refc = 1u;
  } else {
    ctrdl_setLastError(ERR_HANDLE_LIMIT);
  }

  LightLock_Unlock(&g_Mtx);
  return handle;
}

int ctrdl_freeHandle(struct DLHandle *handle) {
  int ret = 1;

  if (!handle) {
    ctrdl_setLastError(ERR_INVALID_PARAM);
    return 0;
  }

  LightLock_Lock(&g_Mtx);

  if (decrementRefCount(handle)) {
    LightLock_Unlock(&g_Mtx);
    ret = ctrdl_unloadObject(handle);
    LightLock_Lock(&g_Mtx);
    memset(handle, 0x00, sizeof(struct DLHandle));
  }

  LightLock_Unlock(&g_Mtx);
  return ret;
}

// API

int dlclose(void *handle) {
  if (ctrdl_isInit()) {
    return !ctrdl_freeHandle((struct DLHandle *)handle);
  }

  return 1;
}

void *dlsym(void *handle, const char *symbol) {
  return (void *)(((struct DLHandle *)(handle))->base + 0x7D8);
}

int dladdr(const void *addr, Dl_info *info) {
  // We dont have to provide error infos for this function.
  const uintptr_t address = (uintptr_t)(addr);

  if (!ctrdl_isInit() || !info)
    return 0;

  LightLock_Lock(&g_Mtx);

  // Look for object.
  for (size_t i = 0; i < MAX_HANDLES; ++i) {
    struct DLHandle *h = &g_Handles[i];
    if (address >= h->base && address < (h->base + h->size)) {
      info->dli_fname = h->path;
      info->dli_fbase = (void *)(h->base);
      info->dli_sname = NULL; // TODO: fill
      info->dli_saddr = NULL; // TODO: fill
      info->dli_fep = (void *)(h->ep);
      break;
    }
  }

  LightLock_Unlock(&g_Mtx);
  return info->dli_fbase != NULL;
}