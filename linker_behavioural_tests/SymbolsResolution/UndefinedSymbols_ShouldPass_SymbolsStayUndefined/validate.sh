#!/bin/bash

$MY_LINKER -o out.o -f *.o > /dev/null 2>&1

SYMBOL_TYPE=$(arm-none-eabi-nm out.o | grep -w "C" | awk '{print $1}')

if [ "$SYMBOL_TYPE" = "U" ]; then
    exit 0
else
    echo "Error: Symbol C is not undefined (Type: $SYMBOL_TYPE)"
    exit 1
fi