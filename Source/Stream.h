#ifndef _CTRDL_STREAM_H
#define _CTRDL_STREAM_H

#include <dlfcn.h>
#include <stdio.h>

typedef bool(*CTRDLSeekFn)(void* stream, size_t offset);
typedef bool(*CTRDLReadFn)(void* stream, void* out, size_t size);

typedef struct {
    void* handle;     // Opaque handle.
    CTRDLSeekFn seek; // Seek function.
    CTRDLReadFn read; // Read function.
    size_t size;      // Stream size (memory only).
    size_t offset;    // Stream offset (memory only).
} CTRDLStream;

void ctrdl_makeFileStream(CTRDLStream* stream, FILE* f);
void ctrdl_makeMemStream(CTRDLStream* stream, const void* buffer, size_t size);

#endif /* _CTRDL_STREAM_H */