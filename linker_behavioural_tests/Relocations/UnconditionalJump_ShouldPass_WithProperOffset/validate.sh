#!/bin/bash

$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1

TARGET_ADDR=$(arm-none-eabi-nm out.o | grep -w "Target" | awk '{print $1}' | sed 's/^0*//')
if [ -z "$TARGET_ADDR" ]; then TARGET_ADDR="0"; fi

DISASM=$(arm-none-eabi-objdump -d out.o | grep -A 20 "<main>:")

TARGET_IN_DISASM=$(echo "$DISASM" | grep "bl" | awk '{print $(NF-1)}' | sed 's/^0*//')

if [ "$TARGET_ADDR" == "$TARGET_IN_DISASM" ]; then
    exit 0
else
    echo "Error: Unconditional jump (bl) in 'main' does not point to Target (0x$TARGET_ADDR)"
    echo "Found in disassembly: 0x$TARGET_IN_DISASM"
    echo "$DISASM"
    exit 1
fi