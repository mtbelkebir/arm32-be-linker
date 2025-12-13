/**
 * @file bit_operations.h
 * @author DUC Corentin
 * @brief Header files for operations to bytes and bits
 * @version 0.1
 * @date 2025-12-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __BIT_OPERATIONS__
#define __BIT_OPERATIONS__

#include <stdbool.h>
#include <stdint.h>

uint16_t swap_16(uint16_t value);
uint32_t swap_32(uint32_t value);
uint64_t swap_64(uint64_t value);

#define byte_swap(x)                                                           \
  _Generic((x), uint16_t: swap_16, uint32_t: swap_32, uint64_t: swap_64)(x)

#endif //__BIT_OPERATIONS__