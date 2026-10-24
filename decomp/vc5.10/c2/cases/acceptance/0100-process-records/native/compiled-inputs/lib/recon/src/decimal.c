#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <locale.h>
#include <float.h>
#include <recon.h>

#if LDBL_MANT_DIG != 64 || LDBL_MAX_EXP != 16384
#error The native decimal boundary requires the i386 x87 long double format
#endif

/* The target supplies its own typed declarations for these opaque CRT ABIs.
 * Do not expose this private structure in recon.h. */
#pragma pack(push, 2)
typedef struct Decimal12 {
    uint16_t extra;
    uint32_t low, high;
    uint16_t exponent;
} Decimal12;
#pragma pack(pop)
typedef char Decimal12Is12[(sizeof(Decimal12) == 12) ? 1 : -1];

int recon_atoldbl(void *output, const char *text)
{
    static locale_t c_locale;
    long double value;
    uint16_t exponent;
    int saved_errno = errno;
    if (!c_locale) {
        c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
        if (!c_locale) recon_unsupported("decimal C locale");
    }
    value = strtold_l(text, NULL, c_locale);
    /* Extended has a separate format field after the ten-byte number. */
    memcpy(output, &value, 10);
    memcpy(&exponent, (unsigned char *)output + 8, 2);
    errno = saved_errno;
    /* The historical _ld12told converter reports overflow as 1. */
    return (exponent & 0x7fff) == 0x7fff;
}

/* A coefficient needs at most 5^4096 and 80 guard bits. Generate the powers
 * mathematically instead of embedding the historical CRT's data section. */
#define DECIMAL_LIMBS 416
typedef struct BigInteger {
    unsigned count;
    uint32_t words[DECIMAL_LIMBS];
} BigInteger;

static void big_small(BigInteger *number, uint32_t value)
{
    memset(number, 0, sizeof(*number));
    number->count = value != 0;
    number->words[0] = value;
}

static void big_trim(BigInteger *number)
{
    while (number->count && !number->words[number->count - 1]) --number->count;
}

static void big_multiply_small(BigInteger *number, unsigned factor, unsigned add)
{
    uint64_t carry = add, product;
    unsigned index;
    for (index = 0; index < number->count; ++index) {
        product = (uint64_t)number->words[index] * factor + carry;
        number->words[index] = (uint32_t)product;
        carry = product >> 32;
    }
    if (carry) {
        if (number->count == DECIMAL_LIMBS) recon_unsupported("decimal integer capacity");
        number->words[number->count++] = (uint32_t)carry;
    }
}

static int big_bits(const BigInteger *number)
{
    uint32_t high;
    int bits;
    if (!number->count) return 0;
    high = number->words[number->count - 1];
    bits = (number->count - 1) * 32;
    while (high) { high >>= 1; ++bits; }
    return bits;
}

static int big_compare(const BigInteger *left, const BigInteger *right)
{
    unsigned index;
    if (left->count != right->count) return left->count > right->count ? 1 : -1;
    index = left->count;
    while (index) {
        --index;
        if (left->words[index] != right->words[index])
            return left->words[index] > right->words[index] ? 1 : -1;
    }
    return 0;
}

static void big_shift(BigInteger *output, const BigInteger *input, unsigned bits)
{
    unsigned whole = bits / 32, part = bits % 32, index;
    uint64_t value;
    big_small(output, 0);
    if (!input->count) return;
    if (input->count + whole + (part != 0) > DECIMAL_LIMBS)
        recon_unsupported("decimal shift capacity");
    for (index = 0; index < input->count; ++index) {
        value = (uint64_t)input->words[index] << part;
        output->words[index + whole] |= (uint32_t)value;
        if (part) output->words[index + whole + 1] = (uint32_t)(value >> 32);
    }
    output->count = input->count + whole + (part != 0);
    big_trim(output);
}

static void big_subtract(BigInteger *left, const BigInteger *right)
{
    uint64_t subtrahend;
    unsigned index, borrow = 0;
    for (index = 0; index < left->count; ++index) {
        subtrahend = (uint64_t)(index < right->count ? right->words[index] : 0) + borrow;
        borrow = (uint64_t)left->words[index] < subtrahend;
        left->words[index] -= (uint32_t)subtrahend;
    }
    big_trim(left);
}

