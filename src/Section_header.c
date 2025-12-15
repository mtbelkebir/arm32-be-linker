#include <stdbool.h>
#include <stdint.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/Section_header.h"


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


Elf32_Shdr *extract_section_headers(FILE* f,uint16_t e_shoff){ // echoff est le décalage
    if(f==NULL){return NULL;}

    if (fseek(f,e_shoff,SEEK_SET)!=0) { return NULL;}
    // le fichier est incompler

    Elf32_Shdr *SH=malloc(sizeof(Elf32_Shdr));
    if(SH==NULL){return NULL;}


    if (fread(SH, sizeof(Elf32_Shdr), 1, f)!=1) { // permet de mettre tout les info dans SH
        free(SH);
        return NULL;
    }

    return SH;
}


void affichage(Elf32_Shdr SH){
    printf("sh_name:      0x%08X\n",(uint32_t)SH.sh_name);

    printf("sh_type:      0x%08X",(uint32_t)SH.sh_type);
    if ((int)SH.sh_type>=0 && 11>=(int)SH.sh_type){
        printf("          %s\n",section_type_names[(int)SH.sh_type]);
    }
    else{
        printf("          %s\n",section_type_names[12]);
    }
   
    printf("sh_flags:     0x%08X",(uint32_t)SH.sh_flags);
    if ((int)SH.sh_flags>=0 && 4>=(int)SH.sh_flags){
        printf("          %s\n",section_flags_names[(int)SH.sh_flags]);
    }
    else{
        printf("          %s\n",section_flags_names[5]);
    }
    
    printf("sh_addr:      0x%08X\n",(uint32_t)SH.sh_addr);

    printf("sh_size:      0x%08X\n",(uint32_t)SH.sh_size);
    printf("          %d\n",(int)SH.sh_size);

    printf("sh_link:      0x%08X\n",(uint32_t)SH.sh_link);

    printf("sh_info:      0x%08X\n",(uint32_t)SH.sh_info);

    printf("sh_addralign: 0x%08X\n",(uint32_t)SH.sh_addralign);

    printf("sh_entsize:   0x%08X",(uint32_t)SH.sh_entsize);
    printf("          %d\n",(int)SH.sh_entsize);
}