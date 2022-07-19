#include <sys/stat.h>
#include <unistd.h>

#include "Error.hxx"
#include "Handle.hxx"
#include "Loader.hxx"
#include "dlfcn.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace ctr;

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

void *dlopen(const char *filename, int flags) {
  return dlopen_ex(filename, nullptr, flags);
}

void *dlopen_ex(const char *path, void *(*cb)(char const *symbol), int flags) {
  std::uint8_t *buffer = nullptr;
  std::size_t fileSize = 0u;

  // No reason to support main handle by NULL.
  if (!path) {
    ctr::setLastError(ctr::ERR_INVALID_PARAM);
    return nullptr;
  }

  // Validate path length.
  if (std::strlen(path) > ctr::MAX_PATH) {
    ctr::setLastError(ctr::ERR_BIG_PATH);
    return nullptr;
  }

  // Avoid reading if already open.
  if (auto handle = ctr::findHandle(path)) {
    handle->flags = flags;
    return reinterpret_cast<void *>(handle);
  }

  // Only read if load is required.
  if (!(flags & RTLD_NOLOAD)) {
    fileSize = getFileSize(path);

    if (!fileSize) {
      ctr::setLastError(ctr::ERR_READ_FAILED);
      return nullptr;
    }

    buffer = readBinary(path, fileSize);

    if (!buffer) {
      ctr::setLastError(ctr::ERR_READ_FAILED);
      return nullptr;
    }
  }

  auto ret = ctr::openOrLoadObject(path, buffer, fileSize, flags);

  std::free(reinterpret_cast<void *>(buffer));
  return reinterpret_cast<void *>(ret);
}

void *dlmap(const unsigned char *buffer, const size_t size, int flags) {
  return dlmap_ex(buffer, size, nullptr, flags);
}

void *dlmap_ex(const unsigned char *buffer, const size_t size,
               void *(*cb)(char const *symbol), int flags) {
  return reinterpret_cast<void *>(
      ctr::openOrLoadObject(nullptr, buffer, size, flags));
}