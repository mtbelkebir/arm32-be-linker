#include "elf_ops.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <elf.h>
static ElfParsingStatus _ParseElfHeader(ElfFile *file);
static ElfParsingStatus _ParseElfSections(ElfFile* file);
static void ElfFreeSectionTable(ElfFile* elf);

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
  new_file->sections = NULL;
  new_file->sym = NULL;

  ElfParsingStatus status = _ParseElfHeader(new_file);
  if (status != Success) {
    fclose(f);
    free(new_file);
    return status;
  }

  status = _ParseElfSections(new_file);
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

  memcpy(&file->header, header_buffer, sizeof(Elf32_Ehdr));

  // Reverse endianness of all values in host is little endian
  if (!is_big_endian()) {
    file->header.e_type = byte_swap(file->header.e_type);
    file->header.e_machine = byte_swap(file->header.e_machine);
    file->header.e_version = byte_swap(file->header.e_version);
    file->header.e_entry = byte_swap(file->header.e_entry);
    file->header.e_phoff = byte_swap(file->header.e_phoff);
    file->header.e_shoff = byte_swap(file->header.e_shoff);
    file->header.e_flags = byte_swap(file->header.e_flags);
    file->header.e_ehsize = byte_swap(file->header.e_ehsize);
    file->header.e_phentsize = byte_swap(file->header.e_phentsize);
    file->header.e_phnum = byte_swap(file->header.e_phnum);
    file->header.e_shentsize = byte_swap(file->header.e_shentsize);
    file->header.e_shnum = byte_swap(file->header.e_shnum);
    file->header.e_shstrndx = byte_swap(file->header.e_shstrndx);
  }
  return Success;
}
static ElfParsingStatus _ParseElfSections(ElfFile* file) {
  if (!file || file->header.e_shnum == 0) return Success;

  if (fseek(file->file, file->header.e_shoff, SEEK_SET) != 0) {
    return IoError;
  }

  size_t section_table_size = file->header.e_shnum * sizeof(ElfSection);
  file->sections = malloc(section_table_size);

  if (!file->sections) {
    return MemoryError;
  }

  memset(file->sections, 0, section_table_size);

  for (uint16_t i = 0; i < file->header.e_shnum; i++) {
    Elf32_Shdr shdr;
    if (fread(&shdr, sizeof(Elf32_Shdr), 1, file->file) != 1) {
      return FileTooShort;
    }
    if (!is_big_endian()) {
      shdr.sh_name      = byte_swap(shdr.sh_name);
      shdr.sh_type      = byte_swap(shdr.sh_type);
      shdr.sh_flags     = byte_swap(shdr.sh_flags);
      shdr.sh_addr      = byte_swap(shdr.sh_addr);
      shdr.sh_offset    = byte_swap(shdr.sh_offset);
      shdr.sh_size      = byte_swap(shdr.sh_size);
      shdr.sh_link      = byte_swap(shdr.sh_link);
      shdr.sh_info      = byte_swap(shdr.sh_info);
      shdr.sh_addralign = byte_swap(shdr.sh_addralign);
      shdr.sh_entsize   = byte_swap(shdr.sh_entsize);
    }
    file->sections[i].header = shdr;
  }
  // All sections are properly parsed but their names are inexistant or inaccessible. We can stop here.
  if (file->header.e_shstrndx == SHN_UNDEF || file->header.e_shstrndx >= file->header.e_shnum) {
    return Success;
  }

  Elf32_Shdr strtab_shdr = file->sections[file->header.e_shstrndx].header;
  /* What's coming is going to be some pretty dark magic.
   * Indeed, since we can't know in advance the size of a section's name, and we don't want to keep using `fgetc`
   * and realloc (that's kind of slow and heavy on the drive), we'll just load the entire string table at once,
   * and from it use `strdup` to get the full name safely.
   */

  char* string_table = malloc(strtab_shdr.sh_size);
  if (!string_table) {
    return MemoryError;
  }

  if (fseek(file->file, strtab_shdr.sh_offset, SEEK_SET) != 0) {
    free(string_table);
    return IoError;
  }

  if (fread(string_table, strtab_shdr.sh_size, 1, file->file) != 1) {
    free(string_table);
    return IoError;
  }

  for (uint16_t i = 0; i < file->header.e_shnum; i++) {
    uint32_t name_offset = file->sections[i].header.sh_name;
    if (name_offset < strtab_shdr.sh_size) {
      file->sections[i].name = strdup(string_table + name_offset);
    } else {
      file->sections[i].name = strdup("<corrupt>");
    }
  }
  free(string_table);
  return Success;
}

