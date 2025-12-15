

#ifndef __SECTION_HEADER__
#define __SECTION_HEADER__

#include <elf.h>
#include <stdint.h>
#include <stdio.h>


Elf32_Shdr *extract_section_headers(FILE* f,uint32_t e_shoff , uint32_t e_phnum);

void affichage(Elf32_Shdr *SH,uint32_t e_phnum);
#endif






