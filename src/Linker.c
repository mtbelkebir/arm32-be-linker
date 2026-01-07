#include "Linker.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "uthash.h"
#include "util.h"

/**
 * Defines exactly *how* to merge two sections.
 */
typedef enum MergeStrategy {
  Concatenate, /**< For sections such a .text, .data, etc. Add them
                  together. */
  Singleton, /**< For sections like .ARM.attributes that should only be kept
                  once. */
  NoBits, /**< For sections like .bss whose merging is just adding up their
                  sizes */
  Ignore, /**< For .symtab, .shstrab, etc. Those will be treated in an future
              step */
} MergeStrategy;

/**
 * This struct is meant to keep track of sections merging (who goes into
 * whom?). The use of uthash for this greatly simplifies lookup by name and
 * allows matching in O(n) instead of O(n²).
 *
 * It is only for internal use hence why it's declared here instead of the .h
 */
typedef struct SectionMap {
  char* name; /*< Will be used as a key */
  ElfSection* target;
  ElfSection* source;
  uint32_t offset; /*< Position of f2 in target */
  uint32_t new_idx;
  MergeStrategy strategy;
  UT_hash_handle hh;
} SectionMap;

typedef struct SymbolMap {
  ElfSymbol* symbol;
  size_t offset;
  bool is_defined;
  char* name;
  UT_hash_handle hh;
} SymbolMap;

typedef struct LinkerContext {
  ElfFile* out;
  ElfFile* in1;
  ElfFile* in2;
  SectionMap* section_map;

  // Keep track of what end up where in the merged file s.t.
  // f1_sec_to_out_idx[old_idx] = new_idx
  uint32_t* f1_sec_to_out_idx;
  uint32_t* f2_sec_to_out_idx;

  // Same thing as earlier but with symbols.
  uint32_t* f1_sym_to_out_idx;
  uint32_t* f2_sym_to_out_idx;

  // idx of symtab in the result file.
  uint32_t symtab_idx;
  // Merge offsets.
  uint32_t* f2_offsets;
  SymbolMap* symbol_map;
} LinkerContext;

static void SectionMapUpdateSource(const SectionMap* map, const char* name,
                                   ElfSection* source, uint32_t delta);
static ElfSection MergeSections(SectionMap* entry);
bool isSingleton(const ElfSection* section);
static void SectionMapAddSection(SectionMap** map, const char* name,
                                 ElfSection* target);
static MergeStrategy GetMergeStrategy(ElfSection* section);
static LinkerStatus LinkerGenerateShstrtab(const LinkerContext* ctx);
static LinkerStatus LinkerMergeSections(LinkerContext* ctx);
static bool IsSymbolFrom(const ElfFile* file, const ElfSymbol* sym);
static LinkerStatus LinkerProcessRelocations(LinkerContext* ctx);
static void LinkerPatchRelocationBinary(uint8_t* instruction_ptr, uint32_t type,
                                        int32_t delta);
static void SectionMapDestroy(SectionMap** map);
static void LinkerContextDestroy(LinkerContext* ctx);

//-- SymbolsMap

void SymbolMapAdd(SymbolMap** map, char* name, ElfSymbol* sym) {
  SymbolMap* s = NULL;
  HASH_FIND_STR(*map, name, s);

  if (s == NULL) {
    s = malloc(sizeof(SymbolMap));
    if (!s) return;

    s->name = strdup(name);
    s->symbol = sym;
    // Temp value, is updated later.
    s->offset = 0;
    s->is_defined = (sym->sym.st_shndx != SHN_UNDEF);

    HASH_ADD_KEYPTR(hh, *map, s->name, strlen(s->name), s);
  }
}

void SymbolMapFreeAll(SymbolMap** map) {
  SymbolMap *current, *tmp;
  HASH_ITER(hh, *map, current, tmp) {
    HASH_DEL(*map, current);
    free(current->name);
    free(current);
  }
}

