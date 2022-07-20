extern "C" {
#include <3ds/result.h>
#include <3ds/svc.h>
}

#include "ELF.hpp"
#include "Error.hpp"
#include "Loader.hpp"
#include "Relocs.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace ctr_dl;

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

static std::string getDepPath(const char *basePath, const char *name) {
  std::string path(basePath);
  auto pos = path.rfind('/');

  if (pos != std::string::npos) {
    path = path.substr(0, pos);
    return path + name;
  }

  return {};
}

static bool loadDependencies(DLHandle &handle, const Elf32_Ehdr *header,
                             const SymResolver resolver) {
  std::uint32_t depCount = 0;
  auto base = reinterpret_cast<std::uintptr_t>(header);
  auto strtab = getELFDynEntry(header, DT_STRTAB);

  if (!strtab) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_OBJECT);
    return false;
  }

  auto depEntries = getELFDynEntries(header, DT_NEEDED);

  if (depEntries.size() > ctr_dl::MAX_DEPS) {
    ctr_dl::setLastError(ctr_dl::ERR_DEPS_LIMIT);
    return false;
  }

  for (auto entry : depEntries) {
    auto path = getDepPath(handle.path,
                           reinterpret_cast<const char *>(
                               base + strtab->d_un.d_ptr + entry->d_un.d_ptr));
    auto depHandle = dlopen_ex(path.c_str(), resolver, RTLD_NOW);

    if (!depHandle) {
      ctr_dl::setLastError(ctr_dl::ERR_DEP_FAIL);
      return false;
    }

    handle.deps[++depCount] = reinterpret_cast<std::uintptr_t>(depHandle);
  }

  return true;
}

static bool mapObject(DLHandle &handle, const std::uint8_t *buffer,
                      const std::size_t size, const SymResolver resolver) {
  auto header = reinterpret_cast<const Elf32_Ehdr *>(buffer);

  if (!ctr_dl::validateELF(header, size))
    return false;

  // Load dependencies.
  if (!loadDependencies(handle, header, resolver)) {
    // References may be solved by the user.
    if (!resolver) {
      ctr_dl::unloadObject(handle);
      return false;
    }
  }

  // Allocate space for load segments.
  std::size_t allocSize = 0u;
  auto loadSegments = ctr_dl::getELFSegments(header, PT_LOAD);

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
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_OBJECT);
    return 0u;
  }

  handle.base = allocateAligned(allocSize);

  if (!handle.base) {
    ctr_dl::setLastError(ctr_dl::ERR_MAP_ERROR);
    ctr_dl::unloadObject(handle);
    return false;
  }

  // Copy buffer contents.
  for (auto s : loadSegments) {
    std::memcpy(reinterpret_cast<void *>(handle.base + s->p_vaddr),
                buffer + s->p_offset, s->p_filesz);
  }

  // Apply relocations.
  if (!ctr_dl::handleRelocs(handle, header, resolver)) {
    ctr_dl::setLastError(ctr_dl::ERR_RELOC_FAIL);
    ctr_dl::unloadObject(handle);
    return false;
  }

  // Restore permissions.
  for (auto s : loadSegments) {
    if (!changePermission(handle.base + s->p_vaddr,
                          alignSize(s->p_memsz, s->p_align),
                          wrapPermission(s->p_flags))) {
      ctr_dl::setLastError(ctr_dl::ERR_MAP_ERROR);
      ctr_dl::unloadObject(handle);
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

DLHandle *ctr_dl::openOrLoadObject(const char *name, const std::uint8_t *buffer,
                                   const std::size_t size,
                                   const SymResolver resolver,
                                   const std::uint32_t flags) {
  // Check flags.
  if (!checkFlags(flags)) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_PARAM);
    return nullptr;
  }

  // Check if already loaded, and update flags.
  if (auto handle = ctr_dl::findHandle(name)) {
    handle->flags = flags;
    return handle;
  }

  if (flags & RTLD_NOLOAD)
    return nullptr;

  // At this point we expect buffer and size to be defined.
  if (!buffer || !size) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_PARAM);
    return nullptr;
  }

  // Get handle.
  auto handle = ctr_dl::createHandle(name, flags);

  if (handle) {
    // Map and initialize.
    if (mapObject(*handle, buffer, size, resolver))
      return handle;

    ctr_dl::freeHandle(handle);
    handle = nullptr;
  }

  return handle;
}

bool ctr_dl::unloadObject(DLHandle &handle) {
  // Run finalizers.
  if (handle.fini && handle.finiSize) {
    auto finiArray = reinterpret_cast<Elf32_Addr *>(handle.fini);
    for (auto i = 0u; i < handle.finiSize; ++i)
      reinterpret_cast<void (*)()>(handle.base + finiArray[i])();
  }

  // Unmap segments.
  std::free(reinterpret_cast<void *>(handle.base));

  // Unload dependencies.
  for (auto i = 0u; i < ctr_dl::MAX_DEPS; ++i) {
    auto h = reinterpret_cast<DLHandle *>(handle.deps[i]);
    if (h) {
      if (!ctr_dl::freeHandle(h))
        return false;
    }
  }

  return true;
}