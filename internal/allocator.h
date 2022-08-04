#ifndef _3DS_DL_ALLOCATOR_H
#define _3DS_DL_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

// Allocate aligned memory on heap.
uintptr_t ctrdl_allocateAligned(const size_t size);

// Free heap memory.
void ctrdl_freeHeap(const uintptr_t address);

// Initialize CODE allocator (thread-unsafe).
void ctrdl_initCodeAllocator();

// Allocate memory in the CODE range.
uintptr_t ctrdl_allocateCode(const uintptr_t source, const size_t size);

// Free memory from the CODE range.
int ctrdl_freeCode(const uintptr_t address, const uintptr_t source,
                   const size_t size);

#endif /* _3DS_DL_ALLOCATOR_H */