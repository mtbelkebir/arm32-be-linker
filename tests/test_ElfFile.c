#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ElfFile.h"
#include "unity.h"

#define TEST_FILES_DIR "test_files/"

static void create_test_file(const char* filename, const void* data,
                             size_t size) {
  char path[256];
  snprintf(path, sizeof(path), "%s%s", TEST_FILES_DIR, filename);

  FILE* f = fopen(path, "wb");
  TEST_ASSERT_NOT_NULL_MESSAGE(f, "Failed to create test file");
  size_t written = fwrite(data, 1, size, f);
  fclose(f);
  TEST_ASSERT_EQUAL_MESSAGE(size, written, "Failed to write data");
}

static char* get_path(const char* filename) {
  static char path[256];
  snprintf(path, sizeof(path), "%s%s", TEST_FILES_DIR, filename);
  return path;
}

void setUp(void) {
#ifdef _WIN32
  system("if not exist " TEST_FILES_DIR " mkdir " TEST_FILES_DIR);
#else
  system("mkdir -p " TEST_FILES_DIR);
#endif
}

void tearDown(void) {
}

void test_ElfFileNew_ValidElfFile_ReturnsSuccess(void) {
  uint8_t data[1024] = {0};
  data[0] = 0x7f;
  data[1] = 'E';
  data[2] = 'L';
  data[3] = 'F';
  data[EI_CLASS] = ELFCLASS32;
  data[EI_DATA] = ELFDATA2MSB;
  data[EI_VERSION] = EV_CURRENT;
  data[EI_OSABI] = ELFOSABI_SYSV;

  Elf32_Ehdr* h = (Elf32_Ehdr*)data;
  h->e_type = 0x0200;     // ET_EXEC BE
  h->e_machine = 0x2800;  // EM_ARM BE
  h->e_version = 0x01000000;
  h->e_ehsize = 0x3400;     // 52 bytes
  h->e_shentsize = 0x2800;  // 40 bytes

  create_test_file("valid.o", data, sizeof(data));

  ElfFile* elf = NULL;
  TEST_ASSERT_EQUAL(Success, ElfFileNew(get_path("valid.o"), &elf));
  TEST_ASSERT_NOT_NULL(elf);
  TEST_ASSERT_EQUAL(0x0028, elf->header.e_machine);

  ElfFileDestroy(elf);
}

void test_ElfFileNew_TruncatedFile_ReturnsFileTooShort(void) {
  uint8_t data[20] = {0x7f, 'E', 'L', 'F', ELFCLASS32, ELFDATA2MSB};
  create_test_file("trunc.o", data, sizeof(data));

  ElfFile* elf = NULL;
  TEST_ASSERT_EQUAL(FileTooShort, ElfFileNew(get_path("trunc.o"), &elf));
  TEST_ASSERT_NULL(elf);
}

void test_ElfFileNew_InvalidMagic_ReturnsNotAnElfFile(void) {
  uint8_t data[64] = {0x89, 'P', 'N', 'G'};
  create_test_file("notelf.o", data, sizeof(data));

  ElfFile* elf = NULL;
  TEST_ASSERT_EQUAL(NotAnElfFile, ElfFileNew(get_path("notelf.o"), &elf));
}

void test_ElfFileNew_InsufficientSections_DoesNotCrash(void) {
  uint8_t data[512] = {0};
  data[0] = 0x7f;
  data[1] = 'E';
  data[2] = 'L';
  data[3] = 'F';
  data[EI_CLASS] = ELFCLASS32;
  data[EI_DATA] = ELFDATA2MSB;

  Elf32_Ehdr* h = (Elf32_Ehdr*)data;
  h->e_shnum = 0x0500;      // 5 sections BE
  h->e_shoff = 0x34000000;  // Offset 52
  create_test_file("short_secs.o", data, sizeof(data));

  ElfFile* elf = NULL;
  TEST_ASSERT_NOT_EQUAL(Success, ElfFileNew(get_path("short_secs.o"), &elf));
}

void test_ElfFileNew_LittleEndian_ReturnsUnsupportedEndianness(void) {
  uint8_t data[64] = {0x7f, 'E', 'L', 'F', ELFCLASS32, ELFDATA2LSB};
  create_test_file("le.o", data, sizeof(data));

  ElfFile* elf = NULL;
  TEST_ASSERT_EQUAL(UnsupportedEndianness, ElfFileNew(get_path("le.o"), &elf));
}

