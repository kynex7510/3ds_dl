#ifndef _3DS_DL_HANDLE_HPP
#define _3DS_DL_HANDLE_HPP

#include <cstdint>

namespace ctr_dl {
constexpr static std::size_t MAX_PATH = 355u;
constexpr static std::size_t MAX_DEPS = 32u;

struct DLHandle {
  /* 0x0000 */ char path[MAX_PATH + 1];
  /* 0x0164 */ std::uintptr_t deps[MAX_PATH];
  /* 0x01E4 */ std::uintptr_t base;
  /* 0x01E8 */ std::size_t size;
  /* 0x01EC */ std::uintptr_t ep;
  /* 0x01F0 */ std::uint32_t flags;
  /* 0x01F4 */ std::uint32_t refc;
  /* 0x01F8 */ std::uintptr_t fini;
  /* 0x01FC */ std::size_t finiSize;
};

DLHandle *findHandle(const char *name);
DLHandle *createHandle(const char *path, const std::uint32_t flags);
bool freeHandle(DLHandle *handle);
} // namespace ctr_dl

#endif /* _3DS_DL_HANDLE_HPP */