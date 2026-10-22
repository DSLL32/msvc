#ifndef FLOATING_H
#define FLOATING_H
#include <recon.h>
/* Ten x87 value bytes followed by the frontend's representation tag. */
typedef struct Extended {
    unsigned low, high;
    unsigned short exponent, format;
} Extended;
typedef char ExtendedSizeMustBe12[(sizeof(Extended) == 12) ? 1 : -1];
extern Extended extended_zero;
int subtract_extended(const Extended *left, const Extended *right, Extended *result);
Extended *__stdcall save_extended_constant(Extended *value);
#endif
