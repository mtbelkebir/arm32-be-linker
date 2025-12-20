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
static void get_value_sym(Elf32_Addr *value);
static void get_size_sym(Elf32_Word *size);
static void get_type_sym(unsigned char st_info);
static void get_other_sym(unsigned char other);
static void get_shndx_sym(Elf32_Half *shndx);

/**
 * @brief display all the element of a Elf32_Sym
 *
 * @param f
 */
void display_sym_tab(Elf32_File *f) {

  // * C'est encore un peu brouillon donc je vais expliquer mais il y a sans
  // doute moyen de refractor si quelqu'un à le temps

  // On doit déjà trouver la section des symboles,
  Elf32_Shdr sym_section =
      get_section_by_type(SHT_SYMTAB, f->e_shrdrs, f->e_ehdr.e_shnum, f->file);
  // On doit trouver la table des chaînes de caractères
  Elf32_Shdr strtab_section = f->e_shrdrs[sym_section.sh_link];
  // On se déplace à l'intérieur de cet section
  Elf32_Addr strtab_addr = strtab_section.sh_offset;

  int nombre_entree_table_symbole =
      sym_section.sh_size / sym_section.sh_entsize;

  printf("\nSymbol table '.symtab' contains %d entries:\n",
         nombre_entree_table_symbole);
  printf("  %-20s %-10s %-8s %-8s %-10s %-10s\n", "Nom", "Valeur", "Taille",
         "Bind", "Type", "Index");
  printf("  "
         "---------------------------------------------------------------------"
         "---------\n");

  for (int i = 0; i < nombre_entree_table_symbole; i++) {
    // On récupère l'index pour le nom en cours dans la table des symboles
    Elf32_Word name_indx = f->sym[i].st_name;

    printf("  ");

    // Tester si c'est un nom de section:
    if (name_indx == 0) {
      printf("%-20s ",
             get_elf_section_name_unlimited(f->e_shrdrs, f->sym[i].st_shndx,
                                            f->e_ehdr.e_shstrndx, f->file));

    } else {
      uint32_t final_addr = strtab_addr + name_indx;

      // On place le curseur à l'endroit ou est le nom
      if (fseek(f->file, final_addr, SEEK_SET) != 0) {
        printf("Erreur de déplacement");
        exit(1);
      }
      // On lit le nom
      printf("%-20s ", get_name_sym(f));
    }

    // The rest:
    get_value_sym(&f->sym[i].st_value);
    get_size_sym(&f->sym[i].st_size);
    get_type_sym(f->sym[i].st_info);
    get_other_sym(f->sym[i].st_other);
    get_shndx_sym(&f->sym[i].st_shndx);
  }
}

/**
 * @brief Get the name sym object
 *
 * @param f
 * @return char*
 */
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

/**
 * @brief Get the value sym object
 *
 * ! Please refer to the type to know how to interprete it
 * @param value
 */
static void get_value_sym(Elf32_Addr *value) { printf("0x%08X ", *value); }

/**
 * @brief Get the size sym object
 *
 * @param size
 */
static void get_size_sym(Elf32_Word *size) { printf("%-8d ", *size); }

/**
 * @brief Get the type sym object
 *
 * @param st_info
 */
static void get_type_sym(unsigned char st_info) {
  uint32_t bind = ELF32_ST_BIND(st_info);
  uint32_t type = ELF32_ST_TYPE(st_info);

  // Manage the binding
  switch (bind) {
  case STB_LOCAL:
    printf("%-10s ", "LOCAL");
    break;
  case STB_GLOBAL:
    printf("%-10s ", "GLOBAL");
    break;
  case STB_WEAK:
    printf("%-10s ", "WEAK");
    break;
  case STB_LOPROC:
  case STB_HIPROC:
    printf("%-10s ", "RESERVED");
    break;
  default:
    printf("%-10s ", "NUM");
    break;
  }

  // Manage the type:
  switch (type) {
  case STT_NOTYPE:
    printf("%-8s ", "NOTYPE");
    break;
  case STT_OBJECT:
    printf("%-8s ", "OBJECT");
    break;
  case STT_FUNC:
    printf("%-8s ", "FUNC");
    break;
  case STT_SECTION:
    printf("%-8s ", "SECTION");
    break;
  case STT_FILE:
    printf("%-8s ", "FILE");
    break;
  case STT_LOPROC:
  case STT_HIPROC:
    printf("%-8s ", "RESERVED");
    break;
  default:
    printf("%-8s ", "UNKNOWN");
    break;
  }
}

/**
 * @brief Get the other sym object
 *
 * @param other
 */
static void get_other_sym(unsigned char other) {
  if (other != 0) {
    printf("%-10s ", "NOT DEFAULT");
    return;
  }
  printf("%-10s ", "DEFAULT");
}

/**
 * @brief Get the shndx sym object
 *
 * @param shndx
 */
static void get_shndx_sym(Elf32_Half *shndx) {
  switch (*shndx) {
  case SHN_UNDEF:
    printf("UNDEF\n");
    break;
  case SHN_ABS:
    printf("ABS\n");
    break;
  case SHN_COMMON:
    printf("COMMON\n");
    break;
  default:
    printf("%d\n", *shndx);
    break;
  }
}