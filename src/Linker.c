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
static void SectionMapUpdateSource(SectionMap* map, const char* name,
                                   ElfSection* source, uint32_t delta);
static ElfSection CreateMergedSection(SectionMap* entry);
bool isSingleton(ElfSection* section);
static void SectionMapAddSection(SectionMap** map, const char* name,
                                 ElfSection* target);
static MergeStrategy GetMergeStrategy(ElfSection* section);

ElfFile* ElfMergeSections(ElfFile* f1, ElfFile* f2) {
  SectionMap* mapping = NULL;
  ElfSection *f1_sec = f1->sections, *f2_sec = f2->sections;
  uint32_t f1_sec_count = f1->header.e_shnum, f2_sec_count = f2->header.e_shnum;

  for (int i = 1; i < f1_sec_count; i++) {
    SectionMapAddSection(&mapping, f1_sec[i].name, &f1_sec[i]);
  }

  // Iterate over f2's sections to find matches
  for (int i = 1; i < f2_sec_count; i++) {
    SectionMap* lookup = NULL;
    HASH_FIND_STR(mapping, f2_sec[i].name, lookup);

    if (lookup != NULL) {
      uint32_t alignment = f2_sec[i].header.sh_addralign;
      // There is a match. Delta is the position of the new section within
      // its match in file 1.
      uint32_t delta = align_up(lookup->target->header.sh_size, alignment);
      SectionMapUpdateSource(mapping, f2_sec[i].name, &f2_sec[i], delta);
    } else {
      // No match, add it as a single section.
      SectionMapAddSection(&mapping, f2_sec[i].name, NULL);
      SectionMapUpdateSource(mapping, f2_sec[i].name, &f2_sec[i], 0);
    }
  }
  // Number of sections within the result file (+1 to account for SHT_NULL)
  uint32_t section_count = HASH_COUNT(mapping) + 1;

  ElfFile* result = ElfFileNewEmpty();
  result->header.e_shnum = section_count;
  result->sections = calloc(section_count, sizeof(ElfSection));
  SectionMap *curr, *tmp;
  int i = 1;
  HASH_ITER(hh, mapping, curr, tmp) {
    if (curr->strategy == Ignore) {
      continue;
    }
    result->sections[i] = CreateMergedSection(curr);
    curr->new_idx = i++;
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

  /* Memory clean-up */
  HASH_ITER(hh, mapping, curr, tmp) {
    HASH_DEL(mapping, curr);
    free(curr->name);
    free(curr);
  }

  result->header.e_shnum = i;
  return result;
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

static LinkerStatus LinkerProcessShstrtab(ElfFile* elf) {
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