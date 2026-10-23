#include "directives.h"
#include <recon.h>

unsigned char __stdcall rc_decimal_integer_kind(int value)
{
    unsigned magnitude;
    int largest_signed=0x7fffffff;

    /* Retain the target's signed comparison and two's-complement negation,
     * including the wrap of INT_MIN. */
    magnitude=value<=0?0U-(unsigned)value:(unsigned)value;
    return (int)magnitude>largest_signed?8:7;
}
