/**
 * @file extract_elf_sym.c
 * @author DUC Corentin
 * @brief implementation of some functions to extract symbol table from a ELF
 * file
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
#include <string.h>

// Manage sym part:
static EXTRACT_STATUS extract_sym_once(Elf32_Sym *sym, FILE *elf_file);
static EXTRACT_STATUS extract_sym_name(FILE *elf_file, Elf32_Sym *sym);
static EXTRACT_STATUS extract_sym_value(FILE *elf_file, Elf32_Sym *sym);
static EXTRACT_STATUS extract_sym_size(FILE *elf_file, Elf32_Sym *sym);
static EXTRACT_STATUS extract_sym_info(FILE *elf_file, Elf32_Sym *sym);
static EXTRACT_STATUS extract_sym_other(FILE *elf_file, Elf32_Sym *sym);
static EXTRACT_STATUS extract_sym_shndx(FILE *elf_file, Elf32_Sym *sym);

// Utils:
static EXTRACT_STATUS file_error(FILE *elf_file);
static bool is_same_endianess();
static bool is_file_big_endian();

// Si LE ou BE
static unsigned char type_data;

/**
 * @brief Set the data type elf file
 *
 * @param e_ident
 */
static void setData(const unsigned char data_type) { type_data = data_type; }

/*Elf32_Shdr sym_section =
          get_section_by_type(SHT_SYMTAB, sections, f->e_ehdr.e_shnum, f->file);

      int nombre_entree_table_symbole =
          sym_section.sh_size / sym_section.sh_entsize;

      // Create the table of symbols
      Elf32_Sym *sym_tab =
          malloc(sizeof(Elf32_Sym) * nombre_entree_table_symbole);*/

Elf32_Sym *initialize_sym(Elf32_Shdr *shdr, Elf32_Half e_shnum,
                          FILE *elf_file) {
  Elf32_Shdr sym_section =
      get_section_by_type(SHT_SYMTAB, shdr, e_shnum, elf_file);

  int nombre_entree_table_symbole =
      sym_section.sh_size / sym_section.sh_entsize;

  Elf32_Sym *sym_tab = malloc(sizeof(Elf32_Sym) * nombre_entree_table_symbole);

  if (sym_tab == NULL) {
    print_warning((unsigned char *)"Erreur allocation : Symbol");
  }

  return sym_tab;
}

void free_sym(Elf32_Sym *sym) {
  if (sym == NULL) {
    return;
  }
  free(sym);
  return;
}

EXTRACT_STATUS extract_sym(Elf32_Sym *sym, unsigned char e_ident[EI_NIDENT],
                           Elf32_Shdr *sections, Elf32_Half e_shnum,
                           FILE *elf_file) {

  // Define endianess
  setData(e_ident[EI_DATA]);

  Elf32_Shdr sym_section =
      get_section_by_type(SHT_SYMTAB, sections, e_shnum, elf_file);

  // Déplacement jusqu'à la table des symboles:
  if (fseek(elf_file, sym_section.sh_offset, SEEK_SET) != 0) {
    printf("Déplacement impossible !");
    exit(1);
  }

  int nombre_entree_table_symbole =
      sym_section.sh_size / sym_section.sh_entsize;

  for (int i = 0; i < nombre_entree_table_symbole; i++) {

    EXTRACT_STATUS extract_status = extract_sym_once(&sym[i], elf_file);

    if (extract_status != SUCCESS_EXTRACT) {
      return extract_status;
    }
  }

  return SUCCESS_EXTRACT;
}

/**
 * @brief Read, extract and manage the endianess of all the informations and
 * stock it in a Elf32_Sym struct and return it
 *
 * @param elf_file
 * @param shdr
 * @return Elf32_Sym
 */
static EXTRACT_STATUS extract_sym_once(Elf32_Sym *sym, FILE *elf_file) {
  // ! Laisser {0} sinon le compilateur prends peur que certains attributs soit
  // retourné n'importe comment
  memset(sym, 0, sizeof(Elf32_Sym));

#define CHECK(p)                                                               \
  if (p != SUCCESS_EXTRACT)                                                    \
    return p;

  CHECK(extract_sym_name(elf_file, sym));
  CHECK(extract_sym_value(elf_file, sym));
  CHECK(extract_sym_size(elf_file, sym));
  CHECK(extract_sym_info(elf_file, sym));
  CHECK(extract_sym_other(elf_file, sym));
  CHECK(extract_sym_shndx(elf_file, sym));

  return SUCCESS_EXTRACT;
}

