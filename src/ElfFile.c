#include "ElfFile.h"

#include <elf.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
static ElfParsingStatus _ParseElfHeader(ElfFile* file);
static ElfParsingStatus _ParseElfSections(ElfFile* file);
static void ElfFreeSectionTable(ElfFile* elf);
static ElfParsingStatus _ParseElfSymbols(ElfFile* file);
static ElfParsingStatus _ParseElfRelocations(ElfFile* file);
static void _ElfFreeRelocationTables(ElfFile* elf);
static void _ElfFreeSymbolsTable(ElfFile* elf);

ElfParsingStatus ElfFileNew(const char* path, ElfFile** out) {
  if (!out || !path) {
    return InvalidArguments;
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    return IoError;
  }

  ElfFile* new_file = ElfFileNewEmpty();
  if (!new_file) {
    return MemoryError;
  }
  new_file->file_stream = f;
  ElfParsingStatus status = _ParseElfHeader(new_file);
  if (status != Success) {
    ElfFileDestroy(new_file);
    return status;
  }

  status = _ParseElfSections(new_file);
  if (status != Success) {
    ElfFileDestroy(new_file);
    return status;
  }

  status = _ParseElfSymbols(new_file);
  if (status != Success) {
    ElfFileDestroy(new_file);
    return status;
  }
  status = _ParseElfRelocations(new_file);
  if (status != Success) {
    ElfFileDestroy(new_file);
    return status;
  }
  *out = new_file;
  return Success;
}

ElfFile* ElfFileNewEmpty() {
  ElfFile* new = malloc(sizeof(ElfFile));
  if (new == NULL) {
    return NULL;
  }
  new->file_stream = NULL;
  new->rel_tables = NULL;
  new->rel_tables_count = 0;
  new->header = (Elf32_Ehdr){
      .e_ehsize = sizeof(Elf32_Ehdr),
      .e_flags = 0,
      .e_ident = {0x7f, 'E', 'L', 'F', ELFCLASS32, ELFDATA2MSB, EV_CURRENT, 0},
      .e_entry = 0,
      .e_machine = EM_ARM,
      .e_version = EV_CURRENT,
      .e_phoff = 0,
      .e_shoff = 0,
      .e_type = ET_REL,
      .e_shentsize = sizeof(Elf32_Shdr),
  };
  return new;
}

