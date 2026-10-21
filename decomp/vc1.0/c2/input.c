/* Intermediate file input. */
#include "c2.h"
void report_context_error(void);
char *intermediate_error_source = "compiler intermediate";
/* Original [00070120,000701e0). */
FILE *__fastcall open_intermediate(char *prefix, char *suffix, char *mode, char *description)
{
    char filename[260];
    char *destination = filename;
    char *source = prefix;
    FILE *file;
    while ((*destination++ = *source++) != 0) ;
    --destination;
    source = suffix;
    while ((*destination++ = *source++) != 0) ;
    destination = filename;
    while (*destination) ++destination;
    if (destination - filename > 260)
        compiler_internal_error("@(#)ioin.c:1.21", 0xd6);
    file = fopen(filename, mode);
    if (!file) {
        __asm {
            lea eax, filename
            push eax
            mov edx, intermediate_error_source
            mov ecx, 53h
            call report_context_error
            add esp, 4
        }
    }
    return file;
}

#include "input.h"
FILE *input_file;

/* Original [0006fb90,0006fbc0); the public result is in AL. */
unsigned char input_byte(void)
{
    return (unsigned char)getc(input_file);
}

/* Original [0006fca0,0006fd10); result in AX. */
unsigned short input_word(void)
{
    union { unsigned short value; unsigned char bytes[2]; } result;
    result.bytes[0] = (unsigned char)getc(input_file);
    result.bytes[1] = (unsigned char)getc(input_file);
    return result.value;
}

/* Original [0006fea0,0006ff60). */
unsigned long input_dword(void)
{
    union { unsigned long value; unsigned char bytes[4]; } result;
    result.bytes[0] = (unsigned char)getc(input_file);
    result.bytes[1] = (unsigned char)getc(input_file);
    result.bytes[2] = (unsigned char)getc(input_file);
    result.bytes[3] = (unsigned char)getc(input_file);
    return result.value;
}

/* Original [0006fbc0,0006fca0). A 15-bit word, optionally extended to 31 bits. */
unsigned long input_varuint(void)
{
    unsigned char first, second, third, fourth;
    unsigned long value;
    first = (unsigned char)getc(input_file);
    second = (unsigned char)getc(input_file);
    value = first | ((unsigned long)(second & 0x7f) << 8);
    if (second & 0x80) {
        third = (unsigned char)getc(input_file);
        fourth = (unsigned char)getc(input_file);
        value |= ((unsigned long)third << 15) | ((unsigned long)fourth << 23);
    }
    return value;
}

/* Original [00070000,00070060). The size check follows the terminating NUL. */
void __fastcall input_string(char *buffer, unsigned limit)
{
    char *destination = buffer;
    char *end = buffer + limit;
    do {
        *destination = (char)getc(input_file);
    } while (*destination++);
    if (destination > end) compiler_internal_error("@(#)ioin.c:1.21", 0xaa);
}

/* Original [000701e0,00070210). A high bit introduces a second type byte. */
unsigned short input_type(void)
{
    unsigned char first = input_byte();
    if (first & 0x80) return (unsigned short)(((first & 0x7f) << 8) | input_byte());
    return first;
}

/* Original [0006fdb0,0006fea0). Byte 80h introduces a signed dword. */
long input_compact_signed(void)
{
    unsigned char first = (unsigned char)getc(input_file);
    union { long value; unsigned char bytes[4]; } result;
    if (first != 0x80) return (signed char)first;
    result.bytes[0] = (unsigned char)getc(input_file);
    result.bytes[1] = (unsigned char)getc(input_file);
    result.bytes[2] = (unsigned char)getc(input_file);
    result.bytes[3] = (unsigned char)getc(input_file);
    return result.value;
}

/* Original [0006fd10,0006fdb0). Byte 80h introduces a signed word. */
short input_compact_short(void)
{
    unsigned char first = (unsigned char)getc(input_file);
    union { short value; unsigned char bytes[2]; } result;
    if (first != 0x80) return (signed char)first;
    result.bytes[0] = (unsigned char)getc(input_file);
    result.bytes[1] = (unsigned char)getc(input_file);
    return result.value;
}