void ElfFileDestroy(ElfFile *elf) {
  if (!elf) return;
  if (elf->file) fclose(elf->file);
  ElfFreeSectionTable(elf);
  free(elf);
}
static void ElfFreeSectionTable(ElfFile* elf) {
  if (!elf->sections) return;
  for (int i = 0; i < elf->header.e_shnum; i++) {
    if (elf->sections[i].name) {
      free(elf->sections[i].name);
    }
  }
  free(elf->sections);
}

void ElfFileDisplayHeader(ElfFile *elf) {
  if (!elf) return;

  Elf32_Ehdr *h = &elf->header;

  printf("ELF Header:\n");
  printf("  Magic:   ");
  for (int i = 0; i < EI_NIDENT; i++) {
    printf("%02x ", h->e_ident[i]);
  }
  printf("\n");

  printf("  Class:                             %s\n",
         h->e_ident[EI_CLASS] == ELFCLASS32 ? "ELF32" :
         (h->e_ident[EI_CLASS] == ELFCLASS64 ? "ELF64" : "None"));

  printf("  Data:                              %s\n",
         h->e_ident[EI_DATA] == ELFDATA2MSB ? "2's complement, big endian" :
         (h->e_ident[EI_DATA] == ELFDATA2LSB ? "2's complement, little endian" : "None"));

  printf("  Version:                           %d%s\n",
         h->e_ident[EI_VERSION], h->e_ident[EI_VERSION] == EV_CURRENT ? " (current)" : "");

  printf("  OS/ABI:                            ");
  switch (h->e_ident[EI_OSABI]) {
    case ELFOSABI_SYSV:       printf("UNIX - System V\n"); break;
    case ELFOSABI_HPUX:       printf("HP-UX\n"); break;
    case ELFOSABI_NETBSD:     printf("NetBSD\n"); break;
    case ELFOSABI_LINUX:      printf("Linux\n"); break;
    case ELFOSABI_SOLARIS:    printf("Solaris\n"); break;
    case ELFOSABI_ARM:        printf("ARM\n"); break;
    case ELFOSABI_STANDALONE: printf("Standalone App\n"); break;
    default:                  printf("<unknown: %x>\n", h->e_ident[EI_OSABI]); break;
  }

  printf("  ABI Version:                       %d\n", h->e_ident[EI_ABIVERSION]);

  printf("  Type:                              ");
  switch (h->e_type) {
    case ET_NONE: printf("NONE (None)\n"); break;
    case ET_REL:  printf("REL (Relocatable file)\n"); break;
    case ET_EXEC: printf("EXEC (Executable file)\n"); break;
    case ET_DYN:  printf("DYN (Shared object file)\n"); break;
    case ET_CORE: printf("CORE (Core file)\n"); break;
    default:      printf("<unknown: %x>\n", h->e_type); break;
  }

  printf("  Machine:                           ");
  switch (h->e_machine) {
    case EM_NONE:  printf("None\n"); break;
    case EM_ARM:   printf("ARM\n"); break;
    case EM_X86_64:printf("Advanced Micro Devices X86-64\n"); break;
    case EM_386:   printf("Intel 80386\n"); break;
    default:       printf("<unknown: %d>\n", h->e_machine); break;
  }

  printf("  Version:                           0x%x\n", h->e_version);
  printf("  Entry point address:               0x%x\n", h->e_entry);
  printf("  Start of program headers:          %d (bytes into file)\n", h->e_phoff);
  printf("  Start of section headers:          %d (bytes into file)\n", h->e_shoff);

  printf("  Flags:                             0x%x", h->e_flags);
  if (h->e_machine == EM_ARM) {
    // Decode common ARM flags
    unsigned int eabi = EF_ARM_EABI_VERSION(h->e_flags);
    if (eabi != 0) printf(", Version%d EABI", eabi >> 24);
    if (h->e_flags & EF_ARM_RELEXEC) printf(", RELEXEC");
    if (h->e_flags & EF_ARM_HASENTRY) printf(", HASENTRY");
  }
  printf("\n");

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


void ElfFileDisplaySections(ElfFile *elf) {
  if (!elf || !elf->sections) return;

  printf("There are %d section headers, starting at offset 0x%x:\n\n",
         elf->header.e_shnum, elf->header.e_shoff);

  printf("Section Headers:\n");
  printf("  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al\n");

  for (int i = 0; i < elf->header.e_shnum; i++) {
    ElfSection *s = &elf->sections[i];
    Elf32_Shdr *h = &s->header;

    printf("  [%2d] %-17.17s ", i, s->name ? s->name : "");

    const char *type_name = "UNKNOWN";
    switch (h->sh_type) {
      case SHT_NULL:     type_name = "NULL";     break;
      case SHT_PROGBITS: type_name = "PROGBITS"; break;
      case SHT_SYMTAB:   type_name = "SYMTAB";   break;
      case SHT_STRTAB:   type_name = "STRTAB";   break;
      case SHT_RELA:     type_name = "RELA";     break;
      case SHT_HASH:     type_name = "HASH";     break;
      case SHT_DYNAMIC:  type_name = "DYNAMIC";  break;
      case SHT_NOTE:     type_name = "NOTE";     break;
      case SHT_NOBITS:   type_name = "NOBITS";   break;
      case SHT_REL:      type_name = "REL";      break;
      case SHT_SHLIB:    type_name = "SHLIB";    break;
      case SHT_DYNSYM:   type_name = "DYNSYM";   break;
      case SHT_INIT_ARRAY: type_name = "INIT_ARRAY"; break;
      case SHT_FINI_ARRAY: type_name = "FINI_ARRAY"; break;
      case SHT_ARM_ATTRIBUTES: type_name = "ARM_ATTRIBUTES"; break;
    }
    printf("%-15s ", type_name);

    printf("%08x %06x %06x %02x ",
           h->sh_addr, h->sh_offset, h->sh_size, h->sh_entsize);

    // Comprehensive Flags Decoding
    char flags_buf[12] = {0};
    int f_idx = 0;
    if (h->sh_flags & SHF_WRITE)            flags_buf[f_idx++] = 'W';
    if (h->sh_flags & SHF_ALLOC)            flags_buf[f_idx++] = 'A';
    if (h->sh_flags & SHF_EXECINSTR)        flags_buf[f_idx++] = 'X';
    if (h->sh_flags & SHF_MERGE)            flags_buf[f_idx++] = 'M';
    if (h->sh_flags & SHF_STRINGS)          flags_buf[f_idx++] = 'S';
    if (h->sh_flags & SHF_INFO_LINK)        flags_buf[f_idx++] = 'I';
    if (h->sh_flags & SHF_LINK_ORDER)       flags_buf[f_idx++] = 'L';
    if (h->sh_flags & SHF_OS_NONCONFORMING) flags_buf[f_idx++] = 'O';
    if (h->sh_flags & SHF_GROUP)            flags_buf[f_idx++] = 'G';
    if (h->sh_flags & SHF_TLS)              flags_buf[f_idx++] = 'T';
    if (h->sh_flags & SHF_EXCLUDE)          flags_buf[f_idx++] = 'E';

    printf("%-3s ", flags_buf);

    printf("%2u %3u %2u\n", h->sh_link, h->sh_info, h->sh_addralign);
  }

  printf("Key to Flags:\n"
         "  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),\n"
         "  L (link order), O (extra OS processing required), G (group), T (TLS),\n"
         "  E (exclude), D (mbind), x (unknown), o (OS specific), p (processor specific)\n");
}