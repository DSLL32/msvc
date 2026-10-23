#ifndef FLOATING_H
#define FLOATING_H
#include "expressions.h"
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
/* Range testing and precision rounding share these original eight bytes. */
typedef union FloatingWorkspace {
    double rounded;
    struct { int negative; unsigned short status, control; } check;
} FloatingWorkspace;
extern FloatingWorkspace floating_workspace;
/* Private x87 ABI: check consumes ST(0)=limit and retains ST(1)=value;
   the Boolean result is returned in EAX. */
int check_extended_range(void);
/* Private input/output ST(0), retaining the caller control word. */
void truncate_extended(void);
/* Private store entry: destination ESI, value ST(0). */
void store_extended(void);
/* Private arithmetic tail: destination ESI, value ST(0), status EAX. */
int finish_extended_arithmetic(void);
#endif
