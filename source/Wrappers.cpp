#include <sys/stat.h>
#include <unistd.h>

#include "Error.hpp"
#include "Handle.hpp"
#include "Loader.hpp"
#include "dlfcn.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace ctr_dl;

// Helpers

static size_t getFileSize(const char *path) {
  struct stat info = {};

  if (!stat(path, &info))
    return info.st_size;

  return 0u;
}

static std::uint8_t *readBinary(const char *path, const size_t size) {
  auto h = std::fopen(path, "rb");

  if (h) {
    auto buffer = std::calloc(1, size);

    if (buffer) {
      std::fread(buffer, size, 1, h);
      if (std::ferror(h) || !std::feof(h)) {
        std::free(buffer);
        buffer = nullptr;
      }
    }

    std::fclose(h);
    return reinterpret_cast<std::uint8_t *>(buffer);
  }

  return nullptr;
}

// Wrappers

void *dlopen(const char *filename, const int flags) {
  return dlopen_ex(filename, nullptr, flags);
}

void *dlopen_ex(const char *path, const SymResolver resolver, const int flags) {
  std::uint8_t *buffer = nullptr;
  std::size_t fileSize = 0u;

  // No reason to support main handle by NULL.
  if (!path) {
    ctr_dl::setLastError(ctr_dl::ERR_INVALID_PARAM);
    return nullptr;
  }

  // Validate path length.
  if (std::strlen(path) > ctr_dl::MAX_PATH) {
    ctr_dl::setLastError(ctr_dl::ERR_BIG_PATH);
    return nullptr;
  }

  // Avoid reading if already open.
  if (auto handle = ctr_dl::findHandle(path)) {
    handle->flags = flags;
    return reinterpret_cast<void *>(handle);
  }

  // Only read if load is required.
  if (!(flags & RTLD_NOLOAD)) {
    fileSize = getFileSize(path);

    if (!fileSize) {
      ctr_dl::setLastError(ctr_dl::ERR_READ_FAILED);
      return nullptr;
    }

    buffer = readBinary(path, fileSize);

    if (!buffer) {
      ctr_dl::setLastError(ctr_dl::ERR_READ_FAILED);
      return nullptr;
    }
  }

  auto ret = ctr_dl::openOrLoadObject(path, buffer, fileSize, resolver, flags);

  std::free(reinterpret_cast<void *>(buffer));
  return reinterpret_cast<void *>(ret);
}

void *dlmap(const unsigned char *buffer, const size_t size, const int flags) {
  return dlmap_ex(buffer, size, nullptr, flags);
}

void *dlmap_ex(const unsigned char *buffer, const size_t size,
               const SymResolver resolver, const int flags) {
  return reinterpret_cast<void *>(
      ctr_dl::openOrLoadObject(nullptr, buffer, size, resolver, flags));
}