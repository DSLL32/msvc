#include "c2.h"
#include <recon.h>

#define IL_GETC(refill) (--c2_current_il->_cnt >= 0 \
    ? (unsigned char)*c2_current_il->_ptr++ : refill(c2_current_il))

int c2_read_il_byte(void)
{
    /* The target expands getc instead of calling the locked CRT entry. */
    if (--c2_current_il->_cnt >= 0)
        return (unsigned char)*c2_current_il->_ptr++;
    return _filbuf(c2_current_il);
}

unsigned short c2_read_il_word(void)
{
    union { unsigned short word; unsigned char bytes[2]; } value;
    int (__cdecl *refill)(FILE *) = _filbuf;

    value.bytes[0] = (unsigned char)IL_GETC(refill);
    value.bytes[1] = (unsigned char)IL_GETC(refill);
    return value.word;
}

long c2_read_il_long(void)
{
    union { long number; unsigned char bytes[4]; } value;
    int (__cdecl *refill)(FILE *) = _filbuf;

    value.bytes[0] = (unsigned char)IL_GETC(refill);
    value.bytes[1] = (unsigned char)IL_GETC(refill);
    value.bytes[2] = (unsigned char)IL_GETC(refill);
    value.bytes[3] = (unsigned char)IL_GETC(refill);
    return value.number;
}

/* Ordinary IL uses a 15-bit word with a two-byte extension. The older
 * extended-header encoding instead packs 6+8+8 bits and reserves the short
 * range 3fff80..3fffff for single-byte values. Neither form diagnoses EOF. */
unsigned input_varuint(void)
{
    unsigned first, second, third, fourth, value;
    int (__cdecl *refill)(FILE *) = _filbuf;

    if (c2_il_extended_header) {
        first = (unsigned char)IL_GETC(refill);
        if (!(first & 0x80))
            return first + 0x3fff80;
        second = (unsigned char)IL_GETC(refill);
        value = (first & 0x3f) | (second << 6);
        if (first & 0x40) {
            third = (unsigned char)IL_GETC(refill);
            value |= third << 14;
        }
        return value;
    }

    first = (unsigned char)IL_GETC(refill);
    second = (unsigned char)IL_GETC(refill);
    value = first | ((second & 0x7f) << 8);
    if (second & 0x80) {
        third = (unsigned char)IL_GETC(refill);
        fourth = (unsigned char)IL_GETC(refill);
        value |= (third << 15) | (fourth << 23);
    }
    return value;
}

void __fastcall input_string(char *buffer, unsigned capacity)
{
    char *destination = buffer;
    char *end = buffer + capacity;
    int (__cdecl *refill)(FILE *) = _filbuf;

    /* The original checks capacity after reading the terminating NUL. */
    do {
        *destination = (char)IL_GETC(refill);
    } while (*destination++);
    if (destination > end)
        c2_report_internal("E:\\utc\\src\\\\Common\\ioin.c", 0x126);
}

void read_global_symbols(void)
{
    c2_current_il = c2_globals_input;
    read_symbols(NULL);
    fclose(c2_globals_input);
}
