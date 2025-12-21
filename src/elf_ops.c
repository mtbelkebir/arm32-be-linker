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
    case ELFDATA2LSB:
      break;
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


