/* Input state and character scanning for resource tokens. */
#include "lexer.h"
#include <string.h>
#pragma intrinsic(memset)

struct LexOperator operator_tokens[] = {
    { ',', TOKEN_COMMA }, { '|', TOKEN_OR },
    { '(', TOKEN_LPAREN }, { ')', TOKEN_RPAREN },
    { '{', TOKEN_BEGIN }, { '}', TOKEN_END },
    { '~', TOKEN_NOT }, { '+', TOKEN_PLUS }, { '-', TOKEN_MINUS },
    { '&', TOKEN_AND }, { '=', TOKEN_EQUALS }, { 127, TOKEN_EOF }, { 0, 0 }
};
unsigned char token_delimiters[128];
char source_character, source_filename[MAX_PATH];
FILE *lexer_input;
int source_line, lexer_status, token_attributes;
static char source_buffer[4097];
static int buffer_position = sizeof(source_buffer);
static int start_of_line = 1;

/* Original 0x01008300..0x0100839a. */
BOOL __stdcall initialize_lexer(FILE *input)
{
    struct LexOperator *operator;
    token_attributes = 0;
    source_line = 1;
    lexer_status = 0;
    lexer_input = input;
    memset(token_delimiters, 1, 33);
    memset(token_delimiters + 33, 0, 94);
    token_delimiters[127] = 1;
    for (operator = operator_tokens; operator->spelling; ++operator)
        token_delimiters[operator->spelling] = 1;
    token_delimiters[';'] = token_delimiters['"'] = token_delimiters['\''] = 1;
    advance_character();
    return TRUE;
}

/* Original 0x010083c0..0x010083fb. */
int __cdecl advance_character(void)
{
    if (read_source_character() != TOKEN_EOF && source_character == ';')
        while (read_source_character() != TOKEN_EOF && source_character != '\n')
            ;
    return (unsigned char)source_character;
}

/* Original 0x01008400..0x0100846d. */
int __cdecl read_buffered_character(void)
{
    int received, character;
    if (buffer_position >= sizeof(source_buffer)) {
        buffer_position = 0;
        received = read_file(lexer_input, source_buffer, sizeof(source_buffer));
        if (received < sizeof(source_buffer)) {
            fclose(lexer_input);
            source_buffer[received] = 0;
        }
    }
    character = source_buffer[buffer_position];
    if (character)
        ++buffer_position;
    return character;
}

/* Original 0x01008470..0x010086a5. */
int __cdecl read_source_character(void)
{
    char *filename;
    int skip_header = 0, extension = 0, lower;
    for (;;) {
        source_character = (char)read_buffered_character();
        switch (source_character) {
        case 0:
            source_character = TOKEN_EOF;
            return TOKEN_EOF;
        case '\t': case ' ':
            if (skip_header)
                continue;
            return source_character;
        case '\n':
            start_of_line = 1;
            ++source_line;
            if (skip_header)
                continue;
            return source_character;
        case '\r':
            source_character = ' ';
            if (skip_header)
                continue;
            return ' ';
        case '#':
            if (!start_of_line)
                break;
            extension = 0;
            if (read_buffered_character() != 'l' || read_buffered_character() != 'i' ||
                read_buffered_character() != 'n' || read_buffered_character() != 'e')
                lexical_error("Invalid directive in preprocessed RC file", "");
            source_line = 0;
            do {
                if (source_character >= '0' && source_character <= '9')
                    source_line = source_line * 10 + source_character - '0';
                source_character = (char)read_buffered_character();
            } while (source_character != '"' && source_character != '\n');
            if (source_character == '\n')
                continue;
            filename = source_filename;
            do {
                source_character = (char)read_buffered_character();
                lower = rc_tolower(source_character);
                switch (lower) {
                case '\n':
                    lexical_error("Invalid directive in preprocessed RC file", "");
                    break;
                case '"':
                    break;
                case '.':
                    extension = 1;
                    *filename++ = source_character;
                    break;
                case 'c': case 'h':
                    skip_header = extension;
                    extension = 0;
                    *filename++ = source_character;
                    break;
                default:
                    skip_header = extension = 0;
                    *filename++ = source_character;
                    break;
                }
            } while (source_character != '"');
            *filename = 0;
            do {
                source_character = (char)read_buffered_character();
            } while (source_character != '\n');
            continue;
        }
        start_of_line = 0;
        if (!skip_header)
            return source_character;
    }
}

void __stdcall lexical_error(char *format, char *context)
{
    __asm int 3
}
