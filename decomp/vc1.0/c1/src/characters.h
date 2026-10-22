#ifndef CHARACTERS_H
#define CHARACTERS_H

#include <recon.h>
/* Dispatch classes used by the frontend lexer. */
enum CharacterClass {
    CC_SPACE = 0,
    CC_CARRIAGE_RETURN = 1,
    CC_SLASH = 2,
    CC_ESCAPE_OR_END = 3,
    CC_STAR = 4,
    CC_NEWLINE = 5,
    CC_APOSTROPHE = 7,
    CC_QUOTE = 8,
    CC_DOT = 9,
    CC_BANG = 10,
    CC_HASH = 11,
    CC_PERCENT = 12,
    CC_EQUAL = 13,
    CC_CARET = 14,
    CC_BAR = 15,
    CC_AMPERSAND = 16,
    CC_PLUS = 17,
    CC_MINUS = 18,
    CC_LESS = 19,
    CC_GREATER = 20,
    CC_LEFT_SHIFT = 21,
    CC_RIGHT_SHIFT = 22,
    CC_INVALID = 23,
    CC_RIGHT_BRACE = 24,
    CC_RIGHT_BRACKET = 25,
    CC_COLON = 26,
    CC_COMMA = 27,
    CC_RIGHT_PAREN = 28,
    CC_DIGIT = 29,
    CC_LEFT_BRACE = 30,
    CC_LEFT_BRACKET = 31,
    CC_LEFT_PAREN = 32,
    CC_SEMICOLON = 34,
    CC_TILDE = 35,
    CC_MACRO_PARAMETER = 36,
    CC_MACRO_STRING = 37,
    CC_MACRO_TOKEN = 38,
    CC_MACRO_END = 39,
    CC_IDENTIFIER = 40,
    CC_AT = 41,
    CC_LEAD_BYTE = 42,
    CC_EXTENSION = 43,
    CC_WIDE_PREFIX = 45
};

extern unsigned char character_class[256];
enum CharacterFlags {
    CF_NUMBER = 1, CF_OCTAL = 2, CF_DECIMAL = 4, CF_HEX = 8,
    CF_IDENTIFIER = 16, CF_RADIX_SUFFIX = 32, CF_SPACE = 64, CF_STOP = 128
};
extern unsigned char character_flags[256];
#endif
