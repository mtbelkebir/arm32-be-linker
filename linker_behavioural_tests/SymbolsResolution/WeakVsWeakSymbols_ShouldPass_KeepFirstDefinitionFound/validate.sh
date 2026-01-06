#!/bin/bash
$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1

ADDR=$(arm-none-eabi-nm out.o | grep -w "Weak" | awk '{print $1}')

if [ "$((16#$ADDR))" -ne 0 ]; then
    exit 0
else
    echo "Error: Linked to f1 (addr 0) instead of f2."
    exit 1
fi