static ElfParsingStatus _ParseElfHeader(ElfFile* file) {
  if (!file) return InvalidArguments;
  uint8_t header_buffer[sizeof(Elf32_Ehdr)] = {0};

  rewind(file->file_stream);
  // File too short to anything anyway
  if (fread(header_buffer, 1, sizeof(Elf32_Ehdr), file->file_stream) <
      sizeof(Elf32_Ehdr)) {
    return FileTooShort;
  }

  const uint8_t expected_magic[4] = {0x7f, 0x45, 0x4c, 0x46};
  if (memcmp(header_buffer, expected_magic, sizeof(expected_magic)) != 0) {
    return NotAnElfFile;
  }

  // Would be checking that we don't have a 64 bits ELF file, or that the value
  // is unknown
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
  if (!is_host_big_endian()) {
    file->header = byte_swap(file->header);
  }

  // Header size is corrupted and the rest of the file cannot be trusted.
  if (file->header.e_ehsize != sizeof(Elf32_Ehdr)) {
    return NotAnElfFile;
  }
  return Success;
}
static ElfParsingStatus _ParseElfSections(ElfFile* file) {
  if (!file || file->header.e_shnum == 0) return Success;

  if (fseek(file->file_stream, file->header.e_shoff, SEEK_SET) != 0) {
    return IoError;
  }
  size_t section_table_size = file->header.e_shnum * sizeof(ElfSection);
  file->sections = calloc(file->header.e_shnum, sizeof(ElfSection));

  if (!file->sections) {
    return MemoryError;
  }

  memset(file->sections, 0, section_table_size);

  for (uint16_t i = 0; i < file->header.e_shnum; i++) {
    Elf32_Shdr shdr;
    if (fread(&shdr, sizeof(Elf32_Shdr), 1, file->file_stream) != 1) {
      return FileTooShort;
    }
    if (!is_host_big_endian()) {
      shdr = byte_swap(shdr);
    }
    file->sections[i].header = shdr;
  }
  // Copy the raw data of the section into memory

  for (int i = 0; i < file->header.e_shnum; i++) {
    ElfSection* section = &file->sections[i];
    // This section doesn't hold any data.
    if (section->header.sh_type == SHT_NOBITS) {
      continue;
    }
    if (fseek(file->file_stream, section->header.sh_offset, SEEK_SET) != 0) {
      return IoError;
    }
    section->data = malloc(section->header.sh_size);
    if (!section->data) {
      return IoError;
    }
    if (fread(section->data, 1, section->header.sh_size, file->file_stream) !=
        section->header.sh_size) {
      return IoError;
    }
  }

  // All sections are properly parsed, but their names are inexistant or
  // inaccessible. We can stop here.
  if (file->header.e_shstrndx == SHN_UNDEF ||
      file->header.e_shstrndx >= file->header.e_shnum) {
    return Success;
  }
  // Retrieval of all section names.
  ElfSection* strtab = &file->sections[file->header.e_shstrndx];
  char* string_table = (char*)strtab->data;

  for (uint16_t i = 0; i < file->header.e_shnum; i++) {
    uint32_t name_offset = file->sections[i].header.sh_name;
    if (name_offset < strtab->header.sh_size) {
      file->sections[i].name = strdup(string_table + name_offset);
    } else {
      file->sections[i].name = strdup("<corrupt>");
    }
  }
  return Success;
}
static ElfParsingStatus _ParseElfSymbols(ElfFile* file) {
  if (!file || !file->sections) return InvalidArguments;
  // File does not have sections, we don't have to do anything.
  if (!file->sections) return Success;

  ElfSection* sym_sec = NULL;
  for (int i = 0; i < file->header.e_shnum; i++) {
    if (file->sections[i].header.sh_type == SHT_SYMTAB) {
      sym_sec = &file->sections[i];
      break;
    }
  }

  // If no symbol table is present, we can just stop here.
  if (!sym_sec) return Success;

  // Prevents a crash should the elf file be corrupted.
  if (sym_sec->header.sh_entsize == 0) {
    return UnknownError;
  }
  file->symbols_table = malloc(sizeof(ElfSymbolsTable));
  if (!file->symbols_table) return MemoryError;

  uint32_t count = sym_sec->header.sh_size / sym_sec->header.sh_entsize;
  file->symbols_table->count = count;
  file->symbols_table->symbols = calloc(count, sizeof(ElfSymbol));
  if (!file->symbols_table->symbols) {
    return MemoryError;
  }

  if (fseek(file->file_stream, sym_sec->header.sh_offset, SEEK_SET) != 0) {
    return IoError;
  }

  for (uint32_t i = 0; i < count; i++) {
    Elf32_Sym sym;
    if (fread(&sym, sizeof(Elf32_Sym), 1, file->file_stream) != 1) {
      return FileTooShort;
    }
    if (!is_host_big_endian()) {
      sym.st_name = byte_swap(sym.st_name);
      sym.st_value = byte_swap(sym.st_value);
      sym.st_size = byte_swap(sym.st_size);
      sym.st_shndx = byte_swap(sym.st_shndx);
    }
    file->symbols_table->symbols[i].sym = sym;
  }

  // All symbols are properly parsed but their names are inexistant or
  // inaccessible. We can stop here.
  if (sym_sec->header.sh_link == SHN_UNDEF ||
      sym_sec->header.sh_link >= file->header.e_shnum) {
    return Success;
  }

  ElfSection* strtab = &file->sections[sym_sec->header.sh_link];
  char* string_table = (char*)strtab->data;
  for (uint32_t i = 0; i < file->symbols_table->count; i++) {
    uint32_t name_offset = file->symbols_table->symbols[i].sym.st_name;
    if (name_offset < strtab->header.sh_size) {
      file->symbols_table->symbols[i].name = strdup(string_table + name_offset);
    } else {
      file->symbols_table->symbols[i].name = strdup("<corrupt>");
    }
  }

  return Success;
}
static ElfParsingStatus _ParseElfRelocations(ElfFile* file) {
  if (!file || !file->sections) return InvalidArguments;
  if (!file->sections) return Success;

  uint32_t rel_section_count = 0;
  ElfSection** rel_sections =
      ElfFileGetSectionsByType(&rel_section_count, SHT_REL, file);

  if (rel_section_count == 0 || !rel_sections) {
    file->rel_tables_count = 0;
    free(rel_sections);
    return Success;
  }

  ElfRelocationTable* reloc_table =
      calloc(rel_section_count, sizeof(ElfRelocationTable));

  if (!reloc_table) {
    free(rel_sections);
    return MemoryError;
  }

  file->rel_tables = reloc_table;

  for (uint32_t i = 0; i < rel_section_count; i++) {
    file->rel_tables_count = i;
    reloc_table[i].name = strdup(rel_sections[i]->name);
    reloc_table[i].target_section = rel_sections[i]->header.sh_info;
    reloc_table[i].offset = rel_sections[i]->header.sh_offset;

    uint32_t entry_count =
        rel_sections[i]->header.sh_size / rel_sections[i]->header.sh_entsize;

    ElfRelocationEntry* entries =
        calloc(entry_count, sizeof(ElfRelocationEntry));
    if (!entries) {
      free(rel_sections);
      return MemoryError;
    }

    reloc_table[i].count = entry_count;
    reloc_table[i].entries = entries;

    if (fseek(file->file_stream, rel_sections[i]->header.sh_offset, SEEK_SET) !=
        0) {
      free(rel_sections);
      return IoError;
    }

    for (uint32_t j = 0; j < entry_count; j++) {
      if (fread(&entries[j].rel, sizeof(Elf32_Rel), 1, file->file_stream) !=
          1) {
        free(rel_sections);
        return IoError;
      }

      if (!is_host_big_endian()) {
        entries[j].rel.r_offset = byte_swap(entries[j].rel.r_offset);
        entries[j].rel.r_info = byte_swap(entries[j].rel.r_info);
      }

      uint32_t sym_idx = ELF32_R_SYM(entries[j].rel.r_info);
      if (file->symbols_table && sym_idx < file->symbols_table->count) {
        entries[j].symbol = &file->symbols_table->symbols[sym_idx];
      } else {
        entries[j].symbol = NULL;
      }
    }
  }

  file->rel_tables_count = rel_section_count;
  free(rel_sections);
  return Success;
}

