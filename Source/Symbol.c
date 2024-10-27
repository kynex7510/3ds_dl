#include "Symbol.h"

typedef struct {
    CTRDLHandle* deps[CTRDL_MAX_HANDLES];
    size_t size;
    size_t index;
} DepQueue;

static CTRL_INLINE void ctrdl_depQueueInit(DepQueue* q) {
    q->size = 0;
    q->index = 0;
}

static CTRL_INLINE bool ctrdl_depQueueIsEmpty(DepQueue* q) { return q->size == 0; }
static CTRL_INLINE bool ctrdl_depQueueIsFull(DepQueue* q) { return q->size >= CTRDL_MAX_HANDLES; }

static void ctrdl_depQueuePush(DepQueue* q, CTRDLHandle* handle) {
    if (handle && !ctrdl_depQueueIsFull(q)) {
        for (size_t i = 0; i < q->size; ++i) {
            if (q->deps[i] == handle)
                return;
        }

        q->deps[q->size % CTRDL_MAX_HANDLES] = handle;
        ++q->size;
    }
}

static CTRDLHandle* ctrdl_depQueuePop(DepQueue* q) {
    if (!ctrdl_depQueueIsEmpty(q)) {
        CTRDLHandle* h = q->deps[q->index];
        q->index = (q->index + 1) % CTRDL_MAX_HANDLES;
        --q->size;
        return h;
    }

    return NULL;
}

u32 ctrdl_findSymbolValueInHandle(CTRDLHandle* handle, const char* sym) {
    u32 found = 0;

    if (handle) {
        ctrdl_lockHandle(handle);

        const Elf32_Word hash = ctrdl_getELFSymHash(sym);
        size_t chainIndex = handle->symBuckets[hash % handle->numSymBuckets];

        while (chainIndex != STN_UNDEF) {
            const Elf32_Sym* s = &handle->symEntries[chainIndex];
            const char* name = &handle->stringTable[s->st_name];
            if (!strcmp(name, sym)) {
                found = handle->base + s->st_value;
                break;
            }

            chainIndex = handle->symChains[chainIndex];
        }

        ctrdl_unlockHandle(handle);
    }

    return found;
}

static u32 ctrdl_findSymbolValueBreadthFirst(DepQueue* q, const char* sym) {
    while (!ctrdl_depQueueIsEmpty(q)) {
        CTRDLHandle* h = ctrdl_depQueuePop(q);
        u32 addr = ctrdl_findSymbolValueInHandle(h, sym);
        if (addr)
            return addr;

        for (size_t i = 0; i < CTRDL_MAX_DEPS; ++i)
            ctrdl_depQueuePush(q, h->deps[i]);
    }

    return 0;
}

u32 ctrdl_findSymbolValue(CTRDLHandle* handle, const char* sym) {
    DepQueue q;
    u32 addr = 0;

    if (handle) {
        ctrdl_lockHandle(handle);
        ctrdl_depQueueInit(&q);
        ctrdl_depQueuePush(&q, handle);
        addr = ctrdl_findSymbolValueBreadthFirst(&q, sym);
        ctrdl_unlockHandle(handle);
    }

    return addr;
}