/* One ordinary translation unit: integer spills/rematerialization, live x87
   expressions, and intrinsic string/memory operations. Caller-owned arrays
   must contain the elements and capacity described by the parameters. */
#include <string.h>
#include <math.h>

extern unsigned pressure_call(unsigned, unsigned, unsigned);

unsigned integer_pressure(const unsigned *p, unsigned seed, int count)
{
    unsigned a = seed + 3, b = seed + 5, c = seed + 7, d = seed + 11;
    unsigned e = seed + 13, f = seed + 17, g = seed + 19, h = seed + 23;
    unsigned j = seed + 29, k = seed + 31, m = seed + 37, n = seed + 41;
    const unsigned *q = p + 3, *r = p + 5, *s = p + 7, *t = p + 11;
    unsigned value;
    int i;
    for (i = 0; i < count; ++i) {
        value = pressure_call(a + c + e + g, b ^ d ^ f ^ h, j + k + m + n);
        a += q[i] + value; b ^= r[i] + a; c += s[i] ^ b; d ^= t[i] + c;
        e += a * 3 + d; f ^= b * 5 + e; g += c * 9 + f; h ^= d * 17 + g;
        j += e + (value << 3); k ^= f + (value >> 2);
        m += g * 33 + j; n ^= h * 65 + k;
        seed += pressure_call((unsigned)(q + i), a + e + j, b + f + k);
    }
    return seed + a + b + c + d + e + f + g + h + j + k + m + n;
}

double floating_permutations(const double *p, double bias)
{
    double a = p[0] + bias, b = p[1] - bias;
    double c = p[2] * 1.25, d = p[3] * 0.75;
    double e = p[4] + 2.0, f = p[5] - 3.0;
    double g = p[6] * 1.5, h = p[7] + 0.5;
    double left = (a - b) / (c + d) - (e - f) / (g + h);
    double right = (g * a - h * b) / (c - d + 2.0);
    double middle = (a * c - b * d) / (e + f + 1.0);
    return left - right + middle + sin(a - e) * cos(f - b) +
        sqrt(c * c + d * d + 1.0) + (g - e) * (h - f);
}

unsigned string_memory(char *out, const char *source, const char *reference,
    unsigned capacity)
{
    unsigned length, result;
    if (!capacity) return 0;
    length = strlen(source);
    if (length < capacity) strcpy(out, source);
    else {
        length = capacity - 1;
        memcpy(out, source, length);
        out[length] = 0;
    }
    result = strcmp(out, reference) == 0;
    if (length > 7) memset(out + 2, '.', length - 4);
    memset(out + length, 0, capacity - length);
    return strlen(out) + result;
}
