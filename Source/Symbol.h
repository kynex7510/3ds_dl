#ifndef _CTRDL_SYMBOL_H
#define _CTRDL_SYMBOL_H

#include "Handle.h"

const Elf32_Sym* ctrdl_findSymbolFromName(CTRDLHandle* handle, const char* name);
const Elf32_Sym* ctrdl_extendedFindSymbolFromName(CTRDLHandle* handle, const char* name);
const Elf32_Sym* ctrdl_findSymbolFromValue(CTRDLHandle* handle, Elf32_Word value);

#endif /* _CTRDL_SYMBOL_H */