/* Fixed direct test at the genuine pre-encoding MOV EAX,FS:[0] context.
   This harness is not application reconstruction or public-source reach. */
#ifndef RECON_ORIGINAL_DIRECT
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include <stdio.h>
#include <string.h>
#include "asm.h"
#include "pch-relocate.h"
#include "options.h"
#include "control-ir.h"
#include "browser.h"
#include "debug-info.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>

#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *original_context;
/* Original RVAs bind actual live storage, never a copied compiler state. */
#define AT(rva) RECON_ORIGINAL_ADDRESS(original_context, rva)
#define asm_operands (*(AsmOperand (*)[3])AT(0x4b028))
#define asm_addresses (*(AsmAddress (*)[3])AT(0x4b074))
#define asm_instruction_record (*(char (*)[128])AT(0x4b0a4))
#define asm_register_properties (*(unsigned short (*)[46])AT(0x631e0))
#define asm_operand_count (*(int *)AT(0x4b024))
#define asm_operation (*(short *)AT(0x4b018))
#define asm_opcode (*(short *)AT(0x4b058))
#define asm_encoded_opcode (*(unsigned char *)AT(0x4b060))
#define asm_instruction_size (*(unsigned char *)AT(0x4b061))
#define asm_instruction_modifier (*(unsigned char *)AT(0x4b064))
#define ASM_PROTOTYPES_ONLY (*(int *)AT(0x4b244))
#define expression_file (*(FILE **)AT(0x4b58c))
#define symbol_file (*(FILE **)AT(0x4b590))
#define line_file (*(FILE **)AT(0x4b594))
#define global_file (*(FILE **)AT(0x4b598))
#define debug_file (*(FILE **)AT(0x4b260))
#define pch_file (*(FILE **)AT(0x4b580))
#define browser_output (*(FILE **)AT(0x4d5a0))
#define auxiliary_ir_stream (*(FILE **)AT(0x5e434))
#define write_asm_segment_prefix \
    (*(void (__cdecl *)(void))AT(0x3f0cd))
typedef char Original_FILE_size[(sizeof(FILE) == 32) ? 1 : -1];
#else
#define ASM_PROTOTYPES_ONLY options.prototypes_only
#endif

static unsigned invoked;
static unsigned char pending_bytes[65536];
static unsigned pending_size, pending_groups;
static void fail(void)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(original_context, 0);
#else
    recon_native_entry_finish(0);
#endif
    for (;;)
        ;
}
static void append_word(unsigned value)
{
    if (pending_size > sizeof pending_bytes - 4)
        fail();
    memcpy(pending_bytes + pending_size, &value, 4);
    pending_size += 4;
}
/* Only the initialized pending bytes are captured. No unwritten FILE buffer
   capacity or heap tail is compared across original and native executions. */
static void capture_pending(void)
{
    FILE *streams[7];
    unsigned i, count;
    streams[0] = expression_file;
    streams[1] = symbol_file;
    streams[2] = line_file;
    streams[3] = global_file;
    streams[4] = debug_file;
    streams[5] = pch_file;
    streams[6] = browser_output;
    if (pending_groups >= 2)
        fail();
    for (i = 0; i < 7; ++i)
    {
        if (!streams[i])
        {
            append_word(0xffffffffU);
            continue;
        }
        if (!streams[i]->_base || streams[i]->_ptr < streams[i]->_base)
            fail();
        count = streams[i]->_ptr - streams[i]->_base;
        if (count > 4096 || count > sizeof pending_bytes - pending_size - 4)
            fail();
        append_word(count);
        memcpy(pending_bytes + pending_size, streams[i]->_base, count);
        pending_size += count;
    }
    ++pending_groups;
}
#ifndef RECON_ORIGINAL_DIRECT
static unsigned char pending_oracle[65536], workspace_oracle[6];
static void read_exact(char *name, void *bytes, unsigned count)
{
    int fd, got;
    unsigned position = 0;
    char extra;
    fd = syscall(SYS_open, name, 0, 0);
    if (fd < 0)
        fail();
    while (position < count)
    {
        got = syscall(SYS_read, fd, (char *)bytes + position, count - position);
        if (got <= 0)
            fail();
        position += got;
    }
    if (syscall(SYS_read, fd, &extra, 1) != 0)
        fail();
    syscall(SYS_close, fd);
}
#endif

/* The callback always stops this direct phase. It does not resume the parser
   after changing its workspace or append a second normal instruction record. */
