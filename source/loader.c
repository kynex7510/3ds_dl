#include "loader.h"
#include "allocator.h"
#include "elfutil.h"
#include "relocs.h"
#include "system.h"

#include <stdlib.h>

#define RTLD_LAZY 0x0001
#define RTLD_DEEPBIND 0x0008
#define RTLD_NODELETE 0x1000

#define MEMPERM_NONE (MemPerm)0

// Helpers

static MemPerm wrapPermission(const Elf32_Word flags) {
  switch (flags) {
  case PF_R:
    return MEMPERM_READ;
  case PF_W:
    return MEMPERM_WRITE;
  case PF_X:
    return MEMPERM_EXECUTE;
  case PF_R | PF_W:
    return MEMPERM_READWRITE;
  case PF_R | PF_X:
    return MEMPERM_READEXECUTE;
  default:
    return MEMPERM_NONE;
  }
}

static int checkFlags(const uint32_t flags) {
  // Unsupported flags.
  if ((flags & RTLD_LAZY) | (flags & RTLD_DEEPBIND) | (flags & RTLD_NODELETE))
    return 0;

  // Required flags.
  if (!(flags & RTLD_NOW))
    return 0;

  return 1;
}

static size_t getSegmentsAllocSize(const struct DLHandle *handle,
                                   const size_t numSegments,
                                   Elf32_Phdr **loadSegments) {
  size_t allocSize = 0;

  for (size_t i = 0; i < numSegments; ++i) {
    const Elf32_Phdr *segment = loadSegments[i];

    if (segment->p_memsz < segment->p_filesz) {
      allocSize = 0;
      break;
    }

    if (segment->p_align > 1)
      allocSize += ctrdl_alignSize(segment->p_memsz, segment->p_align);
    else
      allocSize += segment->p_memsz;
  }

  return allocSize;
}

static char *getDepPath(const char *basePath, const char *name) {
  char *pos = strstr(basePath, "/");

  if (pos && name) {
    size_t baseSize = (uintptr_t)pos - (uintptr_t)basePath - 1;
    size_t depPathSize = baseSize + strlen(name) + 1;
    char *depPath = (char *)malloc(depPathSize);

    if (depPath) {
      memcpy(depPath, basePath, baseSize);
      depPath[baseSize] = '\0';
      strcat(depPath, name);
      depPath[depPathSize] = '\0';
      return depPath;
    }
  }

  return NULL;
}

static int loadDependencies(struct DLHandle *handle, const Elf32_Ehdr *header,
                            const SymResolver resolver) {
  Elf32_Dyn *depEntries[MAX_DEPS];
  uintptr_t base = (uintptr_t)(header);
  const Elf32_Dyn *strtab = ctrdl_getELFDynEntry(header, DT_STRTAB);

  if (!strtab) {
    ctrdl_setLastError(ERR_INVALID_OBJECT);
    return 0;
  }

  size_t depCount = ctrdl_getElfDynSize(header, DT_NEEDED);

  if (depCount > MAX_DEPS) {
    ctrdl_setLastError(ERR_DEPS_LIMIT);
    return 0;
  }

  size_t actualCount = ctrdl_getELFDynEntries(
      header, DT_NEEDED, (Elf32_Dyn **)&depEntries, depCount);

  if (actualCount != depCount) {
    ctrdl_setLastError(ERR_DEP_FAIL);
    return 0;
  }

  for (size_t i = 0; i < depCount; ++i) {
    char *depPath =
        getDepPath(handle->path, (const char *)(base + strtab->d_un.d_ptr +
                                                depEntries[i]->d_un.d_ptr));
    void *depHandle = dlopen_ex(depPath, resolver, RTLD_NOW);
    free(depPath);

    if (!depHandle) {
      ctrdl_setLastError(ERR_DEP_FAIL);
      return 0;
    }

    handle->deps[i] = (uintptr_t)(depHandle);
  }

  return true;
}

