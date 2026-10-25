#include "c2.h"
#include <recon.h>

C2DebugScope *current_debug_scope;
C2SymbolListScope *current_symbol_list_scope;
C2FunctionSymbol *pending_functions;
C2FunctionSymbol **pending_insertion;
C2Symbol *global_special_symbols;
unsigned current_source_file = (unsigned)-1;
C2Filename *filenames;

int __fastcall function_retains_symbols(C2FunctionSymbol *symbol, int conditional)
{
    unsigned flags = symbol->additional_flags;
    return (flags & 0x40) && (conditional || (flags & 0x80)) &&
           !(symbol->symbol.flags & 0x1400) && !(flags & 0x200);
}

void __fastcall record_filename(C2Symbol *symbol)
{
    C2Filename *file = c2_pool_allocate(0, sizeof(C2Filename));
    file->serial = symbol->serial;
    file->name = symbol->name;
    file->next = filenames;
    filenames = file;
}

void __fastcall read_symbols(C2SymbolScope *scope)
{
    C2ScopeHash *table;
    C2FunctionSymbol *symbol, **insertion;
    C2Symbol *existing;
    C2DebugLink *link;
    C2DebugScope *debug_scope;
    C2SymbolListScope *list_scope;
    C2Section *section;
    unsigned attributes, storage;
    int retain_locals;

    if (scope) {
        table = scope->symbols;
        retain_locals = function_retains_symbols(scope->symbol, 1);
    } else {
        table = &global_scope;
        retain_locals = 0;
    }
    if (c2_codeview_types && scope) {
        current_debug_scope = c2_pool_allocate(1, sizeof(C2DebugScope));
        scope->debug_scope = current_debug_scope;
        current_debug_scope->records = NULL;
        current_debug_scope->line = 0;
    }
    for (;;) {
        symbol = (C2FunctionSymbol *)read_symbol(retain_locals);
        if (!symbol)
            return;
        switch (symbol->symbol.kind) {
        case 1:
            symbol->attributes &= ~0x8000U;
            attributes = symbol->attributes;
            storage = attributes & 0x70;
            if ((attributes & 0x780) != 0x200 &&
                (attributes & 0x780) != 0x100 &&
                (storage == 0x20 || storage == 0x50 || storage == 0x40) &&
                !(symbol->symbol.flags & 0x480) &&
                (symbol->symbol.flags & 0x60) != 0x20)
                queue_symbol_debug(&symbol->symbol);
            update_symbol_storage(&symbol->symbol);
            storage = symbol->attributes & 0x70;
            if (retain_locals && !(symbol->symbol.kind == 1 &&
                                  (storage == 0x10 || storage == 0x30))) {
                existing = lookup_symbol(table->previous, symbol->symbol.serial);
                if (existing)
                    symbol = (C2FunctionSymbol *)existing;
                else
                    put_symbol(table->previous, &symbol->symbol);
            } else
                put_symbol(table, &symbol->symbol);
            if ((symbol->attributes & 0x70) != 0x30 &&
                !(symbol->symbol.flags & 0x480))
                reserve_symbol_storage(&symbol->symbol);
            break;
        case 3: case 5: case 7: case 8:
            put_symbol(table, &symbol->symbol);
            break;
        case 4:
            update_symbol_storage(&symbol->symbol);
            if ((symbol->symbol.flags & 0x1c) == 8)
                symbol->attributes = (symbol->attributes & ~0x400U) | 0x300;
            if (symbol->attributes & 0x4000) {
                insertion = pending_insertion;
                if (!insertion || (*insertion)->input_order > symbol->input_order)
                    insertion = &pending_functions;
                while (*insertion && (*insertion)->input_order < symbol->input_order)
                    insertion = &(*insertion)->pending_next;
                pending_insertion = insertion;
                symbol->pending_next = *insertion;
                *insertion = symbol;
                if (c2_il_extended_header)
                    symbol->source_file = current_source_file;
            }
            put_symbol(table, &symbol->symbol);
            break;
        case 9:
            section = (C2Section *)symbol;
            initialize_section_defaults(section);
            if (!section->section_kind)
                section->alignment = stack_alignment;
            append_section(section);
            insert_symbol(&section->symbol, 0);
            break;
        case 11:
            if (table == &global_scope) {
                symbol->symbol.hash_next = global_special_symbols;
                global_special_symbols = &symbol->symbol;
            } else
                put_symbol(table, &symbol->symbol);
            break;
        case 13:
            if (c2_codeview_types) {
                debug_scope = c2_pool_allocate(1, sizeof(C2DebugScope));
                current_debug_scope->next = debug_scope;
                current_debug_scope = debug_scope;
                debug_scope->records = NULL;
                debug_scope->line = symbol->scope_line;
            }
            if (current_symbol_list_scope) {
                list_scope = c2_pool_allocate(1, sizeof(C2SymbolListScope));
                list_scope->parent = current_symbol_list_scope;
                current_symbol_list_scope->child = list_scope;
                current_symbol_list_scope = list_scope;
            }
            continue;
        case 18:
            record_filename(&symbol->symbol);
            continue;
        case 22: case 23:
            continue;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 0x162);
            break;
        }
        if (current_debug_scope) {
            link = c2_pool_allocate(1, sizeof(C2DebugLink));
            link->symbol = &symbol->symbol;
            link->next = current_debug_scope->records;
            current_debug_scope->records = link;
        }
        if (current_symbol_list_scope && symbol->symbol.kind == 1) {
            link = c2_pool_allocate(1, sizeof(C2DebugLink));
            link->symbol = &symbol->symbol;
            link->next = current_symbol_list_scope->records;
            current_symbol_list_scope->records = link;
        }
    }
}
