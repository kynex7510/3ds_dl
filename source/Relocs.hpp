#ifndef _3DS_DL_RELOCS_HPP
#define _3DS_DL_RELOCS_HPP

#include "ELF.hpp"
#include "Handle.hpp"
#include "dlfcn.h"

namespace ctr_dl {
bool handleRelocs(DLHandle &handle, const Elf32_Ehdr *header,
                  const SymResolver resolver);
}

#endif /* _3DS_DL_RELOCS_HPP */