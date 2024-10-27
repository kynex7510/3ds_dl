#include "Symbol.h"

u32 ctrdl_findSymbolValueInHandle(CTRDLHandle* handle, const char* sym) {
    u32 found = 0;

    if (handle) {
        ctrdl_lockHandle(handle);

        const Elf32_Word hash = ctrdl_getELFSymHash(sym);
        size_t chainIndex = handle->symBuckets[hash % handle->numSymBuckets];

        while (chainIndex != STN_UNDEF) {
            const Elf32_Sym* s = &handle->symEntries[chainIndex];
            const char* name = &handle->stringTable[s->st_name];
            if (!strcmp(name, sym)) {
                found = handle->base + s->st_value;
                break;
            }

            chainIndex = handle->symChains[chainIndex];
        }

        ctrdl_unlockHandle(handle);
    }

    return found;
}