#include "c2.h"
#include <recon.h>

#define IL_GETC(refill) (--c2_current_il->_cnt >= 0 \
    ? (unsigned char)*c2_current_il->_ptr++ : refill(c2_current_il))

void __fastcall read_il_block(int bytes, char *buffer)
{
    if (bytes <= 0) return;
    do {
        *buffer++ = (char)IL_GETC(_filbuf);
    } while (--bytes);
}

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

/* 80 escapes the signed-byte range and is followed by a complete DWORD. */
long input_compact_signed(void)
{
    unsigned char first;
    union { long number; unsigned char bytes[4]; } value;
    int (__cdecl *refill)(FILE *) = _filbuf;

    first = (unsigned char)IL_GETC(refill);
    if (first != 0x80)
        return (signed char)first;
    value.bytes[0] = (unsigned char)IL_GETC(refill);
    value.bytes[1] = (unsigned char)IL_GETC(refill);
    value.bytes[2] = (unsigned char)IL_GETC(refill);
    value.bytes[3] = (unsigned char)IL_GETC(refill);
    return value.number;
}

/* The short form shares the signed-byte escape with the DWORD reader. */
short input_compact_short(void)
{
    unsigned char first;
    union { short number; unsigned char bytes[2]; } value;
    int (__cdecl *refill)(FILE *) = _filbuf;

    first = (unsigned char)IL_GETC(refill);
    if (first != 0x80)
        return (signed char)first;
    value.bytes[0] = (unsigned char)IL_GETC(refill);
    value.bytes[1] = (unsigned char)IL_GETC(refill);
    return value.number;
}

unsigned short input_type(void)
{
    unsigned char first = (unsigned char)c2_read_il_byte();
    if (first & 0x80)
        return (unsigned short)(((first & 0x7f) << 8) |
                                (unsigned char)c2_read_il_byte());
    return first;
}

void read_global_symbols(void)
{
    c2_current_il = c2_globals_input;
    read_symbols(NULL);
    fclose(c2_globals_input);
}

void read_initializers(void)
{
    read_initializer_records(c2_input);
}


void __fastcall read_initializer_records(FILE *input)
{
    C2InitRecord *record;
    FILE memory_file, *saved_input;
    char *source;
    unsigned bytes;
    int kind;

    c2_current_il = input;
    for (;;) {
        kind = (signed char)c2_read_il_byte();
        switch (kind) {
        case -1:
        case 15:
            return;
        case 0:
            record = new_initializer(kind);
            record->symbol_serial = input_varuint();
            record->offset = input_compact_signed();
            read_initializer_values(record);
            queue_initializer(record);
            break;
        case 10:
            record = new_initializer(kind);
            record->symbol_serial = 0;
            record->offset = input_compact_signed();
            read_initializer_values(record);
            emit_initializer(record);
            free_initializer(record);
            break;
        case 9:
            input_varuint();
            break;
        case 12:
            c2_read_il_byte();
            break;
        case 14:
            input_string(symbol_name, sizeof(symbol_name));
            source = lookup_source_file(symbol_name, &bytes);
            if (!source)
                c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 0x89d);
            saved_input = c2_current_il;
            /* The other FILE fields are deliberately uninitialized. */
            memory_file._ptr = source;
            memory_file._cnt = bytes;
            memory_file._flag = 1;
            memory_file._file = -1;
            read_initializer_records(&memory_file);
            free(source);
            c2_current_il = saved_input;
            break;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 0x8ad);
            break;
        }
    }
}
