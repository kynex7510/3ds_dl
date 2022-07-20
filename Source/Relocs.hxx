#ifndef _3DS_DL_RELOCS_HXX
#define _3DS_DL_RELOCS_HXX

#include "ELF.hxx"
#include "Handle.hxx"
#include "dlfcn.h"

namespace ctr_dl {
bool handleRelocs(DLHandle &handle, const Elf32_Ehdr *header,
                  const SymResolver resolver);
}

#endif /* _3DS_DL_RELOCS_HXX */