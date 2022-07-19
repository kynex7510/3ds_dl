#include "ELF.hxx"
#include "Loader.hxx"

#include <cstdlib>

using namespace ctr;

// Helpers

static void freeAligned(std::uintptr_t p) {
  // TODO: maybe we should use some SVC.
  std::free(reinterpret_cast<void *>(p));
}

// Loader

bool ctr::unloadObject(DLHandle &handle) {
  // Run finalizers.
  if (handle.fini && handle.finiSize) {
    auto finiArray = reinterpret_cast<Elf32_Addr *>(handle.fini);
    for (auto i = 0u; i < handle.finiSize; ++i)
      reinterpret_cast<void (*)()>(handle.base + finiArray[i])();
  }

  // Unmap segments.
  freeAligned(handle.base);

  // Unload dependencies.
  for (auto i = 0u; i < ctr::MAX_DEPS; ++i) {
    auto h = reinterpret_cast<DLHandle *>(handle.deps[i]);
    if (h) {
      if (!ctr::freeHandle(h))
        return false;
    }
  }

  return true;
}