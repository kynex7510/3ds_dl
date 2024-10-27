#ifndef _CTRDL_SYMBOL_H
#define _CTRDL_SYMBOL_H

#include "Handle.h"

u32 ctrdl_findSymbolValueInHandle(CTRDLHandle* handle, const char* sym);
u32 ctrdl_findSymbolValue(CTRDLHandle* handle, const char* sym);

#endif /* _CTRDL_SYMBOL_H */