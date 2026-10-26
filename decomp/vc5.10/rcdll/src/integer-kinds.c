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

unsigned char __stdcall rc_based_integer_kind(int value)
{
    int largest_signed=0x7fffffff;

    /* The original comparison is signed, even for nondecimal spelling. */
    return value<=largest_signed?7:9;
}


unsigned char __stdcall rc_unsigned_integer_kind(unsigned value)
{
    unsigned magnitude,largest=0xfffffffeU;
    /* The signed test and unsigned negation retain the original wrap. */
    magnitude=(int)value<=0?0U-value:value;
    return magnitude>largest?10:9;
}


unsigned char __stdcall rc_long_integer_kind(int value)
{
    int largest_signed=0x7fffffff;
    /* Preserve the target's signed comparison, as for based integers. */
    return value<=largest_signed?8:10;
}
