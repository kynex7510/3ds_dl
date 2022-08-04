#ifndef _3DS_DL_RELOCS_H
#define _3DS_DL_RELOCS_H

#include "dlfcn.h"
#include "elfutil.h"
#include "handle.h"

int ctrdl_handleRelocs(const struct DLHandle *handle, const Elf32_Ehdr *header,
                       const SymResolver resolver);

#endif /* _3DS_DL_RELOCS_H */