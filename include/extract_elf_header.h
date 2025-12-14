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
 * @brief Extract all informations header from a ELF file into the structure
 * passed in argument, return true if the extraction is successfull
 *
 * @param filename
 * @param header_informations
 * @return true
 * @return false
 */
bool extract_elf_informations(char filename[], Elf32_Ehdr *header_informations);

#endif //__EXTRACT_ELF_HEADER__