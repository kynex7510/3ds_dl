#ifndef _3DS_DL_SYSTEM_H
#define _3DS_DL_SYSTEM_H

#include <3ds/result.h>
#include <3ds/svc.h>

#define PAGE_SIZE 0x1000
#define LOOKUP_ALLOCATED 0
#define LOOKUP_FREE 1

// Check if running on luma.
int ctrdl_isLuma(void);

// Check if running on citra.
int ctrdl_isCitra(void);

// Align address to the specified value.
uintptr_t ctrdl_alignAddress(const uintptr_t address, const size_t align);

// Align size to the specified value.
size_t ctrdl_alignSize(const size_t size, const size_t align);

// Flush processor cache.
Result ctrdl_flushCpuCache(Handle proc, const uintptr_t address,
                           const size_t size);

Result ctrdl_queryMemory(Handle proc, const uintptr_t address, MemInfo *meminfo,
                         PageInfo *pageinfo);

// Change permission for a memory range.
Result ctrdl_changePermission(Handle proc, const uintptr_t address,
                              const size_t size, const MemPerm permission);

// Get size of a memory region.
size_t ctrdl_getRegionSize(Handle proc, const uintptr_t address,
                           const size_t maxSize, int lookupFree);

// Mirror map memory region (page aligned).
Result ctrdl_mirrorRegion(Handle proc, const uintptr_t mirror,
                          const uintptr_t source, const size_t size);

// Unmap mirrored memory region.
Result ctrdl_mirrorUnmap(Handle proc, const uintptr_t mirror,
                         const uintptr_t source, const size_t size);

#endif /* _3DS_DL_SYSTEM_H */