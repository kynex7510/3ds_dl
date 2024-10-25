#include "Handle.h"
#include "Error.h"

#include <string.h>

static CTRDLHandle g_Handles[CTRDL_MAX_HANDLES] = {};
static RecursiveLock g_Mtx;

// Returns true if the handle needs to be freed.
static bool decrementRefCount(CTRDLHandle* handle) {
    if (handle->refc)
        --handle->refc;
        
    return !handle->refc;
}

static void ctrdl_handleMtxLazyInit(void) {
    static u8 initialized = 0;

    if (!__ldrexb(&initialized)) {
        RecursiveLock_Init(&g_Mtx);

        while (__strexb(&initialized, 1))
            __ldrexb(&initialized);
    } else {
        __clrex();
    }
}

void ctrdl_acquireHandleMtx(void) {
    ctrdl_handleMtxLazyInit();
    RecursiveLock_Lock(&g_Mtx);
}

void ctrdl_releaseHandleMtx(void) {
    ctrdl_handleMtxLazyInit();
    RecursiveLock_Unlock(&g_Mtx);
}

CTRDLHandle* ctrdl_createHandle(const char* path, size_t flags) {
    CTRDLHandle* handle = NULL;

    const size_t pathSize = strlen(path);
    if (pathSize > (CTRDL_PATHBUF_SIZE - 1)) {
        ctrdl_setLastError(Err_LargePath);
        return NULL;
    }

    ctrdl_acquireHandleMtx();

    // Look for free handle.
    for (size_t i = 0; i < CTRDL_MAX_HANDLES; ++i) {
        CTRDLHandle* h = ctrdl_getHandleByIndex(i);
        if (!h->refc) {
            handle = h;
            break;
        }
    }

    // Initialize the handle if we have found one.
    if (handle) {
        if (path) {
            memcpy(handle->path, path, pathSize);
            handle->path[pathSize] = '\0';
        }

        handle->flags = flags;
        handle->refc = 1;
    } else {
        ctrdl_setLastError(Err_HandleLimit);
    }

    ctrdl_releaseHandleMtx();
    return handle;
}

bool ctrdl_freeHandle(CTRDLHandle* handle) {
    bool ret = true;

    if (!handle) {
        ctrdl_setLastError(Err_InvalidParam);
        return false;
    }

    ctrdl_acquireHandleMtx();

    if (decrementRefCount(handle)) {
        ret = true; // TODO: ctrdl_unloadObject(handle);
        if (ret) {
            memset(handle, 0, sizeof(*handle));
        }
    }

    ctrdl_releaseHandleMtx();
    return ret;
}

CTRDLHandle* ctrdl_getHandleByIndex(size_t index) {
    if (index < CTRDL_MAX_HANDLES)
        return &g_Handles[index];

    return NULL;
}

CTRDLHandle* ctrdl_findHandleByName(const char* name) {
    CTRDLHandle* found = NULL;

    if (!name) {
        ctrdl_setLastError(Err_InvalidParam);
        return NULL;
    }

    ctrdl_acquireHandleMtx();

    // Look for handle.
    for (size_t i = 0; i < CTRDL_MAX_HANDLES; ++i) {
        CTRDLHandle* h = ctrdl_getHandleByIndex(i);
        if (h->refc && strstr(h->path, name)) {
            ++h->refc;
            found = h;
            break;
        }
    }

    if (!found)
        ctrdl_setLastError(Err_NotFound);

    ctrdl_releaseHandleMtx();
    return found;
}