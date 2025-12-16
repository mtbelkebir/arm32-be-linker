/**
 * @file extract_elf_section.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-16
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "../include/extract_elf_section.h"
#include "../include/logger.h"
#include "../util.h"
#include <elf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Manage section part:
static void setData(const unsigned char data_type);
static Elf32_Shdr extract_section(FILE *elf_file);

// Extraction implementation:
static void extract_section_name(Elf32_Shdr *section, FILE *elf_file);
static void extract_section_type(Elf32_Shdr *section, FILE *elf_file);
static void extract_section_flags(Elf32_Shdr *section, FILE *elf_file);
static void extract_section_addr(Elf32_Shdr *section, FILE *elf_file);
static void extract_section_offset(Elf32_Shdr *section, FILE *elf_file);
static void extract_section_size(Elf32_Shdr *section, FILE *elf_file);
static void extract_section_link(Elf32_Shdr *section, FILE *elf_file);
static void extract_section_info(Elf32_Shdr *section, FILE *elf_file);
static void extract_section_addralign(Elf32_Shdr *section, FILE *elf_file);
static void extract_section_entsize(Elf32_Shdr *section, FILE *elf_file);

// Utils:
static void file_error(FILE *elf_file);
static bool is_same_endianess();
static bool is_file_big_endian();

// Si LE ou BE
unsigned char type_data;

/**
 * @brief Set the data type elf file
 *
 * @param e_ident
 */
static void setData(const unsigned char data_type) { type_data = data_type; }

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
                                    unsigned char e_ident[EI_DATA]) {

  // ! If the file is not correct
  if (fseek(elf_file, e_shoff, SEEK_SET) != 0) {
    return NULL;
  }

  setData(e_ident[EI_DATA]);

  // ! YOU MUST BE SURE THAT E_SHENTSIZE IS CORRECT
  // * It will be a good idea to fix it to sizeof(Elf32_Shdr)
  Elf32_Shdr *sections = malloc(e_shentsize * e_shnum);

  for (int i = 0; i < e_shnum; i++) {
    sections[i] = extract_section(elf_file);
  }

  return sections;
}

/**
 * @brief Read, extract and manage the endianness of a section in a ELF file
 *
 * @param elf_file
 * @return Elf32_Shdr
 */
static Elf32_Shdr extract_section(FILE *elf_file) {
  Elf32_Shdr section;

  extract_section_name(&section, elf_file);
  extract_section_type(&section, elf_file);
  extract_section_flags(&section, elf_file);
  extract_section_addr(&section, elf_file);
  extract_section_offset(&section, elf_file);
  extract_section_size(&section, elf_file);
  extract_section_link(&section, elf_file);
  extract_section_info(&section, elf_file);
  extract_section_addralign(&section, elf_file);
  extract_section_entsize(&section, elf_file);

  return section;
}

/**
 * @brief Read, extract and manage the endianness of name from a ELF file
 * and insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_name(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Word section_name;
  size_t return_fread_value =
      fread(&section_name, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_name = byte_swap(section_name);
    }
    section->sh_name = section_name;
  } else {
    file_error(elf_file);
  }
}

/**
 * @brief Read, extract and manage the endianness of type from a ELF file
 * and insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_type(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Word section_type;
  size_t return_fread_value =
      fread(&section_type, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_type = byte_swap(section_type);
    }
    section->sh_type = section_type;
  } else {
    file_error(elf_file);
  }
}

/**
 * @brief Read, extract and manage the endianness of flags from a ELF file
 * and insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_flags(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Word section_flags;
  size_t return_fread_value =
      fread(&section_flags, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_flags = byte_swap(section_flags);
    }
    section->sh_flags = section_flags;
  } else {
    file_error(elf_file);
  }
}

/**
 * @brief Read, extract and manage the endianness of addr from a ELF file
 * and insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_addr(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Addr section_addr;
  size_t return_fread_value =
      fread(&section_addr, sizeof(Elf32_Addr), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_addr = byte_swap(section_addr);
    }
    section->sh_addr = section_addr;
  } else {
    file_error(elf_file);
  }
}
/**
 * @brief Read, extract and manage the endianness of offset from a ELF file
 * and insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_offset(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Off section_offset;
  size_t return_fread_value =
      fread(&section_offset, sizeof(Elf32_Off), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_offset = byte_swap(section_offset);
    }
    section->sh_offset = section_offset;
  } else {
    file_error(elf_file);
  }
}

/**
 * @brief Read, extract and manage the endianness of size from a ELF file
 * and insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_size(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Word section_size;
  size_t return_fread_value =
      fread(&section_size, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_size = byte_swap(section_size);
    }
    section->sh_size = section_size;
  } else {
    file_error(elf_file);
  }
}

/**
 * @brief Read, extract and manage the endianness of link from a ELF file
 * and insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_link(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Word section_link;
  size_t return_fread_value =
      fread(&section_link, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_link = byte_swap(section_link);
    }
    section->sh_link = section_link;
  } else {
    file_error(elf_file);
  }
}
/**
 * @brief Read, extract and manage the endianness of info from a ELF file
 * and insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_info(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Word section_info;
  size_t return_fread_value =
      fread(&section_info, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_info = byte_swap(section_info);
    }
    section->sh_info = section_info;
  } else {
    file_error(elf_file);
  }
}

/**
 * @brief Read, extract and manage the endianness of addralign from a ELF file
 * and insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_addralign(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Word section_addralign;
  size_t return_fread_value =
      fread(&section_addralign, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_addralign = byte_swap(section_addralign);
    }
    section->sh_addralign = section_addralign;
  } else {
    file_error(elf_file);
  }
}

/**
 * @brief Read, extract and manage the endianness of entsize from a ELF file and
 * insert it into the correct attribute of a structure Elf32_Shdr
 *
 * @param section
 * @param elf_file
 */
