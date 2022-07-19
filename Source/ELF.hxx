#ifndef _3DS_DL_ELF_HXX
#define _3DS_DL_ELF_HXX

#include "Error.hxx"
#include <elf.h>
#include <vector>

namespace ctr {
inline bool validateELF(const Elf32_Ehdr *header, const std::size_t size) {
  // Size
  if (size < sizeof(Elf32_Ehdr) || size < header->e_ehsize) {
    ctr::setLastError(ctr::ERR_INVALID_PARAM);
    return false;
  }

  // Magic
  if (!std::equal(header->e_ident, header->e_ident + SELFMAG, ELFMAG)) {
    ctr::setLastError(ctr::ERR_INVALID_OBJECT);
    return false;
  }

  // Bitness
  if (header->e_ident[EI_CLASS] != ELFCLASS32) {
    ctr::setLastError(ctr::ERR_INVALID_BIT);
    return false;
  }

  // Endianess
  if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
    ctr::setLastError(ctr::ERR_INVALID_OBJECT);
    return false;
  }

  // Position-indipendent executables
  if (header->e_type != ET_DYN) {
    ctr::setLastError(ctr::ERR_INVALID_OBJECT);
    return false;
  }

  // Architecture
  if (header->e_machine != EM_ARM) {
    ctr::setLastError(ctr::ERR_INVALID_ARCH);
    return false;
  }

  return true;
}

inline Elf32_Phdr *getELFProgramHeader(const Elf32_Ehdr *header) {
  if (header->e_phnum) {
    auto base = reinterpret_cast<Elf32_Addr>(header);
    return reinterpret_cast<Elf32_Phdr *>(base + header->e_phoff);
  }

  return nullptr;
}

inline std::vector<Elf32_Phdr *> getELFSegments(const Elf32_Ehdr *header,
                                                const Elf32_Word type) {
  std::vector<Elf32_Phdr *> buffer;
  auto PH = getELFProgramHeader(header);

  if (PH) {
    for (auto i = 0u; i < header->e_phnum; ++i) {
      if (PH[i].p_type == type)
        buffer.push_back(&PH[i]);
    }
  }

  return buffer;
}

inline Elf32_Phdr *getELFSegment(const Elf32_Ehdr *header,
                                 const Elf32_Word type) {
  auto segments = getELFSegments(header, type);
  return segments.empty() ? nullptr : segments[0];
}

inline Elf32_Dyn *getELFDynEntry(const Elf32_Ehdr *header,
                                 const Elf32_Sword tag) {
  auto base = reinterpret_cast<Elf32_Addr>(header);
  auto dyn = getELFSegment(header, PT_DYNAMIC);

  if (dyn) {
    auto entry = reinterpret_cast<Elf32_Dyn *>(base + dyn->p_offset);

    while (entry->d_tag != DT_NULL) {
      if (entry->d_tag == static_cast<Elf32_Sword>(tag))
        return entry;

      ++entry;
    }
  }

  return nullptr;
}

} // namespace ctr

#endif /* _3DS_DL_ELF_HXX */