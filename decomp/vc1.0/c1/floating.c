/* Frontend floating-point environment initialization. */
#include <float.h>
#include <ctype.h>
#include "initialization.h"
#include "floating.h"
#include "initializer-records.h"

extern int _atoldbl(Extended *result, const char *text);

/* Original 0x4c380..0x4c38d. */
void initialize_floating_point(void)
{
    _control87(4, 4);
}

/* Original 0x4c390..0x4c4a1. Parse through the CRT's ten-byte form,
   then round to the requested target representation and expand it again.
   This target gives long double the same precision as double. */
int __stdcall parse_floating_literal(char *text, Extended *number)
{
    Extended parsed;
    char suffix, *end;
    float single;
    double wide;
    _atoldbl(&parsed, text);
    number->low = parsed.low;
    number->high = parsed.high;
    number->exponent = parsed.exponent;
    end = text;
    do { ++end; } while (*end);
    suffix = end[-1] & 0xdf;
    if (suffix == 'L') {
        if (extended_to_double(number, &wide)) {
            number->format = 8;
            return 1;
        }
        double_to_extended(&wide, number);
        number->format = 8;
        return 0;
    }
    if (suffix == 'F') {
        number->format = 4;
        if (extended_to_float(number, &single)) return 1;
        float_to_extended(&single, number);
        return 0;
    }
    number->format = 8;
    if (extended_to_double(number, &wide)) return 1;
    double_to_extended(&wide, number);
    return 0;
}

/* Decimal scanner workspace, original 0x68b00. Values are decimal digits,
   not their character encodings. The 25th digit controls truncation. */
unsigned char decimal_digits[25];
extern char *_decimal_point;
extern void __mtold12(unsigned char *digits, unsigned count, LDouble12 *result);
extern void __multtenpow12(LDouble12 *number, int exponent, int multiply12);

/* Original CRT variant 0x57f40..0x58660, including its twelve-state
   dispatch table. This scanner accepts D/E exponents and the optional
   Fortran-style implicit exponent, retaining at most 25 decimal digits. */
int __strgtold12(LDouble12 *result, char **end, const char *source,
                int multiply12, int scale, int decimal_point, int implicit_exponent)
{
    enum { START, LEADING_ZERO, SIGNED, INTEGER, FRACTION, DOT,
           EXPONENT, EXP_SIGN, EXP_ZERO, EXP_DIGITS, DONE, IMPLICIT_EXP } state;
    const char *cursor, *fallback;
    unsigned char character, *digit;
    unsigned count;
    int seen, point_seen, exponent_seen, exponent_sign;
    int exponent, shift, overflow, underflow, status;
    unsigned short sign;
    LDouble12 converted;
    state = START;
    cursor = fallback = source;
    digit = decimal_digits;
    count = 0;
    seen = point_seen = exponent_seen = 0;
    exponent = shift = overflow = underflow = status = 0;
    exponent_sign = 1;
    sign = 0;
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r') ++cursor;
    while (state != DONE) {
        character = *cursor++;
        switch (state) {
        case START:
            if (character >= '1' && character <= '9') {
                state = INTEGER; --cursor;
            } else if (character == *_decimal_point) state = DOT;
            else if (character == '+') { state = SIGNED; sign = 0; }
            else if (character == '-') { state = SIGNED; sign = 0x8000; }
            else if (character == '0') state = LEADING_ZERO;
            else goto finished_character;
            break;
        case LEADING_ZERO:
            seen = 1;
            if (character >= '1' && character <= '9') { state = INTEGER; --cursor; }
            else if (character == *_decimal_point) state = FRACTION;
            else if (character == '0') state = LEADING_ZERO;
            else goto exponent_or_end;
            break;
        case SIGNED:
            if (character >= '1' && character <= '9') { state = INTEGER; --cursor; }
            else if (character == *_decimal_point) state = DOT;
            else if (character == '0') state = LEADING_ZERO;
            else { state = DONE; cursor = fallback; }
            break;
        case INTEGER:
            seen = 1;
            while (isdigit(character)) {
                if (count < 25) { ++count; *digit++ = character - '0'; }
                else ++shift;
                character = *cursor++;
            }
            if (character == *_decimal_point) state = FRACTION;
            else goto exponent_or_end;
            break;
        case FRACTION:
            seen = point_seen = 1;
            if (!count)
                while (character == '0') { --shift; character = *cursor++; }
            while (isdigit(character)) {
                if (count < 25) { ++count; *digit++ = character - '0'; --shift; }
                character = *cursor++;
            }
exponent_or_end:
            if (character == 'D' || character == 'E' || character == 'd' || character == 'e')
                state = EXPONENT;
            else if (character == '+' || character == '-') { state = IMPLICIT_EXP; --cursor; }
            else goto finished_character;
            break;
        case DOT:
            point_seen = 1;
            if (isdigit(character)) { state = FRACTION; --cursor; }
            else { state = DONE; cursor = fallback; }
            break;
        case EXPONENT:
            fallback = cursor - 2;
            if (character >= '1' && character <= '9') { state = EXP_DIGITS; --cursor; }
            else if (character == '+') state = EXP_SIGN;
            else if (character == '-') { state = EXP_SIGN; exponent_sign = -1; }
            else if (character == '0') state = EXP_ZERO;
            else { state = DONE; cursor = fallback; }
            break;
        case EXP_SIGN:
            if (character >= '1' && character <= '9') { state = EXP_DIGITS; --cursor; }
            else if (character == '0') state = EXP_ZERO;
            else { state = DONE; cursor = fallback; }
            break;
        case EXP_ZERO:
            exponent_seen = 1;
            while (character == '0') character = *cursor++;
            if (character >= '1' && character <= '9') { state = EXP_DIGITS; --cursor; }
            else goto finished_character;
            break;
        case EXP_DIGITS:
            exponent_seen = 1;
            exponent = 0;
            while (isdigit(character)) {
                exponent = exponent * 10 + (signed char)character - '0';
                if (exponent > 5200) { exponent = 5201; break; }
                character = *cursor++;
            }
            while (isdigit(character)) character = *cursor++;
            goto finished_character;
        case IMPLICIT_EXP:
            if (!implicit_exponent) goto finished_character;
            fallback = cursor - 1;
            if (character == '+') state = EXP_SIGN;
            else if (character == '-') { state = EXP_SIGN; exponent_sign = -1; }
            else { state = DONE; cursor = fallback; }
            break;
finished_character:
            state = DONE;
            --cursor;
            break;
        }
    }
    *end = (char *)cursor;
    if (seen) {
        if (count > 24) {
            if ((signed char)decimal_digits[23] >= 5) ++decimal_digits[23];
            count = 24;
            --digit;
            ++shift;
        }
        if (count) {
            --digit;
            while (!*digit) { --digit; ++shift; --count; }
            __mtold12(decimal_digits, count, &converted);
            if (exponent_sign < 0) exponent = -exponent;
            exponent += shift;
            if (!exponent_seen) exponent += scale;
            if (!point_seen) exponent -= decimal_point;
            if (exponent > 5200) overflow = 1;
            else if (exponent < -5200) underflow = 1;
            else __multtenpow12(&converted, exponent, multiply12);
        } else {
            converted.extra = 0;
            converted.low = converted.high = 0;
            converted.exponent = 0;
        }
    }
    if (!seen) {
        converted.extra = 0;
        converted.low = converted.high = 0;
        converted.exponent = 0;
        status = 4;
    } else if (overflow) {
        converted.extra = 0;
        converted.low = 0;
        converted.high = 0x80000000;
        converted.exponent = 0x7fff;
        status = 2;
    } else if (underflow) {
        converted.extra = 0;
        converted.low = converted.high = 0;
        converted.exponent = 0;
        status = 1;
    }
    result->extra = converted.extra;
    result->low = converted.low;
    result->high = converted.high;
    result->exponent = sign | converted.exponent;
    return status;
}

