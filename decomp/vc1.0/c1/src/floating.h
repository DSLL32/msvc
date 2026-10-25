#ifndef FLOATING_H
#define FLOATING_H
#include "expressions.h"
#include <recon.h>
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
int extended_format(const Extended *value);
int copy_extended(const Extended *source, Extended *destination);
int extended_to_ten_bytes(const Extended *source, void *destination);
int ten_bytes_to_extended(const void *source, Extended *destination);
int extended_to_unsigned(const Extended *source, unsigned *destination);
int round_extended_to_integer(const Extended *source, int *destination);
#endif
