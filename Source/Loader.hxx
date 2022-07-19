#ifndef _3DS_DL_LOADER_HXX
#define _3DS_DL_LOADER_HXX

#include "Handle.hxx"

namespace ctr {
DLHandle *openOrLoadObject(const char *name, const std::uint8_t *buffer,
                           const std::size_t size, const std::uint32_t flags);
bool unloadObject(DLHandle &handle);
} // namespace ctr

#endif /* _3DS_DL_LOADER_HXX */