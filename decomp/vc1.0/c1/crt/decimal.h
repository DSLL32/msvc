#ifndef C1_CRT_DECIMAL_H
#define C1_CRT_DECIMAL_H
#include <recon.h>

#pragma pack(2)
typedef struct LDouble12 {
    unsigned short extra;
    unsigned low, high;
    unsigned short exponent;
} LDouble12;
#pragma pack()
typedef char LDouble12_size[sizeof(LDouble12) == 12 ? 1 : -1];
int __strgtold12(LDouble12 *result, char **end, const char *source,
                int multiply12, int scale, int decimal_point, int implicit_exponent);

#endif
