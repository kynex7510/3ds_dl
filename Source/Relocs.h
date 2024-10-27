#ifndef _CTRDL_RELOCS_H
#define _CTRDL_RELOCS_H

#include "ELFUtil.h"
#include "Handle.h"

bool ctrdl_handleRelocs(CTRDLHandle* handle, CTRDLElf* elf, CTRDLResolverFn resolver, void* resolverUserData);

#endif /* _CTRDL_RELOCS_H */