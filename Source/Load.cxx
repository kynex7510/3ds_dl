#include <3ds/result.h>
#include <3ds/svc.h>

#include "ELF.hxx"
#include "Error.hxx"
#include "Loader.hxx"
#include "dlfcn.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

using namespace ctr;

// Macros

#define RTLD_LAZY 0x0001
#define RTLD_DEEPBIND 0x0008
#define RTLD_NODELETE 0x1000

// Globals

constexpr static std::size_t PAGE_SIZE = 0x1000;

// HACK

namespace std {
auto &aligned_alloc = ::aligned_alloc;
}

// Helpers

static std::size_t alignSize(const std::size_t size, const std::size_t align) {
  return (size + (align - 1)) & ~(align - 1);
}

static MemPerm wrapPermission(const Elf32_Word flags) {
  switch (flags) {
  case PF_R:
    return MemPerm::MEMPERM_READ;
  case PF_W:
    return MemPerm::MEMPERM_WRITE;
  case PF_X:
    return MemPerm::MEMPERM_EXECUTE;
  case PF_R | PF_W:
    return MemPerm::MEMPERM_READWRITE;
  case PF_R | PF_X:
    return MemPerm::MEMPERM_READEXECUTE;
  default:
    return MemPerm::MEMPERM_DONTCARE;
  }
}

static bool changePermission(const std::uintptr_t address,
                             const std::size_t size, const MemPerm permission) {
  Handle proc;

  if (R_FAILED(svcDuplicateHandle(&proc, CUR_PROCESS_HANDLE)))
    return false;

  auto ret = R_SUCCEEDED(svcControlProcessMemory(
      proc, address & ~(PAGE_SIZE - 1), 0u, alignSize(size, PAGE_SIZE),
      MEMOP_PROT, permission));

  svcCloseHandle(proc);
  return ret;
}

static std::uintptr_t allocateAligned(const std::size_t size) {
  // TODO: maybe we should use some SVC.
  if (auto p = std::aligned_alloc(PAGE_SIZE, size)) {
    std::memset(p, 0x00, size);
    return reinterpret_cast<std::uintptr_t>(p);
  }

  return 0u;
}

static bool checkFlags(const std::uint32_t flags) {
  // Unsupported flags.
  if ((flags & RTLD_LAZY) | (flags & RTLD_DEEPBIND) | (flags & RTLD_NODELETE))
    return false;

  // Required flags.
  if (!(flags & RTLD_NOW))
    return false;

  return true;
}

static bool mapObject(DLHandle &handle, const std::uint8_t *buffer,
                      const std::size_t size) {
  auto header = reinterpret_cast<const Elf32_Ehdr *>(buffer);

  if (!ctr::validateELF(header, size))
    return false;

  // Allocate space for load segments.
  std::size_t allocSize = 0u;
  auto loadSegments = ctr::getELFSegments(header, PT_LOAD);

  for (auto s : loadSegments) {
    if (s->p_memsz < s->p_filesz) {
      allocSize = 0u;
      break;
    }

    if (s->p_align > 1)
      allocSize += alignSize(s->p_memsz, s->p_align);
    else
      allocSize += s->p_memsz;
  }

  if (!allocSize) {
    ctr::setLastError(ctr::ERR_INVALID_OBJECT);
    return 0u;
  }

  handle.base = allocateAligned(allocSize);

  if (!handle.base) {
    ctr::setLastError(ctr::ERR_MAP_ERROR);
    return false;
  }

  // Copy buffer contents.
  for (auto s : loadSegments) {
    std::memcpy(reinterpret_cast<void *>(handle.base + s->p_vaddr),
                buffer + s->p_offset, s->p_filesz);
  }

  // TODO: Relocations...

  // Restore permissions.
  for (auto s : loadSegments) {
    if (!changePermission(handle.base + s->p_vaddr,
                          alignSize(s->p_memsz, s->p_align),
                          wrapPermission(s->p_flags))) {
      ctr::setLastError(ctr::ERR_MAP_ERROR);
      ctr::unloadObject(handle);
      return false;
    }
  }

  // Run initializers.
  auto initEntry = getELFDynEntry(header, DT_INIT_ARRAY);
  auto initEntrySize = getELFDynEntry(header, DT_INIT_ARRAYSZ);

  if (initEntry && initEntrySize) {
    auto initArray =
        reinterpret_cast<Elf32_Addr *>(handle.base + initEntry->d_un.d_ptr);
    auto initArraySize = initEntrySize->d_un.d_val / sizeof(Elf32_Addr);

    for (auto i = 0u; i < initArraySize; ++i)
      reinterpret_cast<void (*)()>(handle.base + initArray[i])();
  }

  // Fill missing members.
  handle.size = allocSize;
  handle.ep = handle.base + header->e_entry;

  auto finiEntry = getELFDynEntry(header, DT_FINI_ARRAY);
  auto finiEntrySize = getELFDynEntry(header, DT_FINI_ARRAYSZ);

  if (finiEntry && finiEntrySize) {
    handle.fini = handle.base + finiEntry->d_un.d_ptr;
    handle.finiSize = finiEntrySize->d_un.d_val / sizeof(Elf32_Addr);
  }

  return true;
}

// Loader

DLHandle *ctr::openOrLoadObject(const char *name, const std::uint8_t *buffer,
                                const std::size_t size,
                                const std::uint32_t flags) {
  // Check flags.
  if (!checkFlags(flags)) {
    ctr::setLastError(ctr::ERR_INVALID_PARAM);
    return nullptr;
  }

  // Check if already loaded, and update flags.
  if (auto handle = ctr::findHandle(name)) {
    handle->flags = flags;
    return handle;
  }

  if (flags & RTLD_NOLOAD)
    return nullptr;

  // At this point we expect buffer and size to be defined.
  if (!buffer || !size) {
    ctr::setLastError(ctr::ERR_INVALID_PARAM);
    return nullptr;
  }

  // Get handle.
  auto handle = ctr::createHandle(name, flags);

  if (handle) {
    // Map and initialize.
    if (mapObject(*handle, buffer, size))
      return handle;

    ctr::freeHandle(handle);
    handle = nullptr;
  }

  return handle;
}
