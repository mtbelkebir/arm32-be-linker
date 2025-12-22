#ifndef _ELF_OPS_H_
#define _ELF_OPS_H_
#include <elf.h>
#include <stdio.h>

typedef struct ElfSection {
  char* name;
  Elf32_Shdr header;
} ElfSection;

typedef struct ElfSymbol {
  char* name;
  Elf32_Sym sym;
} ElfSymbol;

typedef struct ElfSymbolsTable {
  uint32_t count;
  ElfSymbol* symbols;
} ElfSymbolsTable;

typedef struct ElfRelocationTable {
  char* name;
  uint32_t target_section;
  uint32_t offset;
  uint32_t count;
  Elf32_Rel* entries;

} ElfRelocationTable;
/**
 * @brief In-memory representation of a 32-bit ELF file.
 *
 * The struct owns heap-allocated members and should be freed with
 * free_elf_file().
 */
typedef struct ElfFile {
  FILE* file;        /**< Open FILE* for the underlying file (may be NULL). */
  Elf32_Ehdr header; /**< Parsed ELF header (host endianness). */
  ElfSection* sections;
  ElfSymbolsTable* symbols_table;
  ElfRelocationTable* rel_tables;
  uint16_t rel_tables_count;
  // TODO: Check if there aren't more required fields
} ElfFile;

typedef enum ElfParsingStatus {
  Success,
  FileTooShort,
  IoError,
  NotAnElfFile,
  UnexpectedEof,
  UnsupportedMachineType,
  UnsupportedEndianness,
  UnknownError,
  UnknownDataEncoding,
  InvalidClass,
  UnsupportedClass,
  UnknownClass,
  InvalidDataEncoding,
  MemoryError,
  InvalidArguments,  // Just in case someone calls our functions
                     // with the wrong args

} ElfParsingStatus;

ElfParsingStatus ElfFileNew(const char* path, ElfFile** out);
void ElfFileDisplayHeader(ElfFile* elf);
const char* ElfParsingStatusToString(ElfParsingStatus status);
void ElfFileDestroy(ElfFile* elf);
void ElfFileDisplaySections(ElfFile* elf);
ElfSection* ElfFileGetSectionByName(const char* name, ElfFile* elf);
int ElfFileDisplaySectionContentsByName(const char* name, ElfFile* elf);
void ElfFileDisplaySymbols(ElfFile* elf);
ElfSection** ElfFileGetSectionsByType(uint32_t* out_section_count,
                                      uint32_t type, ElfFile* elf);
void ElfFileDisplayRelocations(ElfFile* elf);
#endif  //_ELF_OPS_H