LinkerStatus GenerateMergedSymbolsTable(LinkerContext* ctx) {
  LinkerStatus status = LinkerSuccess;
  ElfFile *out = ctx->out, *f1 = ctx->in1, *f2 = ctx->in2;
  size_t n_glob1 = 0, n_glob2 = 0;

  // Fetch globals from both
  ElfSymbol** glob1 = ElfFileGetExternalSymbols(f1, &n_glob1);
  ElfSymbol** glob2 = ElfFileGetExternalSymbols(f2, &n_glob2);

  if ((n_glob1 > 0 && !glob1) || (n_glob2 > 0 && !glob2)) {
    status = LinkerMemoryError;
    goto Cleanup;
  }

  for (size_t i = 0; i < n_glob2; i++) {
    SymbolMapAdd(&ctx->symbol_map, glob2[i]->name, glob2[i]);
  }

  for (size_t i = 0; i < n_glob1; i++) {
    ElfSymbol* s1 = glob1[i];
    SymbolMap* lookup = NULL;
    HASH_FIND_STR(ctx->symbol_map, s1->name, lookup);

    if (lookup) {
      bool f1_def = (s1->sym.st_shndx != SHN_UNDEF);
      bool f2_def = (lookup->symbol->sym.st_shndx != SHN_UNDEF);
      // Both defined, check binding
      if (f1_def && f2_def) {
        uint8_t b1 = ELF32_ST_BIND(s1->sym.st_info);
        uint8_t b2 = ELF32_ST_BIND(lookup->symbol->sym.st_info);
        if (b1 != STB_WEAK && b2 != STB_WEAK) {
          // That's a conflict
          status = LinkerDuplicateSymbol;
          goto Cleanup;
        }
        if (b1 != STB_WEAK && b2 == STB_WEAK) {
          lookup->symbol = s1;
          lookup->is_defined = true;
        }
        // if both are weak, there's nothing to do. We keep the first symbol we found.
      } else if (f1_def && !f2_def) {
        // That's the definition
        lookup->symbol = s1;
        lookup->is_defined = true;
      }
    } else {
      // This is a new symbol
      SymbolMapAdd(&ctx->symbol_map, s1->name, s1);
    }
  }

  // Allocate maximum possible size, we will adjust at the end
  size_t total_max = 1 + f1->symbols_table->count + f2->symbols_table->count;
  out->symbols_table->symbols = calloc(total_max, sizeof(ElfSymbol));

  // 0 is for the undefined symbol.
  size_t current_idx = 1;

  // Copy all locals into out
  // We iterate over ALL symbols from f1 to fill mapping tables correctly
  for (size_t i = 1; i < f1->symbols_table->count; i++) {
    ElfSymbol* s_orig = &f1->symbols_table->symbols[i];
    if (ELF32_ST_BIND(s_orig->sym.st_info) == STB_LOCAL) {
      // Filter out empty local symbols that point to nothing (UNDEF)
      if (s_orig->sym.st_shndx == SHN_UNDEF &&
          ELF32_ST_TYPE(s_orig->sym.st_info) != STT_SECTION) {
        continue;
      }

      ctx->f1_sym_to_out_idx[i] = current_idx;
      ElfSymbol s = *s_orig;
      // Relocate section index
      if (s.sym.st_shndx != SHN_UNDEF && s.sym.st_shndx < SHN_LORESERVE) {
        s.sym.st_shndx = ctx->f1_sec_to_out_idx[s.sym.st_shndx];
      }
      out->symbols_table->symbols[current_idx++] = s;
    }
  }

  for (size_t i = 1; i < f2->symbols_table->count; i++) {
    ElfSymbol* s_orig = &f2->symbols_table->symbols[i];
    if (ELF32_ST_BIND(s_orig->sym.st_info) == STB_LOCAL) {
      if (s_orig->sym.st_shndx == SHN_UNDEF &&
          ELF32_ST_TYPE(s_orig->sym.st_info) != STT_SECTION) {
        continue;
      }

      // Merge sections: If it's a section symbol, check if it already exists in
      // out
      if (ELF32_ST_TYPE(s_orig->sym.st_info) == STT_SECTION) {
        uint32_t existing_idx = 0;
        for (uint32_t k = 1; k < current_idx; k++) {
          if (out->symbols_table->symbols[k].name && s_orig->name &&
              strcmp(out->symbols_table->symbols[k].name,
                     s_orig->name) == 0) {
            existing_idx = k;
            break;
          }
        }
        if (existing_idx > 0) {
          ctx->f2_sym_to_out_idx[i] = existing_idx;
          continue; // Do not add duplicate section symbol
        }
      }

      ctx->f2_sym_to_out_idx[i] = current_idx;
      ElfSymbol s = *s_orig;
      // Relocate section index and apply delta (rebase)
      if (s.sym.st_shndx != SHN_UNDEF && s.sym.st_shndx < SHN_LORESERVE) {
        uint32_t old_idx = s.sym.st_shndx;
        s.sym.st_value += ctx->f2_offsets[old_idx];
        s.sym.st_shndx = ctx->f2_sec_to_out_idx[old_idx];
      }
      out->symbols_table->symbols[current_idx++] = s;
    }
  }

  uint32_t first_global_idx = current_idx;

  // Copy of globals
  SymbolMap *curr_s, *tmp_s;
  HASH_ITER(hh, ctx->symbol_map, curr_s, tmp_s) {
    ElfSymbol s = *(curr_s->symbol);
    uint32_t out_idx = current_idx;

    // Update mapping for all files referencing this global
    for (size_t i = 0; i < f1->symbols_table->count; i++) {
      if (f1->symbols_table->symbols[i].name &&
          strcmp(f1->symbols_table->symbols[i].name, curr_s->name) == 0)
        ctx->f1_sym_to_out_idx[i] = out_idx;
    }
    for (size_t i = 0; i < f2->symbols_table->count; i++) {
      if (f2->symbols_table->symbols[i].name &&
          strcmp(f2->symbols_table->symbols[i].name, curr_s->name) == 0)
        ctx->f2_sym_to_out_idx[i] = out_idx;
    }

    if (s.sym.st_shndx != SHN_UNDEF && s.sym.st_shndx < SHN_LORESERVE) {
      uint32_t old_idx = s.sym.st_shndx;
      // Determine if symbol definition came from f1 or f2 to apply correct
      // mapping We compare the pointer to the original symbols arrays
      bool from_f2 = IsSymbolFrom(ctx->in2, curr_s->symbol);

      if (from_f2) {
        s.sym.st_value += ctx->f2_offsets[old_idx];
        s.sym.st_shndx = ctx->f2_sec_to_out_idx[old_idx];
      } else {
        s.sym.st_shndx = ctx->f1_sec_to_out_idx[old_idx];
      }
    }
    out->symbols_table->symbols[current_idx++] = s;
  }

  uint32_t total_symbols = current_idx;
  out->symbols_table->count = total_symbols;

  // Creation of .strtab
  StringBuilder builder = StringBuilderNew();
  for (size_t i = 1; i < total_symbols; i++) {
    ElfSymbol* s = &out->symbols_table->symbols[i];
    // Updating symbols' st_name on the fly.
    if (s->name) {
      s->sym.st_name = StringBuilderAppend(&builder, s->name);
    } else {
      s->sym.st_name = 0;
    }
  }

  uint32_t strtab_idx = out->header.e_shnum;
  out->header.e_shnum++;
  void* realloc_ptr =
      realloc(out->sections, out->header.e_shnum * sizeof(ElfSection));
  if (!realloc_ptr) {
    status = LinkerMemoryError;
    goto Cleanup;
  }
  out->sections = realloc_ptr;

  out->sections[strtab_idx].name = strdup(".strtab");
  out->sections[strtab_idx].data = (uint8_t*)builder.data;
  out->sections[strtab_idx].header.sh_size = builder.size;
  out->sections[strtab_idx].header.sh_type = SHT_STRTAB;
  out->sections[strtab_idx].header.sh_addralign = 1;
  out->sections[strtab_idx].header.sh_entsize = 0;

  // Creation of .symtab
  uint32_t symtab_idx = out->header.e_shnum;
  ctx->symtab_idx = symtab_idx;
  out->header.e_shnum++;
  realloc_ptr =
      realloc(out->sections, out->header.e_shnum * sizeof(ElfSection));
  if (!realloc_ptr) {
    status = LinkerMemoryError;
    goto Cleanup;
  }
  out->sections = realloc_ptr;

  out->sections[symtab_idx].name = strdup(".symtab");
  out->sections[symtab_idx].header.sh_type = SHT_SYMTAB;
  out->sections[symtab_idx].header.sh_entsize = sizeof(Elf32_Sym);
  out->sections[symtab_idx].header.sh_size =
      total_symbols * sizeof(Elf32_Sym);
  out->sections[symtab_idx].header.sh_addralign = 4;
  out->sections[symtab_idx].header.sh_link =
      strtab_idx; // link to the previously created strtab
  out->sections[symtab_idx].header.sh_info =
      first_global_idx; // index of first global sym

  // Also store data as raw binary
  uint32_t sym_size = sizeof(Elf32_Sym);
  out->sections[symtab_idx].data = calloc(total_symbols, sym_size);
  for (size_t i = 0; i < total_symbols; i++) {
    Elf32_Sym s = out->symbols_table->symbols[i].sym;

    if (!is_host_big_endian()) {
      s.st_name = byte_swap(s.st_name);
      s.st_value = byte_swap(s.st_value);
      s.st_size = byte_swap(s.st_size);
      s.st_shndx = byte_swap(s.st_shndx);
    }
    memcpy(out->sections[symtab_idx].data + (i * sym_size), &s, sym_size);
  }

Cleanup:
  if (glob1) free(glob1);
  if (glob2) free(glob2);
  return status;
}

