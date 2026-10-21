#ifndef EXTENDED_H
#define EXTENDED_H
/* Ten x87 value bytes followed by the compiler's two-byte format tag. */
typedef struct Extended {
    unsigned long low, high;
    unsigned short exponent, format;
} Extended;
extern Extended extended_zero, extended_one;
void integer_to_extended(const int *integer, Extended *number);
void store_extended(void); /* private ABI: ESI destination, ST(0) value */
#endif
