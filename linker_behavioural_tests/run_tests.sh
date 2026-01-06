#!/bin/bash

export MY_LINKER=$(realpath ./ld)
export REF_LINKER="arm-none-eabi-ld"
export REF_LINKER_ARGS="-r -EB"
export CC="arm-none-eabi-gcc"
export CFLAGS="-mbig-endian -mno-thumb-interwork -c -nostdlib -ffreestanding"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
DIM='\033[2m'
NC='\033[0m'

LOG_FILE=$(mktemp)

printf "${BLUE}%-70s %-10s${NC}\n" "TEST NAME" "RESULT"
printf "${BLUE}%-70s %-10s${NC}\n" "----------------------------------------------------------------------" "------"

find . -type d -name "*_Should*" | sort | while read -r testdir; do
    test_name=$(basename "$testdir")
    
    (
        cd "$testdir" || exit 1
        if [ -f "./validate.sh" ]; then
            rm -f *.o
            {
                $CC $CFLAGS *.[csS] 2>&1
                bash ./validate.sh 2>&1
            } > "$LOG_FILE"
            exit $?
        else
            exit 255
        fi
    )
    
    result_code=$?

    if [ $result_code -eq 0 ]; then
        printf "%-70s [${GREEN}PASS${NC}]\n" "$test_name"
    elif [ $result_code -eq 255 ]; then
        printf "%-70s [${YELLOW}SKIP${NC}]\n" "$test_name"
    else
        printf "%-70s [${RED}FAIL${NC}]\n" "$test_name"
        while IFS= read -r line; do
            printf "    ${DIM}Reason: %s${NC}\n" "$line"
        done < "$LOG_FILE"
    fi
done

rm -f "$LOG_FILE"