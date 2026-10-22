#ifndef NUMERIC_SCANNING_H
#define NUMERIC_SCANNING_H
#include "lexer.h"
enum IntegerLiteralToken {
    TOK_INT_CONSTANT = 7, TOK_LONG_CONSTANT = 8,
    TOK_UINT_CONSTANT = 9, TOK_ULONG_CONSTANT = 10
};
unsigned char read_numeric_character(void);
unsigned __stdcall convert_integer_digits(char *digits, int length, int radix);
int __stdcall integer_digit_value(unsigned character);
unsigned char __stdcall classify_decimal_integer(unsigned value);
unsigned char __stdcall classify_radix_integer(unsigned value);
unsigned char __stdcall classify_long_integer(unsigned value);
unsigned char __stdcall classify_unsigned_integer(unsigned value);
#endif
