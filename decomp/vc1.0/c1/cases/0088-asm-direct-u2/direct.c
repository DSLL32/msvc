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
static void check_before(void)
{
    static const int expected[11] = {2, 2, 1, 0, 4, 0, 1, 0, 0, 0, 0};
    static const unsigned char control[16] = {0x18, 0xfc, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
                                              0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int actual[11];
    AsmAddress *a;
    if (!asm_operand || !(a = asm_operand->address) || !type_0000)
        fail();
    actual[0] = asm_operand_count;
    actual[1] = asm_operand->size;
    actual[2] = asm_operand->flags;
    actual[3] = a->displacement;
    actual[4] = a->address_flags;
    actual[5] = a->flags;
    actual[6] = a->first_register;
    actual[7] = a->second_register;
    actual[8] = a->symbol != NULL;
    actual[9] = a->symbol ? a->symbol->storage[0] : 0;
    actual[10] = a->symbol ? a->symbol->index : 0;
    if (error_count != 3 || asm_current_token != 34 || memcmp(actual, expected, sizeof(actual)) ||
        memcmp(&asm_parser_control, control, 16))
        fail();
    pending("before");
}
static void check_after(void)
{
    static const int expected[11] = {2, 2, 1, 0, 4, 0, 1, 0, 0, 0, 0};
    static const unsigned char control[16] = {0x18, 0xfc, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
                                              0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int actual[11];
    AsmAddress *a;
    if (!asm_operand || !(a = asm_operand->address) || !type_0000)
        fail();
    actual[0] = asm_operand_count;
    actual[1] = asm_operand->size;
    actual[2] = asm_operand->flags;
    actual[3] = a->displacement;
    actual[4] = a->address_flags;
    actual[5] = a->flags;
    actual[6] = a->first_register;
    actual[7] = a->second_register;
    actual[8] = a->symbol != NULL;
    actual[9] = a->symbol ? a->symbol->storage[0] : 0;
    actual[10] = a->symbol ? a->symbol->index : 0;
    if (error_count != 3 || asm_current_token != 34 || memcmp(actual, expected, sizeof(actual)) ||
        memcmp(&asm_parser_control, control, 16))
        fail();
    pending("after");
}
static void __stdcall at_marker(unsigned code, int operand)
{
    unsigned before, after;
    unsigned short cb, ca;
    ParserValue value;
    if (code != 421 || operand != 1)
        fail();
    check_before();
    __asm { mov before,esp }
    __asm {fnstcw cb} RECON_ASSIGN_RETURN8(value, asm_recovery_value(0));
    if (value.words.low != 0)
        fail();
    __asm { mov after,esp }
    __asm { fnstcw ca }
    if (before != after || cb != ca)
        fail();
    check_after();
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
    result = recon_native_at_entry((const void *)report_asm_operand_error, (const void *)at_marker,
                                   invoke_main, NULL);
    /* External harness completion, not a target exit: preserve the captured
       paused-context disk bytes without unrelated atexit/stdio flushing. */
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group, result == 1 ? 0 : 1);
    return 0;
}
