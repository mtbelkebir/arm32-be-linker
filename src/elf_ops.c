#include "elf_ops.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
static ElfParsingStatus _ParseElfHeader(ElfFile *file);

ElfParsingStatus ElfFileNew(const char *path, ElfFile **out) {
  if (!out || !path) {
    return InvalidArguments;
  }

  FILE *f = fopen(path, "rb");
  if (!f) {
    return IoError;
  }

  ElfFile *new_file = (ElfFile *)malloc(sizeof(ElfFile));
  if (!new_file) {
    fclose(f);
    return MemoryError;
  }

  // Initialize members
  new_file->file = f;
  new_file->e_shrdrs = NULL;
  new_file->sym = NULL;

  ElfParsingStatus status = _ParseElfHeader(new_file);
  if (status != Success) {
    fclose(f);
    free(new_file);
    return status;
  }

  *out = new_file;
  return Success;
}

static ElfParsingStatus _ParseElfHeader(ElfFile *file) {
  if (!file) return InvalidArguments;
  uint8_t header_buffer[sizeof(Elf32_Ehdr)] = {0};

  rewind(file->file);
  // File too short to anything anyway
  if (fread(header_buffer, 1, sizeof(Elf32_Ehdr), file->file) < sizeof(Elf32_Ehdr)) {

      return FileTooShort;
  }

  const uint8_t expected_magic[4] = {0x7f, 0x45, 0x4c, 0x46};
  if (memcmp(header_buffer, expected_magic, sizeof(expected_magic)) != 0) {
    return NotAnElfFile;
  }

  // Would be checking that we don't have a 64 bits ELF file, or that the value is unknown
  switch (header_buffer[EI_CLASS]) {
    case ELFCLASSNONE:
      return InvalidClass;
    case ELFCLASS32:
      // This is a 32 bits file, so we can continue.
      break;
    case ELFCLASS64:
      return UnsupportedClass;
    default:
      return UnknownClass;
  }
  // Verifies that the passed in file is actually Big Endian
  switch (header_buffer[EI_DATA]) {
    case ELFDATANONE:
      return InvalidDataEncoding;
    case ELFDATA2MSB:
      break;
    case ELFDATA2LSB:
      return UnsupportedEndianness;
    default:
      return UnknownDataEncoding;
  }

  memcpy(&file->e_ehdr, header_buffer, sizeof(Elf32_Ehdr));

  // Reverse endianness of all values in host is little endian
  if (!is_big_endian()) {
    file->e_ehdr.e_type = byte_swap(file->e_ehdr.e_type);
    file->e_ehdr.e_machine = byte_swap(file->e_ehdr.e_machine);
    file->e_ehdr.e_version = byte_swap(file->e_ehdr.e_version);
    file->e_ehdr.e_entry = byte_swap(file->e_ehdr.e_entry);
    file->e_ehdr.e_phoff = byte_swap(file->e_ehdr.e_phoff);
    file->e_ehdr.e_shoff = byte_swap(file->e_ehdr.e_shoff);
    file->e_ehdr.e_flags = byte_swap(file->e_ehdr.e_flags);
    file->e_ehdr.e_ehsize = byte_swap(file->e_ehdr.e_ehsize);
    file->e_ehdr.e_phentsize = byte_swap(file->e_ehdr.e_phentsize);
    file->e_ehdr.e_phnum = byte_swap(file->e_ehdr.e_phnum);
    file->e_ehdr.e_shentsize = byte_swap(file->e_ehdr.e_shentsize);
    file->e_ehdr.e_shnum = byte_swap(file->e_ehdr.e_shnum);
    file->e_ehdr.e_shstrndx = byte_swap(file->e_ehdr.e_shstrndx);
  }
  return Success;
}


void ElfFileDisplayHeader(ElfFile *elf) {
  if (!elf) {
    return;
  }

  Elf32_Ehdr *h = &elf->e_ehdr;

  printf("ELF Header:\n");
  printf("  Magic:   ");
  for (int i = 0; i < EI_NIDENT; i++) {
    printf("%02x ", h->e_ident[i]);
  }
  printf("\n");

  printf("  Class:                             %s\n",
         h->e_ident[EI_CLASS] == ELFCLASS32 ? "ELF32" : "ELF64");
  printf("  Data:                              %s\n",
         h->e_ident[EI_DATA] == ELFDATA2MSB ? "2's complement, big endian" : "2's complement, little endian");
  printf("  Version:                           %d (current)\n", h->e_ident[EI_VERSION]);
  printf("  OS/ABI:                            UNIX - System V\n"); // Simplified
  printf("  ABI Version:                       %d\n", h->e_ident[EI_ABIVERSION]);
  printf("  Type:                              ");
  switch (h->e_type) {
    case ET_REL:  printf("REL (Relocatable file)\n"); break;
    case ET_EXEC: printf("EXEC (Executable file)\n"); break;
    case ET_DYN:  printf("DYN (Shared object file)\n"); break;
    default:      printf("Unknown\n"); break;
  }
  printf("  Machine:                           %d\n", h->e_machine);
  printf("  Version:                           0x%x\n", h->e_version);
  printf("  Entry point address:               0x%x\n", h->e_entry);
  printf("  Start of program headers:          %d (bytes into file)\n", h->e_phoff);
  printf("  Start of section headers:          %d (bytes into file)\n", h->e_shoff);
  printf("  Flags:                             0x%x\n", h->e_flags);
  printf("  Size of this header:               %d (bytes)\n", h->e_ehsize);
  printf("  Size of program headers:           %d (bytes)\n", h->e_phentsize);
  printf("  Number of program headers:         %d\n", h->e_phnum);
  printf("  Size of section headers:           %d (bytes)\n", h->e_shentsize);
  printf("  Number of section headers:         %d\n", h->e_shnum);
  printf("  Section header string table index: %d\n", h->e_shstrndx);
}

const char *ElfParsingStatusToString(ElfParsingStatus status) {
  switch (status) {
    case Success:                return "Success";
    case FileTooShort:           return "File too short";
    case IoError:                return "I/O error (could not open or read file)";
    case NotAnElfFile:           return "Not a valid ELF file (magic mismatch)";
    case UnsupportedMachineType: return "Unsupported machine type";
    case UnsupportedEndianness:  return "Unsupported endianness (only Big Endian is supported)";
    case UnknownError:           return "Unknown error";
    case UnknownDataEncoding:    return "Unknown data encoding";
    case InvalidClass:           return "Invalid ELF class";
    case UnsupportedClass:       return "Unsupported ELF class (only 32-bit is supported)";
    case UnknownClass:           return "Unknown ELF class";
    case InvalidDataEncoding:    return "Invalid data encoding";
    case MemoryError:            return "Memory allocation failed";
    case InvalidArguments:       return "Invalid function arguments";
    default:                     return "Undefined error status";
  }
}