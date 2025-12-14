/**
 * @file display_elf_headers.h
 * @author DUC Corentin
 * @brief Display headers from a ELF files
 * @version 0.1
 * @date 2025-12-14
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <elf.h>

void display_elf_headers(const Elf32_Ehdr *elf);