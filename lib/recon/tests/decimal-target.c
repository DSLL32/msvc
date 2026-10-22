#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

#pragma pack(push, 2)
typedef struct TestDecimal12 {
    uint16_t extra;
    uint32_t low, high;
    uint16_t exponent;
} TestDecimal12;
#pragma pack(pop)

int recon_atoldbl(void *, const char *);
void recon_mtold12(unsigned char *, unsigned, void *);
void recon_multtenpow12(void *, int, int);

static void integer_decimal(TestDecimal12 *number, const char *text)
{
    unsigned char digits[24];
    unsigned count = strlen(text), index;
    assert(count <= sizeof(digits));
    for (index = 0; index < count; ++index) digits[index] = text[index] - '0';
    recon_mtold12(digits, count, number);
}

static void check_integer(void)
{
    TestDecimal12 number;
    unsigned char carried[24];
    integer_decimal(&number, "123456789012345678901234");
    /* Exact integer normalization retains bits below x87's 64-bit mantissa. */
    assert(number.extra == 0x7f90 && number.low == 0x850364b5);
    assert(number.high == 0xd124d8f8 && number.exponent == 0x404b);
    memset(carried, 9, sizeof(carried));
    carried[23] = 10;
    recon_mtold12(carried, sizeof(carried), &number);
    assert(number.extra == 0 && number.low == 0xcceda100);
    assert(number.high == 0xd3c21bce && number.exponent == 0x404e);
    integer_decimal(&number, "0");
    assert(!number.extra && !number.low && !number.high && !number.exponent);
}

static void check_scaling(void)
{
    TestDecimal12 number, unchanged;
    integer_decimal(&number, "1");
    recon_multtenpow12(&number, -1, 1);
    assert(number.extra == 0xcccd && number.low == 0xcccccccc);
    assert(number.high == 0xcccccccc && number.exponent == 0x3ffb);
    integer_decimal(&number, "1");
    recon_multtenpow12(&number, 24, 1);
    assert(number.extra == 0 && number.low == 0xcceda100);
    assert(number.high == 0xd3c21bce && number.exponent == 0x404e);
    integer_decimal(&number, "123456789012345678901234");
    unchanged = number;
    recon_multtenpow12(&number, 0, 0);
    assert(!memcmp(&number, &unchanged, sizeof(number)));
    /* Reduced-precision input discards extra16 once, before scaling. */
    unchanged.extra = 0;
    recon_multtenpow12(&number, -17, 0);
    recon_multtenpow12(&unchanged, -17, 1);
    assert(!memcmp(&number, &unchanged, sizeof(number)));
    integer_decimal(&number, "1");
    recon_multtenpow12(&number, 5200, 1);
    assert(number.high == 0x80000000 && number.exponent == 0x7fff);
    integer_decimal(&number, "1");
    recon_multtenpow12(&number, -5200, 1);
    assert(!number.extra && !number.low && !number.high && !number.exponent);
}

static void check_ten_bytes(void)
{
    unsigned char output[16];
    uint32_t mantissa_low, mantissa_high;
    uint16_t exponent;
    memset(output, 0xa5, sizeof(output));
    errno = EDOM;
    assert(recon_atoldbl(output, "1.5F") == 0);
    assert(errno == EDOM);
    memcpy(&mantissa_low, output, 4);
    memcpy(&mantissa_high, output + 4, 4);
    memcpy(&exponent, output + 8, 2);
    assert(!mantissa_low && mantissa_high == 0xc0000000 && exponent == 0x3fff);
    assert(output[10] == 0xa5 && output[11] == 0xa5 && output[15] == 0xa5);
    assert(recon_atoldbl(output, "1e5000") == 1);
    assert(recon_atoldbl(output, "-0.0") == 0);
    memcpy(&exponent, output + 8, 2);
    assert(exponent == 0x8000);
}

int main(int argc, char **argv)
{
    TestDecimal12 number;
    if (argc == 3) {
        /* Native-only analysis mode for independent mathematical vectors. */
        integer_decimal(&number, argv[1]);
        recon_multtenpow12(&number, atoi(argv[2]), 1);
        printf("%04x %08x %08x %04x\n", number.extra, number.low,
            number.high, number.exponent);
        return 0;
    }
    check_integer();
    check_scaling();
    check_ten_bytes();
    return 0;
}
