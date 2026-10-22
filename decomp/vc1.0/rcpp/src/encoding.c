/* Byte/word input classification predating the Win32 Unicode text helpers. */
#include "lexer.h"
#include "legacy-ctype.h"
#include <recon.h>
/* Original [01009fa0,0100a104). Sample 40-byte blocks until printable bytes
 * establish single-byte text or alternating NULs establish a wide encoding.
 * The stream is restored to its original physical position in every case. */
int __stdcall rc_detect_encoding(FILE *stream)
{
    char sample[40];
    int total = 0, readable = 0, even_zero = 0, odd_zero = 0;
    int count, i, limit, result = 0;
    int byte_order = rc_byte_order();
    long original_position = ftell(stream);
    if (original_position % 2) fgetc(stream);
    do {
        count = fread(sample, 1, sizeof(sample), stream);
        for (i = 0; i < count; ++i) {
            if (!sample[i]) {
                if (i % 2) ++odd_zero;
                else ++even_zero;
            }
            if (rc_native_isprint((signed char)sample[i]) || sample[i] == '\t' ||
                sample[i] == '\n' || sample[i] == '\r') ++readable;
        }
        total += count;
        limit = (total * 4) / 5;
        if (readable > limit) result = 1;
        else if (odd_zero > limit / 2) result = byte_order == 1 ? 2 : 3;
        else if (even_zero > limit / 2) result = byte_order == 1 ? 3 : 2;
    } while (count == sizeof(sample) && !result);
    fseek(stream, original_position, SEEK_SET);
    return result;
}

/* Original [0100a110,0100a13c). Both outcomes of its nominal endian probe
 * return one in this x86 binary; preserve that behavior, including the typo. */
int rc_byte_order(void)
{
    return 1;
}
