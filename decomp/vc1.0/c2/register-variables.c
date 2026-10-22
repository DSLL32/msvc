/* Register-variable records shared by global and machine allocation. */
#include "register-variables.h"
#include "backend.h"
/* Original [0007c730,0007c8b0). Constructor/lookup indexing selects a
   128-byte table from the register's class bits (00, 4000 or 8000), then
   selects one of 32 hash heads with its low five bits. The reset clears
   exactly 96 pointers; record contents remain owned by their arena. */
RegisterVariable *register_variable_buckets[REGISTER_VARIABLE_CLASSES][REGISTER_VARIABLE_BUCKETS];

/* Original [0002db30,0002db50). */
void reset_global_register_map(void)
{
    unsigned group, bucket;
    for (group = 0; group < REGISTER_VARIABLE_CLASSES; ++group)
        for (bucket = 0; bucket < REGISTER_VARIABLE_BUCKETS; ++bucket)
            register_variable_buckets[group][bucket] = 0;
}

/* Original [0002da40,0002dad0). Record numbered-block bounds and a memory
   home. A double-word integer range has two adjacent four-byte register
   records, preserving the original same-home metadata for both halves. */
void __fastcall record_register_variable_range(unsigned short number, short first_block, short last_block,
    int observable, int parameter, unsigned type, SymbolHeader *symbol, unsigned offset)
{
    RegisterVariable *variable;
    for (;;) {
        variable = create_register_variable(number);
        variable->flags = (variable->flags & ~0x200U) | (((unsigned)parameter << 9) & 0x200U);
        variable->flags = (variable->flags & ~0x100U) | (((unsigned)observable << 8) & 0x100U);
        variable->last_block = last_block;
        variable->first_block = first_block;
        variable->home_symbol = symbol;
        variable->home_offset = offset;
        if (type != 8) break;
        variable->type = 4;
        ++number;
        type = 4;
    }
    variable->type = (unsigned char)type;
}

/* Original [0002b6d0,0002b700). Link a newly constructed variable into
   its physical/tree/machine class and low-five-bit hash bucket. */
RegisterVariable *__fastcall create_register_variable(unsigned short number)
{
    RegisterVariable **bucket, *variable;
    bucket = &register_variable_buckets[number >> 14][number & 31];
    variable = new_register_variable(number);
    variable->next = *bucket;
    *bucket = variable;
    return variable;
}

/* Original [0002d9e0,0002da40). Initialize the native 52-byte record.
   Physical registers retain a six-bit number; either virtual class receives
   the unassigned value32 in the low field of flags. */
RegisterVariable *__fastcall new_register_variable(unsigned short number)
{
    RegisterVariable *variable = arena_alloc(2, sizeof(RegisterVariable));
    variable->next = 0;
    variable->field04 = variable->field08 = variable->field0c = 0;
    variable->field10 = variable->field14 = 0;
    variable->number = variable->field1a = number;
    variable->flags = 0;
    variable->field20 = 0;
    variable->first_block = variable->last_block = 0;
    variable->type = variable->field29 = 0;
    variable->field2a = 0;
    variable->home_symbol = 0;
    variable->home_offset = 0;
    if (number & 0xc000) variable->flags = (variable->flags & ~31U) | 32;
    else variable->flags = (variable->flags & ~63U) | (number & 63);
    return variable;
}
