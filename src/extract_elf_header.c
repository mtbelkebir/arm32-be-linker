/**
 * @file extract_elf_header.c
 * @author DUC Corentin
 * @brief Functions implementations for extracting informations from ELF files
 * @version 0.1
 * @date 2025-12-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/extract_elf_header.h"
#include "../include/bit_operations.h"
#include "../include/logger.h"
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ! All the functions manage endianness before storing
static void extract_ident(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_type(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_machine(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_version(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_entry(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_phoff(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_shoff(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_flags(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_ehsize(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_phentsize(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_phnum(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_shentsize(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_shnum(FILE *elf_file, Elf32_Ehdr *header_informations);
static void extract_shstrndx(FILE *elf_file, Elf32_Ehdr *header_informations);
static void file_error(FILE *elf_file);
static bool is_same_endianess(const Elf32_Ehdr *header_informations);
static bool is_file_big_endian(const Elf32_Ehdr *header_informations);

/**
 * @brief Extract all informations in a ELF file and insert it in the structure
 * passed in argument. Return true if everything is ok
 *
 * @param filename
 * @param header_informations
 * @return true
 * @return false
 */
bool extract_elf_informations(char filename[],
                              Elf32_Ehdr *header_informations) {
  FILE *elf_file = fopen(filename, "rb");
  if (elf_file == NULL) {
    unsigned char error_message[] = "The file can't be opened";
    print_error(error_message);
  }

  // Extract all the informations:
  extract_ident(elf_file, header_informations);
  extract_type(elf_file, header_informations);
  extract_machine(elf_file, header_informations);
  extract_version(elf_file, header_informations);
  extract_entry(elf_file, header_informations);
  extract_phoff(elf_file, header_informations);
  extract_shoff(elf_file, header_informations);
  extract_flags(elf_file, header_informations);
  extract_ehsize(elf_file, header_informations);
  extract_phentsize(elf_file, header_informations);
  extract_phnum(elf_file, header_informations);
  extract_shentsize(elf_file, header_informations);
  extract_shnum(elf_file, header_informations);
  extract_shstrndx(elf_file, header_informations);

  return true;
}

