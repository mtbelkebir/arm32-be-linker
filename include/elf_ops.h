#ifndef _ELF_OPS_H
#define _ELF_OPS_H
#include <elf.h>
#include <stdio.h>

/**
 * @brief In-memory representation of a 32-bit ELF file.
 *
 * The struct owns heap-allocated members and should be freed with
 * free_elf_file().
 */
typedef struct Elf32_File
{
    FILE *file;           /**< Open FILE* for the underlying file (may be NULL). */
    Elf32_Ehdr e_ehdr;    /**< Parsed ELF header (host endianness). */
    Elf32_Shdr *e_shrdrs; /**< Array of section headers (heap-allocated), NULL if none. */
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
 * @param file Pointer returned by read_elf(). If NULL the function does nothing.
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
#endif //_ELF_OPS_H