LinkerStatus MergeFiles(ElfFile** out, ElfFile* f1, ElfFile* f2) {
  LinkerContext ctx = {.in1 = f1,
                       .in2 = f2,
                       .section_map = NULL,
                       .out = NULL,
                       .symbol_map = NULL,
                       .symtab_idx = SHN_UNDEF};

  LinkerStatus status = LinkerMergeSections(&ctx);

  ctx.out->symbols_table = calloc(1, sizeof(ElfSymbolsTable));
  status = GenerateMergedSymbolsTable(&ctx);
  if (status != LinkerSuccess) {
    goto MergeFilesFail;
  }

  status = LinkerGenerateShstrtab(&ctx);
  if (status != LinkerSuccess) {
    goto MergeFilesFail;
  }

  status = LinkerProcessRelocations(&ctx);
  if (status != LinkerSuccess) {
    goto MergeFilesFail;
  }

  *out = ctx.out;
  LinkerContextDestroy(&ctx);
  return status;

MergeFilesFail:
  *out = NULL;
  if (ctx.out) ElfFileDestroy(ctx.out);
  LinkerContextDestroy(&ctx);
  return status;
}

LinkerStatus LinkerMergeSections(LinkerContext* ctx) {
  ElfFile *f1 = ctx->in1, *f2 = ctx->in2;
  ElfSection *f1_sec = f1->sections, *f2_sec = f2->sections;
  uint32_t f1_sec_count = f1->header.e_shnum, f2_sec_count = f2->header.
               e_shnum;
  ctx->f1_sec_to_out_idx = calloc(ctx->in1->header.e_shnum, sizeof(uint32_t));
  ctx->f2_sec_to_out_idx = calloc(ctx->in2->header.e_shnum, sizeof(uint32_t));
  ctx->f2_offsets = calloc(ctx->in2->header.e_shnum, sizeof(uint32_t));

  uint32_t n_syms_f1 = 0, n_syms_f2 = 0;
  ElfSection** s1 =
      ElfFileGetSectionsByType(&n_syms_f1, SHT_SYMTAB, ctx->in1);
  uint32_t f1_sym_count =
      (s1) ? (s1[0]->header.sh_size / sizeof(Elf32_Sym)) : 0;
  ctx->f1_sym_to_out_idx = calloc(f1_sym_count, sizeof(uint32_t));
  ElfSection** s2 =
      ElfFileGetSectionsByType(&n_syms_f2, SHT_SYMTAB, ctx->in2);
  uint32_t f2_sym_count =
      (s2) ? (s2[0]->header.sh_size / sizeof(Elf32_Sym)) : 0;
  ctx->f2_sym_to_out_idx = calloc(f2_sym_count, sizeof(uint32_t));

  free(s1);
  free(s2);
  for (int i = 1; i < f1_sec_count; i++) {
    SectionMapAddSection(&ctx->section_map, f1_sec[i].name, &f1_sec[i]);
  }

  // Iterate over f2's sections to find matches
  for (int i = 1; i < f2_sec_count; i++) {
    SectionMap* lookup = NULL;
    HASH_FIND_STR(ctx->section_map, f2_sec[i].name, lookup);

    if (lookup != NULL) {
      uint32_t alignment = f2_sec[i].header.sh_addralign;
      // There is a match. Delta is the position of the new section within
      // its match in file 1.
      uint32_t delta = align_up(lookup->target->header.sh_size, alignment);
      SectionMapUpdateSource(ctx->section_map, f2_sec[i].name, &f2_sec[i],
                             delta);
    } else {
      // No match, add it as a single section.
      SectionMapAddSection(&ctx->section_map, f2_sec[i].name, NULL);
      SectionMapUpdateSource(ctx->section_map, f2_sec[i].name, &f2_sec[i], 0);
    }
  }
  // Number of sections within the result file (+1 to account for SHT_NULL)
  uint32_t section_count = HASH_COUNT(ctx->section_map) + 1;

  ElfFile* result = ElfFileNewEmpty();
  result->header.e_shnum = section_count;
  // FIXME : This is not how it actually works, is enough for the presentation
  // hopefully.
  result->header.e_flags = ctx->in1->header.e_flags;
  result->sections = calloc(section_count, sizeof(ElfSection));
  SectionMap *curr, *tmp;
  int i = 1;
  HASH_ITER(hh, ctx->section_map, curr, tmp) {
    if (curr->strategy == Ignore) {
      continue;
    }
    result->sections[i] = MergeSections(curr);
    curr->new_idx = i++;

    // Tracking of how sections have moved.
    if (curr->target) {
      uint32_t old_idx = curr->target - ctx->in1->sections;
      ctx->f1_sec_to_out_idx[old_idx] = curr->new_idx;
    }
    if (curr->source) {
      uint32_t old_idx = curr->source - ctx->in2->sections;
      ctx->f2_sec_to_out_idx[old_idx] = curr->new_idx;
      ctx->f2_offsets[old_idx] = curr->offset;
    }
  }
  // It's the null section, so...
  result->sections[0].name = NULL;
  result->sections[0].header = (Elf32_Shdr){.sh_type = SHT_NULL,
                                            .sh_addr = 0,
                                            .sh_offset = 0,
                                            .sh_size = 0,
                                            .sh_name = 0,
                                            .sh_link = 0,
                                            .sh_info = 0,
                                            .sh_addralign = 0};
  result->sections[0].data = NULL;

  result->header.e_shnum = i;
  ctx->out = result;
  return LinkerSuccess;
}

