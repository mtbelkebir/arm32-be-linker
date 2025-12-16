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