/* Largest float-rounding threshold in the native ten-byte representation. */
Extended float_round_limit = { 0xffffffffU, 0xffffff7fU, 0x407e, 0 };

/* Original 0x4c625..0x4c64c. MSVC 1.0 C has no ten-byte scalar type,
   so narrow x87 transfers preserve that value and the private helper ABI.
   Range decisions and output control remain ordinary C. */
int extended_to_float(const Extended *value, float *result)
{
    int overflow;
    __asm {
        mov eax, value
        fld tbyte ptr [eax]
        fld tbyte ptr float_round_limit
    }
    overflow = check_extended_range();
    if (!overflow) {
        __asm {
            mov eax, result
            fstp dword ptr [eax]
            fwait
        }
    }
    return overflow;
}

FloatingWorkspace floating_workspace; /* original 0x710aa..0x710b2 */

/* Original 0x4c8f5..0x4c93b. The private ABI supplies a ten-byte limit
   in ST(0), value in ST(1), and leaves the signed value in ST(0).
   C cannot express those inputs or preserve x87 condition codes; the narrow
   blocks only perform the comparisons, while C handles sign and range. */
int check_extended_range(void)
{
    unsigned short status;
    floating_workspace.check.negative = 0;
    __asm {
        fxch st(1)
        ftst
        fstsw status
    }
    floating_workspace.check.status = status;
    if (status & 0x100) {
        ++floating_workspace.check.negative;
        __asm { fchs }
    }
    __asm {
        fxch st(1)
        fcomp st(1)
        fstsw status
    }
    floating_workspace.check.status = status;
    if (floating_workspace.check.negative) {
        __asm { fchs }
    }
    return (status & 0x100) != 0;
}

/* Original 0x4c696..0x4c6b8. The store entry takes ESI and ST(0),
   so these narrow blocks bridge that original private x87 ABI. */
int float_to_extended(const float *value, Extended *number)
{
    __asm {
        mov eax, value
        fld dword ptr [eax]
    }
    number->format = 4;
    __asm {
        mov esi, number
        call store_extended
    }
    return 0;
}

/* Original 0x4c8f1..0x4c8f5 is a shared four-byte x87 store tail.
   Its ESI/ST(0) ABI and ten-byte destination cannot be declared in C.
   No C frame is needed or permitted at this private entry. */
__declspec(naked) void store_extended(void)
{
    __asm {
        fstp tbyte ptr [esi]
        fwait
        ret
    }
}