static ElfSection MergeSections(SectionMap* entry) {
  /* Not sure of all the copying going on here, but I'd rather
   * have each ElfFile own its data. */
  ElfSection section = {0};

  if (!entry) return section;

  char* name;
  if (entry->target) {
    name = entry->target->name;
  } else if (entry->source) {
    name = entry->source->name;
  } else {
    name = "";
  }

  switch (entry->strategy) {
    case Ignore:
#ifdef DEBUG
      // This case should have already been filtered.
      assert(0 && "Section with Ignored merge strategy was not filtered");
      __builtin_unreachable();
#endif
      break;
    case Singleton:
      const ElfSection* singleton =
          entry->target == NULL ? entry->source : entry->target;
      section = (ElfSection){.header = singleton->header,
                             .data = malloc(singleton->header.sh_size)};
      memcpy(section.data, singleton->data, singleton->header.sh_size);
      break;
    case Concatenate:
      if (entry->target && entry->source) {
        /* If those two sections have, for example, 4 and 8 alignment,
         * the merged section's alignment has to be 8. Otherwise, it simply
         * won't be loaded by the OS. */
        const uint32_t alignment = max(entry->target->header.sh_addralign,
                                       entry->source->header.sh_addralign);

        // Merge both sections
        section.header = entry->target->header;
        section.header.sh_size =
            entry->offset + entry->source->header.sh_size;
        section.header.sh_addralign = alignment;
        section.data = malloc(section.header.sh_size);
        if (section.data) {
          memset(section.data, 0, section.header.sh_size);
          memcpy(section.data, entry->target->data,
                 entry->target->header.sh_size);

          memcpy(section.data + entry->offset, entry->source->data,
                 entry->source->header.sh_size);
        }
      } else if (!entry->target && !entry->source) {
#ifdef DEBUG
        assert(0 && "Merge mapping entry with both NULL source and target");
        __builtin_unreachable();
#endif
      } else {
        // Section that is only on one file
        const ElfSection* single =
            entry->source ? entry->source : entry->target;
        section.header = single->header;
        section.data = malloc(section.header.sh_size);
        if (section.data) {
          memcpy(section.data, single->data, section.header.sh_size);
        }
      }
      break;
    case NoBits:
      section.header =
          (entry->target ? entry->target->header : entry->source->header);
      section.data = NULL;
      if (entry->target && entry->source) {
        section.header.sh_size =
            entry->offset + entry->source->header.sh_size;
      } else {
        section.header.sh_size =
        (entry->target
           ? entry->target->header.sh_size
           : entry->source->header.sh_size);
      }
      break;
  }
  section.name = strdup(name);
  return section;
}

