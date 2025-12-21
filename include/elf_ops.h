#ifndef _ELF_OPS_H_
#define _ELF_OPS_H_
#include <elf.h>
#include <stdio.h>

typedef struct ElfSection {
    char* name;
    Elf32_Shdr header;
} ElfSection;

/**
 * @brief In-memory representation of a 32-bit ELF file.
 *
 * The struct owns heap-allocated members and should be freed with
 * free_elf_file().
 */
typedef struct ElfFile {
  FILE *file;        /**< Open FILE* for the underlying file (may be NULL). */
  Elf32_Ehdr header; /**< Parsed ELF header (host endianness). */
  ElfSection* sections;
  Elf32_Sym *sym;
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
  InvalidArguments, // Just in case someone calls our functions
                    // with the wrong args

} ElfParsingStatus;




ElfParsingStatus ElfFileNew(const char *path, ElfFile **out);
void ElfFileDisplayHeader(ElfFile *elf);
const char *ElfParsingStatusToString(ElfParsingStatus status);
void ElfFileDestroy(ElfFile *elf);
#endif //_ELF_OPS_H
