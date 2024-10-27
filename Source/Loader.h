#ifndef _CTRDL_LOADER_H
#define _CTRDL_LOADER_H

#include "Handle.h"
#include "Stream.h"

CTRDLHandle* ctrdl_loadObject(const char* name, int flags, CTRDLStream* stream, CTRDLResolverFn resolver, void* resolverUserData);
bool ctrdl_unloadObject(CTRDLHandle* handle);

#endif /* _CTRDL_LOADER_H */