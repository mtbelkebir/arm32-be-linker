#!/bin/bash

$MY_LINKER -o out.o -f file1.o file2.o > /dev/null 2>&1

PING_ADDR=$(arm-none-eabi-nm out.o | grep -w "Ping" | awk '{print $1}' | sed 's/^0*//')
PONG_ADDR=$(arm-none-eabi-nm out.o | grep -w "Pong" | awk '{print $1}' | sed 's/^0*//')

if [ -z "$PING_ADDR" ]; then PING_ADDR="0"; fi
if [ -z "$PONG_ADDR" ]; then PONG_ADDR="0"; fi

DIS_PING=$(arm-none-eabi-objdump -d out.o | grep -A 15 "<Ping>:")
TARGET_IN_PING=$(echo "$DIS_PING" | grep "bl" | awk '{print $(NF-1)}' | sed 's/^0*//')

if [ "$PONG_ADDR" != "$TARGET_IN_PING" ]; then
    echo "Error: Ping does not call Pong at 0x$PONG_ADDR (found 0x$TARGET_IN_PING)"
    exit 1
fi

DIS_PONG=$(arm-none-eabi-objdump -d out.o | grep -A 15 "<Pong>:")
TARGET_IN_PONG=$(echo "$DIS_PONG" | grep "bl" | awk '{print $(NF-1)}' | sed 's/^0*//')

if [ "$PING_ADDR" != "$TARGET_IN_PONG" ]; then
    echo "Error: Pong does not call Ping at 0x$PING_ADDR (found 0x$TARGET_IN_PONG)"
    exit 1
fi

exit 0