/**
 * @file elf_ops.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-16
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef __ELF_OPS__
#define __ELF_OPS__

#include <elf.h>
#include <stdio.h>

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

#endif //__ELF_OPS__