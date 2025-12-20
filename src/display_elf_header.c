/**
 * @file display_elf_headers.c
 * @author DUC Corentin
 * @brief Implements functions to display headers from ELF files
 * @version 0.2
 * @date 2025-12-14
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <elf.h>
#include <stdint.h>
#include <stdio.h>

// ! EM_MIPS_RS4_BE seems to be undeclared in elf.h (10 value)
#define EM_MIPS_RS4_BE 0xA

/**
 * @brief Display all the informations in the elf structure
 *
 * @pre Elf structure correctly initiated
 * @post Display all informations with translation if necessary
 *
 * @param elf
 */
void __display_elf_headers(const Elf32_Ehdr *elf)
{
  printf("ELF Header:\n");
  printf("  Magic:   ");
  for (int i = 0; i < EI_NIDENT; i++)
  {
    if (i >= 4 && i < 16)
      continue;
    printf("%02x ", elf->e_ident[i]);
  }
  printf("\n");

  printf("  %-34s ", "Class:");
  switch (elf->e_ident[EI_CLASS])
  {
  case ELFCLASSNONE:
    printf("Invalid class\n");
    break;
  case ELFCLASS32:
    printf("ELF32\n");
    break;
  case ELFCLASS64:
    printf("ELF64\n");
    break;
  default:
    printf("Unknown (%d)\n", elf->e_ident[EI_CLASS]);
  }

  printf("  %-34s ", "Data:");
  switch (elf->e_ident[EI_DATA])
  {
  case ELFDATANONE:
    printf("Invalid data encoding\n");
    break;
  case ELFDATA2LSB:
    printf("2's complement, little endian\n");
    break;
  case ELFDATA2MSB:
    printf("2's complement, big endian\n");
    break;
  default:
    printf("Unknown (%d)\n", elf->e_ident[EI_DATA]);
  }

  printf("  %-34s ", "Version:");
  switch (elf->e_ident[EI_VERSION])
  {
  case EV_NONE:
    printf("Invalid version\n");
    break;
  case EV_CURRENT:
    printf("1 (current)\n");
    break;
  default:
    printf("%d\n", elf->e_ident[EI_VERSION]);
  }

  printf("  %-34s ", "OS/ABI:");
  switch (elf->e_ident[EI_OSABI])
  {
  case ELFOSABI_SYSV:
    printf("UNIX - System V\n");
    break;
  case ELFOSABI_HPUX:
    printf("HP-UX\n");
    break;
  case ELFOSABI_NETBSD:
    printf("NetBSD\n");
    break;
  case ELFOSABI_LINUX:
    printf("Linux\n");
    break;
  case ELFOSABI_SOLARIS:
    printf("Sun Solaris\n");
    break;
  case ELFOSABI_AIX:
    printf("IBM AIX\n");
    break;
  case ELFOSABI_IRIX:
    printf("SGI Irix\n");
    break;
  case ELFOSABI_FREEBSD:
    printf("FreeBSD\n");
    break;
  case ELFOSABI_TRU64:
    printf("Compaq TRU64 UNIX\n");
    break;
  case ELFOSABI_MODESTO:
    printf("Novell Modesto\n");
    break;
  case ELFOSABI_OPENBSD:
    printf("OpenBSD\n");
    break;
  case ELFOSABI_ARM_AEABI:
    printf("ARM EABI\n");
    break;
  case ELFOSABI_ARM:
    printf("ARM\n");
    break;
  case ELFOSABI_STANDALONE:
    printf("Standalone (embedded) application\n");
    break;
  default:
    printf("<unknown: %d>\n", elf->e_ident[EI_OSABI]);
  }

  printf("  %-34s %d\n", "ABI Version:", elf->e_ident[EI_ABIVERSION]);

  printf("  %-34s ", "Type:");
  switch (elf->e_type)
  {
  case ET_NONE:
    printf("NONE (No file type)\n");
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
  case ET_LOPROC:
    printf("<processor specific>\n");
    break;
  case ET_HIPROC:
    printf("<processor specific>\n");
    break;
  default:
    if (elf->e_type >= ET_LOOS && elf->e_type <= ET_HIOS)
      printf("OS Specific: (%x)\n", elf->e_type);
    else if (elf->e_type >= ET_LOPROC && elf->e_type <= ET_HIPROC)
      printf("Processor Specific: (%x)\n", elf->e_type);
    else
      printf("Unknown (%x)\n", elf->e_type);
  }

  printf("  %-34s ", "Machine:");
  switch (elf->e_machine)
  {
  case EM_NONE:
    printf("No machine\n");
    break;
  case EM_M32:
    printf("AT&T WE 32100\n");
    break;
  case EM_SPARC:
    printf("SUN SPARC\n");
    break;
  case EM_386:
    printf("Intel 80386\n");
    break;
  case EM_68K:
    printf("Motorola m68k family\n");
    break;
  case EM_88K:
    printf("Motorola m88k family\n");
    break;
  case EM_860:
    printf("Intel 80860\n");
    break;
  case EM_MIPS:
    printf("MIPS R3000 big-endian\n");
    break;
  case EM_MIPS_RS4_BE:
    printf("MIPS R4000 big-endian\n");
    break;
  case EM_ARM:
    printf("ARM\n");
    break;
  case EM_X86_64:
    printf("Advanced Micro Devices X86-64\n");
    break;
  case EM_AARCH64:
    printf("ARM AARCH64\n");
    break;
  case EM_RISCV:
    printf("RISC-V\n");
    break;
  default:
    printf("Unknown machine (%d)\n", elf->e_machine);
  }

  printf("  %-34s ", "Version:");
  switch (elf->e_version)
  {
  case EV_NONE:
    printf("Invalid version\n");
    break;
  case EV_CURRENT:
    printf("0x1\n");
    break;
  default:
    printf("%#x\n", elf->e_version);
  }

  printf("  %-34s 0x%08x\n", "Entry point address:", elf->e_entry);
  printf("  %-34s %u (bytes into file)\n", "Start of program headers:", elf->e_phoff);
  printf("  %-34s %u (bytes into file)\n", "Start of section headers:", elf->e_shoff);
  printf("  %-34s 0x%08x\n", "Flags:", elf->e_flags);
  printf("  %-34s %u (bytes)\n", "Size of this header:", elf->e_ehsize);
  printf("  %-34s %u (bytes)\n", "Size of program headers:", elf->e_phentsize);
  printf("  %-34s %u\n", "Number of program headers:", elf->e_phnum);
  printf("  %-34s %u (bytes)\n", "Size of section headers:", elf->e_shentsize);
  printf("  %-34s %u\n", "Number of section headers:", elf->e_shnum);

  printf("  %-34s ", "Section header string table index:");
  if (elf->e_shstrndx == SHN_UNDEF)
  {
    printf("<none>\n");
  }
  else
  {
    printf("%u\n", elf->e_shstrndx);
  }

  if (elf->e_flags != 0 && elf->e_machine == EM_ARM)
  {
    printf("\n  ARM-specific flags:\n");

    uint32_t version_mask = (elf->e_flags & 0xFF000000) >> 24;
    if (version_mask)
    {
      printf("    Version5 ABI: %d\n", version_mask);
    }

    if (elf->e_flags & 0x00800000)
    {
      printf("    BE-8\n");
    }

    if (elf->e_flags & 0x00400000)
    {
      printf("    Legacy code\n");
    }

    uint32_t float_abi = elf->e_flags & 0x00000F00;
    switch (float_abi)
    {
    case 0x00000000:
      printf("    Float ABI: Soft float (base standard)\n");
      break;
    case 0x00000100:
      printf("    Float ABI: Soft float\n");
      break;
    case 0x00000200:
      printf("    Float ABI: Soft float (VFP)\n");
      break;
    case 0x00000300:
      printf("    Float ABI: Hard float (VFP)\n");
      break;
    case 0x00000400:
      printf("    Float ABI: Hard float\n");
      break;
    default:
      if (float_abi)
        printf("    Float ABI: Unknown (%#x)\n", float_abi);
    }
  }
}