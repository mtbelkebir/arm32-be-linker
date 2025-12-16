#include <stdio.h>
#include <stdlib.h>
#include "elf_ops.h"
#include "debug.h"
#include "extract_elf_header.h"
#include "display_elf_header.h"
#include "extract_elf_section.h"

Elf32_File *read_elf(const char *path)
{
    Elf32_File *f = malloc(sizeof(Elf32_File));
    if (!f)
    {
        error("Fatal allocation error\n");
        return NULL;
    }

    FILE *associated_file = fopen(path, "rb");
    if (!associated_file)
    {
        error("Error trying to read file %s\n, please check that it exists and has read permissions", path);
        free(f);
        return NULL;
    }

    f->file = associated_file;

    f->e_ehdr = extract_elf_informations(associated_file);
    f->e_shrdrs = extract_section_headers(associated_file,
                                          f->e_ehdr.e_shoff,
                                          f->e_ehdr.e_shnum,
                                          f->e_ehdr.e_shentsize,
                                          f->e_ehdr.e_ident);

    if (!f->e_shrdrs)
    {
        error("Failed to retrieve section headers for file %s\n", path);
        free(f);
        return NULL;
    }

    return f;
}

void free_elf_file(Elf32_File *elf_file)
{
    if (!elf_file)
        return;
    if (elf_file->file)
    {
        fclose(elf_file->file);
    }
    else
    {
        fprintf(stderr, "[WARNING] Tried to free already freed stream in elf_file\n");
    }
    free(elf_file);
}

void print_elf_header(Elf32_File *f)
{
    display_elf_headers(&(f->e_ehdr));
}

void print_section_table(Elf32_File *f)
{
}

char *get_elf_section_name(Elf32_Shdr *shdr, Elf32_File *file)
{
    if (!shdr || !file || !file->e_shrdrs)
        return NULL;

    uint16_t strtab_idx = file->e_ehdr.e_shstrndx;
    if (strtab_idx == SHN_UNDEF || strtab_idx >= file->e_ehdr.e_shnum)
    {
        return NULL;
    }

    Elf32_Shdr *strtab_shdr = &file->e_shrdrs[strtab_idx];
    char *name = malloc(4096); // TODO: Actual names are not limited
    if (fseek(file->file, strtab_shdr->sh_offset + shdr->sh_name, SEEK_SET) != 0)
    {
        free(name);
        return NULL;
    }
    if (!fgets(name, 4096, file->file))
    {
        free(name);
        return NULL;
    }
    return name;
}

void display_elf_sections(Elf32_File *f)
{
    if (!f)
        return;
    uint16_t shnum = f->e_ehdr.e_shnum;
    uint16_t i;
    printf("Section Headers:\n");
    printf("  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al\n");

    for (i = 0; i < shnum; ++i)
    {
        Elf32_Shdr *shdr = &f->e_shrdrs[i];
        char *name = get_elf_section_name(shdr, f);
        if (!name)
            name = "";

        // Get section type name
        const char *type_name;
        if (shdr->sh_type <= SHT_DYNSYM)
        {
            const char *type_names[] = {
                "NULL", "PROGBITS", "SYMTAB", "STRTAB", "RELA", "HASH",
                "DYNAMIC", "NOTE", "NOBITS", "REL", "SHLIB", "DYNSYM"};
            type_name = type_names[shdr->sh_type];
        }
        else
        {
            type_name = "UNKNOWN";
        }

        // Building of flag string
        char flags[8] = {0};
        int f_idx = 0;
        if (shdr->sh_flags & SHF_WRITE)
            flags[f_idx++] = 'W';
        if (shdr->sh_flags & SHF_ALLOC)
            flags[f_idx++] = 'A';
        if (shdr->sh_flags & SHF_EXECINSTR)
            flags[f_idx++] = 'X';
        if (shdr->sh_flags & SHF_MERGE)
            flags[f_idx++] = 'M';
        if (shdr->sh_flags & SHF_STRINGS)
            flags[f_idx++] = 'S';
        if (shdr->sh_flags & SHF_INFO_LINK)
            flags[f_idx++] = 'I';
        if (shdr->sh_flags & SHF_LINK_ORDER)
            flags[f_idx++] = 'L';
        flags[f_idx] = '\0';

        printf("  [%2u] %-17s %-15s %08x %06x %06x %2x %3s %2u %3u %2u\n",
               i,
               name,
               type_name,
               shdr->sh_addr,
               shdr->sh_offset,
               shdr->sh_size,
               shdr->sh_entsize,
               flags,
               shdr->sh_link,
               shdr->sh_info,
               shdr->sh_addralign);

        if (name && name[0] != '\0')
            free(name);
    }
}