static LinkerStatus LinkerGenerateShstrtab(const LinkerContext* ctx) {
  ElfFile* elf = ctx->out;

  uint32_t new_count = elf->header.e_shnum + 1;
  ElfSection* new_ptr =
      realloc(elf->sections, new_count * sizeof(ElfSection));
  if (!new_ptr) {
    return LinkerMemoryError;
  }
  StringBuilder builder = StringBuilderNew();
  elf->sections = new_ptr;
  uint32_t shstrtab_idx = elf->header.e_shnum;
  elf->header.e_shnum = new_count;
  elf->header.e_shstrndx = shstrtab_idx;

  elf->sections[shstrtab_idx].name = strdup(".shstrtab");
  if (!elf->sections[shstrtab_idx].name) return LinkerMemoryError;
  elf->sections[shstrtab_idx].data = NULL;
  elf->sections[shstrtab_idx].header =
      (Elf32_Shdr){.sh_type = SHT_STRTAB, .sh_size = 0, .sh_addralign = 1};

  uint32_t name_offset = 1;
  for (uint32_t i = 1; i < elf->header.e_shnum; ++i) {
    name_offset = StringBuilderAppend(&builder, elf->sections[i].name);
    elf->sections[i].header.sh_name = name_offset;
  }
  // the shstrtab takes ownership of this. should never be freed.
  elf->sections[shstrtab_idx].data = (uint8_t*)builder.data;
  elf->sections[shstrtab_idx].header.sh_size = builder.size;
  return LinkerSuccess;
}

