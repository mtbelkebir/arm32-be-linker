#!/bin/bash

$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1

SECRET_ADDR=$(arm-none-eabi-nm out.o | grep -w "getSecret" | awk '{print $1}' | sed 's/^0*//')
if [ -z "$SECRET_ADDR" ]; then SECRET_ADDR="0"; fi

DISASM=$(arm-none-eabi-objdump -d out.o | grep -A 10 "<main>:")
TARGET_IN_DISASM=$(echo "$DISASM" | grep "bl" | awk '{print $(NF-1)}' | sed 's/^0*//')

if [ "$SECRET_ADDR" == "$TARGET_IN_DISASM" ]; then
    exit 0
else
    echo "Relocation Error: 'main' does not call 'getSecret' at expected address 0x$SECRET_ADDR"
    echo "Found target in disassembly: 0x$TARGET_IN_DISASM"
    echo "Disassembly snippet:"
    echo "$DISASM"
    exit 1
fi