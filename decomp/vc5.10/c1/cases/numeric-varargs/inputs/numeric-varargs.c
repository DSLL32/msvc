#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#pragma pack(2)
typedef struct Sample {
    unsigned tag : 3;
    signed delta : 5;
    unsigned code : 8;
    float amount;
    double value;
    union { unsigned long whole; unsigned char bytes[4]; } data;
    struct { short x, y; } point;
} Sample;
#pragma pack()

enum Operation { OP_ADD, OP_CONVERT, OP_CHECK };
typedef double (__cdecl *Accumulator)(int, ...);

static volatile float samples[] = { 1.25F, 2.5F, 3.75F, .5F };
static const double exponents[] = { 1.0e12, 3.125e-8, 1.0e-20 };
static const wchar_t wide_name[] = L"numeric";
static long double extended_value = 9.75L;

static double __cdecl accumulate(int count, ...)
{
    va_list arguments;
    double total = 0.0;
    int index;
    va_start(arguments, count);
    for (index = 0; index < count; ++index)
        total += va_arg(arguments, double);
    va_end(arguments);
    return total;
}

static long convert_value(enum Operation operation, double value)
{
    switch (operation) {
    case OP_ADD: return (long)(value + .5);
    case OP_CONVERT: return (long)value;
    default: return value < 0.0 ? -1L : 1L;
    }
}

int main(void)
{
    Sample sample = { 5, -3, 0x41, 2.5F, 3.75, { 0 }, { 7, 11 } };
    Accumulator function = accumulate;
    char buffer[64];
    double total;
    long integer;
    int index;
    sample.data.whole = 0x01020304UL;
    total = function(3, samples[0], samples[1], samples[2]);
    integer = convert_value(OP_CONVERT, (double)extended_value);
    sprintf(buffer, "%.2f:%ld", total, integer);
    if (strcmp(buffer, "7.50:9")) return 1;
    if (sample.tag != 5 || sample.delta != -3 || sample.code != 0x41) return 2;
    if (sample.amount != 2.5F || sample.value != 3.75) return 3;
    if (sample.data.bytes[0] != 4 || sample.point.x + sample.point.y != 18) return 4;
    if (wide_name[0] != L'n' || samples[3] != .5F) return 5;
    for (index = 0; index < 3; ++index)
        if (exponents[index] <= 0.0) return 6;
    if (convert_value(OP_ADD, 2.75) != 3L) return 7;
    if (convert_value(OP_CHECK, -2.5) != -1L) return 8;
    return 0;
}
