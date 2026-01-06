#!/bin/bash


$MY_LINKER -f *.o -o out.o


BINDS=$(arm-none-eabi-readelf -s out.o | grep "DoubleIt" | awk '{print $5}')

HAS_LOCAL=$(echo "$BINDS" | grep -q "LOCAL" && echo 1 || echo 0)
HAS_GLOBAL=$(echo "$BINDS" | grep -q "GLOBAL" && echo 1 || echo 0)

if [ "$HAS_LOCAL" -eq 1 ] && [ "$HAS_GLOBAL" -eq 1 ]; then
    exit 0
else
    echo "  Missing symbols: Local=$HAS_LOCAL, Global=$HAS_GLOBAL"
    exit 1
fi