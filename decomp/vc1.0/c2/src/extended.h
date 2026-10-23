#ifndef EXTENDED_H
#define EXTENDED_H
#include <recon.h>
/* Ten x87 value bytes followed by the compiler's two-byte format tag. */
typedef struct Extended {
    unsigned long low, high;
    unsigned short exponent, format;
} Extended;
typedef struct DoubleBits { unsigned long low, high; } DoubleBits;
extern Extended extended_zero, extended_one;
int compare_extended(unsigned operation, const Extended *left, const Extended *right);
int extended_format(const Extended *number);
int extended_to_float(const Extended *number, float *result);
int extended_to_double(const Extended *number, DoubleBits *result);
void float_to_extended(const float *number, Extended *result);
void double_to_extended(const DoubleBits *number, Extended *result);
void integer_to_extended(const int *integer, Extended *number);
void unsigned_integer_to_extended(const unsigned *integer, Extended *number);
int extended_to_integer(const Extended *number, int *result);
int check_extended_range(void); /* private ABI: ST(0) limit, ST(1) value */
void store_extended(void); /* private ABI: ESI destination, ST(0) value */
#endif
