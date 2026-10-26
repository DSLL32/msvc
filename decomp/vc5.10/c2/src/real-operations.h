#ifndef C2_REAL_OPERATIONS_H
#define C2_REAL_OPERATIONS_H
#include "c2.h"
#include <recon.h>

/* Actual cdecl table ABI: three real pointers, status in EAX, caller cleanup12.
 * The arithmetic bodies remain genuine pending application functions. */
typedef int (__cdecl *C2RealOperation)(C2Real *, C2Real *, C2Real *);
int c2_real_add(C2Real *left, C2Real *right, C2Real *result);
int c2_real_subtract(C2Real *left, C2Real *right, C2Real *result);
int c2_real_multiply(C2Real *left, C2Real *right, C2Real *result);
int c2_real_divide(C2Real *left, C2Real *right, C2Real *result);
#endif
