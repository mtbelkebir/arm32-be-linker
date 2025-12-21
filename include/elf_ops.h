#ifndef _ELF_OPS_H_
#define _ELF_OPS_H_
#include <elf.h>
#include <stdio.h>

#define SECTION_NAME_SIZE 64
#define SYM_NAME_SIZE 64

/**
 * @brief In-memory representation of a 32-bit ELF file.
 *
 * The struct owns heap-allocated members and should be freed with
 * free_elf_file().
 */
typedef struct ElfFile {
  FILE *file;        /**< Open FILE* for the underlying file (may be NULL). */
  Elf32_Ehdr e_ehdr; /**< Parsed ELF header (host endianness). */
  Elf32_Shdr *
      e_shrdrs; /**< Array of section headers (heap-allocated), NULL if none. */
  Elf32_Sym *sym;
  // TODO: Check if there aren't more required fields
} ElfFile;

typedef enum ElfParsingStatus {
  Success,
  FileTooShort,
  IoError,
  NotAnElfFile,
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
#endif //_ELF_OPS_H