static LinkerStatus LinkerProcessRelocations(LinkerContext* ctx) {
  LinkerStatus status = LinkerSuccess;
  uint32_t rel_count = 0;
  ElfSection** rel_sections =
      ElfFileGetSectionsByType(&rel_count, SHT_REL, ctx->out);
  if (rel_count == 0) {
    return LinkerSuccess;
  }
  if (rel_count > 0 && !rel_sections) {
    return LinkerMemoryError;
  }

  for (uint32_t i = 0; i < rel_count; ++i) {
    ElfSection* rel = rel_sections[i];

    if (!rel->data || rel->header.sh_size == 0 || rel->header.sh_entsize == 0) {
      continue;
    }
    size_t rel_entires_count = rel->header.sh_size / sizeof(Elf32_Rel);
    Elf32_Rel* rel_entries = (Elf32_Rel*)rel->data;

    // Check if the current section is present in f1.
    uint32_t f1_rel_entry_count = 0;
    ElfSection* f1_rel = ElfFileGetSectionByName(rel->name, ctx->in1);
    if (f1_rel && f1_rel->header.sh_size > 0) {
      f1_rel_entry_count = f1_rel->header.sh_size / sizeof(Elf32_Rel);
    }

    for (int j = 0; j < rel_entires_count; ++j) {
      // From this point on, we're manipulating raw data, so it's capital to manage
      // endianness.
      Elf32_Rel* current_rel_entry = rel_entries + j;
      uint32_t r_offset = is_host_big_endian()
                            ? current_rel_entry->r_offset
                            : byte_swap(current_rel_entry->r_offset);
      uint32_t r_info = is_host_big_endian()
                          ? current_rel_entry->r_info
                          : byte_swap(current_rel_entry->r_info);
      unsigned int r_type = ELF32_R_TYPE(r_info);
      uint32_t r_sym = ELF32_R_SYM(r_info);

      bool is_current_rel_entry_from_f1 = j < f1_rel_entry_count;

      // FIXME: what if the section's name is just ".rel"? does this even happen?
      char* og_section_name = rel_sections[i]->name + 4; // To remove the .rel
      // Retrieve relocation symbol
      SectionMap* lookup = NULL;
      HASH_FIND_STR(ctx->section_map, og_section_name, lookup);
      if (!lookup) {
        // Destination section is not found, nothing to be done here
        continue;
      }

      // Get the current relocated symbol's final position.
      uint32_t new_sym_idx = is_current_rel_entry_from_f1
                               ? ctx->f1_sym_to_out_idx[r_sym]
                               : ctx->f2_sym_to_out_idx[r_sym];

      ElfSymbol* original_symbol = is_current_rel_entry_from_f1
                                     ? &ctx->in1->symbols_table->symbols[r_sym]
                                     : &ctx->in2->symbols_table->symbols[r_sym];
      ElfSymbol* resolved_symbol =
          &ctx->out->symbols_table->symbols[new_sym_idx];

      // Resolving adresses s.t P is the instruction and S the symbol it's supposed to point to.

      int32_t S = (int32_t)resolved_symbol->sym.st_value;
      int32_t P = (int32_t)r_offset + (is_current_rel_entry_from_f1
                                         ? 0
                                         : lookup->offset);

      int32_t patch_value = 0;
      // Determine by how much the immediate needs to shift
      if (r_type == R_ARM_ABS32) {
        // this is absolute, so it's just the address of the symbol
        patch_value = S - (int32_t)original_symbol->sym.st_value;
      } else if (r_type == R_ARM_JUMP24 || r_type == R_ARM_CALL) {
        // since it's a call, we'll put the relative distance
        if (original_symbol->sym.st_shndx == SHN_UNDEF) {
          patch_value = S - P;
        } else {
          // this is an internal jump, we just adjust the relative distance.
          int32_t S_orig = (int32_t)original_symbol->sym.st_value;
          int32_t P_orig = (int32_t)r_offset;
          patch_value = (S - P) - (S_orig - P_orig);
        }
      }

      uint32_t final_r_offset = is_current_rel_entry_from_f1
                                  ? r_offset
                                  : r_offset + lookup->offset;
      if (r_type != R_ARM_V4BX) {
        ElfSection* target_sec = ElfFileGetSectionByName(
            og_section_name, ctx->out);
        LinkerPatchRelocationBinary(target_sec->data + final_r_offset, r_type,
                                    patch_value);
      }
      current_rel_entry->r_offset = !is_host_big_endian()
                                      ? byte_swap(final_r_offset)
                                      : final_r_offset;
      current_rel_entry->r_info = !is_host_big_endian()
                                    ? byte_swap(
                                        ELF32_R_INFO(new_sym_idx, r_type))
                                    : ELF32_R_INFO(new_sym_idx, r_type);
    }
  }

  free(rel_sections);
  return status;
}

