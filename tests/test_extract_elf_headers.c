/**
 * @file test_extract_elf_headers.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-13
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "elf_ops.h"
#include <elf.h>
#include <stdio.h>

#include "Unity/src/unity.h"

void setUp() {}
void tearDown() {}

void test_Something() {
    TEST_ASSERT_TRUE(1);
}


int main(int argc, char *argv[]) {
    UNITY_BEGIN();
    RUN_TEST(test_Something);
    return 0;
}