static void big_divide(BigInteger *quotient, BigInteger *remainder,
    const BigInteger *numerator, const BigInteger *denominator)
{
    BigInteger shifted;
    int bit = big_bits(numerator) - big_bits(denominator);
    *remainder = *numerator;
    big_small(quotient, 0);
    for (; bit >= 0; --bit) {
        big_shift(&shifted, denominator, bit);
        if (big_compare(remainder, &shifted) >= 0) {
            big_subtract(remainder, &shifted);
            quotient->words[bit / 32] |= (uint32_t)1 << (bit % 32);
            if (quotient->count < (unsigned)bit / 32 + 1)
                quotient->count = bit / 32 + 1;
        }
    }
}

static void decimal_pack(Decimal12 *number, const BigInteger *mantissa, unsigned exponent)
{
    number->extra = (uint16_t)mantissa->words[0];
    number->low = (mantissa->words[0] >> 16) | (mantissa->words[1] << 16);
    number->high = (mantissa->words[1] >> 16) | (mantissa->words[2] << 16);
    number->exponent = (uint16_t)exponent;
}

static void power_coefficient(Decimal12 *number, int power)
{
    BigInteger numerator, denominator, shifted, quotient, remainder;
    int count = power < 0 ? -power : power;
    int binary_power, shift, comparison, index;
    big_small(&numerator, 1);
    big_small(&denominator, 1);
    for (index = 0; index < count; ++index)
        big_multiply_small(power < 0 ? &denominator : &numerator, 5, 0);
    binary_power = big_bits(&numerator) - big_bits(&denominator);
    if (binary_power >= 0) {
        big_shift(&shifted, &denominator, binary_power);
        if (big_compare(&numerator, &shifted) < 0) --binary_power;
    } else {
        big_shift(&shifted, &numerator, -binary_power);
        if (big_compare(&shifted, &denominator) < 0) --binary_power;
    }
    shift = 79 - binary_power;
    if (shift >= 0) {
        big_shift(&shifted, &numerator, shift);
        numerator = shifted;
    } else {
        big_shift(&shifted, &denominator, -shift);
        denominator = shifted;
    }
    big_divide(&quotient, &remainder, &numerator, &denominator);
    big_shift(&shifted, &remainder, 1);
    comparison = big_compare(&shifted, &denominator);
    if (comparison > 0 || (comparison == 0 && (quotient.words[0] & 1)))
        big_multiply_small(&quotient, 1, 1);
    /* None of the encountered integer powers rounds across a binary binade. */
    if (big_bits(&quotient) != 80) recon_unsupported("decimal coefficient normalization");
    decimal_pack(number, &quotient, 16383 + binary_power + power);
}

void recon_mtold12(unsigned char *digits, unsigned count, void *output)
{
    BigInteger integer, normalized;
    Decimal12 number;
    unsigned index;
    int bits;
    if (count > 24) recon_unsupported("decimal digit count exceeds 24");
    big_small(&integer, 0);
    for (index = 0; index < count; ++index) {
        /* The scanner may increment its final retained digit to 10 before
         * discarding the next digit; integer accumulation carries that value. */
        if (digits[index] > 10) recon_unsupported("decimal digit value");
        big_multiply_small(&integer, 10, digits[index]);
    }
    bits = big_bits(&integer);
    if (!bits) memset(&number, 0, sizeof(number));
    else {
        big_shift(&normalized, &integer, 80 - bits);
        decimal_pack(&number, &normalized, 16382 + bits);
    }
    memcpy(output, &number, sizeof(number));
}

static void decimal_words(uint16_t words[5], const Decimal12 *number)
{
    words[0] = number->extra;
    words[1] = (uint16_t)number->low;
    words[2] = (uint16_t)(number->low >> 16);
    words[3] = (uint16_t)number->high;
    words[4] = (uint16_t)(number->high >> 16);
}

static void shift_product_left(uint16_t product[6])
{
    unsigned index, carry = 0, next;
    for (index = 0; index < 6; ++index) {
        next = product[index] >> 15;
        product[index] = (uint16_t)((product[index] << 1) | carry);
        carry = next;
    }
}

