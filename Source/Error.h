#ifndef _CTRDL_ERROR_H
#define _CTRDL_ERROR_H

#include <dlfcn.h>

typedef enum {
    Err_OK,
    Err_InvalidParam,
    Err_ReadFailed,
    Err_NoMemory,
    Err_HandleLimit,
    Err_RefLimit,
    Err_NotFound,
    Err_InvalidObject,
    Err_InvalidBit,
    Err_NotPIE,
    Err_InvalidArch,
    Err_MapError,
    Err_LargePath,
    Err_RelocFailed,
    Err_DepsLimit,
    Err_DepFailed,
    Err_FreeFailed,
} CTRDLError;

CTRDLError ctrdl_getLastError(void);
void ctrdl_setLastError(CTRDLError error);
void ctrdl_resetLastError(void);
const char* ctrdl_getErrorAsString(CTRDLError error);

#endif /* _CTRDL_ERROR_H */