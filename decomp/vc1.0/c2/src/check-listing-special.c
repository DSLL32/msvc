/* Actual parent body; printing children are explicit argument/event mocks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "listing.h"
#include "instruction-selection.h"
#include <recon.h>
#undef printf
#undef strcpy
unsigned listing_column;
static MachineInstruction instruction;
static char events[4096], register_text[32];
static unsigned used;
static void event(char *text) { strcpy(events + used, text); used += strlen(text); }
void __fastcall write_instruction_mnemonic(MachineInstruction *p)
{
    if (p != &instruction) abort();
    event("M;");
}
void __fastcall write_instruction_operand(MachineInstruction *p, MachineOperand *o, unsigned bytes)
{
    char text[64];
    if (p != &instruction || (o != p->operands && o != p->operands + 1)) abort();
    sprintf(text, "O%d:%x:%u;", o == p->operands + 1, o->flags, bytes); event(text);
}
char *__fastcall machine_register_name(unsigned reg, unsigned bytes)
{
    char text[64];
    sprintf(text, "R%u:%u;", reg, bytes); event(text);
    sprintf(register_text, "r%u_%u", reg, bytes); return register_text;
}
int recon_printf(const char *format, ...)
{
    char text[512]; int length;
    va_list arguments;
    va_start(arguments, format); length = vsnprintf(text, sizeof(text), format, arguments); va_end(arguments);
    if (length < 0 || length >= sizeof(text)) abort();
    event("P["); event(text); event("];"); return length;
}
int main(void)
{
    unsigned operation, mode, bytes;
    MachineAddress address;
    for (operation = 0; operation < MC_OPCODE_COUNT; ++operation)
        for (mode = 0; mode < 4; ++mode)
            for (bytes = 1; bytes <= 10; bytes += bytes == 1 ? 3 : 6) {
                memset(&instruction, 0, sizeof(instruction)); memset(&address, 0, sizeof(address));
                instruction.opcode = (operation << 8) | (bytes << 4);
                instruction.record.source_location = mode & 1 ? 0xe000 : 0;
                instruction.operands[0].flags = mode == 3 ? 0x102 : 0x101;
                instruction.operands[0].value.reg.value = 5;
                address.base.words.number = 3; address.first.displacement = -9;
                address.symbol = mode == 2 ? (SymbolHeader *)1 : NULL;
                instruction.operands[1].flags = mode ? 0x204 : 0x201;
                if (mode) instruction.operands[1].value.address = &address;
                else instruction.operands[1].value.reg.value = 2;
                /* IMUL3 always owns an address even when the input tag is
                   deliberately unusual; only parent request shape is tested. */
                if (operation == MC_IMUL3) instruction.operands[1].value.address = &address;
                used = 0; events[0] = 0; listing_column = 24;
                {
                    int result = write_special_instruction(&instruction);
                    printf("%u %u %u %d %u %x %x %s\n", operation, mode, bytes, result,
                        listing_column, instruction.operands[0].flags, instruction.operands[1].flags, events);
                }
            }
    return 0;
}
