#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../src/asm.h"
#include "../../src/pch.h"
#include "../../src/browser.h"
#include "../../src/expressions.h"
#include "../../src/declaration-text.h"
#include "../../src/debug-info.h"
#include "../../src/literal-names.h"
#include "../../src/initializer-records.h"
#include <recon-native.h>
#include <recon.h>
extern int main(int, char **, char **);
extern unsigned char asm_parser_control[16];
extern IrToken *make_reusable_zero(void);
extern IrToken *__stdcall copy_expression_tree(IrToken *, int);
static void fail(void) { recon_native_entry_finish(0); }

static void invoke_main(void *unused)
{
    char *arguments[2] = {"C13232.EXE", NULL};
    (void)unused;
    if (syscall(SYS_dup2, 1, 2) != 2)
        syscall(SYS_exit_group, 89);
    main(1, arguments, (char **)0);
}
/* Defined pending-stream oracle, packed from the frozen original into capture.bin.
 * The extra fixture-owned input is not a target artifact or an observer log. */
static unsigned char pending_oracle[65536];
static unsigned pending_bytes, pending_cursor, pending_index, pending_groups;
static unsigned pending_word(void)
{
    unsigned value;
    if (pending_cursor > pending_bytes || pending_bytes - pending_cursor < 4)
        fail();
    memcpy(&value, pending_oracle + pending_cursor, 4);
    pending_cursor += 4;
    return value;
}
static int load_pending(void)
{
    int fd;
    char extra;
    fd = syscall(SYS_open, ".recon-pending.bin", 0, 0);
    if (fd < 0)
        return 0;
    pending_bytes = syscall(SYS_read, fd, pending_oracle, sizeof(pending_oracle));
    if (pending_bytes < 8 || pending_bytes >= sizeof(pending_oracle) ||
        syscall(SYS_read, fd, &extra, 1) != 0)
    {
        syscall(SYS_close, fd);
        return 0;
    }
    syscall(SYS_close, fd);
    pending_cursor = 0;
    if (pending_word() != 0x31444e50)
        return 0;
    pending_groups = pending_word();
    return pending_groups > 0;
}
static void pending(char *when)
{
    FILE *streams[7];
    unsigned i, count, want;
    (void)when;
    streams[0] = expression_file;
    streams[1] = symbol_file;
    streams[2] = line_file;
    streams[3] = global_file;
    streams[4] = debug_file;
    streams[5] = pch_file;
    streams[6] = browser_output;
    if (pending_index >= pending_groups)
        fail();
    for (i = 0; i < 7; ++i)
    {
        want = pending_word();
        if (want == 0xffffffffU)
        {
            if (streams[i])
                fail();
            continue;
        }
        if (!streams[i])
            fail();
        count = streams[i]->_ptr - streams[i]->_base;
        if (count > 4096 || count != want || want > pending_bytes - pending_cursor ||
            memcmp(streams[i]->_base, pending_oracle + pending_cursor, want))
            fail();
        pending_cursor += want;
    }
    if (++pending_index == pending_groups && pending_cursor != pending_bytes)
        fail();
}
static FrontendSymbol *symbol;
static void check_owner(unsigned after)
{
    CType *type = symbol->type;
    TypeShape *shape = type->related;
    if (!symbol || !symbol->name || !type)
        fail();
    if (after == 0 &&
        (symbol->kind != 9 || symbol->attributes != 0x8004 || symbol->references != 0 ||
         type->flags != 0x108 || type->attributes != 0x1010 || !!shape != 0 ||
         (shape ? shape->flags : 0) != 0x0 || (shape ? shape->attributes : 0) != 0x0 ||
         options.debug_info != 1 || error_count != 0 || based_declarator_symbol != NULL))
        fail();
    if (after == 1 &&
        (symbol->kind != 9 || symbol->attributes != 0x8004 || symbol->references != 0 ||
         type->flags != 0x108 || type->attributes != 0x1010 || !!shape != 0 ||
         (shape ? shape->flags : 0) != 0x0 || (shape ? shape->attributes : 0) != 0x0 ||
         options.debug_info != 1 || error_count != 0 || based_declarator_symbol != NULL))
        fail();
}
static void __stdcall at_marker(FrontendSymbol *actual, IrToken *token, FILE *stream)
{
    unsigned sp0, sp1, depth = type_description_depth;
    unsigned short cw0, cw1, value;
    unsigned result = 0;
    CType *returned;
    IrToken *leaf;
    FrontendSymbol *materialized;
    char *old_interned;
    unsigned char token_before[20], literal_before[12], symbol_before[44];
    FILE *line = line_file;
    if (!actual || strcmp(actual->name, "support_double") || stream || !token || !line ||
        auxiliary_ir_stream || *(unsigned short *)token != 0x0305 || token->type->flags != 0x0108 ||
        actual->index != 109)
        fail();
    symbol = actual;
    check_owner(0);
    pending("before");
    memcpy(token_before, token, 20);
    memcpy(literal_before, token->value.pointer, 12);
    memcpy(symbol_before, symbol, 44);
    __asm { mov sp0,esp }
    __asm {fnstcw cw0} emit_literal_initializer(symbol, token);
    __asm { mov sp1,esp }
    __asm { fnstcw cw1 }
    if (sp0 != sp1 || cw0 != cw1 || type_description_depth != depth ||
        memcmp(token_before, token, 20) || memcmp(literal_before, token->value.pointer, 12) ||
        memcmp(symbol_before, symbol, 44) || auxiliary_ir_stream != line)
        fail();
    check_owner(1);
    pending("after");
    recon_native_entry_finish(1);
}

int c1_direct(void)
{
    int result;
    if (!load_pending())
    {
        recon_record_terminal_status(1);
        syscall(SYS_exit_group, 1);
    }
    result = recon_native_at_entry((const void *)write_initializer_value, (const void *)at_marker,
                                   invoke_main, NULL);
    /* External harness completion, not a target exit: preserve the captured
       paused-context disk bytes without unrelated atexit/stdio flushing. */
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group, result == 1 ? 0 : 1);
    return 0;
}
