#include "CTRL/Types.h"

#include "Relocs.h"
#include "dlfcn.h"

typedef struct {
    CTRDLHandle* handle;
    CTRDLElf* elf;
    CTRDLSymResolver resolver;
    void* resolverUserData;
} RelContext;

typedef struct {
  uintptr_t offset;
  uintptr_t symbol;
  uint32_t addend;
  uint8_t type;
} RelEntry;

static u32 ctrdl_resolveSymbol(const RelContext* ctx, Elf32_Word index) {
    if (index == STN_UNDEF)
        return 0;

    const char* sym = ctrdl_getELFStringByIndex(ctx->elf, index);

    // If we have a resolver, use it first.
    if (ctx->resolver) {
        void* addr = ctx->resolver(sym, ctx->resolverUserData);
        if (addr)
            return (u32)addr;
    }

    // Look in dependencies.
    for (size_t i = 0; i < CTRDL_MAX_DEPS; ++i) {
        CTRDLHandle* dep = ctx->handle->deps[i];
        if (dep) {
            void* addr = dlsym(dep, sym);
            if (addr)
                return (u32)addr;
        }
    }

    return 0;
}

static bool ctrdl_handleSingleReloc(RelContext* ctx, RelEntry* entry) {
    u32* dst = (u32*)entry->offset;

    switch (entry->type) {
        case R_ARM_RELATIVE:
            if (entry->addend) {
                *dst = ctx->handle->base + entry->addend;
            } else {
                *dst += ctx->handle->base;
            }
            return true;
        case R_ARM_ABS32:
        case R_ARM_GLOB_DAT:
        case R_ARM_JUMP_SLOT:
            if (entry->symbol) {
                *dst = entry->symbol + entry->addend;
                return true;
            }
            break;
    }

    return false;
}

static bool ctrdl_handleRel(RelContext* ctx) {
    const Elf32_Rel* relArray = ctx->elf->relArray;

    if (relArray) {
        const size_t size = ctx->elf->relArraySize;
        for (size_t i = 0; i < size; ++i) {
            RelEntry entry;
            const Elf32_Rel* rel = &relArray[i];

            entry.offset = ctx->handle->base + rel->r_offset;
            entry.symbol = ctrdl_resolveSymbol(ctx, ELF32_R_SYM(rel->r_info));
            entry.addend = 0;
            entry.type = ELF32_R_TYPE(rel->r_info);

            if (!ctrdl_handleSingleReloc(ctx, &entry))
                return false;
        }
    }

    return true;
}

static bool ctrdl_handleRela(RelContext* ctx) {
    const Elf32_Rela* relaArray = ctx->elf->relaArray;

    if (relaArray) {
        const size_t size = ctx->elf->relaArraySize;

        for (size_t i = 0; i < size; ++i) {
            RelEntry entry;
            const Elf32_Rela* rela = &relaArray[i];

            entry.offset = ctx->handle->base + rela->r_offset;
            entry.symbol = ctrdl_resolveSymbol(ctx, ELF32_R_SYM(rela->r_info));
            entry.addend = rela->r_addend;
            entry.type = ELF32_R_TYPE(rela->r_info);

            if (!ctrdl_handleSingleReloc(ctx, &entry))
                return false;
        }
    }

    return true;
}

bool ctrdl_handleRelocs(CTRDLHandle* handle, CTRDLElf* elf, CTRDLSymResolver resolver, void* resolverUserData) {
    RelContext ctx;
    ctx.handle = handle;
    ctx.elf = elf;
    ctx.resolver = resolver;
    ctx.resolverUserData = resolverUserData;
    return ctrdl_handleRel(&ctx) && ctrdl_handleRela(&ctx);
}