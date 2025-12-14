/**
 * @file display_elf_headers.c
 * @author DUC Corentin
 * @brief Implements functions to display headers from ELF files
 * @version 0.1
 * @date 2025-12-14
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/display_elf_header.h"
#include "../include/logger.h"
#include <elf.h>
#include <stdio.h>

// ! EM_MIPS_RS4_BE seems to be undeclared in elf.h (10 value)
#define EM_MIPS_RS4_BE 0xA

static void display_ident(const unsigned char e_ident[]);
static void display_type(const Elf32_Half *e_type);
static void display_machine(const Elf32_Half *e_machine);
static void display_version(const Elf32_Word *e_version);
static void display_entry(const Elf32_Addr *e_entry);
static void display_phoff(const Elf32_Off *e_phoff);
static void display_shoff(const Elf32_Off *e_shoff);
static void display_flags(const Elf32_Word *flags);
static void display_ehsize(const Elf32_Half *ehsize);
static void display_phentsize(const Elf32_Half *phentsize);
static void display_phnum(const Elf32_Half *phnum);
static void display_shentsize(const Elf32_Half *shentsize);
static void display_shnum(const Elf32_Half *shnum);
static void display_shstrndx(const Elf32_Half *shstrndx);

void display_elf_headers(const Elf32_Ehdr *elf) {
  display_ident(elf->e_ident);
  display_type(&elf->e_type);
  display_machine(&elf->e_machine);
  display_version(&elf->e_version);
  display_entry(&elf->e_entry);
  display_phoff(&elf->e_phoff);
  display_shoff(&elf->e_shoff);
  display_flags(&elf->e_flags);
  display_ehsize(&elf->e_ehsize);
  display_phentsize(&elf->e_phentsize);
  display_phnum(&elf->e_phnum);
  display_shentsize(&elf->e_shentsize);
  display_shnum(&elf->e_shnum);
  display_shstrndx(&elf->e_shstrndx);

  return;
}

/**
 * @brief Display all informations in the e_ident array
 *
 * @param e_ident
 */
static void display_ident(const unsigned char e_ident[]) {
  print_notification((unsigned char *)"ELF Identification:");

  // Identification
  print_notification((unsigned char *)"File identification");
  printf("EI_MAG0 -> (%X)\n", e_ident[EI_MAG0]);
  printf("EI_MAG1 -> (%c)\n", e_ident[EI_MAG1]);
  printf("EI_MAG2 -> (%c)\n", e_ident[EI_MAG2]);
  printf("EI_MAG3 -> (%c)\n", e_ident[EI_MAG3]);

  // Class
  print_notification((unsigned char *)"File class");
  switch (e_ident[EI_CLASS]) {
  case ELFCLASSNONE:
    print_warning((unsigned char *)"Class invalid !");
    break;
  case ELFCLASS32:
    print_notification((unsigned char *)"32-bit objects");
    break;
  case ELFCLASS64:
    print_notification((unsigned char *)"64-bit objects");
    break;
  default:
    print_warning(
        (unsigned char *)"La classe ne correspond à aucun type prédéfinis !");
  }

  // Data:
  print_notification((unsigned char *)"Data encoding");
  switch (e_ident[EI_DATA]) {
  case ELFDATANONE:
    print_warning((unsigned char *)"Invalid data encoding !");
    break;
  case ELFDATA2LSB:
    print_notification((unsigned char *)"Little Endian");
    break;
  case ELFDATA2MSB:
    print_notification((unsigned char *)"Big Endian");
    break;
  default:
    print_warning(
        (unsigned char
             *)"La data encodé ne correspond à aucun type prédéfinis !");
  }

  // Version:
  print_notification((unsigned char *)"File version");
  switch (e_ident[EI_VERSION]) {
  case EV_CURRENT:
    printf("(%d)\n", EV_CURRENT);
    break;
  default:
    print_warning((unsigned char *)"La version n'est pas bonne !");
  }

  // EI_PAD:
  print_notification((unsigned char *)"File pad");
  printf("(%d)\n", e_ident[EI_PAD]);

  // EI_NINDENT:
  print_notification((unsigned char *)"e_ident size");
  printf("(%d)\n", e_ident[EI_NIDENT]);

  return;
}

/**
 * @brief Display the type's file depending on the e_type
 *
 * @param e_type
 */
static void display_type(const Elf32_Half *e_type) {
  switch (*e_type) {
  case ET_NONE:
    print_warning((unsigned char *)"No file type");
    break;
  case ET_REL:
    print_notification((unsigned char *)"Relocatable file");
    break;
  case ET_EXEC:
    print_notification((unsigned char *)"Executable file");
    break;
  case ET_DYN:
    print_notification((unsigned char *)"Shared file");
    break;
  case ET_CORE:
    print_notification((unsigned char *)"Core file");
    break;
  case ET_LOPROC:
    print_warning((const unsigned char *)"Processor specific start");
    break;
  case ET_HIPROC:
    print_warning((const unsigned char *)"Processor specific end");
    break;
  default:
    print_warning((unsigned char *)"File type not defined !");
  }

  return;
}

