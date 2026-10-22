/* Prepare the RCPP input, original 0x01007e90..0x010080bf. */
#include "prepare.h"
#include <ctype.h>
#include <string.h>
#pragma intrinsic(strlen)

int input_encoding;
int __cdecl native_byte_order(void);
BOOL __stdcall prepare_input(char *filename)
{
    FILE *input, *output;
    char line[512], escaped[MAX_PATH * 2], formatted[4096];
    char *next, *content;
    int number = 0, resynchronize = 1;

    input = fopen(filename, "r");
    if (!input)
        return FALSE;
    output = fopen(temporary_input, "w+b");
    next = escaped;
    while (*filename) {
        if (*filename == '\\')
            *next++ = '\\';
        *next++ = *filename++;
    }
    *next = 0;
    sprintf(formatted, "#line 1\"%s\"\r\n", escaped);
    write_file(output, formatted, strlen(formatted));
    input_encoding = detect_encoding(input);
    while (read_source_line(line, sizeof(line), input)) {
        ++number;
        content = line;
        while (*content && isspace(*content))
            ++content;
        if (prefix_matches("rcinclude", content)) {
            content = trim_token(content + 9);
            write_file(output, "#include \"", 10);
            write_file(output, content, strlen(content));
            write_file(output, "\"\r\n", 3);
        } else if (!*content)
            resynchronize = 1;
        else {
            if (resynchronize) {
                sprintf(formatted, "#line %d\r\n", number);
                write_file(output, formatted, strlen(formatted));
                resynchronize = 0;
            }
            write_file(output, content, strlen(content));
            write_file(output, "\r\n", 2);
        }
    }
    fclose(output);
    fclose(input);
    return TRUE;
}

int __stdcall detect_encoding(FILE *input)
{
    char sample[40];
    int odd_zeros = 0, even_zeros = 0, printable = 0, total = 0;
    int encoding = 0, order = native_byte_order(), length, index, threshold;
    long position = ftell(input);

    if (position % 2)
        fgetc(input);
    do {
        length = fread(sample, 1, sizeof(sample), input);
        for (index = 0; index < length; ++index) {
            if (!sample[index]) {
                if (index % 2)
                    ++odd_zeros;
                else
                    ++even_zeros;
            }
            if ((unsigned short)__mb_cur_max > 1 ?
                _isctype(sample[index], 0x157) : (_pctype[sample[index]] & 0x157))
                ++printable;
        }
        total += length;
        threshold = total * 4 / 5;
        if (printable > threshold)
            encoding = 1;
        else if (odd_zeros > threshold / 2)
            encoding = order == 1 ? 2 : 3;
        else if (even_zeros > threshold / 2)
            encoding = order == 1 ? 3 : 2;
    } while (length == sizeof(sample) && !encoding);
    fseek(input, position, SEEK_SET);
    return encoding;
}
BOOL __stdcall read_source_line(char *line, int capacity, FILE *input)
{
    char *next = line;
    int remaining = capacity - 1, character = 0, spaces, index;
    while (remaining) {
        character = fgetc(input);
        if (character == EOF || character == '\n')
            break;
        if (character == '\t') {
            spaces = 8 - ((next - line) & 7);
            if (spaces >= remaining)
                spaces = remaining;
            remaining -= spaces;
            for (index = spaces; index-- > 0; )
                *next++ = ' ';
            /* The original decrements the exhausted loop counter once more,
             * then subtracts it, restoring one unit of remaining capacity. */
            ++remaining;
        } else if (character != '\r') {
            *next++ = (char)character;
            --remaining;
        }
    }
    *next = 0;
    return character != EOF || next != line;
}
BOOL __stdcall prefix_matches(char *prefix, char *text)
{
    while (*prefix) {
        if (!*text || rc_toupper(*prefix) != rc_toupper(*text))
            return 0;
        ++prefix;
        ++text;
    }
    return -1;
}

/* Original 0x01007d30..0x01007dc5. Retain only the first bare filename. */
char * __stdcall trim_token(char *text)
{
    char *start;
    while (*text && isspace(*text))
        ++text;
    start = text;
    while (*text && !isspace(*text))
        ++text;
    *text = 0;
    return start;
}

/* Original 0x010082d0..0x010082fc. Both compiled branches return one. */
int __cdecl native_byte_order(void)
{
    return 1;
}
