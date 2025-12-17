/**
 * @file display_elf_sym.c
 * @author DUC Corentin
 * @brief debug the dislay for sym
 * @version 0.1
 * @date 2025-12-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/elf_ops.h"
#include "../include/logger.h"
#include "../util.h"
#include <elf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static char *get_name_sym(Elf32_File *f);

void display_sym_tab(Elf32_File *f) {
  printf("%s\n", get_name_sym(f));
  return;
}

static char *get_name_sym(Elf32_File *f) {

  Elf32_Shdr *sym_tab = get_shdr_by_name(".symtab", f);

  uint32_t strtab_index = sym_tab->sh_link;
  uint32_t final_adresse = f->e_shrdrs[strtab_index].sh_offset + f->sym.st_name;

  if (fseek(f->file, final_adresse, SEEK_SET) != 0) {
    printf("Erreur de déplacement");
    exit(1);
  }

  int current_size = SYM_NAME_SIZE;
  char *name_sym = malloc(current_size * sizeof(char));

  if (name_sym == NULL) {
    printf("Allocation échoué !");
    exit(1);
  }

  char current_char = 0;
  int current_i = 0;
  while ((current_char = fgetc(f->file)) != '\0') {
    if (current_i >= current_size) {
      current_size *= 2;
      name_sym = realloc(name_sym, current_size);
      if (name_sym == NULL) {
        printf("Allocation échoué !");
        exit(1);
      }
    }

    name_sym[current_i] = current_char;
    current_i += 1;
  }
  name_sym[current_i] = '\0';

  return name_sym;
}