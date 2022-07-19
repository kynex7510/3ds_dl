#ifndef _3DS_DL_ERROR_HXX
#define _3DS_DL_ERROR_HXX

#include <cstdint>

namespace ctr {
constexpr static std::size_t ERR_INVALID_PARAM = 0u;
constexpr static std::size_t ERR_READ_FAILED = 1u;
constexpr static std::size_t ERR_NO_MEMORY = 2u;
constexpr static std::size_t ERR_HANDLE_LIMIT = 3u;
constexpr static std::size_t ERR_REF_LIMIT = 4u;
constexpr static std::size_t ERR_NOT_FOUND = 5u;
constexpr static std::size_t ERR_INVALID_OBJECT = 6u;
constexpr static std::size_t ERR_INVALID_BIT = 7u;
constexpr static std::size_t ERR_NOT_PIE = 8u;
constexpr static std::size_t ERR_INVALID_ARCH = 9u;
constexpr static std::size_t ERR_MAP_ERROR = 10u;
constexpr static std::size_t ERR_BIG_PATH = 11u;

std::size_t getLastError();
void setLastError(const std::size_t error);
void resetLastError();
const char *getErrorAsString(const std::size_t index);
} // namespace ctr

#endif /* _3DS_DL_ERROR_HXX */