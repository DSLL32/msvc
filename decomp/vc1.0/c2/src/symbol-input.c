/* Typed symbol variants encoded by the frontend. */
#include "records.h"
#include "input.h"
#include "segments.h"
#include "symbol-constants.h"
#include <string.h>
#include <recon.h>
extern int string_blocks, frame_pointer;
FunctionSymbol symbol_template;
long il_option_flags;
char *auxiliary_prefix;
FILE *aux_expression_file, *aux_symbol_file;
/* Original [0004d240,0004da40), including the kind-dispatch table. */
SymbolHeader *__fastcall read_symbol(int retain_locals)
{
    FunctionSymbol temporary, *symbol;
    SymbolHeader *target;
    Segment *segment;
    char name[256];
    signed char flag;
    int kind, arena = 0, count;
    unsigned attributes, storage, bytes, i;
    unsigned *words;
    unsigned char *source, *destination;
    kind = (signed char)input_byte();
    switch (kind) {
    case 1: case 2:
        words = (unsigned *)&temporary;
        for (i = 0; i != sizeof(temporary)/4; ++i) *words++ = 0;
        flag = (signed char)input_byte();
        temporary.symbol.kind = 1;
        temporary.symbol.identifier = input_varuint();
        temporary.symbol.field0d = input_byte();
        input_string(name, 255);
        copy_symbol_type(&temporary.symbol);
        input_byte();
        temporary.field28 = (temporary.field28 & 0xff00ffffU) | ((unsigned)(unsigned char)flag << 16);
        temporary.field24 = (temporary.field24 & ~0x70U) | (((unsigned)(signed char)input_byte() << 4) & 0x70);
        temporary.field24 = (temporary.field24 & ~0x780U) | (((unsigned)(signed char)input_byte() << 7) & 0x780);
        storage = temporary.field24 & 0x780;
        if (storage == 0x400) {
            temporary.field24 |= 0x1000;
            temporary.field24 = (temporary.field24 & 0xfffff97fU) | 0x100;
        } else if (storage == 0x300 || storage == 0x380) {
            temporary.field24 = (temporary.field24 & 0xfffff8ffU) | 0x80;
            temporary.field24 |= 0x800;
        }
        temporary.symbol.value = input_compact_signed();
        temporary.symbol.field0e = (unsigned short)input_compact_short();
        temporary.symbol.field18.identifier = (temporary.symbol.field0e & 0x200) ? input_varuint() : 0;
        if (string_blocks && (temporary.symbol.field0e & 0x400) && current_symbol &&
            !(current_symbol->symbol.field0e & 0x20)) {
            temporary.symbol.field18.identifier = current_symbol->symbol.field18.identifier;
            temporary.symbol.field0e |= 0x200;
            temporary.field24 = (temporary.field24 & ~0x50U) | 0x20;
        }
        if (temporary.symbol.field0e & 0x1000) {
            if (!temporary.symbol.field18.identifier) {
                temporary.symbol.field18.identifier = 9;
                temporary.symbol.field0e |= 0x200;
            } else {
                target = lookup_global_symbol(temporary.symbol.field18.identifier);
                if (strncmp(target->name, ".tls$", 5)) {
                    compiler_warning(1, 0x31a, name, target->name, ".tls$");
                    temporary.symbol.field18.identifier = 9;
                }
            }
        }
        temporary.field20 = (temporary.field20 & 0xffff0000U) | (unsigned short)input_compact_short();
        if (kind == 2) {
            temporary.field2c = input_compact_signed();
            if (temporary.field2c) temporary.field24 |= 0x10000;
        }
        if (*name) temporary.symbol.name = intern_symbol_name(&temporary.symbol, name, 0);
        storage = temporary.field24 & 0x70;
        if ((!retain_locals || (temporary.symbol.kind == 1 && (storage == 0x10 || storage == 0x30)) ||
            temporary.symbol.kind == 19) && flag > 0) arena = 1;
        symbol = allocate_descriptor((signed char)kind, arena);
        bytes = descriptor_size((signed char)kind);
        source = (unsigned char *)&temporary;
        destination = (unsigned char *)symbol;
        while (bytes--) *destination++ = *source++;
        if (symbol->field24 & 0x1000) clone_alias_symbol(&symbol->symbol);
        return &symbol->symbol;
    case 3:
        flag = (signed char)input_byte();
        if (flag > 0) arena = 1;
        symbol = allocate_descriptor((signed char)kind, arena);
        symbol->symbol.kind = 3;
        symbol->field30 = (symbol->field30 & 0xffff00ffU) | ((unsigned)(unsigned char)flag << 8);
        symbol->symbol.identifier = input_varuint();
        symbol->symbol.field0d = input_byte();
        input_string(name, 255);
        symbol->field2c = (signed char)input_compact_short();
        symbol->field30 = (symbol->field30 & ~0xffU) | input_byte();
        if (*name) symbol->symbol.name = intern_symbol_name(&symbol->symbol, name, arena);
        return &symbol->symbol;
    case 4: case 14: case 16:
        i = input_varuint();
        flag = (signed char)input_byte();
        symbol = allocate_descriptor((signed char)kind, 0);
        symbol->symbol.kind = 4;
        symbol->symbol.identifier = i;
        symbol->symbol.field0d = (unsigned char)flag;
        input_string(name, 255);
        copy_symbol_type(&symbol->symbol);
        symbol->field24 = (symbol->field24 & ~0xf0U) | (((unsigned)(signed char)input_byte() << 4) & 0xf0);
        storage = symbol->field24 & 0xf0;
        if (storage == 0x60) {
            symbol->field24 = (symbol->field24 & 0xffffff4fU) | 0x40;
            queue_symbol_debug(&symbol->symbol);
        } else if (storage == 0xa0) {
            symbol->field24 = (symbol->field24 & 0xffffff4fU) | 0x40;
            symbol->field24 |= 0x1000;
        } else if (storage == 0x70 || storage == 0x90) {
            symbol->field24 = (symbol->field24 & 0xffffff5fU) | 0x50;
            symbol->field24 |= 0x800;
        }
        symbol->field28 = input_compact_signed();
        symbol->symbol.field0e = (unsigned short)input_compact_short();
        symbol->symbol.field18.identifier = (symbol->symbol.field0e & 0x200) ? input_varuint() : 5;
        symbol->field20 = (symbol->field20 & 0xffff0000U) | (unsigned short)input_compact_short();
        if (kind == 14) {
            symbol->field24 |= 0x4000;
            symbol->field40 = input_compact_signed();
            symbol->field44 = input_compact_signed();
            symbol->field50 = (unsigned short)input_compact_short();
            symbol->flags52 = input_byte();
            symbol->field48 = (symbol->field48 & 0xffff0000U) | (unsigned short)(signed char)input_byte();
            if (frame_pointer) symbol->symbol.field0e |= 0x20;
        } else if (kind == 16) {
            symbol->field24 |= 0x8000;
            symbol->field40 = input_varuint();
        }
        symbol->symbol.name = intern_symbol_name(&symbol->symbol, name, 0);
        if (symbol->field24 & 0x1000) clone_alias_symbol(&symbol->symbol);
        return &symbol->symbol;
    case 5:
        symbol = allocate_descriptor((signed char)kind, 0);
        symbol->field2c = (unsigned)lookup_global_symbol(input_varuint());
        if (!symbol->field2c) compiler_internal_error("@(#)p2symtab.c:1.89", 0x256);
        symbol->symbol.identifier = input_varuint();
        symbol->symbol.value = input_compact_signed();
        symbol->symbol.field0d = input_byte();
        symbol->field20 = (symbol->field20 & 0xffff0000U) | (unsigned short)input_compact_short();
        return &symbol->symbol;
    case 6:
        return NULL;
    case 7:
        if ((signed char)input_byte() > 0) arena = 1;
        symbol = allocate_descriptor((signed char)kind, arena);
        symbol->symbol.identifier = input_varuint();
        input_string(name, 255);
        symbol->symbol.name = intern_symbol_name(&symbol->symbol, name, arena);
        copy_symbol_type(&symbol->symbol);
        input_byte();
        symbol->symbol.value = input_compact_signed();
        symbol->field24 = (symbol->field24 & ~0x780U) | (((unsigned)(signed char)input_byte() << 7) & 0x780);
        symbol->symbol.field04 = input_compact_signed();
        symbol->field20 = (symbol->field20 & 0xffff0000U) | (unsigned short)input_compact_short();
        symbol->field28 = input_varuint();
        symbol->field2c = input_compact_signed();
        if (symbol->field2c) symbol->field24 |= 0x10000;
        queue_symbol_debug(&symbol->symbol);
        return &symbol->symbol;
    case 8:
        if ((signed char)input_byte() > 0) arena = 1;
        symbol = allocate_descriptor((signed char)kind, arena);
        symbol->symbol.identifier = input_varuint();
        input_string(name, 255);
        symbol->symbol.name = intern_symbol_name(&symbol->symbol, name, arena);
        symbol->field20 = (symbol->field20 & 0xffff0000U) | (unsigned short)input_compact_short();
        copy_symbol_type(&symbol->symbol);
        input_byte();
        symbol->symbol.value = input_compact_signed();
        read_qualified_symbol(&symbol->symbol, arena);
        queue_symbol_debug(&symbol->symbol);
        return &symbol->symbol;
    case 9:
        segment = allocate_descriptor((signed char)kind, 0);
        segment->symbol.identifier = input_varuint();
        input_string(name, 255);
        segment->section_number = input_byte();
        segment->symbol.name = intern_symbol_name(&segment->symbol, name, 0);
        input_string(name, 255);
        segment->class_name = arena_strdup(name, 0);
        segment->format_flags = segment->section_number != 1;
        return &segment->symbol;
    case 10:
        il_option_flags = input_compact_signed();
        input_string(name, 255);
        if (*name) {
            auxiliary_prefix = arena_strdup(name, 0);
            aux_expression_file = open_intermediate(auxiliary_prefix, "", "r", "Pch exp il");
            aux_symbol_file = open_intermediate(auxiliary_prefix, "", "r", "Pch sym il");
        }
        count = (signed char)input_byte();
        while (count--) read_module_directive((signed char)input_byte());
        return NULL;
    case 11:
        if ((signed char)input_byte() > 0) arena = 1;
        symbol = allocate_descriptor((signed char)kind, arena);
        symbol->symbol.identifier = input_varuint();
        input_string(name, 255);
        symbol->symbol.name = intern_symbol_name(&symbol->symbol, name, arena);
        input_byte();
        symbol->field20 = (symbol->field20 & 0xffff0000U) | (unsigned short)input_compact_short();
        queue_symbol_debug(&symbol->symbol);
        return &symbol->symbol;
    case 13:
        symbol_template.symbol.kind = 13;
        symbol_template.field30 = (symbol_template.field30 & 0xffff00ffU) | ((unsigned)input_byte() << 8);
        return &symbol_template.symbol;
    case 18:
        symbol_template.symbol.kind = (signed char)kind;
        symbol_template.symbol.identifier = input_varuint();
        input_string(name, 255);
        symbol_template.symbol.name = intern_symbol_name(&symbol_template.symbol, name, 0);
        return &symbol_template.symbol;
    default:
        compiler_internal_error("@(#)p2symtab.c:1.89", 0x2e0);
        return NULL;
    }
}

unsigned short input_type(void);
/* Original [0004da40,0004da70). */
void __fastcall copy_symbol_type(SymbolHeader *header)
{
    FunctionSymbol *symbol = (FunctionSymbol *)header;
    unsigned type = input_type() & 0x1f;
    if (type & 0x10) {
        type &= ~0x10;
        symbol->field24 |= 0x4000;
    }
    symbol->field24 = (symbol->field24 & ~0xfU) | (type & 0xf);
}

/* Original4da70..4daae, ECX kind8 descriptor/EDX arena, RET0. The
   provisional name predates recovery of this counted constant-value payload.
   A signed length is stored before allocating and reading exactly that many
   bytes; the original supplies no terminator or extra range check. */
void __fastcall read_qualified_symbol(SymbolHeader *symbol, int arena)
{
    SymbolConstant *constant = (SymbolConstant *)symbol;
    unsigned char *destination;
    int count;
    constant->value_size = input_compact_short();
    constant->value_bytes = destination = arena_alloc(arena, constant->value_size);
    for (count = constant->value_size; count > 0; --count)
        *destination++ = input_byte();
}