static void LinkerPatchRelocationBinary(uint8_t* instruction_ptr,
                                        const uint32_t type,
                                        const int32_t delta) {
  int32_t inst = *(int32_t*)instruction_ptr;
  if (!is_host_big_endian()) inst = byte_swap(inst);

  if (type == R_ARM_ABS32) {
    inst += delta;
  } else if (type == R_ARM_JUMP24 || type == R_ARM_CALL || type ==
             R_ARM_PC24) {
    int32_t imm24 = inst & 0x00ffffff;
    if (imm24 & 0x00800000) imm24 |= (int32_t)0xff000000;
    imm24 += (delta / 4);

    inst = (inst & 0xFF000000) | (imm24 & 0x00FFFFFF);
  }

  if (!is_host_big_endian()) inst = byte_swap(inst);
  *(int32_t*)instruction_ptr = inst;
}

bool isSingleton(const ElfSection* section) {
  // Those sections should only be present once in the result file.
  return strcmp(section->name, ".comment") == 0 ||
         strcmp(section->name, ".ARM.attributes") == 0;
}

static void SectionMapAddSection(SectionMap** map, const char* name,
                                 ElfSection* target) {
  SectionMap* s = malloc(sizeof(SectionMap));
  s->name = strdup(name);
  s->target = target;
  s->source = NULL;
  s->offset = 0;
  s->strategy = GetMergeStrategy(target);
  HASH_ADD_KEYPTR(hh, *map, s->name, strlen(s->name), s);
}

