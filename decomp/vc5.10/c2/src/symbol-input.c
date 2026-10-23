#include "c2.h"
#include <recon.h>

/* Shared input scratch and the64-byte block/filename record at497308. The
 * frontend emits these transient records without allocating a symbol node. */
char symbol_name[2058];
C2BlockSymbol symbol_template;
unsigned last_section_serial;
long il_option_flags;
char *auxiliary_prefix;
FILE *aux_expression_file, *aux_symbol_file;
extern int c2_constant_strings, c2_omit_frame_pointer;
C2Symbol *current_symbol;
extern unsigned current_source_file;
/* Input capacity plus the optional leading decoration character. The byte
 * before c2_constant_strings in the image is not credited as this object's
 * payload or declared storage. */
char decorated_name[2059];

C2Symbol *__fastcall read_symbol(int retain_locals)
{
    C2InputSymbol temporary, *symbol;
    C2Symbol *target;
    C2Section *section;
    FILE memory_file, *saved_input;
    char *source;
    signed char flag;
    unsigned attributes, storage, bytes;
    int kind, arena = 0, count;

    kind = (signed char)c2_read_il_byte();
    switch (kind) {
    case 1: case 2:
        memset(&temporary, 0, sizeof(temporary));
        flag = (signed char)c2_read_il_byte();
        temporary.symbol.kind = 1;
        temporary.symbol.serial = input_varuint();
        temporary.symbol.source_kind = c2_read_il_byte();
        input_string(symbol_name, sizeof(symbol_name));
        copy_symbol_type(&temporary.symbol);
        c2_read_il_byte();
        temporary.storage.local.lifetime = flag;
        temporary.attributes = (temporary.attributes & ~0x70U) |
                               ((c2_read_il_byte() & 7) << 4);
        temporary.attributes = (temporary.attributes & ~0x780U) |
                               ((c2_read_il_byte() & 15) << 7);
        temporary.symbol.value = input_compact_signed();
        temporary.symbol.flags = c2_il_extended_header ?
                                 input_extended_flags() : input_varuint();
        storage = temporary.attributes & 0x70;
        if (storage == 0x30 || storage == 0x10)
            temporary.symbol.flags &= ~0x2000U;
        storage = temporary.attributes & 0x780;
        if (storage == 0x300 || storage == 0x380)
            temporary.attributes = (temporary.attributes & ~0x700U) | 0x880;
        else if (storage == 0x480)
            temporary.attributes = (temporary.attributes & ~0x600U) | 0x980;
        else if (storage == 0x400)
            temporary.attributes = ((temporary.attributes | 0x1000) & ~0x680U) | 0x100;
        if (temporary.symbol.flags & 0x200) {
            temporary.symbol.association.section_serial = input_varuint();
            last_section_serial = temporary.symbol.association.section_serial;
        } else
            temporary.symbol.association.section_serial = 0;
        if (c2_constant_strings && (temporary.symbol.flags & 0x400) && current_symbol) {
            temporary.symbol.association.section_serial = current_symbol->association.section_serial;
            if (current_symbol->flags & 0x20)
                temporary.symbol.flags |= 0x20;
            temporary.symbol.flags |= 0x200;
            temporary.attributes = (temporary.attributes & ~0x50U) | 0x20;
        }
        if (temporary.symbol.flags & 0x1000) {
            if (!temporary.symbol.association.section_serial) {
                temporary.symbol.association.section_serial = 9;
                temporary.symbol.flags |= 0x200;
            } else {
                target = lookup_global_symbol(temporary.symbol.association.section_serial);
                if (strncmp(target->name, ".tls$", 5)) {
                    compiler_warning(1, 0x31a, symbol_name, target->name, ".tls$");
                    temporary.symbol.association.section_serial = 9;
                }
            }
        }
        temporary.symbol.source_line = c2_il_source_table ?
                                       input_compact_signed() : input_compact_short();
        if (kind == 2) {
            temporary.detail.variable.auxiliary = input_compact_signed();
            if (temporary.detail.variable.auxiliary)
                temporary.attributes |= 0x10000;
        }
        if (symbol_name[0])
            temporary.symbol.name = intern_symbol_name(&temporary.symbol, symbol_name, 0);
        storage = temporary.attributes & 0x70;
        if ((!retain_locals || (temporary.symbol.kind == 1 &&
                                (storage == 0x10 || storage == 0x30))) && flag > 0)
            arena = 1;
        symbol = c2_allocate_node((signed char)kind, arena);
        bytes = c2_node_size(kind);
        memcpy(symbol, &temporary, bytes);
        if (symbol->attributes & 0x1000)
            clone_alias_symbol(&symbol->symbol);
        return &symbol->symbol;

    case 3:
        flag = (signed char)c2_read_il_byte();
        if (flag > 0)
            arena = 1;
        symbol = c2_allocate_node((signed char)kind, arena);
        symbol->symbol.kind = 3;
        symbol->detail.variable.scope_line = flag;
        symbol->symbol.serial = input_varuint();
        symbol->symbol.source_kind = c2_read_il_byte();
        input_string(symbol_name, sizeof(symbol_name));
        input_compact_short();
        symbol->storage.frame_size = 0;
        symbol->detail.variable.flags = (c2_read_il_byte() >> 2) & 1;
        if (symbol_name[0])
            symbol->symbol.name = intern_symbol_name(&symbol->symbol, symbol_name, arena);
        return &symbol->symbol;

    case 4: case 14: case 16:
        bytes = input_varuint();
        flag = c2_read_il_byte();
        symbol = c2_allocate_node((signed char)kind, 0);
        symbol->symbol.kind = 4;
        symbol->symbol.serial = bytes;
        symbol->symbol.source_kind = flag;
        input_string(symbol_name, sizeof(symbol_name));
        copy_symbol_type(&symbol->symbol);
        symbol->attributes = (symbol->attributes & ~0xf0U) |
                             ((c2_read_il_byte() & 15) << 4);
        attributes = symbol->attributes & 0xf0;
        if (attributes == 0x60) {
            symbol->attributes = (symbol->attributes & ~0xb0U) | 0x40;
            queue_symbol_debug(&symbol->symbol);
        } else if (attributes == 0xa0)
            symbol->attributes = (symbol->attributes & ~0xb0U) | 0x1040;
        else if (attributes == 0x70 || attributes == 0x90)
            symbol->attributes = (symbol->attributes & ~0xa0U) | 0x850;
        symbol->storage.frame_size = input_compact_signed();
        symbol->symbol.flags = c2_il_extended_header ?
                               input_extended_flags() : input_varuint();
        symbol->symbol.association.section_serial = (symbol->symbol.flags & 0x200) ? input_varuint() : 5;
        symbol->symbol.source_line = c2_il_source_table ?
                                    input_compact_signed() : input_compact_short();
        if (kind == 14) {
            symbol->attributes |= 0x4000;
            if (!c2_il_extended_header) {
                symbol->input_order = input_compact_signed();
                symbol->source_file = input_compact_signed();
            }
            symbol->field64 = input_compact_short();
            symbol->additional_flags = c2_il_extended_header ?
                                       input_extended_flags() : input_varuint();
            symbol->field62 = input_compact_short();
            if (c2_il_extended_header) {
                symbol->input_order = input_compact_signed();
                input_compact_short();
            }
            if (c2_omit_frame_pointer)
                symbol->symbol.flags |= 0x20;
        } else if (kind == 16) {
            symbol->attributes |= 0x8000;
            symbol->input_order = input_varuint();
        }
        symbol->symbol.name = intern_symbol_name(&symbol->symbol, symbol_name, 0);
        if (symbol->attributes & 0x1000)
            clone_alias_symbol(&symbol->symbol);
        return &symbol->symbol;

    case 5:
        symbol = c2_allocate_node((signed char)kind, 0);
        symbol->detail.alias.target = lookup_global_symbol(input_varuint());
        if (!symbol->detail.alias.target)
            c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 0x2ca);
        symbol->symbol.serial = input_varuint();
        symbol->symbol.value = input_compact_signed();
        symbol->symbol.source_kind = c2_read_il_byte();
        symbol->symbol.source_line = c2_il_source_table ?
                                    input_compact_signed() : input_compact_short();
        return &symbol->symbol;

    case 6:
        return NULL;

    case 7:
        if ((signed char)c2_read_il_byte() > 0)
            arena = 1;
        symbol = c2_allocate_node((signed char)kind, arena);
        symbol->symbol.serial = input_varuint();
        input_string(symbol_name, sizeof(symbol_name));
        symbol->symbol.name = intern_symbol_name(&symbol->symbol, symbol_name, arena);
        copy_symbol_type(&symbol->symbol);
        c2_read_il_byte();
        symbol->symbol.value = input_compact_signed();
        symbol->attributes = (symbol->attributes & ~0x780U) |
                             ((c2_read_il_byte() & 15) << 7);
        symbol->symbol.parameter_offset = input_compact_signed();
        symbol->symbol.source_line = c2_il_source_table ?
                                    input_compact_signed() : input_compact_short();
        symbol->storage.frame_size = input_varuint();
        symbol->detail.variable.auxiliary = input_compact_signed();
        if (symbol->detail.variable.auxiliary)
            symbol->attributes |= 0x10000;
        queue_symbol_debug(&symbol->symbol);
        return &symbol->symbol;

    case 8:
        if ((signed char)c2_read_il_byte() > 0)
            arena = 1;
        symbol = c2_allocate_node((signed char)kind, arena);
        symbol->symbol.serial = input_varuint();
        input_string(symbol_name, sizeof(symbol_name));
        symbol->symbol.name = intern_symbol_name(&symbol->symbol, symbol_name, arena);
        symbol->symbol.source_line = c2_il_source_table ?
                                    input_compact_signed() : input_compact_short();
        copy_symbol_type(&symbol->symbol);
        c2_read_il_byte();
        symbol->symbol.value = input_compact_signed();
        read_qualified_symbol(&symbol->symbol, arena);
        queue_symbol_debug(&symbol->symbol);
        return &symbol->symbol;

    case 9:
        section = c2_allocate_node((signed char)kind, 0);
        section->symbol.serial = input_varuint();
        input_string(symbol_name, sizeof(symbol_name));
        section->section_kind = c2_read_il_byte();
        section->symbol.name = intern_symbol_name(&section->symbol, symbol_name, 0);
        input_string(symbol_name, sizeof(symbol_name));
        section->class_name = arena_strdup(symbol_name, 0);
        section->attribute = section->section_kind != 1;
        return &section->symbol;

    case 10:
        il_option_flags = input_compact_signed();
        input_string(symbol_name, sizeof(symbol_name));
        if (symbol_name[0] && !c2_il_extended_header) {
            auxiliary_prefix = arena_strdup(symbol_name, 0);
            aux_expression_file = c2_open_intermediate(auxiliary_prefix, "", "r", "Pch exp il");
            aux_symbol_file = c2_open_intermediate(auxiliary_prefix, "", "r", "Pch sym il");
        }
        count = input_compact_signed();
        while (count--)
            read_module_directive((signed char)c2_read_il_byte());
        return NULL;

    case 11:
        if ((signed char)c2_read_il_byte() > 0)
            arena = 1;
        symbol = c2_allocate_node((signed char)kind, arena);
        symbol->symbol.serial = input_varuint();
        input_string(symbol_name, sizeof(symbol_name));
        symbol->symbol.name = intern_symbol_name(&symbol->symbol, symbol_name, arena);
        c2_read_il_byte();
        symbol->symbol.source_line = c2_il_source_table ?
                                    input_compact_signed() : input_compact_short();
        queue_symbol_debug(&symbol->symbol);
        return &symbol->symbol;

    case 13:
        symbol_template.symbol.kind = 13;
        symbol_template.scope_line = c2_read_il_byte();
        return &symbol_template.symbol;

    case 18:
        symbol_template.symbol.kind = 18;
        symbol_template.symbol.serial = input_varuint();
        input_string(symbol_name, sizeof(symbol_name));
        symbol_template.symbol.name = intern_symbol_name(&symbol_template.symbol, symbol_name, 0);
        return &symbol_template.symbol;

    case 22:
        symbol_template.symbol.kind = 22;
        input_string(symbol_name, sizeof(symbol_name));
        source = c2_pool_allocate(4, strlen(symbol_name) + 1);
        strcpy(source, symbol_name);
        if (!register_source_file(source))
            c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 0x352);
        ++current_source_file;
        return &symbol_template.symbol;

    case 23:
        input_string(symbol_name, sizeof(symbol_name));
        source = lookup_source_file(symbol_name, &bytes);
        if (!source)
            c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 0x361);
        saved_input = c2_current_il;
        /* Only these four FILE fields are initialized in the original. */
        memory_file._ptr = source;
        memory_file._cnt = bytes;
        memory_file._flag = 1;
        memory_file._file = -1;
        c2_current_il = &memory_file;
        read_symbols(NULL);
        free(source);
        c2_current_il = saved_input;
        symbol_template.symbol.kind = 23;
        return &symbol_template.symbol;

    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 0x373);
        return NULL;
    }
}

