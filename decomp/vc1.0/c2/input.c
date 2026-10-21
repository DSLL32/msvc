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
