#include <3ds/synchronization.h>

#include "Allocator.h"
#include "System.h"

#include <stdlib.h>
#include <string.h>

#define CODE_BASE 0x100000
#define CODE_MAX 0x3F00000

// Types

struct AllocatorState {
  uintptr_t allocBase;
  size_t maxAllocSize;
};

// Globals

static struct AllocatorState g_State;
static LightLock g_Mtx;

// Allocator

uintptr_t ctrdl_allocateAligned(const size_t size) {
  void *buffer = aligned_alloc(PAGE_SIZE, size);

  if (buffer)
    memset(buffer, 0x00, size);

  return (uintptr_t)buffer;
}

void ctrdl_freeHeap(const uintptr_t address) { free((void *)address); }

void ctrdl_initCodeAllocator() {
  if (!g_State.allocBase || !g_State.maxAllocSize) {
    LightLock_Init(&g_Mtx);

    size_t codeSize = ctrdl_getRegionSize(CUR_PROCESS_HANDLE, CODE_BASE,
                                          CODE_MAX, LOOKUP_ALLOCATED);

    g_State.allocBase = CODE_BASE + codeSize;
    g_State.maxAllocSize = CODE_MAX - codeSize;
  }
}

uintptr_t ctrdl_allocateCode(const uintptr_t source, const size_t size) {
  const size_t alignedSize = ctrdl_alignSize(size, PAGE_SIZE);
  uintptr_t address = 0;
  size_t regionOffset = 0;

  if (source % PAGE_SIZE)
    return 0;

  LightLock_Lock(&g_Mtx);

  // Find suitable address.
  while (regionOffset < g_State.maxAllocSize) {
    size_t regionSize = ctrdl_getRegionSize(
        CUR_PROCESS_HANDLE, g_State.allocBase + regionOffset,
        g_State.maxAllocSize - regionOffset, LOOKUP_FREE);

    if (regionSize >= alignedSize) {
      address = g_State.allocBase + regionOffset;
      break;
    }

    regionOffset += regionSize;
    regionOffset += ctrdl_getRegionSize(
        CUR_PROCESS_HANDLE, g_State.allocBase + regionOffset,
        g_State.maxAllocSize - regionOffset, LOOKUP_ALLOCATED);
  }

  // Map address if any.
  if (address) {
    if (R_FAILED(ctrdl_mirrorRegion(CUR_PROCESS_HANDLE, address, source,
                                    alignedSize)))
      address = 0;
  }

  LightLock_Unlock(&g_Mtx);
  return address;
}

int ctrdl_freeCode(const uintptr_t address, const uintptr_t source,
                   const size_t size) {
  const size_t alignedSize = ctrdl_alignSize(size, PAGE_SIZE);

  if ((address % PAGE_SIZE) || (source % PAGE_SIZE))
    return 0;

  LightLock_Lock(&g_Mtx);

  Result ret = ctrdl_changePermission(CUR_PROCESS_HANDLE, address, alignedSize,
                                      MEMPERM_READWRITE);

  if (R_SUCCEEDED(ret)) {
    ret = ctrdl_mirrorUnmap(CUR_PROCESS_HANDLE, address, source, alignedSize);
  }

  LightLock_Unlock(&g_Mtx);
  return R_SUCCEEDED(ret);
}