#include "code-records.h"
#include "output-callbacks.h"
#include "code-input.h"
#include <recon.h>

unsigned record_source_line;
unsigned short record_line_delta;
extern int c2_internal_listing;
extern char *c2_output_name;

void __fastcall link_code_nodes(C2Record *first, C2Record *last)
{
    C2Record *old = first->next;
    first->next = last;
    last->previous = first;
    if (old) {
        while (last->next)
            last = last->next;
        last->next = old;
        old->previous = last;
    }
}

C2DebugLink *__fastcall prepare_function_records(C2Function *function)
{
    C2FunctionSymbol *owner;
    C2Record *record;
    C2Expression *expression;
    C2Symbol *symbol;
    C2InputSymbol *declaration;
    C2DebugLink *head, *link, *debug;
    C2Section *section;
    int side;

    owner = function ? function->symbol : NULL;
    record = function ? function->flow->last->records : NULL;
    for (; record; record = record->next) {
        if (record->line) {
            record_line_delta = record->line;
            record_source_line = function->source_line_base + record->line;
        }
        switch (record->kind) {
        case 11: case 12: case 13: case 14: case 15:
        case 16: case 17: case 18: case 19: case 20:
            for (side = 2; side > 0; --side) {
                expression = side == 2 ? record->results : record->value.arguments;
                for (; expression; expression = expression->next) {
                    switch (expression->kind) {
                    case 2: case 3:
                        symbol = expression->value.reference->reference->symbol;
                        break;
                    case 4:
                        symbol = expression->value.symbol;
                        if (record->opcode != 187 && symbol->kind == 3 &&
                            expression->opcode == 298)
                            symbol = NULL;
                        break;
                    case 5: case 6:
                        symbol = expression->tail.indirect ?
                            expression->tail.indirect->reference->symbol : NULL;
                        break;
                    default:
                        symbol = NULL;
                        break;
                    }
                    if (symbol) {
                        queue_symbol_debug(symbol);
                        if (symbol->kind == 4 &&
                            (((C2FunctionSymbol *)symbol)->attributes & 0x4000) &&
                            !(((C2FunctionSymbol *)symbol)->attributes & 0x1000))
                            ((C2FunctionSymbol *)symbol)->additional_flags |= 0x20;
                    }
                }
            }
            break;
        case 22:
            queue_symbol_debug(record->owner.reference->symbol);
            break;
        case 25:
            declaration = record->value.declaration;
            if (c2_codeview_types) {
                if (declaration->symbol.source_kind != 0) {
                    if (declaration->symbol.source_kind != 2 &&
                        !declaration->detail.variable.flags)
                        break;
                    debug = c2_pool_allocate(1, sizeof(*debug));
                    debug->symbol = &declaration->symbol;
                    debug->next = function->debug_scope->records;
                    function->debug_scope->records = debug;
                }
                queue_symbol_debug(&declaration->symbol);
            } else if (declaration->detail.variable.flags)
                queue_symbol_debug(&declaration->symbol);
            break;
        }
    }
    head = prepare_global_records();
    for (link = head; link; link = link->next) {
        if (owner && (c2_internal_listing || (owner->symbol.flags & 0x20))) {
            symbol = link->symbol;
            if (has_local_symbol(symbol->serial)) {
                section = select_symbol_section(owner, symbol);
                symbol->association.section_serial = section->symbol.serial;
            }
        }
        reserve_symbol_storage(link->symbol);
    }
    return head;
}

C2InitRecord *global_initializers;
int global_records_processing;
extern int c2_zero_alignment;

