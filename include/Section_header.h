

#ifndef __SECTION_HEADER__
#define __SECTION_HEADER__

#include <elf.h>
#include <stdint.h>
#include <stdio.h>


Elf32_Shdr *extract_section_headers(FILE* f,uint16_t e_shentsize);

void affichage(Elf32_Shdr SH);
#endif






