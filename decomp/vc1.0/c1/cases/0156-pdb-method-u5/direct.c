#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>
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
static unsigned char record_payload[160];
static unsigned char expected_header[64];
/* Exact defined header/payload oracle is fixture-owned capture data. */
static int load_pdb_reference(void)
{
    unsigned char bytes[224];
    char extra;
    int fd = syscall(SYS_open, ".recon-pdb-reference.bin", 0, 0);
    if (fd < 0)
        return 0;
    if (syscall(SYS_read, fd, bytes, 224) != 224 || syscall(SYS_read, fd, &extra, 1) != 0)
    {
        syscall(SYS_close, fd);
        return 0;
    }
    syscall(SYS_close, fd);
    memcpy(expected_header, bytes, 64);
    memcpy(record_payload, bytes + 64, 160);
    return 1;
}
static ProgramDatabase *database;
static PdbBuffer *head_records, *head_blocks;
static unsigned char constructor_before[116];
static void check_loaded(unsigned repetitions, unsigned empty)
{
    static const unsigned offsets[8] = {0, 34, 44, 54, 68, 78, 92, 126};
    static const unsigned buckets[8] = {321, 80, 583, 311, 3467, 496, 1949, 543};
    PdbBuffer *records = head_records, *block = head_blocks->next, *tail = NULL;
    PdbTypeEntry *entry;
    unsigned i, j, seen;
    if (memcmp(&database->header, expected_header, 64) || !database->type_offsets ||
        !database->type_index || database->records.head != head_records ||
        database->blocks.head != head_blocks || database->records.count != repetitions * 160 ||
        database->blocks.count != repetitions * 96 || head_records->available ||
        head_records->cursor != (unsigned char *)(head_records + 1) || head_blocks->available ||
        head_blocks->cursor != (unsigned char *)(head_blocks + 1) || database->incremental != 1 ||
        database->cumulative != 1)
        fail();
    for (i = 0; i < repetitions; ++i)
    {
        records = records->next;
        if (!records || records == head_records || records->available ||
            records->cursor != (unsigned char *)(records + 1) + 160 ||
            memcmp(records + 1, record_payload, 160))
            fail();
        tail = records;
    }
    if (records->next || database->records.tail != records || database->loaded_tail != records)
        fail();
    if (empty)
    {
        if (block || database->blocks.tail != head_blocks)
            fail();
    }
    else
    {
        if (!block || block->next || database->blocks.tail != block ||
            block->available != 8192 - 96 * repetitions ||
            block->cursor != (unsigned char *)(block + 1) + 96 * repetitions)
            fail();
        records = head_records->next;
        for (i = 0; i < repetitions; ++i, records = records->next)
            for (j = 0; j < 8; ++j)
            {
                entry = (PdbTypeEntry *)((unsigned char *)(block + 1) + 96 * i + 12 * j);
                if (entry->next || entry->record != (unsigned char *)(records + 1) + offsets[j] ||
                    entry->index != 0x1000 + j)
                    fail();
            }
        for (j = 0; j < 8; ++j)
            if (database->type_offsets[j] != (unsigned char *)(tail + 1) + offsets[j])
                fail();
    }
    for (i = 0; i < 4096; ++i)
    {
        seen = 0;
        if (!empty)
            for (j = 0; j < 8; ++j)
                if (i == buckets[j])
                {
                    entry = (PdbTypeEntry *)((unsigned char *)(block + 1) + 96 * (repetitions - 1) +
                                             12 * j);
                    if (database->type_index[i] != entry)
                        fail();
                    seen = 1;
                }
        if (!seen && database->type_index[i])
            fail();
    }
}
static int __fastcall at_marker(ProgramDatabase *self, unsigned ignored, char *path,
                                unsigned signature)
{
    unsigned unit = 5, sp0, sp1, i, result, position;
    unsigned short cw0, cw1;
    unsigned char object_before[116];
    unsigned char *record;
    unsigned required, length, stage;
    int handle;
    (void)ignored;
    (void)path;
    (void)signature;
    database = self;
    if (sizeof(*self) != 116 || !self || self->type_offsets || self->type_index ||
        self->handle != -1 || self->writable != 1 || self->incremental != 1 ||
        !(head_records = self->records.head) || head_records != self->records.tail ||
        self->records.count || !(head_blocks = self->blocks.head) ||
        head_blocks != self->blocks.tail || self->blocks.count || head_blocks == head_records ||
        head_records->available || head_records->next ||
        head_records->cursor != (unsigned char *)(head_records + 1) || head_blocks->available ||
        head_blocks->next || head_blocks->cursor != (unsigned char *)(head_blocks + 1) ||
        errno != 2 || _doserrno != 2 || GetLastError() != 2)
        fail();
    memcpy(constructor_before, self, 116);
    __asm { mov sp0,esp }
    __asm {fnstcw cw0} result = open_program_database_object(
        self, 0, "unit.pdb", unit == 1 || unit == 3 ? "rc" : "wc", 0x31415926);
    __asm { mov sp1,esp }
    __asm { fnstcw cw1 }
    if (sp0 != sp1 || cw0 != cw1 || result != (unit == 3 ? 0 : 1) ||
        self->writable != (unit == 1 || unit == 3 ? 0 : 1) || self->incremental != 1 ||
        self->cumulative != 1)
        fail();
    if (unit == 3)
    {
        if (memcmp(self, constructor_before, 64) ||
            memcmp((char *)self + 96, constructor_before + 96, 4) || self->type_offsets ||
            self->type_index || self->handle != -1 || self->records.head != head_records ||
            self->records.tail != head_records || self->records.count ||
            self->blocks.head != head_blocks || self->blocks.tail != head_blocks ||
            self->blocks.count)
            fail();
    }
    else
        check_loaded(unit == 2 ? 0 : 1, unit == 2);
    handle = self->handle;
    if (unit == 1 || unit == 3)
    {
        if (handle != -1)
            fail();
    }
    else if (handle < 0)
        fail();
    if (unit == 4 || unit == 5)
    {
        if (unit == 5 && _chsize(handle, 32))
            fail();
        if (_lseek(handle, 0, 0))
            fail();
        __asm { mov sp0,esp }
        __asm {fnstcw cw0} result = reload_program_database(self);
        __asm { mov sp1,esp }
        __asm { fnstcw cw1 }
        if (sp0 != sp1 || cw0 != cw1 || result != (unit == 5 ? 0 : 1))
            fail();
        check_loaded(unit == 4 ? 2 : 1, 0);
    }
    if (handle >= 0)
    {
        position = _lseek(handle, 0, 1);
        if (position != (unit == 2 ? 64 : unit == 5 ? 32 : 224))
            fail();
    }
    recon_native_entry_finish(1);
}

int c1_direct(void)
{
    int result;
    if (!load_pdb_reference())
    {
        recon_record_terminal_status(1);
        syscall(SYS_exit_group, 1);
    }
    result = recon_native_at_entry((const void *)create_program_database, (const void *)at_marker,
                                   invoke_main, NULL);
    /* External harness completion, not a target exit: preserve the captured
       paused-context disk bytes without unrelated atexit/stdio flushing. */
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group, result == 1 ? 0 : 1);
    return 0;
}
