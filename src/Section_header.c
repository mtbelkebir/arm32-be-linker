#include <stdbool.h>
#include <stdint.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "Section_header.h"
#include "util.h"
// #include "../include/Section_header.h"

const char *section_type_names[] = {
    "NULL",
    "PROGBITS",
    "SYMTAB",
    "STRTAB",
    "RELA",
    "HASH",
    "DYNAMIC",
    "NOTE",
    "NOBITS",
    "REL",
    "SHLIB",
    "DYNSYM",
    "Erreur(valeur non assinier)"};

const char *section_flags_names[] = {
    "Erreur(valeur non autoriser)",
    "W", // Write
    "A", // Alloc
    "Erreur(valeur non autoriser)",
    "X", // Execute
    "Erreur(valeur non autoriser)"};

/**
 * @brief Extracts the section header table from file f
 *
 * @param f
 * @param e_shoff Offset of the Section Header Table from the beginning of the file
 * @param e_shnum Number of entries
 * @return An array of section headers, NULL in case of error
 */
Elf32_Shdr *extract_section_headers(FILE *f, uint32_t e_shoff, uint32_t e_shnum)
{
    if (f == NULL)
    {
        return NULL;
    }

    if (fseek(f, e_shoff, SEEK_SET) != 0)
    {
        return NULL;
    }
    // le fichier est incompler

    Elf32_Shdr *SH = malloc(e_shnum * sizeof(Elf32_Shdr));
    if (SH == NULL)
    {
        return NULL;
    }

    uint32_t read = 0;
    if ((read = fread(SH, sizeof(Elf32_Shdr), e_shnum, f)) != e_shnum)
    {
        error("Mismatch of entries read in section header. %d expected, %d found", e_shnum, read);
        free(SH);
        return NULL;
    }
    // Reverse Endianness from big to little for use in our machines.
    for (uint32_t i = 0; i < e_shnum; i++)
    {
        SH[i].sh_name = byte_swap(SH[i].sh_name);
        SH[i].sh_type = byte_swap(SH[i].sh_type);
        SH[i].sh_flags = byte_swap(SH[i].sh_flags);
        SH[i].sh_addr = byte_swap(SH[i].sh_addr);
        SH[i].sh_offset = byte_swap(SH[i].sh_offset);
        SH[i].sh_size = byte_swap(SH[i].sh_size);
        SH[i].sh_link = byte_swap(SH[i].sh_link);
        SH[i].sh_info = byte_swap(SH[i].sh_info);
        SH[i].sh_addralign = byte_swap(SH[i].sh_addralign);
        SH[i].sh_entsize = byte_swap(SH[i].sh_entsize);
    }
    return SH;
}
void affichage(Elf32_Shdr *SH, uint32_t e_shnum)
{

    printf("Idx | Name  | Type | Flg | Addr | Off | Size  | Lk | Inf | Al | sh_entsize\n");
    printf("----|----------|------------|----------|----------|----------|----------|--------|--------|--------------|----------\n");

    for (int i = 0; i < e_shnum; i++)
    {

        const Elf32_Shdr current_sh = SH[i];

        printf("[%d]", i);

        printf(" 0x%08X", current_sh.sh_name);

        const char *type_name;
        if (current_sh.sh_type >= 0 && current_sh.sh_type <= 11)
        {
            type_name = section_type_names[current_sh.sh_type];
        }
        else
        {
            type_name = section_type_names[12]; // Type inconnu
        }
        printf(" | %-10s", type_name);
        // Printing flags
        char type_flag[5];
        int i = 0;
        int flags = current_sh.sh_flags;
        if (flags & SHF_WRITE)
        {
            type_flag[i++] = 'W';
        }
        if (flags & SHF_ALLOC)
            type_flag[i++] = 'A';
        if (flags & SHF_EXECINSTR)
            type_flag[i++] = 'X';
        if (flags & SHF_MASKPROC)
            type_flag[i++] = 'M';
        type_flag[i] = '\0';
        // End Printing Flags
        printf(" | %-10s", type_flag);

        printf(" | 0x%08X", current_sh.sh_addr);

        printf(" | 0x%08X", current_sh.sh_offset);

        printf(" | 0x%08X", current_sh.sh_size);

        printf(" | 0x%08X", current_sh.sh_link);

        printf(" | 0x%08X", current_sh.sh_info);

        printf(" | 0x%08X", current_sh.sh_addralign);

        printf(" | 0x%08X", current_sh.sh_entsize);

        printf("\n");
    }
}
