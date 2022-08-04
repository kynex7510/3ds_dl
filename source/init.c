#include <3ds/synchronization.h>

#include "allocator.h"
#include "handle.h"
#include "init.h"

#include <stdint.h>

// Globals

static s32 g_Initialized = 0;

// Init

void ctrdl_lazyInit() {
  if (!AtomicSwap(&g_Initialized, 1)) {
    ctrdl_initCodeAllocator();
    ctrdl_initHandleManager();
  }
}

int ctrdl_isInit() { return __ldrex(&g_Initialized); }