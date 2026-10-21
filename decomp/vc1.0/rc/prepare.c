/* Prepare the RCPP input, original 0x01007e90..0x010080bf. */
#include "prepare.h"
#include <ctype.h>
#include <string.h>
#pragma intrinsic(strlen)

int input_encoding;
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
    __asm int 3
    return 0;
}
BOOL __stdcall read_source_line(char *line, int capacity, FILE *input)
{
    __asm int 3
    return 0;
}
BOOL __stdcall prefix_matches(char *prefix, char *text)
{
    __asm int 3
    return 0;
}
char * __stdcall trim_token(char *text)
{
    __asm int 3
    return 0;
}
