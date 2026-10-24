#include "compiler-state.h"
#include <recon.h>

/* Encountered feedback polynomial positions, terminated before unrelated
 * following original data. Tables are generated, never copied from an image. */
static unsigned feedback_taps[] = { 0, 1, 3, 6, 8, 13, 15, (unsigned)-1 };

void __cdecl initialize_checksums(void)
{
    unsigned input, bit, tap, shift, mask;
    BYTE low, high, transformed;
    for (input = 0; input < 256; ++input) {
        c1_bss.checksums.low[input] = 0;
        c1_bss.checksums.high[input] = 0;
        for (bit = 0; bit < 8; ++bit) {
            low = high = 0;
            for (tap = 0; ; ++tap) {
                shift = feedback_taps[tap] + bit;
                if (shift < 8)
                    low ^= (BYTE)(input >> shift);
                else if (shift < 16)
                    high ^= (BYTE)(input >> (shift - 8));
                else
                    break;
            }
            c1_bss.checksums.low[input] |= (low & 1) << bit;
            c1_bss.checksums.high[input] |= (high & 1) << bit;
        }
    }
    for (input = 0; input < 256; ++input) {
        transformed = (BYTE)input;
        for (bit = 1, mask = 2; bit < 8; ++bit, mask += mask) {
            transformed ^= (transformed * 2) & mask;
            if (bit > 2)
                transformed ^= (transformed * 8) & mask;
        }
        c1_bss.checksums.transform[input] = transformed;
    }
}
