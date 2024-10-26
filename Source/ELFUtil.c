#include "ELFUtil.h"

#include <stdlib.h>
#include <string.h>

Elf32_Word ctrdl_getELFSymHash(const char* name) {
    Elf32_Word h = 0;
    Elf32_Word g = 0;

    for (u8 c = (u8)*name; c; ++name) {
        h = ((h << 4) + c);
        g = h & 0xF0000000;

        if (g)
            h ^= g >> 24;

        h &= ~g;
    }

    return h;
}

bool ctrdl_parseELF(CTRDLStream* stream, CTRDLElf* out) {
    // Read header.
    if (!stream->seek(stream, 0)) {
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    if (!stream->read(stream, &out->header, sizeof(Elf32_Ehdr))) {
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    if (memcmp(out->header.e_ident, ELFMAG, SELFMAG)) {
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    if (out->header.e_ident[EI_CLASS] != ELFCLASS32) {
        ctrdl_setLastError(Err_InvalidBit);
        return false;
    }

    if (out->header.e_ident[EI_DATA] != ELFDATA2LSB) {
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    if (out->header.e_type != ET_DYN) {
        ctrdl_setLastError(Err_NotSO);
        return false;
    }

    if (out->header.e_machine != EM_ARM) {
        ctrdl_setLastError(Err_InvalidArch);
        return false;
    }

    // Read program headers.
    if (!stream->seek(stream, out->header.e_phoff)) {
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    out->segments = malloc(out->header.e_phoff * sizeof(Elf32_Phdr));
    if (!out->segments) {
        ctrdl_setLastError(Err_NoMemory);
        return false;
    }

    for (size_t i = 0; i < out->header.e_phoff; ++i) {
        if (!stream->read(stream, &out->segments[i], sizeof(Elf32_Phdr))) {
            ctrdl_freeELF(out);
            ctrdl_setLastError(Err_ReadFailed);
            return false;
        }
    }

    // Read dyn entries.
    Elf32_Phdr dyn;
    if (!ctrdl_getELFSegmentByType(out, &dyn, PT_DYNAMIC)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    if (!stream->seek(stream, dyn.p_offset)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    out->dynEntries = malloc(dyn.p_filesz);
    if (!out->dynEntries) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_NoMemory);
        return false;
    }

    if (!stream->read(stream, out->dynEntries, dyn.p_filesz)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    // Read sym hash table.
    Elf32_Dyn hash;
    if (!ctrdl_getELFDynEntryWithTag(out, DT_HASH, &hash)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    if (!stream->seek(stream, hash.d_un.d_ptr)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    if (!stream->read(stream, &out->numOfSymBuckets, sizeof(Elf32_Word))) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    Elf32_Word numChains;
    if (!stream->read(stream, &numChains, sizeof(Elf32_Word))) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    out->symBuckets = malloc(out->numOfSymBuckets * sizeof(Elf32_Word));
    if (!out->symBuckets) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_NoMemory);
        return false;
    }

    out->symChains = malloc(numChains * sizeof(Elf32_Word));
    if (!out->symChains) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_NoMemory);
        return false;
    }

    if (!stream->read(stream, out->symBuckets, out->numOfSymBuckets * sizeof(Elf32_Word))) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    if (!stream->read(stream, out->symChains, numChains * sizeof(Elf32_Word))) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    // Read sym entries.
    Elf32_Dyn symtab;
    if (!ctrdl_getELFDynEntryWithTag(out, DT_SYMTAB, &symtab)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    if (!stream->seek(stream, symtab.d_un.d_ptr)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    out->symEntries = malloc(numChains * sizeof(Elf32_Sym));
    if (!out->symEntries) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_NoMemory);
        return false;
    }

    if (!stream->read(stream, out->symEntries, numChains * sizeof(Elf32_Sym))) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    // Read string table.
    Elf32_Dyn strtab;
    if (!ctrdl_getELFDynEntryWithTag(out, DT_STRTAB, &strtab)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    if (!stream->seek(stream, strtab.d_un.d_ptr)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    Elf32_Dyn strsz;
    if (!ctrdl_getELFDynEntryWithTag(out, DT_STRSZ, &strsz)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    out->stringTable = (char*)malloc(strsz.d_un.d_val);
    if (!out->stringTable) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_NoMemory);
        return false;
    }

    if (!stream->read(stream, &out->stringTable, strsz.d_un.d_val)) {
        ctrdl_freeELF(out);
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    return true;
}

void ctrdl_freeELF(CTRDLElf* elf) {
    free(elf->segments);
    free(elf->dynEntries);
    free(elf->symBuckets);
    free(elf->symChains);
    free(elf->symEntries);
}

size_t ctrdl_getELFNumSegmentsByType(CTRDLElf* elf, Elf32_Word type) {
    size_t count = 0;

    for (size_t i = 0; i < elf->header.e_phnum; ++i) {
        Elf32_Phdr* ph = &elf->segments[i];
        if (ph->p_type == type)
            ++count;
    }

    return count;
}

size_t ctrdl_getELFSegmentsByType(CTRDLElf* elf, Elf32_Word type, Elf32_Phdr* out, size_t maxSize) {
    size_t count = 0;

    for (size_t i = 0; i < elf->header.e_phnum; ++i) {
        Elf32_Phdr* ph = &elf->segments[i];
        if (ph->p_type == type) {
            memcpy(&out[count], ph, sizeof(Elf32_Phdr));
            ++count;

            if (count >= maxSize)
                break;
        }
    }

    return count;
}

size_t ctrdl_getELFNumDynEntriesWithTag(CTRDLElf* elf, Elf32_Sword tag) {
    size_t count = 0;
    Elf32_Dyn* entry = elf->dynEntries;

    while (entry->d_tag != DT_NULL) {
        ++count;
        ++entry;
    }

    return count;
}

size_t ctrdl_getELFDynEntriesWithTag(CTRDLElf* elf, Elf32_Sword tag, Elf32_Dyn* out, size_t maxSize) {
    size_t count = 0;
    Elf32_Dyn* entry = elf->dynEntries;

    while (entry->d_tag != DT_NULL) {
        if (entry->d_tag == tag) {
            memcpy(&out[count], entry, sizeof(Elf32_Dyn));
            ++count;
        }

        ++entry;
    }

    return count;
}