C2DebugLink *prepare_global_records(void)
{
    C2InitRecord *record, *value;
    C2Symbol *symbol;
    C2DebugLink *head = NULL, **tail = &head, *link;
    unsigned attributes, flags;

    global_records_processing = 1;
    for (record = global_initializers; record; record = record->list_next) {
        if (record->emitted)
            continue;
        symbol = record->symbol;
        if (!symbol) {
            symbol = lookup_global_symbol(record->symbol_serial);
            if (!symbol)
                continue;
            record->symbol = symbol;
        }
        if ((symbol->kind == 4 || symbol->kind == 1) &&
            (((C2InputSymbol *)symbol)->attributes & 0x1000))
            symbol = resolve_alias_symbol(symbol);
        if (symbol->output_flags & 3)
            continue;
        if (symbol->output_flags & 4) {
            record_symbol_references(symbol);
            continue;
        }
        if ((symbol->flags & 0x60) == 0x20)
            continue;
        symbol->output_flags |= 4;
        if (symbol_requires_references(symbol)) {
            symbol->output_flags &= ~4;
            record_symbol_references(symbol);
        } else
            symbol->output_flags &= ~4;
    }
    for (record = global_initializers; record; record = record->list_next) {
        if (record->emitted)
            continue;
        symbol = record->symbol;
        if (!symbol) {
            symbol = lookup_global_symbol(record->symbol_serial);
            if (!symbol)
                continue;
        }
        if ((symbol->kind == 4 || symbol->kind == 1) &&
            (((C2InputSymbol *)symbol)->attributes & 0x1000))
            symbol = resolve_alias_symbol(symbol);
        if (!(symbol->output_flags & 4) || (symbol->output_flags & 3))
            continue;
        attributes = ((C2InputSymbol *)symbol)->attributes;
        if ((attributes & 0x780) != 0x80 && (attributes & 0x780) != 0x200)
            goto append;
        if ((attributes & 0x70) == 0x40)
            goto append;
        flags = symbol->flags;
        if ((flags & 0x1200) || c2_zero_alignment)
            goto append;
        for (value = record->next; value; value = value->next) {
            switch (value->kind) {
            case 4: case 8:
                break;
            case 1: case 16:
                if (initializer_is_static(value))
                    break;
                goto append;
            default:
                goto append;
            }
        }
        symbol->flags = flags & ~0x480U;
        symbol->association.section_serial = 0;
        update_symbol_storage(symbol);
        reserve_symbol_storage(symbol);
        record->emitted = 1;
        continue;

append:
        link = new_symbol_link();
        link->next = NULL;
        link->symbol = symbol;
        *tail = link;
        tail = &link->next;
    }
    /* The original shares this single RET0 body with startup's _setargv.
       This reviewed use supplies the link head in ECX, with no stack arguments
       or result. Other unresolved callback views stay stores-only. */
    ((void (__fastcall *)(C2DebugLink *)) c2_set_argv)(head);
    global_records_processing = 0;
    return head;
}

C2DebugLink *pending_debug_symbols;

void __fastcall process_records(C2DebugLink *records)
{
    if (!c2_zero_alignment)
        align_pending_debug_symbols(pending_debug_symbols);
    write_pending_debug_symbols();
    write_assembly_symbols();
    write_record_initializers(records);
    write_assembly_records(records);
}

void __fastcall align_pending_debug_symbols(C2DebugLink *symbols)
{
    C2DebugLink *next;
    C2Symbol *symbol;
    long value, half;

    if (!symbols || !is_known_symbol(symbols->symbol->serial))
        return;
    for (; symbols; symbols = next) {
        next = symbols->next;
        if (!next || symbols->symbol->value < 4096)
            continue;
        symbol = next->symbol;
        value = symbol->value;
        half = (value % 8192) / 2;
        if (half / 32 == 0 && value >= 4096)
            symbol->value = value - half + 32;
    }
}

void write_pending_debug_symbols(void)
{
    while (pending_debug_symbols) {
        output_slot_497cec.target(pending_debug_symbols->symbol);
        pending_debug_symbols = pending_debug_symbols->next;
    }
}

void __fastcall write_record_initializers(C2DebugLink *records)
{
    if (c2_output_name) {
        for (; records; records = records->next)
            emit_initializer(((C2InputSymbol *)records->symbol)->initializer);
    }
}

C2Expression *__fastcall new_control_expression(unsigned opcode)
{
    C2Expression *record = (C2Expression *)allocate_small_node(21);
    record->opcode = opcode;
    record->location.line = record_line_delta;
    return record;
}

C2Expression *__fastcall new_function_entry(C2Function *function)
{
    C2Expression *record = (C2Expression *)allocate_small_node(22);
    record->value.function = function;
    record->opcode = 0x17a;
    record->location.line = record_line_delta;
    return record;
}

C2Expression *__fastcall new_typed_register_expression(unsigned opcode,
                               unsigned short type, C2FunctionRegister *reg)
{
    C2Expression *tree = (C2Expression *)allocate_small_node(2);
    tree->opcode = opcode;
    tree->type = type;
    tree->value.reg = reg;
    return tree;
}
