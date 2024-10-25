#ifndef _3DS_DL_ELFUTIL_H
#define _3DS_DL_ELFUTIL_H

#include "Error.h"
#include <elf.h>
#include <string.h>

// Check if buffer is a valid ELF.
inline int ctrdl_validateELF(const Elf32_Ehdr *header, const size_t size) {
  // Size
  if (size < sizeof(Elf32_Ehdr) || size < header->e_ehsize) {
    ctrdl_setLastError(ERR_INVALID_PARAM);
    return 0;
  }

  // Magic
  if (memcmp(header->e_ident, ELFMAG, SELFMAG)) {
    ctrdl_setLastError(ERR_INVALID_OBJECT);
    return 0;
  }

  // Bitness
  if (header->e_ident[EI_CLASS] != ELFCLASS32) {
    ctrdl_setLastError(ERR_INVALID_BIT);
    return 0;
  }

  // Endianess
  if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
    ctrdl_setLastError(ERR_INVALID_OBJECT);
    return 0;
  }

  // Position-indipendent executables
  if (header->e_type != ET_DYN) {
    ctrdl_setLastError(ERR_INVALID_OBJECT);
    return 0;
  }

  // Architecture
  if (header->e_machine != EM_ARM) {
    ctrdl_setLastError(ERR_INVALID_ARCH);
    return 0;
  }

  return 1;
}

// Get ELF program header.
inline Elf32_Phdr *ctrdl_getELFProgramHeader(const Elf32_Ehdr *header) {
  if (header && header->e_phnum) {
    Elf32_Addr base = (Elf32_Addr)(header);
    return (Elf32_Phdr *)(base + header->e_phoff);
  }

  return NULL;
}

// Get number of segments of given type.
inline size_t ctrdl_getELFSegmentsSize(const Elf32_Ehdr *header,
                                       const Elf32_Word type) {
  size_t count = 0;
  Elf32_Phdr *PH = ctrdl_getELFProgramHeader(header);

  if (PH) {
    for (size_t i = 0; i < header->e_phnum; ++i) {
      if (PH[i].p_type == type)
        ++count;
    }
  }

  return count;
}

// Get max segments of given type.
inline size_t ctrdl_getELFSegments(const Elf32_Ehdr *header,
                                   const Elf32_Word type, Elf32_Phdr **out,
                                   size_t maxSize) {
  size_t count = 0;
  Elf32_Phdr *PH = ctrdl_getELFProgramHeader(header);

  if (PH) {
    for (size_t i = 0; i < header->e_phnum; ++i) {
      if (PH[i].p_type == type) {
        out[count++] = &PH[i];

        if (count >= maxSize)
          break;
      }
    }
  }

  return count;
}

// Get one segment of the given type.
inline const Elf32_Phdr *ctrdl_getELFSegment(const Elf32_Ehdr *header,
                                             const Elf32_Word type) {
  Elf32_Phdr *segments[1] = {};
  size_t count =
      ctrdl_getELFSegments(header, type, (Elf32_Phdr **)(&segments), 1);
  return count ? segments[0] : NULL;
}

// Get number of dynamic entries.
inline size_t ctrdl_getElfDynSize(const Elf32_Ehdr *header,
                                  const Elf32_Sword tag) {
  size_t count = 0;
  Elf32_Addr base = (Elf32_Addr)(header);
  const Elf32_Phdr *dyn = ctrdl_getELFSegment(header, PT_DYNAMIC);

  if (dyn) {
    Elf32_Dyn *entry = (Elf32_Dyn *)(base + dyn->p_offset);

    while (entry->d_tag != DT_NULL) {
      if (entry->d_tag == tag)
        ++count;

      ++entry;
    }
  }

  return count;
}

// Get max dynamic entries.
inline size_t ctrdl_getELFDynEntries(const Elf32_Ehdr *header,
                                     const Elf32_Sword tag, Elf32_Dyn **out,
                                     const size_t maxSize) {
  size_t count = 0;
  Elf32_Addr base = (Elf32_Addr)(header);
  const Elf32_Phdr *dyn = ctrdl_getELFSegment(header, PT_DYNAMIC);

  if (dyn) {
    Elf32_Dyn *entry = (Elf32_Dyn *)(base + dyn->p_offset);

    while (entry->d_tag != DT_NULL) {
      if (entry->d_tag == tag) {

        out[count++] = entry;

        if (count >= maxSize)
          break;
      }

      ++entry;
    }
  }

  return count;
}

// Get one dynamic entry.
inline const Elf32_Dyn *ctrdl_getELFDynEntry(const Elf32_Ehdr *header,
                                             const Elf32_Sword tag) {
  Elf32_Dyn *entries[1] = {};
  size_t count =
      ctrdl_getELFDynEntries(header, tag, (Elf32_Dyn **)(&entries), 1);
  return count ? entries[0] : NULL;
}

// Get ELF symtab.
inline const Elf32_Sym *ctrdl_getELFSymTab(const Elf32_Ehdr *header) {
  const Elf32_Dyn *symtabEntry = ctrdl_getELFDynEntry(header, DT_SYMTAB);

  if (symtabEntry) {
    Elf32_Addr base = (Elf32_Addr)(header);
    return (const Elf32_Sym *)(base + symtabEntry->d_un.d_ptr);
  }

  return NULL;
}

// Get symbol name from strtab.
inline const char *ctrdl_getELFSymName(const Elf32_Ehdr *header,
                                       const Elf32_Sym *symtab,
                                       const Elf32_Dyn *strtab,
                                       const Elf32_Word index) {
  if (header && symtab && strtab) {
    Elf32_Addr base = (Elf32_Addr)(header);
    return (const char *)(base + strtab->d_un.d_ptr + symtab[index].st_name);
  }

  return NULL;
}

#endif /* _3DS_DL_ELFUTIL_H */