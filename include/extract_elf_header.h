/**
 * @file extract_elf_header.h
 * @author This header file contains some functions to extract content from
 * header ELF files
 * @brief
 * @version 0.1
 * @date 2025-12-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __EXTRACT_ELF_HEADER__
#define __EXTRACT_ELF_HEADER__

#include <elf.h>
#include <stdbool.h>
#include <stdint.h>
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
 *
 * @exception CLOSES THE ENTIRE PROGRAM IF THE ELF HEADER IS INVALID
 */
Elf32_Ehdr extract_elf_informations(FILE *elf_file);

#endif //__EXTRACT_ELF_HEADER__