void test_ElfFileNew_NonARMMachine_ParsesSuccessfully(void) {
  uint8_t data[512] = {0};
  data[0] = 0x7f;
  data[1] = 'E';
  data[2] = 'L';
  data[3] = 'F';
  data[EI_CLASS] = ELFCLASS32;
  data[EI_DATA] = ELFDATA2MSB;

  Elf32_Ehdr* h = (Elf32_Ehdr*)data;
  h->e_machine = 0x0300;  // x86 BE
  h->e_ehsize = 0x3400;

  create_test_file("x86.o", data, sizeof(data));

  ElfFile* elf = NULL;
  TEST_ASSERT_EQUAL(Success, ElfFileNew(get_path("x86.o"), &elf));
  TEST_ASSERT_EQUAL(0x0003, elf->header.e_machine);
  ElfFileDestroy(elf);
}

void test_ElfFileNew_SectionOffsetBeyondFile_DoesNotCrash(void) {
  uint8_t data[512] = {0x7f, 'E', 'L', 'F', ELFCLASS32, ELFDATA2MSB};
  Elf32_Ehdr* h = (Elf32_Ehdr*)data;
  h->e_shnum = 0x0100;
  h->e_shoff = 0x00100000;  // 1MB offset

  create_test_file("bad_off.o", data, sizeof(data));
  ElfFile* elf = NULL;
  TEST_ASSERT_NOT_EQUAL(Success, ElfFileNew(get_path("bad_off.o"), &elf));
}

void test_ElfFileNew_CorruptedHeader_DoesNotCrash(void) {
  uint8_t data[sizeof(Elf32_Ehdr)] = {0x7f, 'E',        'L',
                                      'F',  ELFCLASS32, ELFDATA2MSB};
  for (int i = EI_PAD; i < sizeof(Elf32_Ehdr); i++) data[i] = 0xAA;

  create_test_file("corrupt.o", data, sizeof(data));
  ElfFile* elf = NULL;
  ElfParsingStatus status = ElfFileNew(get_path("corrupt.o"), &elf);

  if (status == Success && elf) ElfFileDestroy(elf);
}

void test_ElfFileNew_NonExistentFile_ReturnsIoError(void) {
  ElfFile* elf = NULL;
  TEST_ASSERT_EQUAL(IoError, ElfFileNew(get_path("missing.o"), &elf));
}

void test_ElfFileNew_InvalidArguments(void) {
  ElfFile* elf = NULL;
  TEST_ASSERT_EQUAL(InvalidArguments, ElfFileNew(NULL, &elf));
  TEST_ASSERT_EQUAL(InvalidArguments, ElfFileNew("path.o", NULL));
}

void test_ElfFileNew_ELF64_ReturnsUnsupportedClass(void) {
  uint8_t data[64] = {0x7f, 'E', 'L', 'F', ELFCLASS64, ELFDATA2MSB};
  create_test_file("64.o", data, sizeof(data));
  ElfFile* file = NULL;
  TEST_ASSERT_EQUAL(UnsupportedClass, ElfFileNew(get_path("64.o"), &file));
  if (file) {
    ElfFileDestroy(file);
  }
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_ElfFileNew_ValidElfFile_ReturnsSuccess);
  RUN_TEST(test_ElfFileNew_TruncatedFile_ReturnsFileTooShort);
  RUN_TEST(test_ElfFileNew_InvalidMagic_ReturnsNotAnElfFile);
  RUN_TEST(test_ElfFileNew_InsufficientSections_DoesNotCrash);
  RUN_TEST(test_ElfFileNew_LittleEndian_ReturnsUnsupportedEndianness);
  RUN_TEST(test_ElfFileNew_NonARMMachine_ParsesSuccessfully);
  RUN_TEST(test_ElfFileNew_SectionOffsetBeyondFile_DoesNotCrash);
  RUN_TEST(test_ElfFileNew_CorruptedHeader_DoesNotCrash);
  RUN_TEST(test_ElfFileNew_NonExistentFile_ReturnsIoError);
  RUN_TEST(test_ElfFileNew_InvalidArguments);
  RUN_TEST(test_ElfFileNew_ELF64_ReturnsUnsupportedClass);
  return UNITY_END();
}