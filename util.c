/*
ELF Loader - chargeur/implanteur d'exécutables au format ELF à but pédagogique
Copyright (C) 2012 Guillaume Huard
Ce programme est libre, vous pouvez le redistribuer et/ou le modifier selon les
termes de la Licence Publique Générale GNU publiée par la Free Software
Foundation (version 2 ou bien toute autre version ultérieure choisie par vous).

Ce programme est distribué car potentiellement utile, mais SANS AUCUNE
GARANTIE, ni explicite ni implicite, y compris les garanties de
commercialisation ou d'adaptation dans un but spécifique. Reportez-vous à la
Licence Publique Générale GNU pour plus de détails.

Vous devez avoir reçu une copie de la Licence Publique Générale GNU en même
temps que ce programme ; si ce n'est pas le cas, écrivez à la Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
États-Unis.

Contact: Guillaume.Huard@imag.fr
         ENSIMAG - Laboratoire LIG
         51 avenue Jean Kuntzmann
         38330 Montbonnot Saint-Martin
*/
#include "util.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int is_host_big_endian() {
  static uint32_t one = 1;
  return ((*(uint8_t*)&one) == 0);
}

Elf32_Ehdr _SwapElf32_Ehdr(Elf32_Ehdr h) {
  h.e_type = byte_swap(h.e_type);
  h.e_machine = byte_swap(h.e_machine);
  h.e_version = byte_swap(h.e_version);
  h.e_entry = byte_swap(h.e_entry);
  h.e_phoff = byte_swap(h.e_phoff);
  h.e_shoff = byte_swap(h.e_shoff);
  h.e_flags = byte_swap(h.e_flags);
  h.e_ehsize = byte_swap(h.e_ehsize);
  h.e_phentsize = byte_swap(h.e_phentsize);
  h.e_phnum = byte_swap(h.e_phnum);
  h.e_shentsize = byte_swap(h.e_shentsize);
  h.e_shnum = byte_swap(h.e_shnum);
  h.e_shstrndx = byte_swap(h.e_shstrndx);
  return h;
}

Elf32_Shdr _SwapElf32_Shdr(Elf32_Shdr s) {
  s.sh_name = byte_swap(s.sh_name);
  s.sh_type = byte_swap(s.sh_type);
  s.sh_flags = byte_swap(s.sh_flags);
  s.sh_addr = byte_swap(s.sh_addr);
  s.sh_offset = byte_swap(s.sh_offset);
  s.sh_size = byte_swap(s.sh_size);
  s.sh_link = byte_swap(s.sh_link);
  s.sh_info = byte_swap(s.sh_info);
  s.sh_addralign = byte_swap(s.sh_addralign);
  s.sh_entsize = byte_swap(s.sh_entsize);
  return s;
}

StringBuilder StringBuilderNew() {
  return (StringBuilder){
      .data = calloc(1, sizeof(char)),
      .size = 1,
      .capacity = 1,
  };
}

uint32_t StringBuilderAppend(StringBuilder* builder, const char* str) {
  if (!str || *str == '\0') return 0;
  size_t len = strlen(str) + 1;  // +1 for the \0
  size_t required = builder->size + len;

  if (required > builder->capacity) {
    // ×1.5
    size_t new_capacity = required;

    char* next = realloc(builder->data, new_capacity);
    if (!next) return (uint32_t)-1;

    builder->data = next;
    builder->capacity = new_capacity;
  }

  uint32_t offset = (uint32_t)builder->size;
  memcpy(builder->data + offset, str, len);
  builder->size += len;

  return offset;
}

void StringBuilderDestroy(StringBuilder* builder) {
  if (builder->data) {
    free(builder->data);
  }
}