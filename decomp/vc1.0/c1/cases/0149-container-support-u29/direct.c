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
static void __stdcall at_marker(FrontendSymbol *member)
{
    Vector *source = forced_source_inputs;
    void *items[3];
    unsigned char vector_before[12];
    FrontendSymbol *symbol;
    CType *type, *made;
    TypeShape *shape, *last;
    VectorIterator *iterator;
    BitSet *a, *b, *c;
    unsigned i, before, after, result, depth = type_description_depth,
                                       counter = unnamed_entity_next_index, errors = error_count;
    unsigned short cb, ca;
    char *text;
    unsigned *flags;
    unsigned char context_before[24];
    AggregateContext *context;
    if (!source || source != options.forced_includes || source->count != 3 ||
        source->capacity != 40 || source->arena != -1)
        fail();
    items[0] = source->items[0];
    items[1] = source->items[1];
    items[2] = source->items[2];
    if (strcmp(items[0], "forced-a.h") || strcmp(items[1], "forced-b.h") ||
        strcmp(items[2], "forced-c.h"))
        fail();
    memcpy(vector_before, source, 12);
    if (aggregate_depth < 0 || aggregate_depth >= 16 || !member || !member->type)
        fail();
    context = &aggregate_contexts[aggregate_depth];
    if (!context->tag || strcmp(context->tag->name, "ContainerItem") || context->size != 2 ||
        context->alignment != 2 || context->bitfield_type || context->bitfield_used_bits ||
        member->type->flags != 0x4042)
        fail();
    memcpy(context_before, context, 24);
    pending("before");
    __asm { mov before,esp }
    __asm { fnstcw cb }
    if (layout_type_member(member->type) != 2)
        fail();
    __asm { mov after,esp }
    __asm { fnstcw ca }
    if (before != after || cb != ca || type_description_depth != depth)
        fail();
    if (memcmp(vector_before, source, 12))
        fail();
    if (memcmp(items, source->items, 12))
        fail();
    if (error_count != errors + 0 || unnamed_entity_next_index != counter - 0 ||
        declaration_output_override || function_parameter_declarations)
        fail();
    ((AggregateContext *)context_before)->size += 2;
    if (memcmp(context_before, context, 24))
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
    result = recon_native_at_entry((const void *)append_aggregate_member, (const void *)at_marker,
                                   invoke_main, NULL);
    /* External harness completion, not a target exit: preserve the captured
       paused-context disk bytes without unrelated atexit/stdio flushing. */
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group, result == 1 ? 0 : 1);
    return 0;
}