void ElfFileDisplaySymbols(ElfFile* elf) {
  if (!elf || !elf->symbols_table) return;

  printf("\nSymbol table '.symtab' contains %d entries:\n",
         elf->symbols_table->count);
  printf("   Num:    Value  Size Type    Bind   Vis      Ndx Name\n");

  for (uint32_t i = 0; i < elf->symbols_table->count; i++) {
    ElfSymbol* s = &elf->symbols_table->symbols[i];
    Elf32_Sym* sym = &s->sym;

    printf("%6d: ", i);

    printf("%08x %5d ", sym->st_value, sym->st_size);

    const char* type_name = "NOTYPE";
    switch (ELF32_ST_TYPE(sym->st_info)) {
      case STT_OBJECT:
        type_name = "OBJECT";
        break;
      case STT_FUNC:
        type_name = "FUNC";
        break;
      case STT_SECTION:
        type_name = "SECTION";
        break;
      case STT_FILE:
        type_name = "FILE";
        break;
      case STT_COMMON:
        type_name = "COMMON";
        break;
      case STT_TLS:
        type_name = "TLS";
        break;
    }
    printf("%-7s ", type_name);

    const char* bind_name = "LOCAL";
    switch (ELF32_ST_BIND(sym->st_info)) {
      case STB_GLOBAL:
        bind_name = "GLOBAL";
        break;
      case STB_WEAK:
        bind_name = "WEAK";
        break;
    }
    printf("%-6s ", bind_name);

    printf("DEFAULT  ");

    // Ndx (Section Index)
    if (sym->st_shndx == SHN_UNDEF) {
      printf("UND ");
    } else if (sym->st_shndx == SHN_ABS) {
      printf("ABS ");
    } else if (sym->st_shndx == SHN_COMMON) {
      printf("COM ");
    } else {
      printf("%3d ", sym->st_shndx);
    }

    // Name
    printf("%s\n", s->name ? s->name : "");
  }
}

