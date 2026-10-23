#ifndef FLOATING_H
#define FLOATING_H
#include <recon.h>
/* Ten x87 value bytes followed by the frontend's representation tag. */
typedef struct Extended {
    unsigned low, high;
    unsigned short exponent, format;
} Extended;
typedef char ExtendedSizeMustBe12[(sizeof(Extended) == 12) ? 1 : -1];
extern Extended extended_zero, scanned_floating_value;
int __stdcall convert_floating_literal(char *text, Extended *value);
int subtract_extended(const Extended *left, const Extended *right, Extended *result);
Extended *__stdcall save_extended_constant(Extended *value);
int compare_extended(unsigned short mask, const Extended *left, const Extended *right);
#endif
