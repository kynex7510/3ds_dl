#include "Error.hxx"
#include "dlfcn.h"

using namespace ctr;

// Globals

constexpr static auto MAX_ERRORS = 12u;

constexpr static const char *ERRORS[] = {
    "Invalid parameter",
    "Could not read input file",
    "No memory",
    "Hit the handle limit",
    "Hit the reference limit",
    "Not found",
    "Invalid object",
    "The object is not 32-bit",
    "The object is not position indipendent",
    "Unknown architecture",
    "Could not map object",
    "Path too long",
};

thread_local std::size_t g_LastError = MAX_ERRORS;

// Error

std::size_t ctr::getLastError() {
  auto error = g_LastError;
  resetLastError();
  return error;
}

void ctr::setLastError(const std::size_t error) { g_LastError = error; }

void ctr::resetLastError() { setLastError(MAX_ERRORS); }

const char *ctr::getErrorAsString(const std::size_t index) {
  if (index < MAX_ERRORS)
    return ERRORS[index];

  return nullptr;
}

// API

const char *dlerror() { return ctr::getErrorAsString(ctr::getLastError()); }