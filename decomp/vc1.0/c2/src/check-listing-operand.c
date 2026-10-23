/* Whole parent with explicit prefix/symbol/displacement/register contracts. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "listing.h"
#include "instruction-selection.h"
#include <recon.h>
#undef printf
#undef strcpy
unsigned listing_column;
int dos_object;
static MachineInstruction instruction;
static SymbolHeader symbol;
static unsigned expected_bytes, used, events_used;
static char output[4096], events[4096], reg_text[32];
static void event(char *text) { strcpy(events + events_used, text); events_used += strlen(text); }
int recon_printf(const char *format, ...)
{
    int n; va_list args;
    va_start(args, format); n = vsnprintf(output + used, sizeof(output) - used, format, args); va_end(args);
    if (n < 0 || (unsigned)n >= sizeof(output) - used) abort(); used += n; return n;
}
#ifndef CHECK_PREFIX
void __fastcall write_operand_prefix(MachineInstruction *p, MachineOperand *o, unsigned bytes)
{
    char text[32];
    if (p != &instruction || o != p->operands || bytes != expected_bytes) abort();
    sprintf(text, "Q%u;", bytes); event(text);
}
#else
char *__fastcall listing_data_type(long bytes)
{
    char text[32]; sprintf(text, "T%ld;", bytes); event(text);
    sprintf(reg_text, "t%ld", bytes); return reg_text;
}
#endif
void __fastcall write_listing_symbol(SymbolHeader *p)
{
    if (p != &symbol && p != NULL) abort(); event(p ? "S1;" : "S0;");
    if (p) listing_column += recon_printf("symbol");
}
void __fastcall write_listing_displacement(long displacement, int hint)
{
    char text[64]; sprintf(text, "D%ld:%d;", displacement, hint); event(text);
}
char *__fastcall machine_register_name(unsigned reg, unsigned bytes)
{
    char text[32]; sprintf(text, "R%u:%u;", reg, bytes); event(text);
    sprintf(reg_text, "r%u_%u", reg, bytes); return reg_text;
}
int main(int argc, char **argv)
{
    MachineAddress address; unsigned kind, i;
    if (argc != 11) return 2;
    kind = strtoul(argv[1], NULL, 0); expected_bytes = strtoul(argv[3], NULL, 0);
    instruction.opcode = (strtoul(argv[2], NULL, 0) << 8) | (expected_bytes << 4);
    instruction.record.source_location = strtoul(argv[5], NULL, 0) << 13;
    instruction.operands[0].flags = kind | strtoul(argv[4], NULL, 0);
    address.first.displacement = strtol(argv[6], NULL, 0);
    address.symbol = atoi(argv[7]) ? &symbol : NULL;
    address.base.value = strtoul(argv[8], NULL, 0); address.index.value = strtoul(argv[9], NULL, 0);
    dos_object = atoi(argv[10]); listing_column = 32;
    if ((kind >= 3 && kind <= 7) || kind == 12) instruction.operands[0].value.address = &address;
    else if (kind == 8) instruction.operands[0].value.symbol = address.symbol;
    else if (kind == 1 || kind == 11) instruction.operands[0].value.reg.value = address.base.value;
    else instruction.operands[0].value.constant = address.first.displacement;
#ifdef CHECK_PREFIX
    write_operand_prefix(&instruction, instruction.operands, expected_bytes);
#else
    write_instruction_operand(&instruction, instruction.operands, expected_bytes);
#endif
    printf("%u %s\n", listing_column, events);
    for (i = 0; i < used; ++i) printf("%02x", (unsigned char)output[i]);
    printf("\n"); return 0;
}
