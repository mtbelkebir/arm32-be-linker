#!/bin/bash

OUTPUT=$($MY_LINKER -o out.o -f file.o file.o > /dev/null)

if [[ "$OUTPUT" == *"LinkerDuplicateError" ]];then
    exit 0
else
    exit 1
fi