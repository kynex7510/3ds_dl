#ifndef _CTRDL_ELFUTIL_H
#define _CTRDL_ELFUTIL_H

#include "Error.h"
#include "Stream.h"

#include <elf.h>
#include <string.h>

typedef struct {
    Elf32_Ehdr header;
    Elf32_Phdr* segments;
    Elf32_Dyn* dynEntries;
    Elf32_Word numOfSymBuckets;
    Elf32_Word* symBuckets;
    Elf32_Word* symChains;
    Elf32_Sym* symEntries;
    char* stringTable;
} CTRDLElf;

Elf32_Word ctrdl_getELFSymHash(const char* name);
bool ctrdl_parseELF(CTRDLStream* stream, CTRDLElf* out);
void ctrdl_freeELF(CTRDLElf* elf);

size_t ctrdl_getELFNumSegmentsByType(CTRDLElf* elf, Elf32_Word type);
size_t ctrdl_getELFSegmentsByType(CTRDLElf* elf, Elf32_Word type, Elf32_Phdr* out, size_t maxSize);

inline bool ctrdl_getELFSegmentByType(CTRDLElf* elf, Elf32_Word type, Elf32_Phdr* out) {
    return ctrdl_getELFSegmentsByType(elf, type, out, 1);
}

size_t ctrdl_getELFNumDynEntriesWithTag(CTRDLElf* elf, Elf32_Sword tag);
size_t ctrdl_getELFDynEntriesWithTag(CTRDLElf* elf, Elf32_Sword tag, Elf32_Dyn* out, size_t maxSize);

inline bool ctrdl_getELFDynEntryWithTag(CTRDLElf* elf, Elf32_Sword tag, Elf32_Dyn* out) {
    return ctrdl_getELFDynEntriesWithTag(elf, tag, out, 1);
}

inline const char* ctrdl_getELFStringByIndex(CTRDLElf* elf, Elf32_Word index) {
    return &elf->stringTable[elf->symEntries[index].st_name];
}

#endif /* _CTRDL_ELFUTIL_H */