static unsigned shift_product_right(uint16_t product[6])
{
    unsigned index = 6, carry = 0, next;
    unsigned discarded = product[0] & 1;
    while (index) {
        --index;
        next = product[index] & 1;
        product[index] = (uint16_t)((product[index] >> 1) | (carry << 15));
        carry = next;
    }
    return discarded;
}

static void decimal_multiply(Decimal12 *number, const Decimal12 *factor)
{
    uint16_t left[5], right[5], product[6] = { 0, 0, 0, 0, 0, 0 };
    unsigned row, column, index, carry, discarded = 0;
    uint32_t partial, sum;
    unsigned left_exponent = number->exponent & 0x7fff;
    unsigned right_exponent = factor->exponent & 0x7fff;
    unsigned sign = (number->exponent ^ factor->exponent) & 0x8000;
    int exponent = left_exponent + right_exponent - 16382;
    decimal_words(left, number);
    decimal_words(right, factor);
    if (left_exponent + right_exponent > 0xbffd) goto overflow;
    /* Preserve the old multiplication helper's early denormal cutoff. */
    if (left_exponent + right_exponent <= 0x3fbf) {
        memset(number, 0, sizeof(*number));
        return;
    }
    if (!left_exponent && !number->extra && !number->low && !number->high) {
        memset(number, 0, sizeof(*number));
        return;
    }
    if (!left_exponent) ++exponent;
    if (!right_exponent) ++exponent;
    if (left_exponent == 0x7fff || right_exponent == 0x7fff) goto overflow;
    /* The historical helper accumulates the top six 16-bit columns, omitting
     * partial products below them. Its extra guard word rounds the retained
     * 80 bits only when strictly above half; an exact half remains unchanged. */
    for (row = 0; row < 5; ++row) {
        for (column = 4 - row; column < 5; ++column) {
            index = row + column - 4;
            partial = (uint32_t)left[row] * right[column];
            sum = (uint32_t)product[index] + (partial & 0xffff);
            product[index] = (uint16_t)sum;
            carry = sum >> 16;
            sum = (uint32_t)product[index + 1] + (partial >> 16) + carry;
            product[index + 1] = (uint16_t)sum;
            if (sum >> 16) ++product[index + 2];
        }
    }
    while (exponent > 0 && !(product[5] & 0x8000)) {
        shift_product_left(product);
        --exponent;
    }
    if (exponent <= 0) {
        int shifts = 1 - exponent;
        if (shifts > 96) {
            memset(number, 0, sizeof(*number));
            return;
        }
        while (shifts--) discarded |= shift_product_right(product);
        if (discarded) product[0] |= 1;
        exponent = 0;
    }
    if (product[0] > 0x8000) {
        for (index = 1; index < 6; ++index) {
            if (++product[index]) break;
        }
        if (index == 6) {
            product[5] = 0x8000;
            ++exponent;
        }
    }
    if (exponent >= 0x7fff) goto overflow;
    number->extra = product[1];
    number->low = product[2] | ((uint32_t)product[3] << 16);
    number->high = product[4] | ((uint32_t)product[5] << 16);
    number->exponent = (uint16_t)(sign | exponent);
    return;
overflow:
    number->extra = 0;
    number->low = 0;
    number->high = 0x80000000;
    number->exponent = (uint16_t)(sign | 0x7fff);
}

void recon_multtenpow12(void *output, int exponent, int multiply12)
{
    Decimal12 number, factor;
    unsigned remaining, step = 1, digit;
    int negative = exponent < 0;
    memcpy(&number, output, sizeof(number));
    if (!exponent) return;
    if (exponent < -5200 || exponent > 5200)
        recon_unsupported("decimal exponent exceeds scanner range");
    if (!multiply12) number.extra = 0;
    remaining = negative ? -exponent : exponent;
    /* Match the original base-eight power selection and intermediate rounds. */
    while (remaining) {
        digit = remaining & 7;
        if (digit) {
            power_coefficient(&factor, negative ? -(int)(digit * step) : (int)(digit * step));
            decimal_multiply(&number, &factor);
        }
        remaining >>= 3;
        step *= 8;
    }
    memcpy(output, &number, sizeof(number));
}