char *__fastcall intern_symbol_name(C2Symbol *symbol, char *name, int pool)
{
    char *result = decorated_name + 1;

    decorated_name[0] = '_';
    decorated_name[1] = 0;
    strcpy(result, name);
    if (name[0] != '?' || symbol->source_kind) {
        switch (symbol->kind) {
        case 1: case 7: case 8: case 11:
            result = decorated_name;
            break;
        case 3:
            if (symbol->source_kind)
                return NULL;
            result = decorated_name;
            result[0] = '$';
            break;
        case 4:
            switch (symbol->flags & 0x1c) {
            case 0: case 16: case 28:
                result = decorated_name;
                break;
            case 4: case 20: case 24:
                result = decorated_name;
                result[0] = '@';
                break;
            case 8: case 12:
                if (symbol->source_kind != 0x19)
                    uppercase_symbol_name(decorated_name + 1);
                break;
            }
            break;
        }
    }
    if ((symbol->kind == 1 || symbol->kind == 4) &&
        (((C2FunctionSymbol *)symbol)->attributes & 0x1000)) {
        memmove(result + 6, result, strlen(result) + 1);
        memcpy(result, "__imp_", 6);
    }
    c2_node_bytes[pool] += strlen(result);
    return arena_strdup(result, pool);
}

char *__fastcall arena_strdup(char *name, int pool)
{
    char *copy = c2_pool_allocate(pool, strlen(name) + 1);
    strcpy(copy, name);
    return copy;
}
