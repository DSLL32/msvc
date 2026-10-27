#include <string.h>
#include "compiler-state.h"
#include <recon-tool.h>
#include <recon.h>
extern int __cdecl skip_newline_comment(void);
extern void __cdecl skip_line_comment(void);
static int comment_cursor(void)
{
    BYTE ordinary[] = "xrest";
    BYTE line[] = "/ discarded\r\ntrailer";
    BYTE block[] = "* block\n comment */trailer";
    c1_bss.options.preprocess_stdout = 0;
    c1_bss.options.preserve_comments = 0;
    c1_data.lexer_flags = 0x100000;
    c1_bss.source_cursor = ordinary;
    if (skip_newline_comment() || c1_bss.source_cursor != ordinary ||
        c1_data.lexer_flags != 0x100000) return 0;
    c1_bss.source_cursor = line;
    if (!skip_newline_comment() || *c1_bss.source_cursor != '\n' ||
        strcmp((const char *)c1_bss.source_cursor + 1, "trailer") ||
        c1_data.lexer_flags != 0x100000) return 0;
    c1_bss.current_context.value = 37;
    c1_bss.source_cursor = block;
    if (!skip_newline_comment() || strcmp((const char *)c1_bss.source_cursor, "trailer") ||
        c1_bss.current_context.value != 38 || c1_data.lexer_flags != 0x100000) return 0;
    return 1;
}
const ReconTest preprocessed_tests[1] = {
    { "comment probe preserves ordinary input, newline position and unrelated lexer flags", 1, comment_cursor }
};