static void extract_section_entsize(Elf32_Shdr *section, FILE *elf_file) {
  Elf32_Word section_entsize;
  size_t return_fread_value =
      fread(&section_entsize, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess()) {
      section_entsize = byte_swap(section_entsize);
    }
    section->sh_entsize = section_entsize;
  } else {
    file_error(elf_file);
  }
}

/*void affichage(Elf32_Shdr *SH, uint32_t e_shnum) {

  printf("Idx | Name  | Type | Flg | Addr | Off | Size  | Lk | Inf | Al | "
         "sh_entsize\n");
  printf("----|----------|------------|----------|----------|----------|-------"
         "---|--------|--------|--------------|----------\n");

  for (int i = 0; i < e_shnum; i++) {

    const Elf32_Shdr current_sh = SH[i];

    printf("[%d]", i);

    printf(" 0x%08X", current_sh.sh_name);

    const char *type_name;
    if (current_sh.sh_type >= 0 && current_sh.sh_type <= 11) {
      type_name = section_type_names[current_sh.sh_type];
    } else {
      type_name = section_type_names[12]; // Type inconnu
    }
    printf(" | %-10s", type_name);
    // Printing flags
    char type_flag[5];
    int i = 0;
    int flags = current_sh.sh_flags;
    if (flags & SHF_WRITE) {
      type_flag[i++] = 'W';
    }
    if (flags & SHF_ALLOC)
      type_flag[i++] = 'A';
    if (flags & SHF_EXECINSTR)
      type_flag[i++] = 'X';
    if (flags & SHF_MASKPROC)
      type_flag[i++] = 'M';
    type_flag[i] = '\0';
    // End Printing Flags
    printf(" | %-10s", type_flag);

    printf(" | 0x%08X", current_sh.sh_addr);

    printf(" | 0x%08X", current_sh.sh_offset);

    printf(" | 0x%08X", current_sh.sh_size);

    printf(" | 0x%08X", current_sh.sh_link);

    printf(" | 0x%08X", current_sh.sh_info);

    printf(" | 0x%08X", current_sh.sh_addralign);

    printf(" | 0x%08X", current_sh.sh_entsize);

    printf("\n");
  }
}*/

// ! PLUSIEURS FICHIER L'IMPLEMANTE !
/**
 * @brief If the fread has an error, handle the error
 *
 * @param elf_file
 */
static void file_error(FILE *elf_file) {
  if (feof(elf_file)) {
    print_error((unsigned char *)"End of file unexpected");
  } else if (ferror(elf_file)) {
    perror("Error reading elf files");
    print_error((unsigned char *)"Error reading elf file");
  } else {
    print_error((unsigned char *)"Unknow error reading elf file");
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
bool is_same_endianess() {
  return (is_big_endian() && is_file_big_endian()) ||
         (!is_big_endian() && !is_file_big_endian());
}