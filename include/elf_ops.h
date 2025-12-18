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
typedef struct Elf32_File {
  FILE *file;        /**< Open FILE* for the underlying file (may be NULL). */
  Elf32_Ehdr e_ehdr; /**< Parsed ELF header (host endianness). */
  Elf32_Shdr *
      e_shrdrs; /**< Array of section headers (heap-allocated), NULL if none. */
  Elf32_Sym *sym;
  // TODO: Check if there aren't more required fields
} Elf32_File;

/**
 * @brief Read and parse an ELF file into a heap-allocated container.
 *
 * This function opens the file, reads the ELF header and section headers
 * (converting to host endianness when necessary) and returns an
 * allocated Elf32_File structure.
 *
 * @param path Path to the ELF file to read.
 * @return Pointer to allocated Elf32_File on success, NULL on error.
 *
 * @note Caller is responsible for calling free_elf_file() to release resources.
 */
Elf32_File *read_elf(const char *path);

/**
 * @brief Free an Elf32_File and its owned resources.
 *
 * Closes the underlying FILE* (if open), frees the section headers array
 * and then frees the Elf32_File structure itself.
 *
 * @param file Pointer returned by read_elf(). If NULL the function does
 * nothing.
 */
void free_elf_file(Elf32_File *file);

/**
 * @brief Prints the header of the specified ELF File
 *
 */
void print_elf_header(Elf32_File *f);

/**
 * @brief Prints the section table of the specified
 *
 * @param f
 */
void print_section_table(Elf32_File *f);

/**
 * @brief Extract all informations in a ELF file and return the structure
 * contains all the informations
 *
 * @pre A file correctly open
 * @post A Elf32_Ehdr with all informations insert from the ELF file
 *
 * @param elf_file
 * @return Elf32_Ehdr
 */
Elf32_Ehdr __internal_extract_elf_header(FILE *elf_file);

/**
 * @brief Display all the informations in the elf structure
 *
 * @pre Elf structure correctly initiate
 * @post Display all informations with traduction if necessary
 *
 * @param elf
 */
void __internal_display_elf_header(Elf32_Ehdr header_informations);

/**
 * @brief Extract all the informations from the section table of a ELF File
 *
 * ! free() is required after using the structure
 *
 * @param elf_file
 * @param e_shoff
 * @param e_shnum
 * @param e_shentsize
 * @param e_ident
 * @return Elf32_Shdr*
 */
Elf32_Shdr __internal_extract_elf_section(FILE *elf_file, uint32_t e_shoff,
                                          uint32_t e_shnum,
                                          uint32_t e_shentsize,
                                          unsigned char e_ident[EI_DATA]);

// ! I don't know the params of this one
void __internal_display_elf_section();

/**
 * @brief Get the elf section name object
 *
 * @param section
 * @param target_id
 * @param e_shstrndx
 * @param elf_file
 * @return char*
 */
char *get_elf_section_name_unlimited(Elf32_Shdr *section, int target_id,
                                     Elf32_Half e_shstrndx, FILE *elf_file);

/**
 * @brief Displays all sections of the given ELF file in a formatted table.
 *
 * Prints a section header table matching the format of `readelf -S`, showing
 * all sections with their properties including name, type, address, offset,
 * size, flags, and other metadata.
 *
 * @param f Pointer to an Elf32_File structure opened with read_elf().
 *
 * @pre f is not NULL and contains valid section header data.
 * @post Formatted section table printed to stdout.
 *
 * @note Section names are retrieved from the section header string table.
 * @note Flags displayed: W=Write, A=Alloc, X=Exec, M=Merge, S=Strings, I=Info,
 * L=Link.
 * @note If a section name cannot be retrieved, an empty string is displayed.
 *
 * @see display_elf_section_contents() to display section contents.
 */
void display_elf_sections(Elf32_File *f);

/**
 * @brief Display the contents of a section by name.
 *
 * Searches for a section with the given name in the ELF file and displays
 * its raw contents as 32-bit words in hexadecimal format (4 words per line).
 *
 * @param section_name Name of the section to display (e.g., ".text", ".data")
 * @param f Pointer to an Elf32_File structure opened with read_elf()
 *
 * @return 1 on success, -1 if section not found, 0 on I/O error
 *
 * @note The section contents are byte-swapped if necessary to match host
 * endianness.
 * @note Section names are matched exactly (case-sensitive).
 */
int display_elf_section_contents(const char *section_name, Elf32_File *f);

/**
 * @brief Extract all informations in a ELF file and return the structure
 * contains all the informations
 *
 * @pre A file correctly open
 * @post A Elf32_Ehdr with all informations insert from the ELF file
 *
 * @param elf_file
 * @return Elf32_Ehdr
 *
 * @exception CLOSES THE ENTIRE PROGRAM IF THE ELF HEADER IS INVALID
 */
Elf32_Ehdr extract_elf_informations(FILE *elf_file);

/**
 * @brief Extract all the informations from the section table of a ELF File
 *
 * ! free() is required after using the structure
 *
 * @param elf_file
 * @param e_shoff
 * @param e_shnum
 * @param e_shentsize
 * @param e_ident
 * @return Elf32_Shdr*
 */
Elf32_Shdr *extract_section_headers(FILE *elf_file, Elf32_Off e_shoff,
                                    Elf32_Half e_shnum, Elf32_Half e_shentsize,
                                    unsigned char e_ident[EI_NIDENT]);

/**
 * @brief Display all the informations in the elf structure
 *
 * @pre Elf structure correctly initiate
 * @post Display all informations with traduction if necessary
 *
 * @param elf
 */
void display_elf_headers(const Elf32_Ehdr *elf);

/**
 * @brief Get the shdr by name object
 *
 * @param section_name
 * @param f
 * @returns Pointer to the section header, NULL if it wasn't found
 */
Elf32_Shdr *get_shdr_by_name(const char *section_name, Elf32_File *f);

/**
 * @brief Returns a pointer to the section header identified by it's index.
 * Returns NULL if not found
 *
 */
Elf32_Shdr *get_shdr_by_nbr(uint32_t section_number, Elf32_File *f);

Elf32_Sym *extract_sym(unsigned char e_ident[EI_NIDENT], Elf32_Shdr *sections,
                       Elf32_Half e_shnum, FILE *elf_file);

void display_sym_tab(Elf32_File *f);

/**
 * @brief Get the section by type object
 *
 * !Return a Elf32_shdr with all is attribute with 0 if no one sections match
 * with the type
 *
 * @param flag
 * @param sections
 * @param e_shnum
 * @param elf_file
 * @return Elf32_Shdr
 */
Elf32_Shdr get_section_by_type(Elf32_Word flag, Elf32_Shdr *sections,
                               Elf32_Half e_shnum, FILE *elf_file);
#endif //_ELF_OPS_H
