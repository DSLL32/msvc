/* Injected native branch controls; these callee mocks earn no reconstruction credit. */
#include "macros.h"
#include "macro-expansion.h"
#include "initialization.h"
#include "directives-internal.h"
#include <recon.h>
CompilerOptions options;
LexerTextStorage lexer_text_storage;
char *source_text_cursor;
static int errors;
unsigned char next_directive_character(void)
{
    while (*source_text_cursor == ' ') ++source_text_cursor;
    return (unsigned char)*source_text_cursor++;
}
unsigned char next_source_character(void) { return (unsigned char)*source_text_cursor++; }
int skip_source_comment(void) { return 0; }
unsigned recover_special_character(void) { return 0; }
void fatal_error(unsigned code, ...) { __asm { int 3 } }
void report_syntax_error(unsigned code, ...) { ++errors; }
int __stdcall find_macro_parameter(char *name) { return name[0] == 'x' && !name[1] ? 1 : -1; }
char *__stdcall copy_macro_literal(char *cursor, char *limit, unsigned char delimiter)
{
    __asm { int 3 }
    return 0;
}
int main(int argc, char **argv)
{
    static char workspace[128];
    static unsigned char strict_expected[] = { '#', '@', 1, 1, 0, 0x44 };
    static unsigned char extension_expected[] = { 3, 1, 0, 0x44 };
    static unsigned char string_expected[] = { 2, 1, 0, 0x44 };
    unsigned char *expected;
    unsigned size, count, i;
    int test = argc > 1 ? argv[1][0] : '0';
    options.configuration.extensions_enabled = test == '1';
    macro_formal_count = 1;
    macro_work_buffer = macro_replacement_text = workspace;
    macro_work_bytes = sizeof(workspace);
    source_text_cursor = test == '2' ? "#x\n" : "#@x\n";
    if (test == '0') { expected = strict_expected; size = sizeof(strict_expected); }
    else if (test == '1') { expected = extension_expected; size = sizeof(extension_expected); }
    else { expected = string_expected; size = sizeof(string_expected); }
    count = read_macro_replacement();
    if (errors || count != size || *source_text_cursor != '\n') return 1;
    for (i = 0; i < size; ++i) if ((unsigned char)workspace[i] != expected[i]) return 2;
    return 0;
}
