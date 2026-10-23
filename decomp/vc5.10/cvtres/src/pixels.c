#include "cvtres.h"
#include <recon.h>

void __stdcall convert_rows(DWORD *palette, WORD bits, DWORD rows,
    DWORD input_stride, DWORD output_stride, BYTE **input, BYTE **output)
{
    BYTE *source = *input, *destination = *output;
    DWORD pixels, row, x, value;
    if (rows) {
        pixels = (input_stride << 5) / bits;
        for (row = 0; row < rows; ++row) {
            for (x = 0; x < pixels; ++x) {
                value = read_pixel(source, x, bits);
                if (bits == 24) value = color_to_gray(value);
                else if (bits == 8 || bits == 4) value = palette[value];
                write_pixel(destination, x, (BYTE)value);
            }
            source += input_stride * 4;
            destination += output_stride * 4;
        }
    }
    *input = source;
    *output = destination;
}

unsigned long __stdcall read_pixel(const BYTE *row, unsigned long index, WORD bits)
{
    unsigned long shift;
    switch (bits) {
    case 24:
        row += index * 3;
        return row[0] | ((DWORD)row[1] << 8) | ((DWORD)row[2] << 16);
    case 8: return row[index];
    case 4: return (index & 1) ? row[index >> 1] & 15 : row[index >> 1] >> 4;
    case 2:
        shift = (3 - (index & 3)) * 2;
        return (row[index >> 2] & (3 << shift)) >> shift;
    case 1:
        shift = index & 7;
        return (row[index >> 3] & (1 << shift)) >> shift;
    }
    /* Original fallthrough leaves AX from bits and residual high EAX. No
     * unsupported-depth consumer has been validated. Stop native execution
     * instead of assigning a result to that unvalidated original behavior. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
    __builtin_debugtrap();
#endif
}

unsigned long __stdcall color_to_gray(unsigned long color)
{
    unsigned long kind = color >> 24;
    if (color == 0 || color == 0x01000000) return 0;
    if (color == 0x555555 || color == 0x808080 ||
        color == 0x01555555 || color == 0x01808080) return 1;
    if (color == 0xaaaaaa || color == 0x01aaaaaa) return 2;
    if (color == 0xffffff || color == 0x01ffffff) return 3;
    if (kind <= 1)
        return ((color & 255) * 90 + ((color >> 8) & 255) * 177 +
                ((color >> 16) & 255) * 33 + 12750) / 25500;
    if (kind == 2 && (color & 0xffffff) < 4) return color & 0xffffff;
    return 0xffffffff;
}

void __stdcall write_pixel(BYTE *row, unsigned long index, BYTE value)
{
    if (windows_ce)
        row[index >> 2] |= (BYTE)(value << (6 - (index & 3) * 2));
    else if (index & 1) row[index >> 1] |= value & 15;
    else row[index >> 1] |= (BYTE)(value << 4);
}
