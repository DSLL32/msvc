/* Fixed test-only calls from the genuine missing-include fatal boundary. */
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "../../src/source-input.h"
#include "../../src/macros.h"
#include "../../src/diagnostics.h"
#include "../../src/frontend.h"
#include "../../src/pch.h"
#include "../../src/browser.h"
#include <recon-native.h>
#include <recon.h>
static unsigned selected_unit = 1, invoked;

static SourceFile *owners[5];
static int owner_handles[5];
static void fail(void) { recon_native_entry_finish(0); }

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
static void string_value(char *label, char *text)
{

    if (!text || strlen(text) >= 260)
        fail();
}
static void snapshot(char *when)
{
    SourceFile *node = active_input;
    unsigned i, j;
    if (include_depth != 4)
        fail();
    if (!strcmp(when, "before") && (errno != 2 || _doserrno != 2))
        fail();
    for (i = 0; i < 5; ++i)
    {
        if (!node || node->input_kind)
            fail();
        if (!strcmp(when, "before"))
        {
            for (j = 0; j < i; ++j)
                if (owners[j] == node || owner_handles[j] == node->handle)
                    fail();
            owners[i] = node;
            owner_handles[i] = node->handle;
        }
        else if (owners[i] != node)
            fail();
        if (!strcmp(when, "before"))
        {
            if (i == 0 &&
                (node->capacity != 4096 || (unsigned)node->handle != 13U || node->saved_line != 3 ||
                 (node->index & 65535) != 10 || node->available != 421 || node->offset != 0 ||
                 node->saved_packing != 8 || node->saved_conditional_depth != 2 ||
                 strcmp(node->filename, "resource4.h")))
                fail();
            if (i == 1 &&
                (node->capacity != 4096 || (unsigned)node->handle != 12U || node->saved_line != 3 ||
                 (node->index & 65535) != 65535 || node->available != 0 || node->offset != 62 ||
                 node->saved_packing != 8 || node->saved_conditional_depth != 1 ||
                 strcmp(node->filename, "resource3.h")))
                fail();
            if (i == 2 &&
                (node->capacity != 4096 || (unsigned)node->handle != 11U || node->saved_line != 4 ||
                 (node->index & 65535) != 65535 || node->available != 0 || node->offset != 62 ||
                 node->saved_packing != 8 || node->saved_conditional_depth != 0 ||
                 strcmp(node->filename, "resource2.h")))
                fail();
            if (i == 3 &&
                (node->capacity != 8192 || (unsigned)node->handle != 10U || node->saved_line != 1 ||
                 (node->index & 65535) != 65535 || node->available != 171 || node->offset != 0 ||
                 node->saved_packing != 8 || node->saved_conditional_depth != 4294967295 ||
                 strcmp(node->filename, "resource1.h")))
                fail();
            if (i == 4 &&
                (node->capacity != 12288 || (unsigned)node->handle != 9U || node->saved_line != 0 ||
                 (node->index & 65535) != 65535 || node->available != 642 || node->offset != 0 ||
                 node->saved_packing != 8 || node->saved_conditional_depth != 4294967295 ||
                 strcmp(node->filename, "test.c")))
                fail();
        }
        if (!strcmp(when, "after"))
        {
            if (i == 0 &&
                (node->capacity != 4096 || (unsigned)node->handle != 13U || node->saved_line != 3 ||
                 (node->index & 65535) != 10 || node->available != 421 || node->offset != 0 ||
                 node->saved_packing != 8 || node->saved_conditional_depth != 2 ||
                 strcmp(node->filename, "resource4.h")))
                fail();
            if (i == 1 &&
                (node->capacity != 4096 || (unsigned)node->handle != 12U || node->saved_line != 3 ||
                 (node->index & 65535) != 65535 || node->available != 0 || node->offset != 62 ||
                 node->saved_packing != 8 || node->saved_conditional_depth != 1 ||
                 strcmp(node->filename, "resource3.h")))
                fail();
            if (i == 2 &&
                (node->capacity != 4096 || (unsigned)node->handle != 4294967295U ||
                 node->saved_line != 4 || (node->index & 65535) != 65535 || node->available != 0 ||
                 node->offset != 62 || node->saved_packing != 8 ||
                 node->saved_conditional_depth != 0 || strcmp(node->filename, "resource2.h")))
                fail();
            if (i == 3 &&
                (node->capacity != 8192 || (unsigned)node->handle != 10U || node->saved_line != 1 ||
                 (node->index & 65535) != 65535 || node->available != 171 || node->offset != 0 ||
                 node->saved_packing != 8 || node->saved_conditional_depth != 4294967295 ||
                 strcmp(node->filename, "resource1.h")))
                fail();
            if (i == 4 &&
                (node->capacity != 12288 || (unsigned)node->handle != 9U || node->saved_line != 0 ||
                 (node->index & 65535) != 65535 || node->available != 642 || node->offset != 0 ||
                 node->saved_packing != 8 || node->saved_conditional_depth != 4294967295 ||
                 strcmp(node->filename, "test.c")))
                fail();
        }
        node = node->next;
    }
    if (node)
        fail();
}

void direct_at_fatal(unsigned code, char *kind, char *filename)
{
    unsigned before, after, result = 0, i;
    unsigned short control_before, control_after;
    void *allocation;

    if (invoked++ || code != 83 || strcmp(kind, "include") || strcmp(filename, "absent-resource.h"))
        fail();
    snapshot("before");
    pending("before");
    string_value("kind", kind);
    string_value("missing", filename);

    __asm { mov before,esp }
    __asm { fnstcw control_before }
    switch (selected_unit)
    {
    case 0:
        allocation = retry_malloc(96);
        if (!allocation)
            fail();

        result = 1;
        break;
    case 1:
        result = release_input_handle();
        if (result != 1)
            fail();
        break;
    case 2:

        report_error(code, kind, filename, owners[0]->filename, source_line_number);
        fail();
        break;
    case 3:
        compiler_signal(2);
        fail();
        break;
    default:
        fail();
    }
    __asm { mov after,esp }
    __asm { fnstcw control_after }
    if (before != after || control_before != control_after)
        fail();

    snapshot("after");
    pending("after");
    for (i = 0; i < 5; ++i)
        if (owners[i]->handle != (selected_unit == 1 && i == 2 ? -1 : owner_handles[i]))
            fail();
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
    result = recon_native_at_entry((const void *)report_fatal, (const void *)direct_at_fatal,
                                   invoke_main, NULL);
    /* Harness completion preserves the original paused-context disk/pending
       buffers. It is not a target exit; real noreturn target exits stay intact. */
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group, result == 1 ? 0 : 1);
    return 0;
}
