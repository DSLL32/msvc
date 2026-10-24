/* Test-only fixed residual interfaces at the genuine parsed seven-argument call. */
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../src/pch-relocate.h"
#include "../../src/expressions.h"
#include "../../src/browser.h"
#include "../../src/debug-info.h"
#include "../../src/declaration-text.h"
#include "../../src/floating.h"
#include <recon-native.h>
#include <recon.h>

static unsigned invoked, selected_unit = 5;
static void fail(void) { recon_native_entry_finish(0); }

static int permanent(void *pointer, unsigned bytes)
{
    unsigned address = (unsigned)pointer, start, i;
    if (!address || address > 0xffffffffU - bytes)
        return 0;
    for (i = 0; i < (unsigned)permanent_page_count; ++i)
    {
        start = (unsigned)permanent_pages[i].start;
        if (address >= start && address - start <= 0xeffc && bytes <= 0xeffc - (address - start))
            return 1;
    }
    return 0;
}
static void temporary(void *pointer, unsigned bytes, unsigned pool_index)
{
    ArenaPool *pool = &compiler_arena.pools[pool_index];
    ArenaBlock *block = pool->first;
    unsigned i, count = 0, used, owned = 0, address = (unsigned)pointer, start;
    if (!address || address > 0xffffffffU - bytes)
        fail();
    while (block)
    {
        if (++count > 32 || block->bytes > 0x3ff4)
            fail();
        used = block == pool->last ? (unsigned)pool->next - (unsigned)block->data : block->bytes;
        if (used > 0x3ff4 ||
            (block == pool->last && (block->next || used + (unsigned)pool->remaining != 0x3ff4)))
            fail();
        start = (unsigned)block->data;
        if (address >= start && address - start <= used && bytes <= used - (address - start))
            owned = 1;
        if (!block->next && block != pool->last)
            fail();
        block = block->next;
    }
    if (!owned)
        fail();
    for (i = 0; i < (unsigned)permanent_page_count; ++i)
    {
        start = (unsigned)permanent_pages[i].start;
        if (address < start + 0xeffc && address + bytes > start)
            fail();
    }
}
static FrontendSymbol *global_symbol(char *name)
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
    return 0;
}
static Extended *number_owner(IrToken *value)
{
    unsigned level;
    ExpressionNode *node;
    Extended *number;
    for (level = 0; level < 4; ++level)
    {
        temporary(value, 20, 2);
        if ((value->flags & 7) == 3)
        {
            if (value->kind < 4 || value->kind > 6)
                fail();
            number = value->value.pointer;
            temporary(number, 12, 2);
            if (number->format != 8)
                fail();
            return number;
        }
        if ((value->flags & 7) != 1 ||
            (value->kind != 60 && value->kind != 99 && value->kind != 89))
            fail();
        node = (ExpressionNode *)value;
        value = node->value.children.left;
    }
    fail();
    return 0;
}
static void owners(void)
{
    unsigned i, j, start, used;
    ArenaBlock *block;
    ArenaPool *pool = &compiler_arena.pools[2];
    for (i = 0; i < (unsigned)permanent_page_count; ++i)
    {
        start = (unsigned)permanent_pages[i].start;
        if (!start || (start & 4095) || start > 0xffffffffU - 0xeffc ||
            !permanent_pages[i].relocations || permanent_pages[i].relocations->bit_count != 0xeffc)
            fail();
        for (j = 0; j < i; ++j)
            if (start < (unsigned)permanent_pages[j].start + 0xeffc &&
                start + 0xeffc > (unsigned)permanent_pages[j].start)
                fail();
    }
    for (block = pool->first, i = 0; block; block = block->next, ++i)
    {
        if (i >= 32)
            fail();
        used = block == pool->last ? (unsigned)pool->next - (unsigned)block->data : block->bytes;
    }
}
static void numeric_workspace(char *when)
{
    (void)when;
    fail(); /* No numeric workspace use in this fixed nonnumeric unit. */
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
void __stdcall direct_at_call(IrToken *call)
{
    ArgumentNode *nodes[7], *node;
    ExpressionNode *source, *source_leaf, *scalar, *result;
    FrontendSymbol *symbol;
    CType *base, *self;
    unsigned i, depth, before, after;
    unsigned short modifier, control_before, control_after;
    unsigned node_bytes[7][5], source_bytes[5], record_bytes[11];
    FrontendSymbol *record, *tag;
    Extended *number;
    unsigned char output[48], number_before[12], input10[32], expected[12];
    unsigned status, number_index, output_bytes;
    if (selected_unit == 3)
    {
        finish_call_arguments(call);
        return;
    }
    if (invoked++ || !call_context || call_context->argument_count != 7 || !call_context->symbol ||
        strcmp(call_context->name, "residual_sink") ||
        strcmp(call_context->symbol->name, "residual_sink") || call->kind != TOK_ARGUMENT_END ||
        call->flags != 2 || compiler_arena.block_bytes != 0x3ff4 || permanent_page_count < 1 ||
        permanent_page_count > 511)
        fail();
    temporary(call, 20, 2);
    temporary(call_context, 36, 2);
    node = (ArgumentNode *)((ExpressionNode *)call)->value.children.right;
    for (i = 0; i < 7; ++i)
    {
        temporary(node, 20, 2);
        if (node->kind != TOK_ARGUMENT || node->flags != 2 || !permanent(node->type, 16) ||
            !node->expression)
            fail();
        nodes[i] = node;
        memcpy(node_bytes[i], node, 20);
        node = node->previous;
    }
    if (node)
        fail();
    for (i = 0; i < 7; ++i)
        if (nodes[i]->next != nodes[(i + 6) % 7])
            fail();
    if (selected_unit >= 4)
    {
        if (selected_unit > 12)
            fail();
        number_index = selected_unit == 10 || selected_unit == 11 ? 1 : selected_unit == 12 ? 0 : 2;
        number = number_owner(nodes[number_index]->expression);
        memcpy(number_before, number, 12);
        memset(output, 0xa5, 48);
        memset(input10, 0xa5, 32);
        owners();

        if (selected_unit >= 6)
            numeric_workspace("before");
        if (selected_unit == 7)
        {
            if (extended_to_ten_bytes(number, input10) || memcmp(input10, number, 10))
                fail();
            for (i = 10; i < 32; ++i)
                if (input10[i] != 0xa5)
                    fail();
            numeric_workspace("prepared");
        }
        pending("before");
        __asm { mov before,esp }
        __asm { fnstcw control_before }
        if (selected_unit == 4)
            status = extended_format(number);
        else if (selected_unit == 5)
            status = copy_extended(number, (Extended *)output);
        else if (selected_unit == 6)
            status = extended_to_ten_bytes(number, output);
        else if (selected_unit == 7)
            status = ten_bytes_to_extended(input10, (Extended *)output);
        else if (selected_unit == 8 || selected_unit == 10 || selected_unit == 12)
            status = extended_to_unsigned(number, (unsigned *)output);
        else
            status = round_extended_to_integer(number, (int *)output);
        __asm { mov after,esp }
        __asm { fnstcw control_after }
        if (before != after || control_before != control_after ||
            status != (selected_unit == 4    ? 8
                       : selected_unit == 10 ? 1
                                             : 0) ||
            memcmp(number_before, number, 12))
            fail();
        output_bytes = selected_unit == 4                         ? 0
                       : selected_unit == 5 || selected_unit == 7 ? 12
                       : selected_unit == 6                       ? 10
                                                                  : 4;
        memcpy(expected, number, 12);
        if (selected_unit == 7)
        {
            expected[10] = 10;
            expected[11] = 0;
        }
        else if (selected_unit >= 8)
        {
            unsigned value = selected_unit == 8    ? 2
                             : selected_unit == 9  ? 3
                             : selected_unit == 10 ? 0xfffffffe
                             : selected_unit == 11 ? 0xfffffffd
                                                   : 0xffffffff;
            memcpy(expected, &value, 4);
        }
        if (output_bytes && memcmp(output, expected, output_bytes))
            fail();

        if (selected_unit >= 6)
            numeric_workspace("after");
        pending("after");
        recon_native_entry_finish(1);
    }
    if (selected_unit)
    {
        if (selected_unit > 2)
            fail();
        record = global_symbol(selected_unit == 1 ? "residual_record" : "residual_record_pointer");
        if (!permanent(record, 44) || !permanent(record->type, 16) ||
            !(record->type->flags & 0x400) || (record->flags & 0x10) || !record->type->field08 ||
            !permanent((void *)record->type->field08, 44))
            fail();
        if (selected_unit == 1 && record->type->related)
            fail();
        if (selected_unit == 2 &&
            (!permanent(record->type->related, 12) || (record->type->related->flags & 7) != 1))
            fail();
        memcpy(record_bytes, record, 44);
        owners();
        pending("before");

        tag = record_type_tag(record);
        if (tag != (selected_unit == 1 ? (FrontendSymbol *)record->type->field08 : 0) ||
            memcmp(record_bytes, record, 44))
            fail();

        pending("after");
        recon_native_entry_finish(1);
    }
    source = (ExpressionNode *)nodes[6]->expression;
    scalar = (ExpressionNode *)nodes[4]->expression;
    temporary(source, 20, 2);
    temporary(scalar, 20, 2);
    if (source->kind != 60 || source->flags != 1 || scalar->kind != 60 || scalar->flags != 1)
        fail();
    source_leaf = (ExpressionNode *)source->value.children.left;
    temporary(source_leaf, 20, 2);
    if (source_leaf->kind != 2 || source_leaf->flags != 4)
        fail();
    symbol = (FrontendSymbol *)source_leaf->value.children.left;
    temporary(symbol, 44, 1);
    if (strcmp(symbol->name, "incoming") || (symbol->storage[0] & 7) != 4)
        fail();
    base = scalar->type;
    if (!permanent(base, 16) || base->related || !permanent(type_0802, 16) ||
        type_0802->flags != 0x802 || type_0802->attributes || type_0802->related ||
        type_0802->field08)
        fail();
    memcpy(source_bytes, source, 20);
    depth = type_description_depth;
    __asm { mov before,esp }
    __asm {fnstcw control_before} modifier = based_type_modifier(type_0802->flags);
    if (modifier != 8)
        fail();
    self = make_qualified_type(base, 1, modifier, 0);
    if (!permanent(self, 16) || !permanent(self->related, 12) || self->flags != base->flags ||
        self->attributes != base->attributes || self->field08 != base->field08 ||
        self->related->flags != 1 || self->related->attributes != 8 || self->related->related ||
        self->related->field08 || type_description_depth != depth ||
        memcmp(source_bytes, source, 20))
        fail();

    owners();

    pending("before");
    result = (ExpressionNode *)convert_based_pointer(self, (IrToken *)source);
    temporary(result, 20, 2);
    if (result->kind != 99 || result->flags != 1 || result->type != self ||
        result->value.children.left != (IrToken *)source || type_description_depth != depth ||
        memcmp(source_bytes, source, 20))
        fail();
    __asm { mov after,esp }
    __asm { fnstcw control_after }
    if (before != after || control_before != control_after)
        fail();
    for (i = 0; i < 7; ++i)
        if (memcmp(node_bytes[i], nodes[i], 20))
            fail();

    owners();
    pending("after");
    recon_native_entry_finish(1);
}
void __stdcall direct_at_diagnostic(unsigned code, CType *type)
{
    FrontendSymbol *symbol;
    unsigned snapshot[11], i, errors, depth, before, after;
    unsigned short control_before, control_after;
    if (selected_unit != 3)
    {
        report_type_syntax_error(code, type);
        return;
    }
    if (invoked++ || code != 208 || !permanent(type, 16) || type->related || type->field08 ||
        (type->flags & 255) != 0x64 || error_count || declaration_text_depth)
        fail();
    symbol = global_symbol("residual_global_pointer");
    if (!permanent(symbol, 44) || !permanent(symbol->type, 16) || symbol->kind != 9 ||
        symbol->type->flags != 0x64 || symbol->type->attributes != 0x10 || symbol->type->field08 ||
        !permanent(symbol->type->related, 12) || symbol->type->related->flags != 1 ||
        symbol->type->related->attributes != 1 || symbol->type->related->related ||
        symbol->type->related->field08)
        fail();
    for (i = 0; i < 32; ++i)
        if (declaration_text_pool[i].handle)
            fail();
    memcpy(snapshot, symbol, 44);
    errors = error_count;
    depth = declaration_text_depth;
    owners();
    pending("before");

    __asm { mov before,esp }
    __asm {fnstcw control_before} report_declaration_syntax_error(code, symbol);
    __asm { mov after,esp }
    __asm { fnstcw control_after }
    if (before != after || control_before != control_after || error_count != errors + 1 ||
        declaration_text_depth != depth || memcmp(snapshot, symbol, 44))
        fail();
    for (i = 0; i < 32; ++i)
        if (declaration_text_pool[i].handle)
            fail();

    pending("after");
    recon_native_entry_finish(1);
}

extern int main(int, char **, char **);
static void invoke_main(void *unused)
{
    char *arguments[2] = {"C13232.EXE", NULL};
    (void)unused;
    if (syscall(SYS_dup2, 1, 2) != 2)
        syscall(SYS_exit_group, 89);
    main(1, arguments, (char **)0);
}
int c1_direct(void)
{
    int result;
    if (!load_pending())
    {
        recon_record_terminal_status(1);
        syscall(SYS_exit_group, 1);
    }
    result = recon_native_at_entry((const void *)finish_call_arguments,
                                   (const void *)direct_at_call, invoke_main, NULL);
    /* Harness completion preserves the original paused-context disk/pending
       buffers. It is not a target exit; real noreturn target exits stay intact. */
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group, result == 1 ? 0 : 1);
    return 0;
}
