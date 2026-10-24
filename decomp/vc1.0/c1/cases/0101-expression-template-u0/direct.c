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
static FrontendSymbol *find_symbol(char *name)
{
    unsigned bucket, count;
    FrontendSymbol *symbol;
    if (!global_scope || global_scope->bucket_mask > 1023)
        fail();
    for (bucket = 0; bucket <= global_scope->bucket_mask; ++bucket)
        for (symbol = global_scope->buckets[bucket], count = 0; symbol; symbol = symbol->next)
        {
            if (++count > 1024)
                fail();
            if (symbol->name && !strcmp(symbol->name, name))
                return symbol;
        }
    fail();
    return NULL;
}
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
typedef struct TemplateInput
{
    ExpressionNode nodes[3];
    Extended extended;
    unsigned char text[8];
} TemplateInput;
static TemplateInput input;
static void at_marker(void)
{
    CType *owners[3];
    FrontendSymbol *symbol;
    unsigned unit = 0, i, consumed = 20, count = 1, zero;
    unsigned before, after;
    unsigned short cb, ca;
    unsigned char input_before[80], expected[80], type_before[3][16], shape_before[12];
    ArenaBlock *oldfree, *free_next;
    ArenaPool *pool = &compiler_arena.pools[2];
    unsigned char *slot;
    IrToken *result;
    if (sizeof(input) != 80 || pool->first || pool->last || pool->next || pool->remaining ||
        pool->field10 || compiler_arena.block_bytes != 0x3ff4 ||
        !(oldfree = compiler_arena.free_blocks) || !oldfree->bytes || oldfree->bytes > 0x3ff4)
        fail();
    owners[0] = type_0064;
    owners[1] = type_0108;
    symbol = find_symbol("template_text");
    owners[2] = symbol->type;
    if (!owners[0] || !owners[1] || !owners[2] || owners[0]->flags != 0x64 ||
        owners[1]->flags != 0x108 || owners[0]->attributes || owners[1]->attributes ||
        owners[0]->related || owners[1]->related || owners[2]->flags != 0x41 ||
        owners[2]->attributes != 0x10 || !owners[2]->related || owners[2]->related->flags != 2 ||
        owners[2]->related->attributes != 1 || owners[2]->related->related ||
        owners[2]->related->field08 != 5)
        fail();
    for (i = 0; i < 3; ++i)
        memcpy(type_before[i], owners[i], 16);
    memcpy(shape_before, owners[2]->related, 12);
    memset(&input, 0, 80);
    for (i = 0; i < 3; ++i)
    {
        input.nodes[i].kind = TOK_INT_CONSTANT;
        input.nodes[i].flags = 3;
        input.nodes[i].type = owners[0];
        input.nodes[i].value.words.low = 17 + i;
    }
    if (unit == 2 || unit == 3)
    {
        input.nodes[0].kind = unit == 2 ? TOK_UNARY_MINUS : TOK_PLUS;
        input.nodes[0].flags = unit == 2 ? 1 : 2;
        input.nodes[0].value.children.left = (IrToken *)&input.nodes[1];
        input.nodes[0].value.children.right = unit == 3 ? (IrToken *)&input.nodes[2] : NULL;
        input.nodes[1].auxiliary = (unsigned)&input;
        if (unit == 3)
            input.nodes[2].auxiliary = (unsigned)&input;
    }
    else if (unit == 4)
    {
        IrToken *s = (IrToken *)&input;
        s->kind = 3;
        s->flags = 5;
        s->type = owners[2];
        s->value.text = (char *)input.text;
        s->length = 5;
        memcpy(input.text, "iris", 5);
    }
    else if (unit == 5)
    {
        input.nodes[0].kind = TOK_DOUBLE_CONSTANT;
        input.nodes[0].type = owners[1];
        input.nodes[0].value.words.low = (unsigned)&input.extended;
        input.extended.high = 0xc0000000;
        input.extended.exponent = 0x3fff;
        input.extended.format = 8;
    }
    free_next = oldfree->next;
    memcpy(input_before, &input, 80);
    slot = oldfree->data;
    memcpy(expected, slot, 80);
    zero = oldfree->bytes < 80 ? oldfree->bytes : 80;
    memset(expected, 0, zero);
    if (!unit)
    {
        expected[0] = 7;
        expected[1] = 0x8b;
        memcpy(expected + 4, &owners[0], 4);
        memset(expected + 12, 0, 4);
    }
    else
    {
        memcpy(expected, &input, count * 20);
        if (unit == 2 || unit == 3)
        {
            *(unsigned *)(expected + 12) = (unsigned)slot + 20;
            *(unsigned *)(expected + 28) = (unsigned)slot;
            if (unit == 3)
            {
                *(unsigned *)(expected + 16) = (unsigned)slot + 40;
                *(unsigned *)(expected + 48) = (unsigned)slot;
            }
        }
        else if (unit == 4)
        {
            *(unsigned *)(expected + 12) = (unsigned)slot + 20;
            memcpy(expected + 20, "iris", 5);
        }
        else if (unit == 5)
        {
            *(unsigned *)(expected + 12) = (unsigned)slot + 20;
            memcpy(expected + 20, &input.extended, 12);
        }
    }
    __asm { mov before,esp }
    __asm { fnstcw cb }
    if (!unit)
        result = make_reusable_zero();
    else
        result = copy_expression_tree((IrToken *)&input, unit >= 4 ? 2 : 6);
    __asm { mov after,esp }
    __asm { fnstcw ca }
    if (before != after || cb != ca || result != (IrToken *)slot || memcmp(slot, expected, 80) ||
        memcmp(&input, input_before, 80) || compiler_arena.free_blocks != free_next ||
        pool->first != oldfree || pool->last != oldfree || pool->next != slot + consumed ||
        pool->remaining != 0x3ff4 - consumed || pool->field10)
        fail();
    for (i = 0; i < 3; ++i)
        if (memcmp(type_before[i], owners[i], 16))
            fail();
    if (memcmp(shape_before, owners[2]->related, 12))
        fail();
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
    result = recon_native_at_entry((const void *)finish_declarations, (const void *)at_marker,
                                   invoke_main, NULL);
    /* External harness completion, not a target exit: preserve the captured
       paused-context disk bytes without unrelated atexit/stdio flushing. */
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group, result == 1 ? 0 : 1);
    return 0;
}
