#include "elf_ops.h"
#include "debug.h"
#include "extract_elf_header.h"
#include "Section_header.h"
#include <stdio.h>
#include "display_elf_header.h"
#include "Section_header.h"

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

    f->file = f;

    f->e_ehdr = extract_elf_informations(f);
    f->e_shrdrs = extract_section_headers(f, f->e_ehdr.e_shoff, f->e_ehdr.e_shnum);

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
    affichage(&(f->e_shrdrs), f->e_ehdr.e_shnum);
}
