/**
 * @file bit_operations.c
 * @author DUC Corentin
 * @brief implementation of some bits operations
 * @version 0.1
 * @date 2025-12-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/bit_operations.h"
#include <stdint.h>

/**
 * @brief Swap a 16 bits value
 *
 * @param value
 * @return uint16_t
 */
uint16_t swap_16(uint16_t value) {

  uint16_t new_msb = (value & (0x00FF)) << 8;
  uint16_t new_lsb = (value & (0xFF00)) >> 8;
  return new_msb | new_lsb;
}

/**
 * @brief Swap a 32 bits value
 *
 * @param value
 * @return uint32_t
 */
uint32_t swap_32(uint32_t value) {
  uint32_t new_msb = (value & 0x000000FF) << 24;
  uint32_t new_lsb = (value & 0xFF000000) >> 24;
  uint32_t new_left_middle = (value & 0x0000FF00) << 8;
  uint32_t new_right_middle = (value & 0x00FF0000) >> 8;
  return new_msb | new_left_middle | new_right_middle | new_lsb;
}
/**
 * @brief Swap a 64 bits value
 *
 * @param value
 * @return uint64_t
 */
uint64_t swap_64(uint64_t value) {
  uint64_t new_msb = (value & 0x00000000000000FFULL) << 56;
  uint64_t new_lsb = (value & 0xFF00000000000000ULL) >> 56;
  uint64_t new_first_part = (value & 0x000000000000FF00ULL) << 40;
  uint64_t new_last_part = (value & 0x00FF000000000000ULL) >> 40;
  uint64_t new_left_middle = (value & 0x0000000000FF0000ULL) << 24;
  uint64_t new_right_middle = (value & 0x0000FF0000000000ULL) >> 24;
  uint64_t new_left_center = (value & 0x00000000FF000000ULL) << 8;
  uint64_t new_right_center = (value & 0x000000FF00000000ULL) >> 8;
  return new_msb | new_first_part | new_left_middle | new_left_center |
         new_right_center | new_right_middle | new_last_part | new_lsb;
}

/**
 * @brief Return true if the host is in big endian. False else
 *
 * @return true
 * @return false
 */
bool is_host_big_endian(void) {
  uint32_t value = 0x00000001;
  unsigned char *byte_ptr = (unsigned char *)&value;
  if (*byte_ptr == 0x01) {
    return false;
  } else {
    return true;
  }
}