void ElfFileDestroy(ElfFile* elf) {
  if (!elf) return;
  if (elf->file_stream) fclose(elf->file_stream);
  ElfFreeSectionTable(elf);
  _ElfFreeSymbolsTable(elf);
  _ElfFreeRelocationTables(elf);
  free(elf);
}
static void ElfFreeSectionTable(ElfFile* elf) {
  if (!elf->sections) return;
  for (int i = 0; i < elf->header.e_shnum; i++) {
    if (elf->sections[i].name) {
      free(elf->sections[i].name);
      elf->sections[i].name = NULL;
    }
    if (elf->sections[i].data) {
      free(elf->sections[i].data);
    }
    elf->sections[i].data = NULL;
  }
  free(elf->sections);
  elf->sections = NULL;
}
static void _ElfFreeRelocationTables(ElfFile* elf) {
  if (!elf || !elf->rel_tables) return;
  for (int i = 0; i < elf->rel_tables_count; i++) {
    if (elf->rel_tables[i].name) free(elf->rel_tables[i].name);
    if (elf->rel_tables[i].entries) free(elf->rel_tables[i].entries);
  }
  free(elf->rel_tables);
  elf->rel_tables = NULL;
  elf->rel_tables_count = 0;
}
static void _ElfFreeSymbolsTable(ElfFile* elf) {
  if (!elf->symbols_table) return;
  if (elf->symbols_table->symbols) {
    for (uint32_t i = 0; i < elf->symbols_table->count; i++) {
      if (elf->symbols_table->symbols[i].name) {
        free(elf->symbols_table->symbols[i].name);
      }
    }
    free(elf->symbols_table->symbols);
  }
  free(elf->symbols_table);
  elf->symbols_table = NULL;
}
void ElfFileDisplayHeader(ElfFile* elf) {
  if (!elf) return;

  Elf32_Ehdr* h = &elf->header;

  printf("ELF Header:\n");
  printf("  Magic:   ");
  for (int i = 0; i < EI_NIDENT; i++) {
    printf("%02x ", h->e_ident[i]);
  }
  printf("\n");

  printf("  Class:                             %s\n",
         h->e_ident[EI_CLASS] == ELFCLASS32
             ? "ELF32"
             : (h->e_ident[EI_CLASS] == ELFCLASS64 ? "ELF64" : "None"));

  printf("  Data:                              %s\n",
         h->e_ident[EI_DATA] == ELFDATA2MSB
             ? "2's complement, big endian"
             : (h->e_ident[EI_DATA] == ELFDATA2LSB
                    ? "2's complement, little endian"
                    : "None"));

  printf("  Version:                           %d%s\n", h->e_ident[EI_VERSION],
         h->e_ident[EI_VERSION] == EV_CURRENT ? " (current)" : "");

  printf("  OS/ABI:                            ");
  switch (h->e_ident[EI_OSABI]) {
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
      printf("Solaris\n");
      break;
    case ELFOSABI_ARM:
      printf("ARM\n");
      break;
    case ELFOSABI_STANDALONE:
      printf("Standalone App\n");
      break;
    default:
      printf("<unknown: %x>\n", h->e_ident[EI_OSABI]);
      break;
  }

  printf("  ABI Version:                       %d\n",
         h->e_ident[EI_ABIVERSION]);

  printf("  Type:                              ");
  switch (h->e_type) {
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
      printf("<unknown: %x>\n", h->e_type);
      break;
  }

  printf("  Machine:                           ");
  switch (h->e_machine) {
    case EM_NONE:
      printf("None\n");
      break;
    case EM_ARM:
      printf("ARM\n");
      break;
    case EM_X86_64:
      printf("Advanced Micro Devices X86-64\n");
      break;
    case EM_386:
      printf("Intel 80386\n");
      break;
    default:
      printf("<unknown: %d>\n", h->e_machine);
      break;
  }

  printf("  Version:                           0x%x\n", h->e_version);
  printf("  Entry point address:               0x%x\n", h->e_entry);
  printf("  Start of program headers:          %d (bytes into file)\n",
         h->e_phoff);
  printf("  Start of section headers:          %d (bytes into file)\n",
         h->e_shoff);

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

const char* ElfParsingStatusToString(ElfParsingStatus status) {
  switch (status) {
    case Success:
      return "Success";
    case FileTooShort:
      return "File too short";
    case IoError:
      return "I/O error (could not open or read file)";
    case NotAnElfFile:
      return "Not a valid ELF file (magic mismatch)";
    case UnsupportedMachineType:
      return "Unsupported machine type";
    case UnsupportedEndianness:
      return "Unsupported endianness (only Big Endian is supported)";
    case UnknownError:
      return "Unknown error";
    case UnknownDataEncoding:
      return "Unknown data encoding";
    case InvalidClass:
      return "Invalid ELF class";
    case UnsupportedClass:
      return "Unsupported ELF class (only 32-bit is supported)";
    case UnknownClass:
      return "Unknown ELF class";
    case InvalidDataEncoding:
      return "Invalid data encoding";
    case MemoryError:
      return "Memory allocation failed";
    case InvalidArguments:
      return "Invalid function arguments";
    default:
      return "Undefined error status";
  }
}

void ElfFileDisplaySections(ElfFile* elf) {
  if (!elf || !elf->sections) return;

  printf("There are %d section headers, starting at offset 0x%x:\n\n",
         elf->header.e_shnum, elf->header.e_shoff);

  printf("Section Headers:\n");
  printf(
      "  [Nr] Name              Type            Addr     Off    Size   ES "
      "Flg "
      "Lk Inf Al\n");

  for (int i = 0; i < elf->header.e_shnum; i++) {
    ElfSection* s = &elf->sections[i];
    Elf32_Shdr* h = &s->header;

    printf("  [%2d] %-17.17s ", i, s->name ? s->name : "");

    const char* type_name = "UNKNOWN";
    switch (h->sh_type) {
      case SHT_NULL:
        type_name = "NULL";
        break;
      case SHT_PROGBITS:
        type_name = "PROGBITS";
        break;
      case SHT_SYMTAB:
        type_name = "SYMTAB";
        break;
      case SHT_STRTAB:
        type_name = "STRTAB";
        break;
      case SHT_RELA:
        type_name = "RELA";
        break;
      case SHT_HASH:
        type_name = "HASH";
        break;
      case SHT_DYNAMIC:
        type_name = "DYNAMIC";
        break;
      case SHT_NOTE:
        type_name = "NOTE";
        break;
      case SHT_NOBITS:
        type_name = "NOBITS";
        break;
      case SHT_REL:
        type_name = "REL";
        break;
      case SHT_SHLIB:
        type_name = "SHLIB";
        break;
      case SHT_DYNSYM:
        type_name = "DYNSYM";
        break;
      case SHT_INIT_ARRAY:
        type_name = "INIT_ARRAY";
        break;
      case SHT_FINI_ARRAY:
        type_name = "FINI_ARRAY";
        break;
      case SHT_ARM_ATTRIBUTES:
        type_name = "ARM_ATTRIBUTES";
        break;
    }
    printf("%-15s ", type_name);

    printf("%08x %06x %06x %02x ", h->sh_addr, h->sh_offset, h->sh_size,
           h->sh_entsize);

    // Comprehensive Flags Decoding
    char flags_buf[12] = {0};
    int f_idx = 0;
    if (h->sh_flags & SHF_WRITE) flags_buf[f_idx++] = 'W';
    if (h->sh_flags & SHF_ALLOC) flags_buf[f_idx++] = 'A';
    if (h->sh_flags & SHF_EXECINSTR) flags_buf[f_idx++] = 'X';
    if (h->sh_flags & SHF_MERGE) flags_buf[f_idx++] = 'M';
    if (h->sh_flags & SHF_STRINGS) flags_buf[f_idx++] = 'S';
    if (h->sh_flags & SHF_INFO_LINK) flags_buf[f_idx++] = 'I';
    if (h->sh_flags & SHF_LINK_ORDER) flags_buf[f_idx++] = 'L';
    if (h->sh_flags & SHF_OS_NONCONFORMING) flags_buf[f_idx++] = 'O';
    if (h->sh_flags & SHF_GROUP) flags_buf[f_idx++] = 'G';
    if (h->sh_flags & SHF_TLS) flags_buf[f_idx++] = 'T';
    if (h->sh_flags & SHF_EXCLUDE) flags_buf[f_idx++] = 'E';

    printf("%-3s ", flags_buf);

    printf("%2u %3u %2u\n", h->sh_link, h->sh_info, h->sh_addralign);
  }

  printf(
      "Key to Flags:\n"
      "  W (write), A (alloc), X (execute), M (merge), S (strings), I "
      "(info),\n"
      "  L (link order), O (extra OS processing required), G (group), T "
      "(TLS),\n"
      "  E (exclude), D (mbind), x (unknown), o (OS specific), p (processor "
      "specific)\n");
}

ElfSection* ElfFileGetSectionByName(const char* name, ElfFile* elf) {
  if (!name || !elf) return NULL;

  for (int i = 0; i < elf->header.e_shnum; i++) {
    if (strcmp(elf->sections[i].name, name) == 0) {
      return &elf->sections[i];
    }
  }
  return NULL;
}
ElfSection** ElfFileGetSectionsByType(uint32_t* out_section_count,
                                      const uint32_t type, ElfFile* elf) {
  if (!elf || !elf->sections || !out_section_count) return NULL;
  ElfSection** sections = calloc(elf->header.e_shnum, sizeof(ElfSection*));
  uint32_t count = 0;
  for (uint32_t i = 0; i < elf->header.e_shnum; i++) {
    if (elf->sections[i].header.sh_type == type) {
      sections[count++] = &elf->sections[i];
    }
  }
  *out_section_count = count;
  return sections;
}
int ElfFileDisplaySectionContentsByName(const char* name, ElfFile* elf) {
  if (!name || !elf) return -1;
  ElfSection* section = ElfFileGetSectionByName(name, elf);
  if (!section) {
    printf("No such section %s found. Nothing to display.\n", name);
    return 1;
  }

  if (fseek(elf->file_stream, section->header.sh_offset, SEEK_SET) != 0) {
    return 0;
  }
  uint8_t* section_contents = section->data;

  // We're printing the raw contents of the section,
  // so there's no need to manage endianness.
  for (int i = 0; i < section->header.sh_size; i++) {
    printf("%02x%c", section_contents[i], ((i + 1) % 16 == 0) ? '\n' : ' ');
  }
  return 1;
}

void ElfFileDisplayRelocations(ElfFile* elf) {
  if (!elf || !elf->rel_tables || elf->rel_tables_count == 0) {
    printf("\nThere are no relocations in this file.\n");
    return;
  }

  for (uint16_t i = 0; i < elf->rel_tables_count; i++) {
    ElfRelocationTable* table = &elf->rel_tables[i];
    printf("\nRelocation section '%s' at offset 0x%x contains %d entries:\n",
           table->name, table->offset, table->count);
    printf(" Offset     Info    Type            Sym.Value  Sym. Name\n");

    for (uint32_t j = 0; j < table->count; j++) {
      ElfRelocationEntry* entry = &table->entries[j];
      uint32_t type = ELF32_R_TYPE(entry->rel.r_info);

      printf("%08x  %08x ", entry->rel.r_offset, entry->rel.r_info);

      const char* type_name = "UNKNOWN";
      switch (type) {
        case 0:
          type_name = "R_ARM_NONE";
          break;
        case 2:
          type_name = "R_ARM_ABS32";
          break;
        case 3:
          type_name = "R_ARM_REL32";
          break;
        case 28:
          type_name = "R_ARM_CALL";
          break;
        case 29:
          type_name = "R_ARM_JUMP24";
          break;
        case 40:
          type_name = "R_ARM_V4BX";
          break;
        case 43:
          type_name = "R_ARM_MOVW_ABS_NC";
          break;
        case 44:
          type_name = "R_ARM_MOVT_ABS";
          break;
      }
      printf("%-15s ", type_name);

      if (entry->symbol) {
        printf("%08x   %s", entry->symbol->sym.st_value,
               entry->symbol->name ? entry->symbol->name : "");
      } else {
        uint32_t sym_idx = ELF32_R_SYM(entry->rel.r_info);
        printf("%08x   [Index %d]", 0, sym_idx);
      }

      printf("\n");
    }
  }
}

ElfParsingStatus ElfFileWriteToDisk(const char* path, ElfFile* f) {
  if (!path || !f) {
    return InvalidArguments;
  }

  FILE* file = fopen(path, "wb");
  if (!file) {
    return IoError;
  }

  const char zero = 0;

  /* * Regrettably, software design is also about sacrifices.
   * * Two options are available from here.
   * - In-place endianness swapping, which would ruin the Elf File struct.
   * - Copying the ElfFile struct, which would consume way too much memory.
   * * Instead, the compromise is to copy the headers, byte swap them and write
   * them to disk. It makes the code less readable, but that's better than the
   * aforementioned options.
   */

  // Writing of file header.
  Elf32_Ehdr ehdr = f->header;  // We keep this in Host format for calculations
  size_t current_offset = 0;

  // Writing back to big endian (for the first write)
  Elf32_Ehdr header_to_disk = is_host_big_endian() ? ehdr : byte_swap(ehdr);

  if ((current_offset += fwrite(&header_to_disk, 1, sizeof(Elf32_Ehdr), file)) <
      sizeof(Elf32_Ehdr)) {
    fclose(file);
    return IoError;
  }

  // Writing of section contents
  for (int i = 0; i < f->header.e_shnum; i++) {
    const uint32_t alignment = f->sections[i].header.sh_addralign;
    size_t required_padding =
        (alignment > 1) ? (alignment - (current_offset % alignment)) % alignment
                        : 0;

    // Apply padding
    if (required_padding > 0) {
      fwrite(&zero, 1, required_padding, file);
      current_offset += required_padding;
    }

    // New data offset in the file.
    f->sections[i].header.sh_offset = (uint32_t)current_offset;

    current_offset +=
        fwrite(f->sections[i].data, 1, f->sections[i].header.sh_size, file);
  }

  // Writing of section headers.
  // Align the section header table to 4 bytes
  const char section_header_table_alignment = 4;
  const uint32_t section_header_required_padding =
      (section_header_table_alignment -
       current_offset % section_header_table_alignment) %
      section_header_table_alignment;

  if (section_header_required_padding > 0) {
    fwrite(&zero, 1, section_header_required_padding, file);
    current_offset += section_header_required_padding;
  }

  // We have to update this (Host format first)
  ehdr.e_shoff = (uint32_t)current_offset;

  for (int i = 0; i < f->header.e_shnum; i++) {
    Elf32_Shdr shdr = f->sections[i].header;
    if (!is_host_big_endian()) {
      shdr = byte_swap(shdr);
    }
    fwrite(&shdr, 1, sizeof(Elf32_Shdr), file);
  }

  // Since we update e_shnum, we have to go back and patch it.
  rewind(file);
  header_to_disk = is_host_big_endian() ? ehdr : byte_swap(ehdr);
  fwrite(&header_to_disk, 1, sizeof(Elf32_Ehdr), file);

  fclose(file);
  return Success;
}