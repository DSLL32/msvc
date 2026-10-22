/* Compiler checksum transform tables, original 0xa3540..0xa3860. */
#include "options.h"
#include <recon.h>

unsigned char checksum_low[256];
unsigned char checksum_high[256];
unsigned char checksum_transform[256];
static unsigned feedback_taps[] = { 0, 1, 3, 6, 8, 13, 15, (unsigned)-1 };

/* Original 0x746f0..0x747ca. */
void initialize_checksums(void)
{
    unsigned input, bit, shift, tap, mask;
    unsigned char low, high, transformed;
    for (input = 0; input < 256; ++input) {
        checksum_low[input] = checksum_high[input] = 0;
        for (bit = 0; bit < 8; ++bit) {
            low = high = 0;
            /* Tap 15 ends shifted rows; UINT_MAX ends the unshifted row. */
            for (tap = 0; tap < 8; ++tap) {
                shift = feedback_taps[tap] + bit;
                if (shift < 8)
                    low ^= (unsigned char)(input >> shift);
                else if (shift < 16)
                    high ^= (unsigned char)(input >> (shift - 8));
                else
                    break;
            }
            checksum_low[input] |= (low & 1) << bit;
            checksum_high[input] |= (high & 1) << bit;
        }
    }
    for (input = 0; input < 256; ++input) {
        transformed = (unsigned char)input;
        for (bit = 1, mask = 2; bit < 8; ++bit, mask += mask) {
            transformed ^= (transformed * 2) & mask;
            if (bit > 2)
                transformed ^= (transformed * 8) & mask;
        }
        checksum_transform[input] = transformed;
    }
}
