#include "Linker.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "uthash.h"
#include "util.h"

// TODO(Mohand Tahar) : Finish verifying all memory allocs.

/**
 * Defines exactly *how* to merge two sections.
 */
typedef enum MergeStrategy {
  Concatenate, /**< For sections such a .text, .data, etc. Add them
                  together. */
  Singleton,   /**< For sections like .ARM.attributes that should only be kept
                  once. */
  NoBits,      /**< For sections like .bss whose merging is just adding up their
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

  // Merge offsets.
  uint32_t* f2_offsets;
  SymbolMap* symbol_map;
} LinkerContext;
static void SectionMapUpdateSource(SectionMap* map, const char* name,
                                   ElfSection* source, uint32_t delta);
static ElfSection CreateMergedSection(SectionMap* entry);
bool isSingleton(ElfSection* section);
static void SectionMapAddSection(SectionMap** map, const char* name,
                                 ElfSection* target);
static MergeStrategy GetMergeStrategy(ElfSection* section);
static LinkerStatus LinkerProcessShstrtab(LinkerContext* ctx);
static LinkerStatus ElfMergeSections(LinkerContext* ctx);
static bool IsSymbolFrom(const ElfFile* file, const ElfSymbol* sym);

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
  ElfSymbol** glob1 = ElfFileGetSymbolsByBinding(f1, STB_GLOBAL, &n_glob1);
  ElfSymbol** glob2 = ElfFileGetSymbolsByBinding(f2, STB_GLOBAL, &n_glob2);

  size_t n_loc1 = 0, n_loc2 = 0;
  ElfSymbol** loc1 = ElfFileGetSymbolsByBinding(f1, STB_LOCAL, &n_loc1);
  ElfSymbol** loc2 = ElfFileGetSymbolsByBinding(f2, STB_LOCAL, &n_loc2);

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
      // Both defined, that's a conflict
      if (f1_def && f2_def) {
        status = LinkerDuplicateSymbol;
        goto Cleanup;
      }
      if (f1_def && !f2_def) {
        // That's the definition
        lookup->symbol = s1;
        lookup->is_defined = true;
      }
    } else {
      // This is a new symbol
      SymbolMapAdd(&ctx->symbol_map, s1->name, s1);
    }
  }

  size_t total_symbols = 1 + n_loc1 + n_loc2 + HASH_COUNT(ctx->symbol_map);
  out->symbols_table->symbols = calloc(total_symbols, sizeof(ElfSymbol));
  out->symbols_table->count = total_symbols;

  // 0 is for the undefined symbol.
  size_t current_idx = 1;

  // Copy all locals into out
  for (size_t i = 0; i < n_loc1; i++) {
    ctx->f1_sym_to_out_idx[i] = current_idx;
    ElfSymbol s = *loc1[i];
    // Relocate section index
    if (s.sym.st_shndx != SHN_UNDEF && s.sym.st_shndx < SHN_LORESERVE) {
      s.sym.st_shndx = ctx->f1_sec_to_out_idx[s.sym.st_shndx];
    }
    out->symbols_table->symbols[current_idx++] = s;
  }
  for (size_t i = 0; i < n_loc2; i++) {
    ctx->f2_sym_to_out_idx[i] = current_idx;
    ElfSymbol s = *loc2[i];
    // Relocate section index and apply delta (rebase)
    if (s.sym.st_shndx != SHN_UNDEF && s.sym.st_shndx < SHN_LORESERVE) {
      uint32_t old_idx = s.sym.st_shndx;
      s.sym.st_value += ctx->f2_offsets[old_idx];
      s.sym.st_shndx = ctx->f2_sec_to_out_idx[old_idx];
    }
    out->symbols_table->symbols[current_idx++] = s;
  }

  // Copy of globals
  SymbolMap *curr_s, *tmp_s;
  HASH_ITER(hh, ctx->symbol_map, curr_s, tmp_s) {
    ElfSymbol s = *(curr_s->symbol);

    if (s.sym.st_shndx != SHN_UNDEF && s.sym.st_shndx < SHN_LORESERVE) {
      uint32_t old_idx = s.sym.st_shndx;
      // Determine if symbol definition came from f1 or f2 to apply correct
      // mapping We compare the pointer to the original symbols arrays
      bool from_f2 = IsSymbolFrom(ctx->in2, curr_s->symbol);

      if (from_f2) {
        s.sym.st_value += ctx->f2_offsets[old_idx];
        s.sym.st_shndx = ctx->f2_sec_to_out_idx[old_idx];
        ctx->f2_sym_to_out_idx[old_idx] = current_idx;
      } else {
        ctx->f1_sym_to_out_idx[old_idx] = current_idx;
        s.sym.st_shndx = ctx->f1_sec_to_out_idx[old_idx];
      }
    }
    out->symbols_table->symbols[current_idx++] = s;
  }

  // Creation of .strtab
  StringBuilder builder = StringBuilderNew();
  for (int i = 1; i < total_symbols; i++) {
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
  out->sections =
      realloc(out->sections, out->header.e_shnum * sizeof(ElfSection));

  out->sections[strtab_idx].name = strdup(".strtab");
  out->sections[strtab_idx].data = (uint8_t*)builder.data;
  out->sections[strtab_idx].header.sh_size = builder.size;
  out->sections[strtab_idx].header.sh_type = SHT_STRTAB;
  out->sections[strtab_idx].header.sh_addralign = 1;

  // Creation of .symtab
  uint32_t symtab_idx = out->header.e_shnum;
  out->header.e_shnum++;
  out->sections =
      realloc(out->sections, out->header.e_shnum * sizeof(ElfSection));

  out->sections[symtab_idx].name = strdup(".symtab");
  out->sections[symtab_idx].header.sh_type = SHT_SYMTAB;
  out->sections[symtab_idx].header.sh_entsize = sizeof(Elf32_Sym);
  out->sections[symtab_idx].header.sh_size = total_symbols * sizeof(Elf32_Sym);
  out->sections[symtab_idx].header.sh_addralign = 4;
  out->sections[symtab_idx].header.sh_link =
      strtab_idx;  // link to the previously created strtab
  out->sections[symtab_idx].header.sh_info =
      1 + n_loc1 + n_loc2;  // index of first global sym

  // Also store data as raw binary
  out->sections[symtab_idx].data =
      malloc(out->sections[symtab_idx].header.sh_size);

  uint32_t sym_size = sizeof(Elf32_Sym);
  out->sections[symtab_idx].data = calloc(total_symbols, sym_size);
  for (size_t i = 0; i < total_symbols; i++) {
    // 2. Récupérer la structure ELF propre
    Elf32_Sym s = out->symbols_table->symbols[i].sym;

    if (!is_host_big_endian()) {
      s.st_name = byte_swap(s.st_name);
      s.st_value = byte_swap(s.st_value);
      s.st_size = byte_swap(s.st_size);
      s.st_shndx = byte_swap(s.st_shndx);
    }

    // 4. Copier EXACTEMENT 16 octets dans le buffer de destination
    memcpy(out->sections[symtab_idx].data + (i * sym_size), &s, sym_size);
  }
Cleanup:
  if (glob1) free(glob1);
  if (glob2) free(glob2);
  if (loc1) free(loc1);
  if (loc2) free(loc2);
  if (ctx->f2_sec_to_out_idx) free(ctx->f2_sec_to_out_idx);
  if (ctx->f1_sec_to_out_idx) free(ctx->f1_sec_to_out_idx);
  // Note: ctx->symbol_map is freed in LinkerContextFree
  return status;
}
LinkerStatus MergeFiles(ElfFile** out, ElfFile* f1, ElfFile* f2) {
  LinkerContext ctx = {.in1 = f1,
                       .in2 = f2,
                       .section_map = NULL,
                       .out = NULL,
                       .symbol_map = NULL};

  LinkerStatus status = ElfMergeSections(&ctx);

  // Initialisation de la table des symboles dans result avant de générer
  ctx.out->symbols_table = calloc(1, sizeof(ElfSymbolsTable));
  status = GenerateMergedSymbolsTable(&ctx);
  if (status != LinkerSuccess) {
    goto MergeFilesFail;
  }

  status = LinkerProcessShstrtab(&ctx);
  if (status != LinkerSuccess) {
    goto MergeFilesFail;
  }
  *out = ctx.out;
  return status;

MergeFilesFail:
  *out = NULL;
  if (ctx.out) ElfFileDestroy(ctx.out);
  return status;
}

LinkerStatus ElfMergeSections(LinkerContext* ctx) {
  ElfFile *f1 = ctx->in1, *f2 = ctx->in2;
  ElfSection *f1_sec = f1->sections, *f2_sec = f2->sections;
  uint32_t f1_sec_count = f1->header.e_shnum, f2_sec_count = f2->header.e_shnum;
  ctx->f1_sec_to_out_idx = calloc(ctx->in1->header.e_shnum, sizeof(uint32_t));
  ctx->f2_sec_to_out_idx = calloc(ctx->in2->header.e_shnum, sizeof(uint32_t));
  ctx->f2_offsets = calloc(ctx->in2->header.e_shnum, sizeof(uint32_t));

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
  result->sections = calloc(section_count, sizeof(ElfSection));
  SectionMap *curr, *tmp;
  int i = 1;
  HASH_ITER(hh, ctx->section_map, curr, tmp) {
    if (curr->strategy == Ignore) {
      continue;
    }
    result->sections[i] = CreateMergedSection(curr);
    curr->new_idx = i++;

    // Tracking of how sections have moved.
    if (curr->target) {
      uint32_t old_idx = curr->target - ctx->in1->sections;
      ctx->f1_sec_to_out_idx[old_idx] = curr->new_idx;
    }
    if (curr->source) {
      uint32_t old_idx = curr->source - ctx->in2->sections;
      ctx->f2_sec_to_out_idx[old_idx] = i;
      ctx->f2_offsets[old_idx] = curr->new_idx;
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

static ElfSection CreateMergedSection(SectionMap* entry) {
  /* Not sure of all the copying going on here, but I'd rather
   * have each ElfFile own its data. */
  ElfSection section;

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
      ElfSection* singleton =
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
        uint32_t alignment = max(entry->target->header.sh_addralign,
                                 entry->source->header.sh_addralign);

        // Merge both sections
        section.header = entry->target->header;
        section.header.sh_size = entry->offset + entry->source->header.sh_size;
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
        ElfSection* single = entry->source ? entry->source : entry->target;
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
        section.header.sh_size = entry->offset + entry->source->header.sh_size;
      } else {
        section.header.sh_size =
            (entry->target ? entry->target->header.sh_size
                           : entry->source->header.sh_size);
      }
      break;
  }
  section.name = strdup(name);
  return section;
}
static LinkerStatus LinkerProcessShstrtab(LinkerContext* ctx) {
  ElfFile* elf = ctx->out;

  uint32_t new_count = elf->header.e_shnum + 1;
  ElfSection* new_ptr = realloc(elf->sections, new_count * sizeof(ElfSection));
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

bool isSingleton(ElfSection* section) {
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

static void SectionMapUpdateSource(SectionMap* map, const char* name,
                                   ElfSection* source, uint32_t delta) {
  SectionMap* s = NULL;
  HASH_FIND_STR(map, name, s);
  if (s != NULL) {
    s->source = source;
    s->offset = delta;
  }
}

static MergeStrategy GetMergeStrategy(ElfSection* section) {
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