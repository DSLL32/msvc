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
static char events[512], output[512];
static unsigned used, events_used;
static SymbolHeader symbol;
static void event(char *text) { strcpy(events + events_used, text); events_used += strlen(text); }
int recon_printf(const char *format, ...)
{
    int n; va_list args;
    va_start(args, format); n = vsnprintf(output + used, sizeof(output) - used, format, args); va_end(args);
    if (n < 0 || (unsigned)n >= sizeof(output) - used) abort(); used += n; return n;
}
int __fastcall write_listing_comment(int first)
{
    if (first != 0 && first != 1) abort(); event(first ? "C1;" : "C0;");
    listing_column += first ? 2 : 1; return 0;
}
int __fastcall write_demangled_listing(SymbolHeader *p, int first)
{
    if (p != &symbol || (first != 0 && first != 1)) abort();
    event(first ? "D1;" : "D0;"); listing_column += 1; return 0;
}
int main(int argc, char **argv)
{
    MachineInstruction instruction;
    MachineAddress address;
    unsigned i, shape, kind;
    if (argc != 8) return 2;
    memset(&instruction, 0, sizeof(instruction)); memset(&address, 0, sizeof(address));
    instruction.opcode = strtoul(argv[7], NULL, 0) << 8;
    instruction.record.source_location = strtoul(argv[3], NULL, 0) << 13;
    shape = strtoul(argv[4], NULL, 0); symbol.kind = shape == 1 ? 1 : shape == 2 ? 4 : shape == 3 ? 2 : 4;
    symbol.field0d = shape == 4; symbol.name = shape == 5 ? "plain" : "?decorated";
    address.symbol = shape ? &symbol : NULL;
    for (i = 0; i < 2; ++i) {
        kind = strtoul(argv[i + 1], NULL, 0); instruction.operands[i].flags = kind;
        if (kind == 3 || kind == 6) instruction.operands[i].value.address = &address;
        else if (kind == 8) instruction.operands[i].value.symbol = address.symbol;
        else instruction.operands[i].value.constant = strtoul(argv[i + 5], NULL, 0);
    }
    listing_column = 35; write_instruction_comment(&instruction);
    printf("%u %s\n", listing_column, events);
    for (i = 0; i < used; ++i) printf("%02x", (unsigned char)output[i]);
    printf("\n"); return 0;
}
