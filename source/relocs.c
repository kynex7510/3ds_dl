#include "relocs.h"
#include "dlfcn.h"

// Types

struct RelContext {
  const struct DLHandle *handle;
  const Elf32_Ehdr *header;
  const Elf32_Sym *symtab;
  const Elf32_Dyn *strtab;
  SymResolver resolver;
};

struct RelEntry {
  uintptr_t offset;
  uintptr_t symbol;
  uint32_t addend;
  uint8_t type;
};

// Helpers

static uintptr_t resolveSymbol(const struct RelContext *ctx,
                               const Elf32_Word index) {
  if (index == STN_UNDEF)
    return 0;

  const char *name =
      ctrdl_getELFSymName(ctx->header, ctx->symtab, ctx->strtab, index);

  if (!name)
    return 0;

  // If we have a resolver, use it first.
  if (ctx->resolver) {
    void *addr = ctx->resolver(name);
    if (addr)
      return (uintptr_t)(addr);
  }

  // TODO: look in dependencies.
  return 0;
}

static int handleSingle(const struct RelContext *ctx,
                        const struct RelEntry *entry) {
  if (entry->type == R_ARM_RELATIVE) {
    if (entry->addend) {
      *(uintptr_t *)(entry->offset) = ctx->handle->base + entry->addend;
    } else {
      *(uintptr_t *)(entry->offset) += ctx->handle->base;
    }
    return 1;
  }

  if (entry->type == R_ARM_ABS32 || entry->type == R_ARM_GLOB_DAT ||
      entry->type == R_ARM_JUMP_SLOT) {
    if (entry->symbol) {
      *(uintptr_t *)(entry->offset) = entry->symbol + entry->addend;
      return 1;
    }
  }

  return 0;
}

static int handleRelArray(const struct RelContext *ctx,
                          const Elf32_Rel *relArray,
                          const size_t relArraySize) {
  for (size_t i = 0; i < relArraySize; ++i) {
    struct RelEntry entry;
    const Elf32_Rel *rel = &relArray[i];

    entry.offset = ctx->handle->base + rel->r_offset;
    entry.symbol = resolveSymbol(ctx, ELF32_R_SYM(rel->r_info));
    entry.addend = 0;
    entry.type = ELF32_R_TYPE(rel->r_info);

    if (!handleSingle(ctx, &entry))
      return 0;
  }

  return 1;
}

static int handleRelaArray(const struct RelContext *ctx,
                           const Elf32_Rela *relaArray,
                           const size_t relaArraySize) {
  for (size_t i = 0; i < relaArraySize; ++i) {
    struct RelEntry entry;
    const Elf32_Rela *rela = &relaArray[i];

    entry.offset = ctx->handle->base + rela->r_offset;
    entry.symbol = resolveSymbol(ctx, ELF32_R_SYM(rela->r_info));
    entry.addend = rela->r_addend;
    entry.type = ELF32_R_TYPE(rela->r_info);

    if (!handleSingle(ctx, &entry))
      return 0;
  }

  return 1;
}

static int handleRel(const struct RelContext *ctx) {
  const Elf32_Dyn *relEntry = ctrdl_getELFDynEntry(ctx->header, DT_REL);
  const Elf32_Dyn *relEntrySize = ctrdl_getELFDynEntry(ctx->header, DT_RELSZ);
  const Elf32_Dyn *relEntryEnt = ctrdl_getELFDynEntry(ctx->header, DT_RELENT);

  if (relEntry && relEntrySize && relEntryEnt) {
    // Get REL array.
    const Elf32_Rel *relArray =
        (const Elf32_Rel *)(ctx->handle->base + relEntry->d_un.d_ptr);
    const size_t relArraySize =
        relEntrySize->d_un.d_val / relEntryEnt->d_un.d_val;
    return handleRelArray(ctx, relArray, relArraySize);
  }

  return 1;
}

static int handleRela(const struct RelContext *ctx) {
  const Elf32_Dyn *relaEntry = ctrdl_getELFDynEntry(ctx->header, DT_RELA);
  const Elf32_Dyn *relaEntrySize = ctrdl_getELFDynEntry(ctx->header, DT_RELASZ);
  const Elf32_Dyn *relaEntryEnt = ctrdl_getELFDynEntry(ctx->header, DT_RELAENT);

  if (relaEntry && relaEntrySize && relaEntryEnt) {
    // Get RELA array.
    const Elf32_Rela *relaArray =
        (const Elf32_Rela *)(ctx->handle->base + relaEntry->d_un.d_ptr);
    const size_t relaArraySize =
        relaEntrySize->d_un.d_val / relaEntryEnt->d_un.d_val;
    return handleRelaArray(ctx, relaArray, relaArraySize);
  }

  return 1;
}

static int handleJmprel(const struct RelContext *ctx) {
  const Elf32_Dyn *jmprelEntry = ctrdl_getELFDynEntry(ctx->header, DT_JMPREL);
  const Elf32_Dyn *jmprelEntrySize =
      ctrdl_getELFDynEntry(ctx->header, DT_PLTRELSZ);
  const Elf32_Dyn *jmprelEntryType =
      ctrdl_getELFDynEntry(ctx->header, DT_PLTREL);

  if (jmprelEntry && jmprelEntrySize && jmprelEntryType) {
    // Get JMPREL array.
    if (jmprelEntryType->d_un.d_val == DT_REL) {
      const Elf32_Rel *jmprelArray =
          (const Elf32_Rel *)(ctx->handle->base + jmprelEntry->d_un.d_ptr);
      const size_t jmprelArraySize =
          jmprelEntrySize->d_un.d_val / sizeof(Elf32_Rel);
      return handleRelArray(ctx, jmprelArray, jmprelArraySize);
    } else if (jmprelEntryType->d_un.d_val == DT_RELA) {
      const Elf32_Rela *jmprelArray =
          (const Elf32_Rela *)(ctx->handle->base + jmprelEntry->d_un.d_ptr);
      const size_t jmprelArraySize =
          jmprelEntrySize->d_un.d_val / sizeof(Elf32_Rela);
      return handleRelaArray(ctx, jmprelArray, jmprelArraySize);
    }

    return 0;
  }

  return 1;
}

// Relocs

int ctrdl_handleRelocs(const struct DLHandle *handle, const Elf32_Ehdr *header,
                       const SymResolver resolver) {
  struct RelContext ctx;

  ctx.handle = handle;
  ctx.header = header;
  ctx.symtab = ctrdl_getELFSymTab(ctx.header);
  ctx.strtab = ctrdl_getELFDynEntry(ctx.header, DT_STRTAB);
  ctx.resolver = resolver;

  if (ctx.symtab && ctx.strtab)
    return handleRel(&ctx) && handleRela(&ctx) && handleJmprel(&ctx);

  return 0;
}