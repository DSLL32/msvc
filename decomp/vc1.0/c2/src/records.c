/* Symbol record input and scope registration. */
#include "records.h"
#include "segments.h"
#include <recon.h>
extern int codeview_debug, stack_alignment;
DebugScope *current_debug_scope;
FunctionSymbol **pending_insertion;
/* Original [0004ceb0,0004d1a0), including its .text dispatch table. */
void __fastcall read_global_records(Function *function)
{
    ScopeHash *scope;
    SymbolHeader *existing;
    int retain_locals;
    FunctionSymbol *symbol;
    FunctionSymbol **insertion;
    Segment *segment;
    DebugScope *debug_scope;
    DebugLink *link;
    unsigned attributes, storage;
    int kind;
    if (!function) {
        scope = &global_scope;
        retain_locals = 0;
    } else {
        scope = function->scope;
        retain_locals = function_retains_symbols((FunctionSymbol *)function->symbol, 1);
    }
    if (codeview_debug && function) {
        current_debug_scope = arena_alloc(1, sizeof(DebugScope));
        function->debug_scope = current_debug_scope;
        current_debug_scope->records = NULL;
        current_debug_scope->line = 0;
    }
    for (;;) {
        symbol = (FunctionSymbol *)read_symbol(retain_locals);
        debug_scope = current_debug_scope;
        if (!symbol) return;
        kind = symbol->symbol.kind;
        switch (kind) {
        case 1:
            attributes = symbol->field24 & ~0x8000U;
            attributes = (attributes & ~0x2000U) | (symbol->symbol.field0e & 0x2000);
            symbol->field24 = attributes;
            storage = attributes & 0x70;
            if ((attributes & 0x780) != 0x200 && (attributes & 0x780) != 0x100 &&
                (storage == 0x20 || storage == 0x50 || storage == 0x40) &&
                !(symbol->symbol.field0e & 0x4a0))
                queue_symbol_debug(&symbol->symbol);
            update_symbol_storage(&symbol->symbol);
            if (retain_locals && !(symbol->symbol.kind == 1 &&
                ((symbol->field24 & 0x70) == 0x10 || (symbol->field24 & 0x70) == 0x30)) && symbol->symbol.kind != 19) {
                existing = lookup_symbol(scope->previous, symbol->symbol.identifier);
                if (existing) symbol = (FunctionSymbol *)existing;
                else put_symbol(scope->previous, &symbol->symbol);
            } else put_symbol(scope, &symbol->symbol);
            if ((symbol->field24 & 0x70) != 0x30 && !(symbol->symbol.field0e & 0x480))
                reserve_symbol_storage(&symbol->symbol);
            break;
        case 3:
            put_symbol(scope, &symbol->symbol);
            break;
        case 4:
            update_symbol_storage(&symbol->symbol);
            if ((symbol->symbol.field0e & 0x1c) == 8)
                symbol->field24 = (symbol->field24 & ~0x400U) | 0x300;
            if (symbol->field24 & 0x4000) {
                insertion = pending_insertion;
                if (!insertion || (long)(*insertion)->field40 > (long)symbol->field40)
                    insertion = &pending_functions;
                while (*insertion && (long)(*insertion)->field40 < (long)symbol->field40) {
                    pending_insertion = insertion;
                    insertion = &(*insertion)->pending_next;
                }
                pending_insertion = insertion;
                symbol->pending_next = *insertion;
                *insertion = symbol;
            }
            put_symbol(scope, &symbol->symbol);
            break;
        case 5: case 7: case 8: case 11:
            put_symbol(scope, &symbol->symbol);
            break;
        case 9:
            segment = (Segment *)symbol;
            segment_defaults(segment);
            if (!segment->section_number) segment->alignment = stack_alignment;
            append_segment(segment);
            insert_symbol(&symbol->symbol, 0);
            break;
        case 13:
            if (codeview_debug) {
                debug_scope->next = arena_alloc(1, sizeof(DebugScope));
                current_debug_scope = current_debug_scope->next;
                current_debug_scope->records = NULL;
                current_debug_scope->line = (signed char)(symbol->field30 >> 8);
            }
            continue;
        case 18:
            record_filename(&symbol->symbol);
            continue;
        default:
            compiler_internal_error("@(#)p2symtab.c:1.89", 0x130);
            break;
        }
        if (current_debug_scope) {
            link = arena_alloc(1, sizeof(DebugLink));
            link->symbol = &symbol->symbol;
            link->next = current_debug_scope->records;
            current_debug_scope->records = link;
        }
    }
}

typedef struct FilenameEntry {
    unsigned identifier;
    char *name;
    struct FilenameEntry *next;
} FilenameEntry;
FilenameEntry *filename_entries;
/* Original [0004d1a0,0004d1d0). */
void __fastcall record_filename(SymbolHeader *symbol)
{
    FilenameEntry *entry = arena_alloc(0, sizeof(FilenameEntry));
    entry->identifier = symbol->identifier;
    entry->name = symbol->name;
    entry->next = filename_entries;
    filename_entries = entry;
}

/* Original [00063f40,00063f60). Flags select persistent local records. */
int __fastcall function_retains_symbols(FunctionSymbol *symbol, int conditional)
{
    return (symbol->flags52 & 0x80) || (conditional && (symbol->flags52 & 0x40)) ? 1 : 0;
}

/* Original [0004e510,0004e530). Input identifiers must name a known file. */
char *__fastcall lookup_filename(unsigned identifier)
{
    FilenameEntry *entry = filename_entries;
    while (entry->identifier != identifier) entry = entry->next;
    return entry->name;
}

/* Original [0004dd30,0004dd83). Imported names begin with __imp_; the
   ordinary target shares the suffix in the same arena-owned string. */
ImportedSymbolAlias *imported_symbol_aliases;
void __fastcall clone_alias_symbol(SymbolHeader *source)
{
    FunctionSymbol *target;
    ImportedSymbolAlias *alias;
    unsigned next_symbol_identifier(void);
    target = (FunctionSymbol *)clone_descriptor(source, 0);
    target->symbol.identifier = next_symbol_identifier();
    put_symbol(&global_scope, &target->symbol);
    target->field24 &= ~0x1000U;
    target->symbol.name = source->name + 6;
    alias = arena_alloc(0, sizeof(ImportedSymbolAlias));
    alias->imported = source;
    alias->target = &target->symbol;
    alias->next = imported_symbol_aliases;
    imported_symbol_aliases = alias;
}

/* Original [0004e2b0,0004e2e1). Copy exactly the kind's descriptor extent,
   retaining pointers and padding as the original REP MOVSD/MOVSB does. */
SymbolHeader *__fastcall clone_descriptor(SymbolHeader *source, unsigned arena)
{
    SymbolHeader *result;
    unsigned bytes;
    unsigned char *from, *to;
    result = allocate_descriptor(source->kind, arena);
    bytes = descriptor_size(source->kind);
    from = (unsigned char *)source;
    to = (unsigned char *)result;
    while (bytes--) *to++ = *from++;
    return result;
}
