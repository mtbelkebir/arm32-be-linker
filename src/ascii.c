//
// Created by mtbelkebir on 12/13/25.
//
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "ascii.h"

int cprintf(colour_t bg, colour_t fg, text_modifier_t mod, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (mod || fg || bg)
    {
        printf("\x1b[");
        bool first = true;
        if (mod & TEXT_MODIFIER_BOLD)
            printf("%s1", first ? "" : ";"), first = false;
        if (mod & TEXT_MODIFIER_ITALIC)
            printf("%s3", first ? "" : ";"), first = false;
        if (mod & TEXT_MODIFIER_UNDERLINE)
            printf("%s4", first ? "" : ";"), first = false;
        if (mod & TEXT_MODIFIER_STRIKETHROUGH)
            printf("%s9", first ? "" : ";"), first = false;
        if (fg)
            printf("%s%d", first ? "" : ";", fg), first = false;
        if (bg)
            printf("%s%d", first ? "" : ";", bg + 10), first = false;
        printf("m");
    }

    int ret = vprintf(fmt, args);

    if (mod || fg || bg)
        printf("\x1b[0m");
    va_end(args);
    return ret;
}