#include <3ds/synchronization.h>

#include "dlfcn.h"
#include "error.h"

// Globals

static const char *ERRORS[MAX_ERRORS] = {
    "invalid parameter",
    "could not read input file",
    "no memory",
    "hit the handle limit",
    "hit the reference limit",
    "not found",
    "invalid object",
    "the object is not 32-bit",
    "the object is not position indipendent",
    "unknown architecture",
    "could not map object",
    "path too long",
    "relocation failed",
    "too many dependencies",
    "could not load dependency",
    "could not unload object",
};

static __thread size_t g_LastError = MAX_ERRORS;

// Error

size_t ctrdl_getLastError() {
  size_t error = g_LastError;
  ctrdl_resetLastError();
  return error;
}

void ctrdl_setLastError(const size_t error) { g_LastError = error; }

void ctrdl_resetLastError() { ctrdl_setLastError(MAX_ERRORS); }

const char *ctrdl_getErrorAsString(const size_t index) {
  if (index < MAX_ERRORS)
    return ERRORS[index];

  return NULL;
}

// API

const char *dlerror() { return ctrdl_getErrorAsString(ctrdl_getLastError()); }