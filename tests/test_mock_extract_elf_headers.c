/**
 * @file test_extract_elf_headers.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/Mockextract_elf_header.h"
#include "../tests/Unity/src/unity.h"
#include <elf.h>

void setUp(void) {}
void tearDown(void) {}

void test_maFonctionQuiAppelleElfExtract_Success(void) {
  Elf32_Ehdr expected_header_info = {0};
  expected_header_info.e_type = 0x0002;                
  expected_header_info.e_ident[EI_CLASS] = ELFCLASS32;

  extract_elf_informations_ExpectAndReturn(
      (char *)"filename_ignore",
      &expected_header_info,     
      true
  );

  extract_elf_informations_IgnoreArg_filename()
  extract_elf_informations_SetArgumentPointee_header_informations(
      &expected_header_info, sizeof(Elf32_Ehdr));

  

  // 4. Vérifier les résultats
  // (Vérifiez ici que ma_fonction_a_tester a bien utilisé les données mockées)
}