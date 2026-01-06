#!/bin/bash


$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1


SECRET_ADDR=$(arm-none-eabi-nm out.o | grep -w "getSecret" | awk '{print $1}')

DISASM=$(arm-none-eabi-objdump -d out.o | grep -A 10 "<main>:")


if echo "$DISASM" | grep -q "bl.*$SECRET_ADDR"; then
    exit 0
else
    echo "Relocation Error: 'main' does not call 'getSecret' at expected address 0x$SECRET_ADDR"
    echo "Disassembly snippet:"
    echo "$DISASM"
    exit 1
fi