static void SectionMapUpdateSource(const SectionMap* map, const char* name,
                                   ElfSection* source, const uint32_t delta) {
  SectionMap* s = NULL;
  HASH_FIND_STR(map, name, s);
  if (s != NULL) {
    s->source = source;
    s->offset = delta;
  }
}

static MergeStrategy GetMergeStrategy(ElfSection* section) {
  if (!section || !section->name) {
    return Ignore;
  }
  if (section->name && (strstr(section->name, ".debug") == section->name ||
                        strstr(section->name, ".rel.debug") == section->
                        name)) {
    return Ignore;
  }
  switch (section->header.sh_type) {
    case SHT_NOBITS:
      return NoBits;
    case SHT_SYMTAB:
    case SHT_STRTAB:
      return Ignore;
    default:
      return isSingleton(section) ? Singleton : Concatenate;
  }
}

static bool IsSymbolFrom(const ElfFile* file, const ElfSymbol* sym) {
  if (!file || !file->symbols_table || !sym) return false;

  return (sym >= file->symbols_table->symbols &&
          sym < file->symbols_table->symbols + file->symbols_table->count);
}

static void LinkerContextDestroy(LinkerContext* ctx) {
  if (ctx->symbol_map) {
    SymbolMapFreeAll(&ctx->symbol_map);
    ctx->symbol_map = NULL;
  }
  if (ctx->f1_sec_to_out_idx) {
    free(ctx->f1_sec_to_out_idx);
    ctx->f1_sec_to_out_idx = NULL;
  }
  if (ctx->f1_sym_to_out_idx) {
    free(ctx->f1_sym_to_out_idx);
    ctx->f1_sym_to_out_idx = NULL;
  }
  if (ctx->f2_offsets) {
    free(ctx->f2_offsets);
    ctx->f2_offsets = NULL;
  }

  if (ctx->f2_sec_to_out_idx) {
    free(ctx->f2_sec_to_out_idx);
    ctx->f2_sec_to_out_idx = NULL;
  }
  if (ctx->f2_sym_to_out_idx) {
    free(ctx->f2_sym_to_out_idx);
    ctx->f2_sym_to_out_idx = NULL;
  }

  if (ctx->section_map) {
    SectionMapDestroy(&ctx->section_map);
  }
}

void SectionMapDestroy(SectionMap** map) {
  SectionMap *current, *tmp;
  HASH_ITER(hh, *map, current, tmp) {
    HASH_DEL(*map, current);
    free(current->name);
    free(current);
  }
}

const char* LinkerStatusToString(const LinkerStatus status) {
  switch (status) {
    case LinkerSuccess:
      return "LinkerSuccess";
    case LinkerDuplicateSymbol:
      return "LinkerDuplicateSymbol";
    case LinkerMemoryError:
      return "LinkerMemoryError";
    default:
      return "Unknown";
  }
}