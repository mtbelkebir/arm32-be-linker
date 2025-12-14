/**
 * @file test_extract_elf_headers.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-13
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/display_elf_header.h"
#include "../include/extract_elf_header.h"
#include <elf.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  Elf32_Ehdr elf;
  bool res = extract_elf_informations(argv[1], &elf);

  // ! Sans la fonction display:
  if (res) {
    printf("\n------Informations ELF------\n");
    printf("Magic Number: ");
    for (int i = 0; i < EI_NIDENT; i++) {
      printf("[%x]", elf.e_ident[i]);
    }
    printf("\nType -> %x", elf.e_type);
    printf("\nMachine -> %x", elf.e_machine);
    printf("\nVersion -> %x", elf.e_version);
    printf("\nEntry -> %x", elf.e_entry);
    printf("\nPhoff -> %x", elf.e_phoff);
    printf("\nShoff -> %x", elf.e_shoff);
    printf("\nFlags -> %x", elf.e_flags);
    printf("\nEhsize -> %x", elf.e_ehsize);
    printf("\nPhentsize -> %x", elf.e_phentsize);
    printf("\nPhnum -> %x", elf.e_phnum);
    printf("\nShentsize -> %x", elf.e_shentsize);
    printf("\nShnum -> %x", elf.e_shnum);
    printf("\nShstrndx -> %x", elf.e_shstrndx);
    printf("\n------------------------------\n");
  }

  // * Avec la fonction display:

  if (res) {
    display_elf_headers(&elf);
  }

  return 0;
}