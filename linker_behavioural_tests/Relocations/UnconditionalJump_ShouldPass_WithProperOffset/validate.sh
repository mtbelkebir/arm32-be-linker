#!/bin/bash


$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1


TARGET_ADDR=$(arm-none-eabi-nm out.o | grep -w "Target" | awk '{print $1}')
MAIN_ADDR=$(arm-none-eabi-nm out.o | grep -w "main" | awk '{print $1}')

# Look for the `b` instruction
DISASM=$(arm-none-eabi-objdump -d out.o | grep -A 5 "<main>:")

if echo "$DISASM" | grep -q "b.*$TARGET_ADDR"; then
    exit 0
else
    echo "Error: Unconditional jump in 'main' does not point to Target ($TARGET_ADDR)"
    echo "$DISASM"
    exit 1
fi