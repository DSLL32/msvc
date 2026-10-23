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

void read_global_symbols(void)
{
    c2_current_il = c2_globals_input;
    read_symbols(NULL);
    fclose(c2_globals_input);
}
