#!/bin/bash

$MY_LINKER -o out.o -f file.o file.o > /dev/null 2>&1

COUNT_X=$(arm-none-eabi-nm out.o | grep -w "x" | wc -l)
COUNT_GETSECRET=$(arm-none-eabi-nm out.o | grep -w "GetSecret" | wc -l)

if [ "$COUNT_X" -eq 2 ] && [ "$COUNT_GETSECRET" -eq 2 ]; then
    exit 0
else
    echo "Error: x found $COUNT_X times, GetSecret found $COUNT_GETSECRET times."
    exit 1
fi