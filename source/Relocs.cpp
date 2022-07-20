#include "Relocs.hpp"
#include "dlfcn.h"

using namespace ctr_dl;

// Types

struct RelContext {
  DLHandle &handle;
  const Elf32_Ehdr *header;
  const Elf32_Sym *symtab;
  const Elf32_Dyn *strtab;
  SymResolver resolver;
};

struct RelEntry {
  std::uintptr_t offset;
  std::uintptr_t symbol;
  std::uint32_t addend;
  std::uint8_t type;
};

// Helpers

/*
static Elf32_Word convert(const Elf32_Word v) {
  return ((v << 24) & 0xFF000000) | ((v << 8) & 0x00FF0000) |
         ((v >> 8) & 0x0000FF00) | ((v >> 24) & 0x000000FF);
}
*/

static std::uintptr_t resolveSymbol(const RelContext &ctx,
                                    const Elf32_Word index) {

  if (index == STN_UNDEF)
    return 0u;

  auto name = getELFSymName(ctx.header, ctx.symtab, ctx.strtab, index);

  if (!name)
    return 0;

  // If we have a resolver, use it first.
  if (ctx.resolver) {
    if (auto addr = ctx.resolver(name))
      return reinterpret_cast<std::uintptr_t>(addr);
  }

  // TODO: look in dependencies.

  return 0u;
}

static bool handleSingle(const RelContext &ctx, const RelEntry &entry) {
  if (entry.type == R_ARM_RELATIVE) {
    if (entry.addend) {
      *reinterpret_cast<std::uintptr_t *>(entry.offset) =
          ctx.handle.base + entry.addend;
    } else {
      *reinterpret_cast<std::uintptr_t *>(entry.offset) += ctx.handle.base;
    }
    return true;
  }

  if (entry.type == R_ARM_ABS32 || entry.type == R_ARM_GLOB_DAT ||
      entry.type == R_ARM_JUMP_SLOT) {
    *reinterpret_cast<std::uintptr_t *>(entry.offset) =
        entry.symbol + entry.addend;
    return true;
  };

  return false;
}

static bool handleRel(const RelContext &ctx) {
  auto relEntry = getELFDynEntry(ctx.header, DT_REL);
  auto relEntrySize = getELFDynEntry(ctx.header, DT_RELSZ);
  auto relEntryEnt = getELFDynEntry(ctx.header, DT_RELENT);

  if (relEntry && relEntrySize && relEntryEnt) {
    // Get REL array.
    auto relArray =
        reinterpret_cast<Elf32_Rel *>(ctx.handle.base + relEntry->d_un.d_ptr);
    auto relArraySize = relEntrySize->d_un.d_val / relEntryEnt->d_un.d_val;

    // Handle relocations.
    for (auto i = 0u; i < relArraySize; ++i) {
      RelEntry entry = {};
      auto rel = &relArray[i];

      entry.offset = ctx.handle.base + rel->r_offset;
      entry.symbol = resolveSymbol(ctx, ELF32_R_SYM(rel->r_info));
      entry.type = ELF32_R_TYPE(rel->r_info);

      if (!handleSingle(ctx, entry))
        return false;
    }
  }

  return true;
}

static bool handleRela(const RelContext &ctx) {
  auto relaEntry = getELFDynEntry(ctx.header, DT_RELA);
  auto relaEntrySize = getELFDynEntry(ctx.header, DT_RELASZ);
  auto relaEntryEnt = getELFDynEntry(ctx.header, DT_RELAENT);

  if (relaEntry && relaEntrySize && relaEntryEnt) {
    // Get RELA array.
    auto relaArray =
        reinterpret_cast<Elf32_Rela *>(ctx.handle.base + relaEntry->d_un.d_ptr);
    auto relaArraySize = relaEntrySize->d_un.d_val / relaEntryEnt->d_un.d_val;

    // Handle relocations.
    for (auto i = 0u; i < relaArraySize; ++i) {
      RelEntry entry = {};
      auto rela = &relaArray[i];

      entry.offset = ctx.handle.base + rela->r_offset;
      entry.symbol = resolveSymbol(ctx, ELF32_R_SYM(rela->r_info));
      entry.addend = rela->r_addend;
      entry.type = ELF32_R_TYPE(rela->r_info);

      if (!handleSingle(ctx, entry))
        return false;
    }
  }

  return true;
}

// Relocs

bool ctr_dl::handleRelocs(DLHandle &handle, const Elf32_Ehdr *header,
                          const SymResolver resolver) {
  RelContext ctx = {handle};

  ctx.header = header;
  ctx.symtab = getELFSymTab(ctx.header);
  ctx.strtab = getELFDynEntry(ctx.header, DT_STRTAB);
  ctx.resolver = resolver;

  if (ctx.symtab && ctx.strtab) {
    if (handleRel(ctx))
      return handleRela(ctx);
  }

  return false;
}