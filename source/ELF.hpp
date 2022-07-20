#ifndef _3DS_DL_ELF_HPP
#define _3DS_DL_ELF_HPP

#include "Error.hpp"
#include <elf.h>
#include <vector>

namespace ctr_dl {
inline bool validateELF(const Elf32_Ehdr *header, const std::size_t size) {
  // Size
  if (size < sizeof(Elf32_Ehdr) || size < header->e_ehsize) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_PARAM);
    return false;
  }

  // Magic
  if (!std::equal(header->e_ident, header->e_ident + SELFMAG, ELFMAG)) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_OBJECT);
    return false;
  }

  // Bitness
  if (header->e_ident[EI_CLASS] != ELFCLASS32) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_BIT);
    return false;
  }

  // Endianess
  if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_OBJECT);
    return false;
  }

  // Position-indipendent executables
  if (header->e_type != ET_DYN) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_OBJECT);
    return false;
  }

  // Architecture
  if (header->e_machine != EM_ARM) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_ARCH);
    return false;
  }

  return true;
}

inline Elf32_Phdr *getELFProgramHeader(const Elf32_Ehdr *header) {
  if (header && header->e_phnum) {
    auto base = reinterpret_cast<Elf32_Addr>(header);
    return reinterpret_cast<Elf32_Phdr *>(base + header->e_phoff);
  }

  return nullptr;
}

inline std::vector<const Elf32_Phdr *> getELFSegments(const Elf32_Ehdr *header,
                                                      const Elf32_Word type) {
  std::vector<const Elf32_Phdr *> buffer;
  auto PH = getELFProgramHeader(header);

  if (PH) {
    for (auto i = 0u; i < header->e_phnum; ++i) {
      if (PH[i].p_type == type)
        buffer.push_back(&PH[i]);
    }
  }

  return buffer;
}

inline const Elf32_Phdr *getELFSegment(const Elf32_Ehdr *header,
                                       const Elf32_Word type) {
  auto segments = getELFSegments(header, type);
  return segments.empty() ? nullptr : segments[0];
}

inline std::vector<const Elf32_Dyn *> getELFDynEntries(const Elf32_Ehdr *header,
                                                       const Elf32_Sword tag) {
  std::vector<const Elf32_Dyn *> buffer;
  auto base = reinterpret_cast<Elf32_Addr>(header);
  auto dyn = getELFSegment(header, PT_DYNAMIC);

  if (dyn) {
    auto entry = reinterpret_cast<Elf32_Dyn *>(base + dyn->p_offset);

    while (entry->d_tag != DT_NULL) {
      if (entry->d_tag == tag)
        buffer.push_back(entry);

      ++entry;
    }
  }

  return buffer;
}

inline const Elf32_Dyn *getELFDynEntry(const Elf32_Ehdr *header,
                                       const Elf32_Sword tag) {
  auto entries = getELFDynEntries(header, tag);
  return entries.empty() ? nullptr : entries[0];
}

inline const Elf32_Sym *getELFSymTab(const Elf32_Ehdr *header) {
  auto symtabEntry = getELFDynEntry(header, DT_SYMTAB);

  if (symtabEntry) {
    auto base = reinterpret_cast<Elf32_Addr>(header);
    return reinterpret_cast<const Elf32_Sym *>(base + symtabEntry->d_un.d_ptr);
  }

  return nullptr;
}

inline const char *getELFSymName(const Elf32_Ehdr *header,
                                 const Elf32_Sym *symtab,
                                 const Elf32_Dyn *strtab,
                                 const Elf32_Word index) {
  if (header && symtab && strtab) {
    auto base = reinterpret_cast<Elf32_Addr>(header);
    return reinterpret_cast<const char *>(base + strtab->d_un.d_ptr +
                                          symtab[index].st_name);
  }

  return nullptr;
}

} // namespace ctr_dl

#endif /* _3DS_DL_ELF_HPP */