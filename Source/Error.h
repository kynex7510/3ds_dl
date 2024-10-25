#ifndef _3DS_DL_ERROR_H
#define _3DS_DL_ERROR_H

#include <stdint.h>

#define ERR_INVALID_PARAM 0
#define ERR_READ_FAILED 1
#define ERR_NO_MEMORY 2
#define ERR_HANDLE_LIMIT 3
#define ERR_REF_LIMIT 4
#define ERR_NOT_FOUND 5
#define ERR_INVALID_OBJECT 6
#define ERR_INVALID_BIT 7
#define ERR_NOT_PIE 8
#define ERR_INVALID_ARCH 9
#define ERR_MAP_ERROR 10
#define ERR_BIG_PATH 11
#define ERR_RELOC_FAIL 12
#define ERR_DEPS_LIMIT 13
#define ERR_DEP_FAIL 14
#define ERR_FREE_FAILED 15
#define MAX_ERRORS 16

size_t ctrdl_getLastError();
void ctrdl_setLastError(const size_t error);
void ctrdl_resetLastError();
const char *ctrdl_getErrorAsString(const size_t index);

#endif /* _3DS_DL_ERROR_H */