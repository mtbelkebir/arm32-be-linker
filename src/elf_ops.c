#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "elf_ops.h"
#include "debug.h"
#include "util.h"

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

int display_elf_section_contents(const char *section_name, Elf32_File *f)
{
    if (!f)
        return 0;

    Elf32_Shdr *section = get_shdr_by_name(section_name, f);
    if (!section)
        return -1;
    printf("Content of section %s : \n", section_name);
    uint32_t section_size = section->sh_size;
    uint32_t section_offset = section->sh_offset;
    if (fseek(f->file, section_offset, SEEK_SET) != 0)
    {
        error("Unknown I/O error\n");
        return 0;
    }

    for (uint32_t j = 0; j < section_size; j += 4)
    {
        uint32_t word;
        if (fread(&word, 4, 1, f->file) != 1)
        {
            error("Unknown I/O error\n");
            return 0;
        }
        word = byte_swap(word);

        if (j % 4 == 0)
        {
            printf("%08x ", word);
        }
        else if (j % 4 == 3)
        {
            printf(" %08x\n", word);
        }
        else
        {
            printf(" %08x ", word);
        }
    }
    printf("\n");

    return 1;
}

Elf32_Shdr *get_shdr_by_nbr(uint32_t section_number, Elf32_File *f)
{
    if (f == NULL)
        return NULL;
    if (section_number >= f->e_ehdr.e_shnum || section_number < 0)
    {
        return NULL;
    }
    return &(f->e_shrdrs[section_number]);
}

Elf32_Shdr *get_shdr_by_name(const char *section_name, Elf32_File *f)
{
    uint16_t shnum = f->e_ehdr.e_shnum;
    uint16_t i = 0;
    while (i < shnum)
    {
        char *current_section_name = get_elf_section_name(&(f->e_shrdrs[i]), f);
        int cmp = strcmp(section_name, current_section_name);
        if (cmp == 0)
        {
            free(current_section_name);
            break;
        }
        free(current_section_name);
        i++;
    }

    if (i >= shnum)
    {
        printf("Section %s is not present in file\n", section_name);
        return NULL;
    }
    return &(f->e_shrdrs[i]);
}