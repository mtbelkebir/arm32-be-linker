/**
 * @file extract_elf_section.h
 * @author BELKHOUJA MOHAMMED Yassine
 * @brief
 * @version 0.1
 * @date 2025-12-16
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __SECTION_HEADER__
#define __SECTION_HEADER__

#include <elf.h>
#include <stdint.h>
#include <stdio.h>

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
Elf32_Shdr *extract_section_headers(FILE *elf_file, uint32_t e_shoff,
                                    uint32_t e_shnum, uint32_t e_shentsize,
                                    unsigned char e_ident[EI_NIDENT]);

// ! Must be move
// void affichage(Elf32_Shdr *SH, uint32_t e_shnum);
#endif
