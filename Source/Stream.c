#include "Stream.h"

static bool ctrdl_fileSeekImpl(CTRDLStream* stream, size_t offset) {
    return !fseek((FILE*)stream->handle, offset, SEEK_SET);
}

static bool ctrdl_fileReadImpl(CTRDLStream* stream, void* out, size_t size) {
    FILE* f = (FILE*)stream->handle;
    size_t dataRead = 0;

    while (dataRead < size) {
        const size_t toRead = size - dataRead;
        size_t ret = fread((u8*)(out) + dataRead, 1, toRead, (FILE*)stream->handle);
        if (ret != toRead) {
            if (feof(f) || ferror(f))
                return false;
        }

        dataRead += ret;
    }

    return true;
}

void ctrdl_makeFileStream(CTRDLStream* stream, FILE* f) {
    stream->handle = (void*)f;
    stream->seek = ctrdl_fileSeekImpl;
    stream->read = ctrdl_fileReadImpl;
}