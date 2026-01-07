#!/bin/bash

OUTPUT=$($MY_LINKER -o out.o -f file.o file.o 2>&1)

if [[ "$OUTPUT" == *"LinkerDuplicateSymbol"* ]];then
    exit 0
else
    exit 1
fi