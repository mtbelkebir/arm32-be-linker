#!/bin/bash

$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1

TYPE=$(arm-none-eabi-nm out.o | grep -w "Weak" | awk '{print $2}')

if [ "$TYPE" = "W" ] || [ "$TYPE" = "V" ]; then
    exit 0
else
    echo "Error: Final symbol is not Weak (Type found: $TYPE)"
    exit 1
fi