/**
 * @brief Read, extract and manage the endianess of name before storing it in
 * the structure from a symbol table
 *
 * @param elf_file
 * @param sym
 */
static EXTRACT_STATUS extract_sym_name(FILE *elf_file, Elf32_Sym *sym) {
  Elf32_Word sym_name;
  const size_t return_fread_value =
      fread(&sym_name, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      sym_name = byte_swap(sym_name);
    }
    sym->st_name = sym_name;
  } else {
    return file_error(elf_file);
  }
  return SUCCESS_EXTRACT;
}

/**
 * @brief Read, extract and manage the endianess of value before storing it in
 * the structure from a symbol table
 *
 * @param elf_file
 * @param sym
 */
static EXTRACT_STATUS extract_sym_value(FILE *elf_file, Elf32_Sym *sym) {
  Elf32_Addr sym_value;
  const size_t return_fread_value =
      fread(&sym_value, sizeof(Elf32_Addr), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      sym_value = byte_swap(sym_value);
    }
    sym->st_value = sym_value;
  } else {
    return file_error(elf_file);
  }

  return SUCCESS_EXTRACT;
}

/**
 * @brief Read, extract and manage the endianess of size before storing it in
 * the structure from a symbol table
 *
 * @param elf_file
 * @param sym
 */
static EXTRACT_STATUS extract_sym_size(FILE *elf_file, Elf32_Sym *sym) {
  Elf32_Word sym_size;
  const size_t return_fread_value =
      fread(&sym_size, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      sym_size = byte_swap(sym_size);
    }
    sym->st_size = sym_size;
  } else {
    return file_error(elf_file);
  }
  return SUCCESS_EXTRACT;
}

/**
 * @brief Read, extract and manage the endianess of info before storing it in
 * the structure from a symbol table
 *
 * @param elf_file
 * @param sym
 */
static EXTRACT_STATUS extract_sym_info(FILE *elf_file, Elf32_Sym *sym) {
  unsigned char sym_info;
  const size_t return_fread_value =
      fread(&sym_info, sizeof(unsigned char), 1, elf_file);

  if (return_fread_value == 1) {
    sym->st_info = sym_info;
  } else {
    return file_error(elf_file);
  }

  return SUCCESS_EXTRACT;
}

/**
 * @brief Read, extract and manage the endianess of other before storing it in
 * the structure from a symbol table
 *
 * @param elf_file
 * @param sym
 */
static EXTRACT_STATUS extract_sym_other(FILE *elf_file, Elf32_Sym *sym) {
  unsigned char sym_other;
  const size_t return_fread_value =
      fread(&sym_other, sizeof(unsigned char), 1, elf_file);

  if (return_fread_value == 1) {
    sym->st_other = sym_other;
  } else {
    return file_error(elf_file);
  }

  return SUCCESS_EXTRACT;
}

/**
 * @brief Read, extract and manage the endianess of shndx before storing it in
 * the structure from a symbol table
 *
 * @param elf_file
 * @param sym
 */
static EXTRACT_STATUS extract_sym_shndx(FILE *elf_file, Elf32_Sym *sym) {
  Elf32_Half sym_shndx;
  const size_t return_fread_value =
      fread(&sym_shndx, sizeof(Elf32_Half), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      sym_shndx = byte_swap(sym_shndx);
    }
    sym->st_shndx = sym_shndx;
  } else {
    return file_error(elf_file);
  }

  return SUCCESS_EXTRACT;
}

// ! PLUSIEURS FICHIER L'IMPLEMANTE !
/**
 * @brief If the fread has an error, handle the error
 *
 * @param elf_file
 */
static EXTRACT_STATUS file_error(FILE *elf_file) {
  if (feof(elf_file)) {
    return ERROR_ELF_FILE_END_OF_FILE_UNEXPECTED;
  } else if (ferror(elf_file)) {
    return ERROR_ELF_FILE_READING;
  } else {
    return ERROR_ELF_FILE_UNKNOW;
  }
}

/**
 * @brief Return true if the file is in big endian
 *
 * @return true
 * @return false
 */
static bool is_file_big_endian() { return (type_data == 2); }

/**
 * @brief return true if the host and the file is in the same endianess
 *
 * @return true
 * @return false
 */
static bool is_same_endianess() {
  return (is_big_endian() && is_file_big_endian()) ||
         (!is_big_endian() && !is_file_big_endian());
}