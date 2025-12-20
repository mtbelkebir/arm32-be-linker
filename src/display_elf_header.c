/**
 * @file display_elf_headers.c
 * @author DUC Corentin
 * @brief Implements functions to display headers from ELF files (readelf style)
 */

#include "../include/elf_ops.h"
#include "../include/logger.h"
#include <elf.h>
#include <stdint.h>
#include <stdio.h>

#define EM_MIPS_RS4_BE 0xA

// Prototypes statiques
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
  print_notification((unsigned char *)"ELF Header:");
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
}

static void display_ident(const unsigned char e_ident[]) {
  printf("  Magic:   ");
  for (int i = 0; i < EI_NIDENT; i++) {
    printf("%02x ", e_ident[i]);
  }
  printf("\n");

  printf("  %-34s ", "Class:");
  switch (e_ident[EI_CLASS]) {
  case ELFCLASSNONE:
    printf("none\n");
    break;
  case ELFCLASS32:
    printf("ELF32\n");
    break;
  case ELFCLASS64:
    printf("ELF64\n");
    break;
  default:
    printf("<unknown: %x>\n", e_ident[EI_CLASS]);
    break;
  }

  printf("  %-34s ", "Data:");
  switch (e_ident[EI_DATA]) {
  case ELFDATANONE:
    printf("none\n");
    break;
  case ELFDATA2LSB:
    printf("2's complement, little endian\n");
    break;
  case ELFDATA2MSB:
    printf("2's complement, big endian\n");
    break;
  default:
    printf("<unknown: %x>\n", e_ident[EI_DATA]);
    break;
  }

  printf("  %-34s %d %s\n", "Version:", e_ident[EI_VERSION],
         (e_ident[EI_VERSION] == EV_CURRENT) ? "(current)" : "");

  printf("  %-34s ", "OS/ABI:");
  if (e_ident[EI_OSABI] == ELFOSABI_ARM_AEABI)
    printf("ARM EABI\n");
  else
    printf("UNIX - System V\n");

  printf("  %-34s %d\n", "ABI Version:", e_ident[EI_ABIVERSION]);
}

static void display_type(const Elf32_Half *e_type) {
  printf("  %-34s ", "Type:");
  switch (*e_type) {
  case ET_NONE:
    printf("NONE (None)\n");
    break;
  case ET_REL:
    printf("REL (Relocatable file)\n");
    break;
  case ET_EXEC:
    printf("EXEC (Executable file)\n");
    break;
  case ET_DYN:
    printf("DYN (Shared object file)\n");
    break;
  case ET_CORE:
    printf("CORE (Core file)\n");
    break;
  default:
    printf("<unknown>: %x\n", *e_type);
    break;
  }
}

static void display_machine(const Elf32_Half *e_machine) {
  printf("  %-34s ", "Machine:");
  switch (*e_machine) {
  case EM_M32:
    printf("AT&T WE 32100\n");
    break;
  case EM_SPARC:
    printf("Sparc\n");
    break;
  case EM_386:
    printf("Intel 80386\n");
    break;
  case EM_68K:
    printf("Motorola 68000\n");
    break;
  case EM_88K:
    printf("Motorola 88000\n");
    break;
  case EM_860:
    printf("Intel 80860\n");
    break;
  case EM_MIPS:
    printf("MIPS R3000\n");
    break;
  case EM_MIPS_RS4_BE:
    printf("MIPS R4000\n");
    break;
  case EM_ARM:
    printf("ARM\n");
    break;
  default:
    printf("Unknown machine (%d)\n", *e_machine);
    break;
  }
}

static void display_version(const Elf32_Word *e_version) {
  printf("  %-34s 0x%x\n", "Version:", *e_version);
}

static void display_entry(const Elf32_Addr *e_entry) {
  printf("  %-34s 0x%x\n", "Entry point address:", *e_entry);
}

static void display_phoff(const Elf32_Off *e_phoff) {
  printf("  %-34s %d (bytes into file)\n",
         "Start of program headers:", *e_phoff);
}

static void display_shoff(const Elf32_Off *e_shoff) {
  printf("  %-34s %d (bytes into file)\n",
         "Start of section headers:", *e_shoff);
}

static void display_flags(const Elf32_Word *flags) {
  printf("  %-34s 0x%x", "Flags:", *flags);

  // Affichage spécifique ARM si applicable
  if (*flags & 0xFF000000)
    printf(", Version%d EABI", (*flags >> 24));
  if ((*flags & 0x00F00000) == 0x00800000)
    printf(", BE8");
  if ((*flags & 0x00000F00) == 0x00000400)
    printf(", hard-float ABI");
  if ((*flags & 0x00000F00) == 0x00000200)
    printf(", soft-float ABI");
  printf("\n");
}

static void display_ehsize(const Elf32_Half *ehsize) {
  printf("  %-34s %d (bytes)\n", "Size of this header:", *ehsize);
}

static void display_phentsize(const Elf32_Half *phentsize) {
  printf("  %-34s %d (bytes)\n", "Size of program headers:", *phentsize);
}

static void display_phnum(const Elf32_Half *phnum) {
  printf("  %-34s %d\n", "Number of program headers:", *phnum);
}

static void display_shentsize(const Elf32_Half *shentsize) {
  printf("  %-34s %d (bytes)\n", "Size of section headers:", *shentsize);
}

static void display_shnum(const Elf32_Half *shnum) {
  printf("  %-34s %d\n", "Number of section headers:", *shnum);
}

static void display_shstrndx(const Elf32_Half *shstrndx) {
  printf("  %-34s %d\n", "Section header string table index:", *shstrndx);
}