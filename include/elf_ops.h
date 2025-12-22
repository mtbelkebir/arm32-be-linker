#ifndef _ELF_OPS_H_
#define _ELF_OPS_H_
#include <elf.h>
#include <stdio.h>

/**
 * @brief Represents a single ELF section.
 *
 * Contains section metadata including the section name and header.
 * The name field is heap-allocated and must be freed.
 */
typedef struct ElfSection {
  char* name;        /**< Section name (heap-allocated, may be "<corrupt>"). */
  Elf32_Shdr header; /**< Standard ELF32 section header. */
} ElfSection;

/**
 * @brief Represents a single ELF symbol.
 *
 * Contains symbol metadata including the symbol name and symbol table entry.
 * The name field is heap-allocated and must be freed.
 */
typedef struct ElfSymbol {
  char* name;    /**< Symbol name (heap-allocated). */
  Elf32_Sym sym; /**< Standard ELF32 symbol table entry. */
} ElfSymbol;

/**
 * @brief Container for a collection of ELF symbols.
 *
 * Owns an array of ElfSymbol structures parsed from a symbol table section.
 */
typedef struct ElfSymbolsTable {
  uint32_t count;     /**< Number of symbols in the array. */
  ElfSymbol* symbols; /**< Heap-allocated array of symbols. */
} ElfSymbolsTable;

/**
 * @brief Represents a single relocation entry with symbol reference.
 *
 * Combines a standard ELF relocation entry with a pointer to the
 * associated symbol for convenient access.
 */
typedef struct ElfRelocationEntry {
  Elf32_Rel rel;     /**< Standard ELF32 relocation entry. */
  ElfSymbol* symbol; /**< Pointer to associated symbol (may be NULL). */
} ElfRelocationEntry;

/**
 * @brief Represents a relocation table for a specific section.
 *
 * Contains metadata about the relocation section and an array of
 * relocation entries with resolved symbol pointers.
 */
typedef struct ElfRelocationTable {
  char* name;                  /**< Relocation section name (heap-allocated). */
  uint32_t target_section;     /**< Index of section being relocated. */
  uint32_t offset;             /**< File offset of relocation entries. */
  uint32_t count;              /**< Number of relocation entries. */
  ElfRelocationEntry* entries; /**< Heap-allocated array of entries. */
} ElfRelocationTable;

/**
 * @brief In-memory representation of a 32-bit ELF file.
 *
 * The struct owns heap-allocated members and should be freed with
 * ElfFileDestroy().
 */
typedef struct ElfFile {
  FILE* file;        /**< Open FILE* for the underlying file (may be NULL). */
  Elf32_Ehdr header; /**< Parsed ELF header (host endianness). */
  ElfSection* sections;           /**< Heap-allocated array of sections. */
  ElfSymbolsTable* symbols_table; /**< Parsed symbol table (may be NULL). */
  ElfRelocationTable* rel_tables; /**< Heap-allocated array of reloc tables. */
  uint16_t rel_tables_count;      /**< Number of relocation tables. */
  // TODO: Check if there aren't more required fields
} ElfFile;

/**
 * @brief Status codes returned by ELF parsing operations.
 *
 * These codes indicate success or specific failure modes when
 * loading and parsing ELF files.
 */
typedef enum ElfParsingStatus {
  Success,                /**< Operation completed successfully. */
  FileTooShort,           /**< File is smaller than minimum ELF size. */
  IoError,                /**< File I/O operation failed. */
  NotAnElfFile,           /**< File lacks ELF magic number. */
  UnexpectedEof,          /**< Unexpected end of file during parsing. */
  UnsupportedMachineType, /**< ELF machine type not supported. */
  UnsupportedEndianness,  /**< ELF endianness not supported. */
  UnknownError,           /**< Unspecified error occurred. */
  UnknownDataEncoding,    /**< Data encoding field has invalid value. */
  InvalidClass,           /**< ELF class field has invalid value. */
  UnsupportedClass,       /**< ELF class not supported (e.g., 64-bit). */
  UnknownClass,           /**< ELF class field has unknown value. */
  InvalidDataEncoding,    /**< Data encoding is invalid. */
  MemoryError,            /**< Memory allocation failed. */
  InvalidArguments,       /**< Function called with invalid arguments. */
} ElfParsingStatus;

/**
 * @brief Creates a new ElfFile by parsing the file at the given path.
 *
 * @param path Path to the ELF file to parse.
 * @param out Pointer to receive the newly allocated ElfFile on success.
 * @return Success on success, or an error code indicating the failure reason.
 *
 * On success, the caller owns the returned ElfFile and must call
 * ElfFileDestroy() to free it.
 */
ElfParsingStatus ElfFileNew(const char* path, ElfFile** out);

/**
 * @brief Displays the ELF header to stdout.
 *
 * @param elf The ELF file whose header should be displayed.
 */
void ElfFileDisplayHeader(ElfFile* elf);

/**
 * @brief Converts an ElfParsingStatus code to a human-readable string.
 *
 * @param status The status code to convert.
 * @return A constant string describing the status.
 */
const char* ElfParsingStatusToString(ElfParsingStatus status);

/**
 * @brief Frees all resources associated with an ElfFile.
 *
 * @param elf The ELF file to destroy. May be NULL (no-op).
 *
 * Closes the file handle and frees all heap-allocated members.
 */
void ElfFileDestroy(ElfFile* elf);

/**
 * @brief Displays the section table to stdout.
 *
 * @param elf The ELF file whose sections should be displayed.
 */
void ElfFileDisplaySections(ElfFile* elf);

/**
 * @brief Retrieves a section by name.
 *
 * @param name The name of the section to find.
 * @param elf The ELF file to search.
 * @return Pointer to the ElfSection if found, NULL otherwise.
 *
 * The returned pointer is owned by the ElfFile and must not be freed.
 */
ElfSection* ElfFileGetSectionByName(const char* name, ElfFile* elf);

/**
 * @brief Displays the contents of a section as hexadecimal dump.
 *
 * @param name The name of the section to display.
 * @param elf The ELF file containing the section.
 * @return 0 on success, non-zero on failure (e.g., section not found).
 */
int ElfFileDisplaySectionContentsByName(const char* name, ElfFile* elf);

/**
 * @brief Displays the symbol table to stdout.
 *
 * @param elf The ELF file whose symbols should be displayed.
 */
void ElfFileDisplaySymbols(ElfFile* elf);

/**
 * @brief Retrieves all sections of a specific type.
 *
 * @param out_section_count Pointer to receive the number of matching sections.
 * @param type The section type (e.g., SHT_PROGBITS, SHT_REL).
 * @param elf The ELF file to search.
 * @return Heap-allocated array of pointers to matching sections, or NULL.
 *
 * The caller must free the returned array (but not the individual sections).
 */
ElfSection** ElfFileGetSectionsByType(uint32_t* out_section_count,
                                      uint32_t type, ElfFile* elf);

/**
 * @brief Displays all relocation tables to stdout.
 *
 * @param elf The ELF file whose relocations should be displayed.
 */
void ElfFileDisplayRelocations(ElfFile* elf);

#endif  //_ELF_OPS_H