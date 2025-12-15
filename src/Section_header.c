#include <stdbool.h>
#include <stdint.h>
#include <elf.h>
#include <stdio.h>
#include "Section_header.h"





Elf32_Shdr extract_section_headers(FILE* f,uint16_t e_shentsize){
    if(f==NULL){return NULL;}

    if (fseek(f,e_shentsize,SEEK_SET)!=0) { return NULL;}
    // le fichier est incompler

    Elf32_Shdr *SH=malloc(sizeof(Elf32_Shdr));
    if(SH==NULL){return NULL;}


    if (fread(SH, sizeof(Elf32_Shdr), 1, f)!=1) { // permet de mettre tout les info dans SH
        free(SH);
        return NULL;
    }

    return SH;
}
