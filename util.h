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
#ifndef __UTIL_H__
#define __UTIL_H__
#include <elf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
int is_host_big_endian();
/*
#define reverse_2(x) ((((x)&0xFF)<<8)|(((x)>>8)&0xFF))
#define reverse_4(x) ((((x)&0xFF)<<24)|((((x)>>8)&0xFF)<<16)|\
        ((((x)>>16)&0xFF)<<8)|(((x)>>24)&0xFF))
*/
inline uint16_t reverse_2(uint16_t x) {
  return ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF));
}
inline uint32_t reverse_4(uint32_t x) {
  return ((((x) & 0xFF) << 24) | ((((x) >> 8) & 0xFF) << 16) |
          ((((x) >> 16) & 0xFF) << 8) | (((x) >> 24) & 0xFF));
}

/**
 * @brief Aligns addr with the next value that is multiple of align.
 */
inline uint32_t align_up(uint32_t addr, uint32_t align) {
  if (align <= 1) return addr;
  return (addr + align - 1) & ~(align - 1);
}
/**
 * Swaps the endianness of all fields of s. Should never be used directly
 * @see byte_swap
 */
Elf32_Shdr _SwapElf32_Shdr(Elf32_Shdr s);
/**
 * Swaps the endianness of all fields of h. Should never be used directly
 * @see byte_swap
 */
Elf32_Ehdr _SwapElf32_Ehdr(Elf32_Ehdr h);

#define byte_swap(x)               \
  _Generic((x),                    \
      uint16_t: reverse_2,         \
      uint32_t: reverse_4,         \
      int16_t: reverse_2,          \
      int32_t: reverse_4,          \
      Elf32_Shdr: _SwapElf32_Shdr, \
      Elf32_Ehdr: _SwapElf32_Ehdr)(x)
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

typedef struct StringBuilder {
  char* data;
  size_t size;
  size_t capacity;
} StringBuilder;
StringBuilder StringBuilderNew();
uint32_t StringBuilderAppend(StringBuilder* builder, const char* str);
void StringBuilderDestroy(StringBuilder* builder);
#endif