void direct_at_asm(void)
{
    unsigned char operands_before[sizeof asm_operands];
    unsigned char addresses_before[sizeof asm_addresses];
    unsigned char workspace_before[sizeof asm_instruction_record];
    unsigned count, before, after, preserved_before[4], preserved_after[4];
    unsigned short control_before, control_after;
    AsmAddress *first, *second;
    if (invoked++ || ASM_PROTOTYPES_ONLY || asm_operand_count != 2 || asm_operation != 1 ||
        asm_opcode != 1 || asm_encoded_opcode || asm_instruction_size != 2 ||
        asm_instruction_modifier ||
        asm_operands[0].address != &asm_addresses[0] ||
        asm_operands[1].address != &asm_addresses[1])
        fail();
    first = asm_operands[0].address;
    second = asm_operands[1].address;
    if (first->flags || second->flags != 29 || asm_register_properties[29] != 0x014c ||
        first->address_flags != 4 || first->first_register != 1 || first->second_register ||
        second->address_flags != 18 || second->first_register || second->second_register ||
        first->symbol || second->symbol || first->displacement || second->displacement ||
        !expression_file || !expression_file->_base)
        fail();
    count = expression_file->_ptr - expression_file->_base;
    if (count > 4096 - 8 || expression_file->_cnt < 8)
        fail();
    memcpy(operands_before, asm_operands, sizeof operands_before);
    memcpy(addresses_before, asm_addresses, sizeof addresses_before);
    memcpy(workspace_before, asm_instruction_record, sizeof workspace_before);
    append_word(0x31444e50);
    append_word(2);
    capture_pending();
    /* Narrow ABI observation only; compiler-generated typed call follows. */
    __asm {
        lea eax,preserved_before
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
        mov before,esp
        fnstcw control_before
    }
    write_asm_segment_prefix();
    __asm {
        mov after,esp
        fnstcw control_after
        lea eax,preserved_after
        mov [eax],ebx
        mov [eax+4],esi
        mov [eax+8],edi
        mov [eax+12],ebp
    }
    if (before != after || control_before != control_after ||
        memcmp(preserved_before, preserved_after, sizeof preserved_before) ||
        memcmp(operands_before, asm_operands, sizeof operands_before) ||
        memcmp(addresses_before, asm_addresses, sizeof addresses_before) ||
        memcmp(asm_instruction_record, "\x05\xaf\x00\x01\x0c\x00", 6) ||
        memcmp(workspace_before + 6, asm_instruction_record + 6,
               sizeof workspace_before - 6) ||
        auxiliary_ir_stream != expression_file ||
        expression_file->_ptr - expression_file->_base != count + 8 ||
        memcmp(expression_file->_base + count, "\x4f\x16\x05\x05\xaf\x00\x01\x0c", 8))
        fail();
    capture_pending();
#ifdef RECON_ORIGINAL_DIRECT
    if (!recon_original_emit(original_context, ".recon-pending.bin", pending_bytes, pending_size) ||
        !recon_original_emit(original_context, ".recon-asm.bin", asm_instruction_record, 6))
        fail();
    recon_original_finish(original_context, 1);
#else
    /* Final oracles are needed only after the real helper returns. Therefore
       frontier discovery can reach an unimplemented helper without them. */
    read_exact(".recon-pending.bin", pending_oracle, pending_size);
    read_exact(".recon-asm.bin", workspace_oracle, sizeof workspace_oracle);
    if (memcmp(pending_bytes, pending_oracle, pending_size) ||
        memcmp(asm_instruction_record, workspace_oracle, sizeof workspace_oracle))
        fail();
    recon_native_entry_finish(1);
#endif
    for (;;)
        ;
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    original_context = context;
    if (!context || context->abi_version != RECON_ORIGINAL_PROBE_VERSION ||
        context->struct_size != sizeof(*context) || context->marker_rva != 0x3eb41 ||
        !context->image_base || !context->original_esp)
        fail();
    direct_at_asm();
}
#else
extern int main(int, char **, char **);
static void invoke_main(void *unused)
{
    char *arguments[2] = {"C13232.EXE", NULL};
    (void)unused;
    main(1, arguments, (char **)0);
}
int c1_direct(void)
{
    int result;
    result = recon_native_at_entry((const void *)normalize_asm_addresses,
                                   (const void *)direct_at_asm, invoke_main, NULL);
    /* Harness completion preserves actual pending data without CRT flushing. */
    recon_record_terminal_status(result == 1 ? 0 : 1);
    syscall(SYS_exit_group, result == 1 ? 0 : 1);
    return 0;
}
#endif
