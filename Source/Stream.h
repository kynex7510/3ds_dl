#ifndef _CTRDL_STREAM_H
#define _CTRDL_STREAM_H

#include <dlfcn.h>
#include <stdio.h>

typedef struct CTRDLStream;

typedef bool(*CTRDLSeekFn)(CTRDLStream*, size_t offset);
typedef bool(*CTRDLReadFn)(CTRDLStream*, void* out, size_t size);

typedef struct {
    void* handle;     // Opaque handle.
    CTRDLSeekFn seek; // Seek function.
    CTRDLReadFn read; // Read function.
} CTRDLStream;

void ctrdl_makeFileStream(CTRDLStream* stream, FILE* f);

#endif /* _CTRDL_STREAM_H */