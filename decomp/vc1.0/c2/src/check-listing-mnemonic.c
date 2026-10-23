/* Whole mnemonic parent and real relative padding; child suffixes mocked. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "listing.h"
#include "instruction-selection.h"
#include <recon.h>
#undef printf
unsigned listing_column;
static char output[4096], suffix[16];
static unsigned used, size_call, directive_call;
int recon_printf(const char *format, ...)
{
    int n; va_list args;
    va_start(args, format); n = vsnprintf(output + used, sizeof(output) - used, format, args); va_end(args);
    if (n < 0 || (unsigned)n >= sizeof(output) - used) abort(); used += n; return n;
}
char *__fastcall listing_size_suffix(unsigned bytes)
{
    if (size_call) abort(); size_call = bytes + 1;
    sprintf(suffix, "s%u", bytes); return suffix;
}
char *__fastcall listing_data_directive(long bytes)
{
    if (directive_call) abort(); directive_call = bytes + 1;
    sprintf(suffix, "d%ld", bytes); return suffix;
}
int main(void)
{
    unsigned operations[] = {MC_RET, MC_JCC, MC_SCC, MC_JCXZ, MC_INC,
        MC_CMPS, MC_INS, MC_LODS, MC_MOVS, MC_OUTS, MC_SCAS, MC_STOS,
        MC_POPA, MC_POPF, MC_PUSHA, MC_PUSHF, MC_IMUL2, MC_IMUL3, MC_NOP,
        MC_LOCK, MC_DATA, MC_MOVZXW, MC_MOVSXW, MC_MOVSR, MC_REP, MC_REPNZ,
        MC_REPZ, MC_MOV, MC_PUSH, MC_FADD, MC_FLD1};
    unsigned widths[] = {1, 2, 4, 8, 10};
    unsigned op, w, mode, condition, i;
    MachineInstruction instruction;
    for (op = 0; op < sizeof(operations) / sizeof(*operations); ++op)
        for (w = 0; w < sizeof(widths) / sizeof(*widths); ++w)
            for (mode = 0; mode < 3; ++mode)
                for (condition = 0; condition < ((operations[op] == MC_JCC || operations[op] == MC_SCC) ? 16 : 1); ++condition) {
                    memset(&instruction, 0, sizeof(instruction));
                    instruction.opcode = (operations[op] << 8) | (widths[w] << 4);
                    instruction.record.source_location = mode ? 0xe000 : 0;
                    instruction.operands[0].flags = mode == 2 ? 2 : 1;
                    instruction.operands[1].flags = condition;
                    used = size_call = directive_call = 0; output[0] = 0; listing_column = 24;
                    write_instruction_mnemonic(&instruction);
                    printf("%u %u %u %u %u %u %u ", operations[op], widths[w], mode, condition, listing_column, size_call, directive_call);
                    for (i = 0; i < used; ++i) printf("%02x", (unsigned char)output[i]);
                    printf("\n");
                }
    return 0;
}
