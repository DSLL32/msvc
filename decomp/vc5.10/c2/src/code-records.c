#include "code-records.h"
#include <recon.h>

unsigned record_source_line;
unsigned short record_line_delta;
extern int c2_internal_listing;

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
    record = function ? function->flow->entry->records : NULL;
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
                        symbol = expression->indirect ?
                            expression->indirect->reference->symbol : NULL;
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
            queue_symbol_debug(record->reference->symbol);
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
