#include "../include/bit_operations.h"
#include "Unity/src/unity.h"
#include "Unity/src/unity_internals.h"
#include <stdint.h>

void test_swap_16_simple() {
  uint16_t big_endian_value = 0x809B;
  uint16_t expected_little_endian_value = 0x9B80;

  TEST_ASSERT_EQUAL_HEX16(expected_little_endian_value,
                          byte_swap(big_endian_value));
}

void test_swap_32_simple() {
  uint32_t big_endian_value = 0x11223344;
  uint32_t expected_little_endian_value = 0x44332211;

  TEST_ASSERT_EQUAL_HEX32(expected_little_endian_value,
                          byte_swap(big_endian_value));
}

void test_swap_64_simple() {
  uint64_t big_endian_value = 0x0102030405060708ULL;
  uint64_t expected_little_endian_value = 0x0807060504030201ULL;

  TEST_ASSERT_EQUAL_HEX64(expected_little_endian_value,
                          byte_swap(big_endian_value));
}

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_swap_16_simple);
  RUN_TEST(test_swap_32_simple);
  RUN_TEST(test_swap_64_simple);

  return UNITY_END();
}