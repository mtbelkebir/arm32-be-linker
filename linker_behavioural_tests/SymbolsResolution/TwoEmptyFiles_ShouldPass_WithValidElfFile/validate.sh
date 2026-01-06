#!/bin/bash

$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1

SECTIONS=$(arm-none-eabi-readelf -S out.o)

HAS_TEXT=$(echo "$SECTIONS" | grep -q ".text" && echo 1 || echo 0)
HAS_COMMENT=$(echo "$SECTIONS" | grep -q ".comment" && echo 1 || echo 0)
HAS_ARM=$(echo "$SECTIONS" | grep -q ".ARM.attributes" && echo 1 || echo 0)

if [ "$HAS_TEXT" -eq 1 ] && [ "$HAS_COMMENT" -eq 1 ] && [ "$HAS_ARM" -eq 1 ]; then
    exit 0
else
    echo "Error: Missing mandatory sections. Text:$HAS_TEXT, Comment:$HAS_COMMENT, ARM:$HAS_ARM"
    exit 1
fi