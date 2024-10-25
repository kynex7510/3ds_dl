#include <sys/stat.h>
#include <unistd.h>

#include "dlfcn.h"
#include "Error.h"
#include "Handle.h"
#include "Init.h"
#include "Loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helpers

static size_t getFileSize(const char *path) {
  struct stat info;
  memset(&info, 0x00, sizeof(struct stat));

  if (!stat(path, &info))
    return info.st_size;

  return 0u;
}

static void *readBinary(const char *path, const size_t size) {
  FILE *h = fopen(path, "rb");

  if (h) {
    void *buffer = calloc(1, size);

    if (buffer) {
      size_t totalRead = 0;
      do {
        size_t count =
            fread((void *)((char *)buffer + totalRead), 1, size - totalRead, h);

        if (ferror(h)) {
          free(buffer);
          buffer = NULL;
          break;
        }

        totalRead += count;
        break; // Workaround.
      } while (!feof(h));
    }

    fclose(h);
    return buffer;
  }

  return NULL;
}

// Wrappers

void *dlopen(const char *filename, const int flags) {
  return dlopen_ex(filename, NULL, flags);
}

void *dlopen_ex(const char *path, const SymResolver resolver, const int flags) {
  void *buffer = NULL;
  size_t fileSize = 0u;

  // Lazy initialization.
  ctrdl_lazyInit();

  // No reason to support main handle by NULL.
  if (!path) {
    ctrdl_setLastError(ERR_INVALID_PARAM);
    return NULL;
  }

  // Validate path length.
  if (strlen(path) > MAX_PATH) {
    ctrdl_setLastError(ERR_BIG_PATH);
    return NULL;
  }

  // Avoid reading if already open.
  struct DLHandle *handle = ctrdl_findHandle(path);
  if (handle) {
    handle->flags = flags;
    return (void *)(handle);
  }

  // Only read if load is required.
  if (!(flags & RTLD_NOLOAD)) {
    fileSize = getFileSize(path);

    if (!fileSize) {
      ctrdl_setLastError(ERR_READ_FAILED);
      return NULL;
    }

    buffer = readBinary(path, fileSize);

    if (!buffer) {
      ctrdl_setLastError(ERR_READ_FAILED);
      return NULL;
    }
  }

  handle = ctrdl_openOrLoadObject(path, buffer, fileSize, resolver, flags);

  free(buffer);
  return (void *)(handle);
}

void *dlmap(const void *buffer, const size_t size, const int flags) {
  return dlmap_ex(buffer, size, NULL, flags);
}

void *dlmap_ex(const void *buffer, const size_t size,
               const SymResolver resolver, const int flags) {
  // Lazy initialization.
  ctrdl_lazyInit();

  return (void *)ctrdl_openOrLoadObject(NULL, buffer, size, resolver, flags);
}