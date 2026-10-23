#ifndef FLOATING_H
#define FLOATING_H
#include <recon.h>
/* Ten x87 value bytes followed by the frontend's representation tag. */
typedef struct Extended {
    unsigned low, high;
    unsigned short exponent, format;
} Extended;
typedef char ExtendedSizeMustBe12[(sizeof(Extended) == 12) ? 1 : -1];
#pragma pack(2)
typedef struct ExtendedLimit {
    unsigned low, high;
    unsigned short exponent;
} ExtendedLimit;
#pragma pack()
typedef char ExtendedLimitSizeMustBe10[(sizeof(ExtendedLimit) == 10) ? 1 : -1];
extern ExtendedLimit double_round_limit;
#pragma pack(2)
typedef struct ExtendedRangeState {
    int negative;
    unsigned short status;
} ExtendedRangeState;
#pragma pack()
typedef char ExtendedRangeStateSizeMustBe6[(sizeof(ExtendedRangeState) == 6) ? 1 : -1];
extern ExtendedRangeState extended_range_state;
/* Private ABI: ST(0) limit and ST(1) value; leaves the value in ST(0). */
int check_extended_range(void);
/* Private ABI: destination in ESI and value in ST(0). */
void store_extended(void);
extern Extended extended_zero, scanned_floating_value;
int __stdcall convert_floating_literal(char *text, Extended *value);
int add_extended(const Extended *left, const Extended *right, Extended *result);
int multiply_extended(const Extended *left, const Extended *right, Extended *result);
int divide_extended(const Extended *left, const Extended *right, Extended *result);
int subtract_extended(const Extended *left, const Extended *right, Extended *result);
Extended *__stdcall save_extended_constant(Extended *value);
Extended *__stdcall save_extended_in_arena(Extended value, int arena);
int compare_extended(unsigned short mask, const Extended *left, const Extended *right);
#endif
