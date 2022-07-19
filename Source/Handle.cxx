#include <3ds/synchronization.h>

#include "Error.hxx"
#include "Handle.hxx"
#include "Loader.hxx"
#include "dlfcn.h"

#include <algorithm>
#include <cstring>
#include <limits>

using namespace ctr;

// Types

class Mutex {
protected:
  LightLock m_Mtx;

public:
  Mutex() { LightLock_Init(&m_Mtx); }

  void lock() { LightLock_Lock(&m_Mtx); }

  void unlock() { LightLock_Unlock(&m_Mtx); }
};

// Globals

constexpr static auto MAX_HANDLES = 16u;

static DLHandle g_Handles[MAX_HANDLES] = {};
static Mutex g_Mtx;

// Helpers

static bool incrementRefCount(DLHandle &handle) {
  if (handle.refc < std::numeric_limits<std::uint32_t>::max()) {
    ++handle.refc;
    return true;
  }

  return false;
}

static bool decrementRefCount(DLHandle &handle) {
  if (handle.refc)
    --handle.refc;

  return !handle.refc;
}

// Handle

DLHandle *ctr::findHandle(const char *name) {
  DLHandle *found = nullptr;

  if (!name) {
    ctr::setLastError(ctr::ERR_INVALID_PARAM);
    return nullptr;
  }

  g_Mtx.lock();

  // Look for handle.
  for (auto i = 0u; i < MAX_HANDLES; ++i) {
    auto h = &g_Handles[i];

    if (h->base && h->path[0] != 0x00 && std::strstr(h->path, name)) {
      found = h;
      break;
    }
  }

  // Increment the refcount.
  if (found) {
    if (!incrementRefCount(*found)) {
      ctr::setLastError(ctr::ERR_REF_LIMIT);
      found = nullptr;
    }
  } else {
    ctr::setLastError(ctr::ERR_NOT_FOUND);
  }

  g_Mtx.unlock();
  return found;
}

DLHandle *ctr::createHandle(const char *path, const std::uint32_t flags) {
  DLHandle *handle = nullptr;

  g_Mtx.lock();

  // Look for free handle.
  for (auto i = 0u; i < MAX_HANDLES; ++i) {
    auto h = &g_Handles[i];

    if (!h->base) {
      handle = h;
      break;
    }
  }

  // If we have a handle, initialize it.
  if (handle) {
    if (path)
      std::memcpy(handle->path, path, strlen(path));
    handle->flags = flags;
    handle->refc = 1u;
  } else {
    ctr::setLastError(ctr::ERR_HANDLE_LIMIT);
  }

  g_Mtx.unlock();
  return handle;
}

bool ctr::freeHandle(DLHandle *handle) {
  bool ret = true;

  if (!handle || !handle->base) {
    ctr::setLastError(ctr::ERR_INVALID_PARAM);
    return false;
  }

  g_Mtx.lock();
  if (decrementRefCount(*handle)) {
    g_Mtx.unlock();
    ret = ctr::unloadObject(*handle);
    g_Mtx.lock();
    std::memset(handle, 0x00, sizeof(DLHandle));
  }

  g_Mtx.unlock();
  return ret;
}

// API

int dlclose(void *handle) {
  return !ctr::freeHandle(reinterpret_cast<DLHandle *>(handle));
}

void *dlsym(void *handle, const char *symbol) {
  // TODO!!!
  return nullptr;
}

int dladdr(const void *addr, Dl_info *info) {
  // We dont have to provide error infos for this function.
  auto address = reinterpret_cast<std::uintptr_t>(addr);
  if (!info)
    return 0;

  g_Mtx.lock();

  // Look for object.
  for (auto i = 0u; i < MAX_HANDLES; ++i) {
    auto h = &g_Handles[i];
    if (address >= h->base && address < (h->base + h->size)) {
      info->dli_fname = h->path;
      info->dli_fbase = reinterpret_cast<void *>(h->base);
      info->dli_sname = nullptr; // TODO: fill
      info->dli_saddr = nullptr; // TODO: fill
      info->dli_fep = reinterpret_cast<void *>(h->ep);
      break;
    }
  }

  g_Mtx.unlock();
  return info->dli_fbase != nullptr;
}