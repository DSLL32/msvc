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
#include "../../src/floating.h"
#include <recon-native.h>
#include <recon.h>

static unsigned invoked, selected_unit = 12;
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
    if (!strcmp(when, "before") &&
        memcmp(&floating_workspace, "\x00\x00\x00\x00\x00\x38\x00\x00", 8))
        fail();
    if (!strcmp(when, "after") &&
        memcmp(&floating_workspace, "\x00\x00\x00\x00\x00\x78\x7f\x02", 8))
        fail();
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
    unsigned i, before, after;
    unsigned short control_before, control_after;
    unsigned node_bytes[7][5];
    Extended *number;
    unsigned char output[48], number_before[12], input10[32], expected[12];
    unsigned status, number_index, output_bytes;
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
        if (memcmp(number, "\x00\x00\x00\x00\xff\xff\xff\xff\x1e\x40\x08\x00", 12))
            fail();
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
        if (before != after || control_before != 0x027f || control_before != control_after ||
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
        for (i = output_bytes; i < 48; ++i)
            if (output[i] != 0xa5)
                fail();
        for (i = 0; i < 7; ++i)
            if (memcmp(nodes[i], node_bytes[i], 20))
                fail();

        if (selected_unit >= 6)
            numeric_workspace("after");
        pending("after");
        recon_native_entry_finish(1);
    }
    fail();
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