/**
 * @brief Display the machine's file depending on the e_machine
 * ! EM_MIPS_RS4_BE seems to be undeclared in elf.h
 * @param e_machine
 */
static void display_machine(const Elf32_Half *e_machine) {
  print_notification((unsigned char *)"Machine:");

  switch (*e_machine) {
  case ET_NONE:
    print_warning((unsigned char *)"No machine");
    break;
  case EM_M32:
    print_notification((unsigned char *)"AT&T WE 32100");
    break;
  case EM_SPARC:
    print_notification((unsigned char *)"SPARC");
    break;
  case EM_386:
    print_notification((unsigned char *)"Intel Architecture");
    break;
  case EM_68K:
    print_notification((unsigned char *)"Motorola 68000");
    break;
  case EM_88K:
    print_notification((unsigned char *)"Motorola 88000");
    break;
  case EM_860:
    print_notification((unsigned char *)"Intel 80860");
    break;
  case EM_MIPS:
    print_notification((unsigned char *)"MIPS RS3000 Big-Endian");
    break;
  case EM_MIPS_RS4_BE:
    print_notification((unsigned char *)"MIPS RS4000 Big-Endian");
    break;
  case EM_ARM:
    print_notification((unsigned char *)"ARM");
    break;
  default:
    print_warning((unsigned char *)"Reserved for futur use");
  }

  return;
}

/**
 * @brief Display the version's file depending on the e_version
 *
 * @param e_version
 */
static void display_version(const Elf32_Word *e_version) {
  print_notification((unsigned char *)"Version:");

  switch (*e_version) {
  case EV_NONE:
    print_warning((unsigned char *)"Invalid version");
    break;
  case EV_CURRENT:
    print_notification((unsigned char *)"Current version");
  }
  return;
}

/**
 * @brief Display the entry's file depending on the e_entry
 *
 * @param e_entry
 */
static void display_entry(const Elf32_Addr *e_entry) {
  print_notification((unsigned char *)"Entry point:");
  if (*e_entry == 0) {
    print_warning((unsigned char *)"No entry point");
  } else {
    printf("%X\n", *e_entry);
  }

  return;
}

/**
 * @brief Display the phoff's file depending on the e_phoff
 *
 * @param e_phoff
 */
static void display_phoff(const Elf32_Off *e_phoff) {
  print_notification((unsigned char *)"Program header table's file offset");
  if (*e_phoff == 0) {
    print_warning((unsigned char *)"No offset");
  } else {
    printf("%X\n", *e_phoff);
  }

  return;
}

/**
 * @brief Display the shoff's file depending on the e_shoff
 *
 * @param e_shoff
 */
static void display_shoff(const Elf32_Off *e_shoff) {
  print_notification((unsigned char *)"Section header table's file offset");
  if (*e_shoff == 0) {
    print_warning((unsigned char *)"No offset");
  } else {
    printf("%d\n", *e_shoff);
  }
  return;
}

/**
 * @brief Display the flags' file depending on the flags
 *
 * @param flags
 */
static void display_flags(const Elf32_Word *flags) {
  print_notification((unsigned char *)"Flags:");
  printf("%X\n", *flags);
  return;
}

/**
 * @brief Display the size of ELF header's file depending on the ehsize
 *
 * @param ehsize
 */
static void display_ehsize(const Elf32_Half *ehsize) {
  print_notification((unsigned char *)"ELF header's size:");
  printf("%d\n", *ehsize);
  return;
}

/**
 * @brief Display the size of one entry from the file depending on the phentsize
 *
 * @param phentsize
 */
static void display_phentsize(const Elf32_Half *phentsize) {
  print_notification((unsigned char *)"Phentsize:");
  printf("%d\n", *phentsize);
  return;
}

/**
 * @brief Display the phnum's file depending on the phnum
 *
 * @param phnum
 */
static void display_phnum(const Elf32_Half *phnum) {
  print_notification((unsigned char *)"Phnum:");
  if (*phnum == 0) {
    print_warning((unsigned char *)"No program header table");
  } else {
    printf("%d\n", *phnum);
  }
  return;
}

/**
 * @brief Display the shentsize's file depending on the shentsize
 *
 * @param phnum
 */
static void display_shentsize(const Elf32_Half *shentsize) {
  print_notification((unsigned char *)"Shentsize:");
  printf("%d\n", *shentsize);
  return;
}

/**
 * @brief Display the shnum's file depending on the shnum
 *
 * @param shnum
 */
static void display_shnum(const Elf32_Half *shnum) {
  print_notification((unsigned char *)"Shnum:");
  if (*shnum == 0) {
    print_warning((unsigned char *)"No section header table");
  } else {
    printf("%d\n", *shnum);
  }
  return;
}

/**
 * @brief Display the shstrndx's file depending on the shstrndx
 *
 * @param shstrndx
 */
static void display_shstrndx(const Elf32_Half *shstrndx) {
  print_notification((unsigned char *)"Shstrndx:");
  if (*shstrndx == SHN_UNDEF) {
    print_warning((unsigned char *)"No section name string table");
  } else {
    printf("%d\n", *shstrndx);
  }
  return;
}