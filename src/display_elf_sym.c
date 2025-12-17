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

  // * C'est encore un peu brouillon donc je vais expliquer mais il y a sans
  // doute moyen de refractor si quelqu'un à le temps

  // On doit déjà trouver la section des symboles,
  Elf32_Shdr *sym_section = get_shdr_by_name(".symtab", f);
  // On doit trouver la table des chaînes de caractères
  Elf32_Shdr strtab_section = f->e_shrdrs[sym_section->sh_link];
  // On se déplace à l'intérieur de cet section
  Elf32_Addr strtab_addr = strtab_section.sh_offset;

  int nombre_entree_table_symbole =
      sym_section->sh_size / sym_section->sh_entsize;

  for (int i = 0; i < nombre_entree_table_symbole; i++) {
    // On récupère l'index pour le nom en cours dans la table des symboles
    Elf32_Word name_indx = f->sym[i].st_name;

    // Tester si c'est un nom de section:
    if (name_indx == 0) {
      printf("%s\n",
             get_elf_section_name_unlimited(f->e_shrdrs, f->sym[i].st_shndx,
                                            f->e_ehdr.e_shstrndx, f->file));
      continue;
    }

    uint32_t final_addr = strtab_addr + name_indx;

    // On place le curseur à l'endroit ou est le nom
    if (fseek(f->file, final_addr, SEEK_SET) != 0) {
      printf("Erreur de déplacement");
      exit(1);
    }
    // On lit le nom
    printf("%s\n", get_name_sym(f));
  }

  return;
}

static char *get_name_sym(Elf32_File *f) {
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