/**
 * @brief Read identification informations from ELF files and insert it in the
 * structure passed in argument
 *
 * @pre A file correctly opened
 * @post The identification's ELF file in the ELF structure
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_ident(FILE *elf_file, Elf32_Ehdr *header_informations) {
  unsigned char indentification[EI_NIDENT];
  const size_t return_fread_value =
      fread(indentification, sizeof(unsigned char), EI_NIDENT, elf_file);

  if (return_fread_value == EI_NIDENT) {
    memcpy(header_informations->e_ident, indentification, EI_NIDENT);
  } else {
    file_error(elf_file);
  }
  return;
}

/**
 * @brief Read type from ELF files and insert it in the structure passed in
 * argument
 *
 * @pre A file correctly opened
 * @post The type's ELF file in the ELF structure
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_type(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Half type;
  const size_t return_fread_value =
      fread(&type, sizeof(Elf32_Half), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      type = byte_swap(type);
    }
    header_informations->e_type = type;
  } else {
    file_error(elf_file);
  }
  return;
}

/**
 * @brief Read machine from ELF files and insert it in the structure passed in
 * argument
 *
 * @pre A file correctly opened
 * @post The machine's ELF file in the ELF structure
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_machine(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Half machine;
  const size_t return_fread_value =
      fread(&machine, sizeof(Elf32_Half), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      machine = byte_swap(machine);
    }
    header_informations->e_machine = machine;
  } else {
    file_error(elf_file);
  }
  return;
}

/**
 * @brief  Read version from ELF files and insert it in the structure passed in
 * argument
 *
 * @pre A file correctly opened
 * @post The version's ELF file in the ELF structure
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_version(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Word version;
  const size_t return_fread_value =
      fread(&version, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      version = byte_swap(version);
    }
    header_informations->e_version = version;
  } else {
    file_error(elf_file);
  }
  return;
}

/**
 * @brief Read entry from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_entry(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Addr entry;

  const size_t return_fread_value =
      fread(&entry, sizeof(Elf32_Addr), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      entry = byte_swap(entry);
    }
    header_informations->e_entry = entry;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief Read phoff from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_phoff(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Off phoff;
  const size_t return_fread_value =
      fread(&phoff, sizeof(Elf32_Off), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      phoff = byte_swap(phoff);
    }
    header_informations->e_phoff = phoff;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief Read shoff from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_shoff(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Off shoff;
  const size_t return_fread_value =
      fread(&shoff, sizeof(Elf32_Off), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      shoff = byte_swap(shoff);
    }
    header_informations->e_shoff = shoff;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief Read flags from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_flags(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Word flags;
  const size_t return_fread_value =
      fread(&flags, sizeof(Elf32_Word), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      flags = byte_swap(flags);
    }
    header_informations->e_flags = flags;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief Read ehsize from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_ehsize(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Half ehsize;
  const size_t return_fread_value =
      fread(&ehsize, sizeof(Elf32_Half), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      ehsize = byte_swap(ehsize);
    }
    header_informations->e_ehsize = ehsize;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief Read phentsize from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_phentsize(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Half phentsize;
  const size_t return_fread_value =
      fread(&phentsize, sizeof(Elf32_Half), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      phentsize = byte_swap(phentsize);
    }
    header_informations->e_phentsize = phentsize;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief Read phnum from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_phnum(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Half phnum;
  const size_t return_fread_value =
      fread(&phnum, sizeof(Elf32_Half), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      phnum = byte_swap(phnum);
    }
    header_informations->e_phnum = phnum;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief Read shentsize from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_shentsize(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Half shentsize;
  const size_t return_fread_value =
      fread(&shentsize, sizeof(Elf32_Half), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      shentsize = byte_swap(shentsize);
    }
    header_informations->e_shentsize = shentsize;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief Read shnum from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_shnum(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Half shnum;
  const size_t return_fread_value =
      fread(&shnum, sizeof(Elf32_Half), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      shnum = byte_swap(shnum);
    }
    header_informations->e_shnum = shnum;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief Read shstrndx from ELF files and insert it in the structure passed in
 * argument
 *
 * @param elf_file
 * @param header_informations
 */
static void extract_shstrndx(FILE *elf_file, Elf32_Ehdr *header_informations) {
  Elf32_Half shstrndx;
  const size_t return_fread_value =
      fread(&shstrndx, sizeof(Elf32_Half), 1, elf_file);

  if (return_fread_value == 1) {
    if (!is_same_endianess(header_informations)) {
      shstrndx = byte_swap(shstrndx);
    }
    header_informations->e_shstrndx = shstrndx;
  } else {
    file_error(elf_file);
  }

  return;
}

/**
 * @brief If the fread has an error, handle the error
 *
 * @param elf_file
 */
static void file_error(FILE *elf_file) {
  if (feof(elf_file)) {
    unsigned char error_message[] =
        "Error reading ELF file: unexpected end of file";
    print_error(error_message);
  } else if (ferror(elf_file)) {
    unsigned char error_message[] = "Error reading elf files";
    perror("Error reading elf files");
    print_error(error_message);
  }
  return;
}

/**
 * @brief Return true if the file is in big endian, false else
 *
 * @param header_informations
 * @return true
 * @return false
 */
static bool is_file_big_endian(const Elf32_Ehdr *header_informations) {
  return (header_informations->e_ident[EI_DATA] == 2);
}

/**
 * @brief return true if the host and the file is in the same endianess
 *
 * @param header_informations
 * @return true
 * @return false
 */
bool is_same_endianess(const Elf32_Ehdr *header_informations) {
  return (is_host_big_endian() && is_file_big_endian(header_informations)) ||
         (!is_host_big_endian() && !is_file_big_endian(header_informations));
}