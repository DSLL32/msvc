/* Direct support only: genuine constructor owner and captured valid records.
   No parser continuation follows the arena mutation or owner destruction. */
#ifndef RECON_ORIGINAL_DIRECT
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug-info.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>

#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
#define AT(rva) RECON_ORIGINAL_ADDRESS(probe, rva)
#define open_program_database_object (*(int (__fastcall *)(ProgramDatabase *, unsigned, char *, char *, unsigned))AT(0x7150))
#define query_program_database_record (*(unsigned char *(__fastcall *)(ProgramDatabase *, unsigned, unsigned short))AT(0x7970))
#define codeview_leaf_offset (*(unsigned (__cdecl *)(unsigned short *))AT(0x7040))
#define reserve_program_database_record (*(unsigned char *(__cdecl *)(unsigned, ProgramDatabase *, unsigned short *))AT(0x7080))
#define close_program_database (*(int (__cdecl *)(ProgramDatabase *))AT(0x7ee0))
#endif

static unsigned invoked;
static unsigned observed[14];
static void fail(void)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe, 0);
#else
    recon_native_entry_finish(0);
#endif
    for (;;) ;
}

static void __fastcall at_create(ProgramDatabase *self, unsigned ignored,
                                char *path, unsigned signature)
{
    PdbBuffer *head, *empty, *loaded, *tail;
    unsigned char *record[8], *first, *second;
    unsigned i, bytes[2], count, capacity, sp0, sp1, saved0[4], saved1[4];
    unsigned short cw0, cw1;
    unsigned char before[116], payload[160];
    (void)ignored;
    (void)signature;
    if (invoked++ || !self || strcmp(path, "S:\\vc1.0\\c1\\legacy-pdb-object\\msvc.pdb") ||
        self->type_offsets || self->type_index || self->handle != -1 ||
        self->writable != 1 || self->incremental != 1 ||
        !(head = self->records.head) || self->records.tail != head ||
        self->records.count || !(empty = self->blocks.head) ||
        self->blocks.tail != empty || self->blocks.count || head == empty ||
        head->available || head->next || head->cursor != (unsigned char *)(head + 1) ||
        empty->available || empty->next || empty->cursor != (unsigned char *)(empty + 1))
        fail();
    /* wc defines cumulative through the actual accepted method. The initial
       wi constructor's unwritten cumulative field is never inspected here. */
    if (!open_program_database_object(self, 0, "unit.pdb", "wc", 0x31415926) ||
        !self->type_offsets || !self->type_index || self->handle < 0 ||
        self->header.first_type != 0x1000 || self->header.last_type != 0x1008 ||
        self->header.record_bytes != 160 || self->records.count != 160 ||
        self->cumulative != 1 || !(loaded = head->next) ||
        loaded != self->records.tail || loaded != self->loaded_tail ||
        loaded->next || loaded->available ||
        loaded->cursor != (unsigned char *)(loaded + 1) + 160)
        fail();
    memcpy(payload, loaded + 1, sizeof payload);
    for (i = 0; i < 8; ++i) {
        record[i] = query_program_database_record(self, 0, (unsigned short)(0x1000 + i));
        if (record[i] < (unsigned char *)(loaded + 1) ||
            record[i] + 6 > loaded->cursor ||
            record[i] + (unsigned)*(unsigned short *)(record[i] + 2) + 4 > loaded->cursor)
            fail();
        __asm {
            lea eax,saved0
            mov [eax],ebx
            mov [eax+4],esi
            mov [eax+8],edi
            mov [eax+12],ebp
            mov sp0,esp
            fnstcw cw0
        }
        observed[i] = codeview_leaf_offset((unsigned short *)(record[i] + 4));
        __asm {
            mov sp1,esp
            fnstcw cw1
            lea eax,saved1
            mov [eax],ebx
            mov [eax+4],esi
            mov [eax+8],edi
            mov [eax+12],ebp
        }
        if (sp0 != sp1 || cw0 != cw1 || memcmp(saved0, saved1, sizeof saved0) ||
            observed[i] != (unsigned)*(unsigned short *)(record[i] + 4) - 2U)
            fail();
    }
    bytes[0] = 2U + (unsigned short)(*(unsigned short *)(record[0] + 2) + 2);
    bytes[1] = 2U + (unsigned short)(*(unsigned short *)(record[1] + 2) + 2);
    if (bytes[0] != 34 || bytes[1] != 10) fail();
    memcpy(before, self, sizeof before);
    count = self->records.count;
    __asm {
        lea eax,saved0
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
        mov sp0,esp
        fnstcw cw0
    }
    first = reserve_program_database_record(2, self, (unsigned short *)(record[0] + 2));
    __asm {
        mov sp1,esp
        fnstcw cw1
        lea eax,saved1
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
    }
    capacity = (bytes[0] + 8191U) & ~4095U;
    tail = loaded->next;
    if (sp0 != sp1 || cw0 != cw1 || memcmp(saved0, saved1, sizeof saved0) ||
        !tail || tail == head || tail == loaded || first != (unsigned char *)(tail + 1) ||
        self->records.head != head || self->records.tail != tail ||
        tail->next || tail->available != capacity - bytes[0] ||
        tail->cursor != first + bytes[0] || self->records.count != count + bytes[0] ||
        memcmp(self, before, 76) || memcmp((char *)self + 84, before + 84, 32))
        fail();
    observed[8] = bytes[0];
    observed[9] = tail->available;
    observed[10] = self->records.count;
    /* Define only the first returned reservation with a real stored record.
       The target's unwritten remaining allocation is neither read nor compared. */
    memcpy(first, record[0], bytes[0]);
    __asm {
        lea eax,saved0
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
        mov sp0,esp
        fnstcw cw0
    }
    second = reserve_program_database_record(2, self, (unsigned short *)(record[1] + 2));
    __asm {
        mov sp1,esp
        fnstcw cw1
        lea eax,saved1
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
    }
    if (sp0 != sp1 || cw0 != cw1 || memcmp(saved0, saved1, sizeof saved0) ||
        second != first + bytes[0] || self->records.tail != tail ||
        loaded->next != tail || tail->next ||
        tail->available != capacity - bytes[0] - bytes[1] ||
        tail->cursor != second + bytes[1] || self->records.count != count + bytes[0] + bytes[1] ||
        memcmp(first, record[0], bytes[0]) || memcmp(self, before, 76) ||
        memcmp((char *)self + 84, before + 84, 32))
        fail();
    observed[11] = bytes[1];
    observed[12] = tail->available;
    observed[13] = self->records.count;
    if (memcmp(payload, loaded + 1, sizeof payload)) fail();
    if (!close_program_database(self)) fail();
#ifdef RECON_ORIGINAL_DIRECT
    if (!recon_original_emit(probe, ".recon-arena.bin", observed, sizeof observed)) fail();
    recon_original_finish(probe, 1);
#else
    {
        unsigned expected[14];
        char extra;
        int fd = syscall(SYS_open, ".recon-arena.bin", 0, 0);
        if (fd < 0 || syscall(SYS_read, fd, expected, sizeof expected) != sizeof expected ||
            syscall(SYS_read, fd, &extra, 1) != 0 ||
            memcmp(expected, observed, sizeof expected)) fail();
        syscall(SYS_close, fd);
    }
    recon_native_entry_finish(1);
#endif
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    probe = context;
    if (!context || context->abi_version != RECON_ORIGINAL_PROBE_VERSION ||
        context->struct_size != sizeof(*context) || context->marker_rva != 0x7300 ||
        !context->image_base || !context->original_ecx || !context->original_esp) fail();
    at_create((ProgramDatabase *)context->original_ecx, context->original_edx,
              *(char **)(context->original_esp + 4),
              *(unsigned *)(context->original_esp + 8));
}
#else
extern int main(int, char **, char **);
static void invoke(void *unused)
{
    char *arguments[2] = {"C13232.EXE", NULL};
    (void)unused;
    main(1, arguments, (char **)0);
}
int c1_direct(void)
{
    int result = recon_native_at_entry((const void *)create_program_database,
                                       (const void *)at_create, invoke, NULL);
    /* Harness completion only: do not flush paused parser streams. */
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group, result == 1 ? 0 : 1);
    return 0;
}
#endif