static int mapObject(struct DLHandle *handle, const void *buffer,
                     const size_t size, const SymResolver resolver) {
  const Elf32_Ehdr *header = (const Elf32_Ehdr *)(buffer);

  if (!ctrdl_validateELF(header, size))
    return 0;

  // Load dependencies.
  if (!loadDependencies(handle, header, resolver)) {
    // References may be solved by the user.
    if (!resolver) {
      ctrdl_unloadObject(handle);
      return 0;
    }
  }

  // Calculate allocation space for load segments.
  size_t numSegments = ctrdl_getELFSegmentsSize(header, PT_LOAD);

  if (!numSegments) {
    ctrdl_setLastError(ERR_INVALID_OBJECT);
    ctrdl_unloadObject(handle);
    return 0;
  }

  Elf32_Phdr **loadSegments =
      (Elf32_Phdr **)(calloc(numSegments, sizeof(Elf32_Phdr *)));

  if (!loadSegments) {
    ctrdl_setLastError(ERR_NO_MEMORY);
    ctrdl_unloadObject(handle);
    return 0;
  }

  size_t actualNumSegments =
      ctrdl_getELFSegments(header, PT_LOAD, loadSegments, numSegments);

  if (actualNumSegments != numSegments) {
    ctrdl_setLastError(ERR_INVALID_OBJECT);
    ctrdl_unloadObject(handle);
    free(loadSegments);
    return 0;
  }

  handle->size = getSegmentsAllocSize(handle, numSegments, loadSegments);

  if (!handle->size) {
    ctrdl_setLastError(ERR_INVALID_OBJECT);
    ctrdl_unloadObject(handle);
    free(loadSegments);
    return 0;
  }

  // Allocate and map segments.
  handle->origin = ctrdl_allocateAligned(handle->size);

  if (!handle->origin) {
    ctrdl_setLastError(ERR_MAP_ERROR);
    ctrdl_unloadObject(handle);
    free(loadSegments);
    return 0;
  }

  for (size_t i = 0; i < numSegments; ++i) {
    const Elf32_Phdr *segment = loadSegments[i];
    memcpy((void *)(handle->origin + segment->p_vaddr),
           (void *)((uintptr_t)buffer + segment->p_offset), segment->p_filesz);
  }

  handle->base = ctrdl_allocateCode(handle->origin, handle->size);

  if (!handle->base) {
    ctrdl_setLastError(ERR_MAP_ERROR);
    ctrdl_unloadObject(handle);
    free(loadSegments);
    return 0;
  }

  // Apply relocations.
  if (!ctrdl_handleRelocs(handle, header, resolver)) {
    ctrdl_setLastError(ERR_RELOC_FAIL);
    ctrdl_unloadObject(handle);
    free(loadSegments);
    return 0;
  }

  // Set right permissions.
  for (size_t i = 0; i < numSegments; ++i) {
    const Elf32_Phdr *segment = loadSegments[i];

    if (R_FAILED(ctrdl_changePermission(
            CUR_PROCESS_HANDLE, handle->base + segment->p_vaddr,
            ctrdl_alignSize(segment->p_memsz, segment->p_align),
            wrapPermission(segment->p_flags)))) {
      ctrdl_setLastError(ERR_MAP_ERROR);
      ctrdl_unloadObject(handle);
      free(loadSegments);
      return 0;
    }
  }

  // Run initializers.
  const Elf32_Dyn *initEntry = ctrdl_getELFDynEntry(header, DT_INIT_ARRAY);
  const Elf32_Dyn *initEntrySize =
      ctrdl_getELFDynEntry(header, DT_INIT_ARRAYSZ);

  if (initEntry && initEntrySize) {
    const Elf32_Addr *initArray =
        (const Elf32_Addr *)(handle->base + initEntry->d_un.d_ptr);
    const size_t initArraySize = initEntrySize->d_un.d_val / sizeof(Elf32_Addr);

    for (size_t i = 0; i < initArraySize; ++i)
      ((void (*)())(initArray[i]))();
  }

  // Fill missing members.
  handle->ep = handle->base + header->e_entry;

  const Elf32_Dyn *finiEntry = ctrdl_getELFDynEntry(header, DT_FINI_ARRAY);
  const Elf32_Dyn *finiEntrySize =
      ctrdl_getELFDynEntry(header, DT_FINI_ARRAYSZ);

  if (finiEntry && finiEntrySize) {
    handle->fini = handle->base + finiEntry->d_un.d_ptr;
    handle->finiSize = finiEntrySize->d_un.d_val / sizeof(Elf32_Addr);
  }

  free(loadSegments);
  return 1;
}

// Loader

struct DLHandle *ctrdl_openOrLoadObject(const char *name, const void *buffer,
                                        const size_t size,
                                        const SymResolver resolver,
                                        const uint32_t flags) {
  // Check flags.
  if (!checkFlags(flags)) {
    ctrdl_setLastError(ERR_INVALID_PARAM);
    return NULL;
  }

  // Check if already loaded, and update flags.
  struct DLHandle *handle = ctrdl_findHandle(name);
  if (handle) {
    handle->flags = flags;
    return handle;
  }

  if (flags & RTLD_NOLOAD)
    return NULL;

  // At this point we expect buffer and size to be defined.
  if (!buffer || !size) {
    ctrdl_setLastError(ERR_INVALID_PARAM);
    return NULL;
  }

  // Get handle.
  handle = ctrdl_createHandle(name, flags);

  if (handle) {
    // Map and initialize.
    if (mapObject(handle, buffer, size, resolver))
      return handle;

    ctrdl_freeHandle(handle);
    handle = NULL;
  }

  return handle;
}

int ctrdl_unloadObject(struct DLHandle *handle) {
  // Run finalizers.
  if (handle->fini && handle->finiSize) {
    const Elf32_Addr *finiArray = (const Elf32_Addr *)(handle->fini);
    for (size_t i = 0; i < handle->finiSize; ++i)
      ((void (*)())(finiArray[i]))();
  }

  // Unmap segments.
  if (handle->base) {
    if (!ctrdl_freeCode(handle->base, handle->origin, handle->size)) {
      ctrdl_setLastError(ERR_FREE_FAILED);
      return 0;
    }

    ctrdl_freeHeap(handle->origin);

    handle->origin = 0;
    handle->base = 0;
    handle->size = 0;
  }

  // Unload dependencies.
  for (size_t i = 0; i < MAX_DEPS; ++i) {
    struct DLHandle *h = (struct DLHandle *)(handle->deps[i]);
    if (h) {
      if (!ctrdl_freeHandle(h))
        return 0;
    }
  }

  return 1;
}