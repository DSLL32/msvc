#include "frontend.h"
#include <recon.h>

/* Encountered polynomial taps at original RVA d5c58. The negative last
 * entry terminates the unsigned shifted-bit domain; unrelated following
 * literals are neither represented nor read. */
void initialize_checksums(void)
{
    static const int taps[] = { 0, 1, 3, 6, 8, 13, 15, -1 };
    unsigned value, bit, shift;
    const int *tap;
    unsigned char low, high, transformed;

    for (value = 0; value < 256; ++value) {
        frontend_bss.checksum_tables.low[value] = 0;
        frontend_bss.checksum_tables.high[value] = 0;
        for (bit = 0; bit < 8; ++bit) {
            low = high = 0;
            for (tap = taps; (unsigned)(*tap + bit) < 16; ++tap) {
                shift = *tap + bit;
                if (shift < 8)
                    low ^= (unsigned char)(value >> shift);
                else
                    high ^= (unsigned char)(value >> (shift - 8));
            }
            frontend_bss.checksum_tables.low[value] |= (low & 1) << bit;
            frontend_bss.checksum_tables.high[value] |= (high & 1) << bit;
        }
    }
    for (value = 0; value < 256; ++value) {
        transformed = (unsigned char)value;
        for (bit = 1; bit < 8; ++bit) {
            transformed ^= (transformed << 1) & (1U << bit);
            if (bit > 2)
                transformed ^= (transformed << 3) & (1U << bit);
        }
        frontend_bss.checksum_tables.update[value] = transformed;
    }
}
