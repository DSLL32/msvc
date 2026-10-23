/* Whole listing-driver control probe; child behavior is explicitly mocked. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "listing.h"
#include "instruction-encoding.h"
#include "machine-layout.h"
#include "options.h"
#include <recon.h>
#undef printf
#undef strcpy
int assembly_code;
unsigned listing_column;
static MachineInstruction first, second, first_next, second_next;
static char events[512];
static unsigned used;
static unsigned char *start;
static void event(char *text) { strcpy(events + used, text); used += strlen(text); }
MachineInstruction *__fastcall next_machine_operation(MachineInstruction *instruction)
{
    if (instruction != &first) abort();
    event("R;"); return &second;
}
unsigned __fastcall encode_machine_instruction(MachineInstruction *instruction,
    unsigned char *buffer, unsigned location, int emit)
{
    char text[64]; unsigned count, i;
    if (emit || (instruction != &first && instruction != &second)) abort();
    if (instruction == &first) { start = buffer; count = 2; }
    else { if (buffer != start + 2) abort(); count = 3; }
    for (i = 0; i < count; ++i) buffer[i] = (instruction == &first ? 0x11 : 0x21) + i;
    sprintf(text, "E%d@%u;", instruction == &first ? 1 : 2, location); event(text);
    return count;
}
void __fastcall write_listing_bytes(unsigned offset, int count, unsigned char *bytes)
{
    char text[64]; unsigned i;
    if (bytes != start || count > 5) abort();
    sprintf(text, "B%u:%u:", offset, count); event(text);
    for (i = 0; i < count; ++i) { sprintf(text, "%02x", bytes[i]); event(text); }
    event(";");
}
void __fastcall write_instruction_text(MachineInstruction *instruction)
{
    if (instruction != &first && instruction != &second) abort();
    event(instruction == &first ? "T1;" : "T2;");
}
void __fastcall write_instruction_comment(MachineInstruction *instruction)
{
    if (instruction != &first) abort(); event("C1;");
}
int recon_printf(const char *format, ...)
{
    if (!strcmp(format, "\n")) event("N;");
    else if (!strcmp(format, "\t")) event("TAB;");
    else if (!strcmp(format, " ")) event("SP;");
    else abort();
    return 1;
}
int main(void)
{
    unsigned operation, code, offset;
    MachineInstruction *result;
    first.next = &first_next; second.next = &second_next;
    for (operation = 0; operation < MC_OPCODE_COUNT; ++operation)
        for (code = 0; code < 2; ++code) {
            first.opcode = operation << 8; assembly_code = code;
            used = 0; events[0] = 0; start = NULL; offset = 17; listing_column = 0;
            result = write_instruction_listing(&first, &offset);
            if (result != &first_next && result != &second_next) abort();
            printf("%u %u %u %u %u %s\n", operation, code, offset, listing_column,
                result == &first_next ? 1 : 2, events);
        }
    return 0;
}
