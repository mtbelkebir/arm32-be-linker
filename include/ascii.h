//
// Created by mtbelkebir on 12/13/25.
//

#ifndef ELF_LINKER_1_0_ASCII_H
#define ELF_LINKER_1_0_ASCII_H
#include <stdint.h>

#define TEXT_MODIFIER_NONE 0
#define TEXT_MODIFIER_BOLD 1
#define TEXT_MODIFIER_ITALIC 2
#define TEXT_MODIFIER_UNDERLINE 4
#define TEXT_MODIFIER_STRIKETHROUGH 8

typedef uint8_t text_modifier_t;
typedef enum colour_t
{
    BLACK = 30,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
    NONE = 39,
} colour_t;

/**
 * @brief Coloured printf - but prints with the specified foreground colour
 * Is used the same way as `printf`
 *
 * @param bg - Background colour
 * @param fg Foreground colour
 * @param mod Text modifiers, works as a flag field so you can combine them.
 * @param fmt Format String
 * @param ... Arguments for format specifiers
 *
 * @example cprintf(RED, YELLOW, BOLD | ITALIC, "This is line %d, and it will be printed with red background, yellow foreground, in bold AND italic\n", 1);
 *
 * @returns The number of printed characters, a negative value in case of error
 * @see printf from <stdio.h>
 * @author Mohand Tahar Belkebir
 */

int cprintf(colour_t bg, colour_t fg, text_modifier_t mod, const char *fmt, ...);
#endif // ELF_LINKER_1_0_ASCII_H