#!/bin/bash

$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1

PING_ADDR=$(arm-none-eabi-nm out.o | grep -w "Ping" | awk '{print $1}')
PONG_ADDR=$(arm-none-eabi-nm out.o | grep -w "Pong" | awk '{print $1}')


DIS_PING=$(arm-none-eabi-objdump -d out.o | grep -A 15 "<Ping>:")
if ! echo "$DIS_PING" | grep -q "bl.*$PONG_ADDR"; then
    echo "Error: Ping does not call Pong at 0x$PONG_ADDR"
    exit 1
fi


DIS_PONG=$(arm-none-eabi-objdump -d out.o | grep -A 15 "<Pong>:")
if ! echo "$DIS_PONG" | grep -q "bl.*$PING_ADDR"; then
    echo "Error: Pong does not call Ping at 0x$PING_ADDR"
    exit 1
fi

exit 0