#include <stdbool.h>
#include <stdint.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include "Section_header.h"
//#include "../include/Section_header.h"


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


Elf32_Shdr *extract_section_headers(FILE* f,uint32_t e_shoff , uint32_t e_phnum){ // echoff est le décalage e_phnum pour le nombrre de section total et 
    if(f==NULL){return NULL;}

    if (fseek(f,e_shoff,SEEK_SET)!=0) { return NULL;}
    // le fichier est incompler

    Elf32_Shdr *SH=malloc(e_phnum*sizeof(Elf32_Shdr));
    if(SH==NULL){return NULL;}

    for (int i=0;i<e_phnum; i++){
        if (fread(&SH[i], sizeof(Elf32_Shdr), 1, f)!=1) { // permet de mettre tout les info dans SH
            for (int i=0;i<e_phnum; i++){
                free(&SH[i]);
            }
            free(SH);
            return NULL;
        }
    }
    return SH;
}
void affichage(Elf32_Shdr *SH, uint32_t e_shnum) {
    

    printf("Idx | sh_name  | sh_type    | sh_flags | sh_addr  | sh_offset| sh_size  | sh_link| sh_info| sh_addralign | sh_entsize\n");
    printf("----|----------|------------|----------|----------|----------|----------|--------|--------|--------------|----------\n");

    for (int i = 0; i < (int)e_shnum; i++) {

        const Elf32_Shdr *current_sh = &SH[i];
        
        printf("[%d]", i); 

   
        printf(" 0x%08X", current_sh->sh_name);
        

        const char *type_name;
        if (current_sh->sh_type >= 0 && current_sh->sh_type <= 11) {
            type_name = section_type_names[current_sh->sh_type];
        } else {
            type_name = section_type_names[12]; // Type inconnu
        }
        printf(" | %-10s", type_name);
        
        
        const char *type_flag;
        if (current_sh->sh_flags >= 0 && current_sh->sh_flags <= 4) {
            type_flag = section_flags_names[current_sh->sh_flags];
        } else {
            type_flag = section_flags_names[5]; // Type inconnu
        }

        printf(" | %-10s", type_flag);


        printf(" | 0x%08X", current_sh->sh_addr);
        

        printf(" | 0x%08X", current_sh->sh_offset);

        printf(" | 0x%08X", current_sh->sh_size);
        

        printf(" | 0x%08X", current_sh->sh_link);

        printf(" | 0x%08X", current_sh->sh_info);


        printf(" | 0x%08X", current_sh->sh_addralign);
        

        printf(" | 0x%08X", current_sh->sh_entsize);

        printf("\n");
    }
}
