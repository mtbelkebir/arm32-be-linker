#!/bin/bash

$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1

SYMBOL_INFO=$(arm-none-eabi-nm out.o | grep -w "TripleIt")
ADDR=$(echo "$SYMBOL_INFO" | awk '{print $1}')
TYPE=$(echo "$SYMBOL_INFO" | awk '{print $2}')

if [ "$ADDR" = "00000024" ] && [ "$TYPE" = "T" ]; then
    exit 0
else
    echo "Error: TripleIt is not pointing to strong definition. Found Addr:$ADDR, Type:$TYPE"
    exit 1
fi