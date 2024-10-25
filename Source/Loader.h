#ifndef _3DS_DL_LOADER_H
#define _3DS_DL_LOADER_H

#include "dlfcn.h"
#include "Handle.h"

struct DLHandle *ctrdl_openOrLoadObject(const char *name, const void *buffer,
                                        const size_t size,
                                        const SymResolver resolver,
                                        const uint32_t flags);
int ctrdl_unloadObject(struct DLHandle *handle);

#endif /* _3DS_DL_LOADER_H */