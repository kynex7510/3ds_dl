#include "System.h"

#include <string.h>

#define LUMA_TYPE 0x10000
#define LUMA_VERSION 0
#define CITRA_TYPE 0x20000
#define CITRA_VERSION 11

#define ACTION_MAP 0
#define ACTION_UNMAP 1

// Helpers

extern void lumaFlushDataCacheRange(uintptr_t address, size_t size);
extern void lumaInvalidateInstructionCacheRange(uintptr_t address, size_t size);

static Result mirrorAction(Handle proc, const uintptr_t mirror,
                           const uintptr_t source, const size_t size,
                           int unmap) {
  Result ret;

  // Original svcControlMemory does not support CODE ranges;
  // svcControlProcessMemory instead does. On citra it's quite the opposite:
  // svcControlMemory works fine, while svcControlProcessMemory is
  // unimplemented.
  if (ctrdl_isCitra()) {
    u32 out;

    if (proc != CUR_PROCESS_HANDLE)
      return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_RO, RD_INVALID_HANDLE);

    ret = svcControlMemory(&out, mirror, source, size,
                           !unmap ? MEMOP_MAP : MEMOP_UNMAP, MEMPERM_READWRITE);
  } else {
    int closeHandle = 0;

    if (proc == CUR_PROCESS_HANDLE) {
      ret = svcDuplicateHandle(&proc, CUR_PROCESS_HANDLE);

      if (R_FAILED(ret))
        return ret;

      closeHandle = 1;
    }

    ret = svcControlProcessMemory(proc, mirror, source, size,
                                  !unmap ? MEMOP_MAP : MEMOP_UNMAP,
                                  MEMPERM_READWRITE);

    if (closeHandle)
      svcCloseHandle(proc);
  }

  return ret;
}

// System

int ctrdl_isLuma(void) {
  s64 version = 0;
  svcGetSystemInfo(&version, LUMA_TYPE, LUMA_VERSION);
  return version != 0;
}

int ctrdl_isCitra(void) {
  s64 version = 0;
  svcGetSystemInfo(&version, CITRA_TYPE, CITRA_VERSION);
  return version != 0;
}

uintptr_t ctrdl_alignAddress(const uintptr_t address, const size_t align) {
  return address & ~(align - 1);
}

size_t ctrdl_alignSize(const size_t size, const size_t align) {
  return (size + (align - 1)) & ~(align - 1);
}

Result ctrdl_flushCpuCache(Handle proc, const uintptr_t address,
                           const size_t size) {
  if (ctrdl_isLuma()) {
    lumaInvalidateInstructionCacheRange(address, size);
    lumaFlushDataCacheRange(address, size);
    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_RO, RD_SUCCESS);
  }

  return svcFlushProcessDataCache(proc, address, size);
}

Result ctrdl_queryMemory(Handle proc, const uintptr_t address, MemInfo *meminfo,
                         PageInfo *pageinfo) {
  MemInfo silly;
  PageInfo dummy;

  memset(&silly, 0x00, sizeof(MemInfo));
  memset(&dummy, 0x00, sizeof(PageInfo));

  Result ret = svcQueryProcessMemory(&silly, &dummy, proc, address);

  if (R_SUCCEEDED(ret)) {
    if (meminfo)
      memcpy(meminfo, &silly, sizeof(MemInfo));

    if (pageinfo)
      memcpy(pageinfo, &dummy, sizeof(PageInfo));
  }

  return ret;
}

Result ctrdl_changePermission(Handle proc, const uintptr_t address,
                              const size_t size, const MemPerm permission) {
  int closeHandle = 0;
  Result ret = MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_RO, RD_INVALID_SIZE);
  const uintptr_t alignedAddress = ctrdl_alignAddress(address, PAGE_SIZE);
  const size_t alignedSize = ctrdl_alignSize(size, PAGE_SIZE);

  if (proc == CUR_PROCESS_HANDLE) {
    Result ret = svcDuplicateHandle(&proc, CUR_PROCESS_HANDLE);

    if (R_FAILED(ret))
      return ret;

    closeHandle = 1;
  }

  // svcControlProcessMemory is bugged: it can't track PROT operations when the
  // size is different for each call. So we always work on pages.
  for (size_t step = 0; step < alignedSize; step += PAGE_SIZE) {
    ret = svcControlProcessMemory(proc, alignedAddress + step, 0, PAGE_SIZE,
                                  MEMOP_PROT, permission);

    if (R_FAILED(ret))
      break;
  }

  if (R_SUCCEEDED(ret))
    ret = ctrdl_flushCpuCache(proc, alignedAddress, alignedSize);

  if (closeHandle)
    svcCloseHandle(proc);

  return ret;
}

size_t ctrdl_getRegionSize(Handle proc, const uintptr_t address,
                           const size_t maxSize, int lookupFree) {
  MemInfo info;
  size_t regionSize = 0;

  memset(&info, 0x00, sizeof(MemInfo));

  while (!maxSize || regionSize < maxSize) {
    if (R_FAILED(ctrdl_queryMemory(proc, address + regionSize, &info, NULL))) {
      regionSize = 0;
      break;
    }

    if ((lookupFree && info.state != MEMSTATE_FREE) ||
        (!lookupFree && info.state == MEMSTATE_FREE))
      break;

    regionSize += info.size;
  }

  return regionSize;
}

Result ctrdl_mirrorRegion(Handle proc, const uintptr_t mirror,
                          const uintptr_t source, const size_t size) {
  return mirrorAction(proc, mirror, source, size, ACTION_MAP);
}

Result ctrdl_mirrorUnmap(Handle proc, const uintptr_t mirror,
                         const uintptr_t source, const size_t size) {
  return mirrorAction(proc, mirror, source, size, ACTION_UNMAP);
}