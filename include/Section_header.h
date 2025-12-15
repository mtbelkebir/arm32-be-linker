

#ifndef __SECTION_HEADER__
#define __SECTION_HEADER__

#include <elf.h>
#include <stdint.h>
#include <stdio.h>


const char *section_type_names[] = {
    "SHT_NULL", 
    "SHT_PROGBITS", 
    "SHT_SYMTAB", 
    "SHT_STRTAB", 
    "SHT_RELA", 
    "SHT_HASH", 
    "SHT_DYNAMIC", 
    "SHT_NOTE", 
    "SHT_NOBITS", 
    "SHT_REL", 
    "SHT_SHLIB", 
    "SHT_DYNSYM",
    "Erreur(valeur non assinier)"
};

const char *section_flags_names[] = {
    "Erreur(valeur non autoriser)", 
    "SHT_WRITE", 
    "SHT_ALLOC", 
    "Erreur(valeur non autoriser)", 
    "SHT_EXECINSTR",
    "Erreur(valeur non autoriser)" 
};

Elf32_Shdr *extract_section_headers(FILE* f,uint16_t e_shentsize);

void affichage(Elf32_Shdr SH);
#endif






