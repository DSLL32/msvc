#include "function-reader.h"
#include "code-input.h"
#include <recon.h>

extern FILE *expression_input, *function_symbol_input;
extern FILE *aux_expression_file, *aux_symbol_file;
extern C2SymbolListScope *current_symbol_list_scope;
extern int c2_global_optimize;

/* These stores precede the function's IL consumers. The pending-list payload
 * is deferred until its actual writer/consumer is reconstructed. */
typedef struct C2PendingInput C2PendingInput;
C2Function *input_function;
C2PendingInput *pending_input_records;
int input_error_seen, input_scope_active, input_inline_mark;
int retain_function_input_pool;
C2InputSymbol *pending_optimized_input;
unsigned input_source_line_base;
unsigned short input_value_type = 0x1004;
C2InputLineMap *input_line_cache;
char *input_line_filename;
unsigned input_last_line;
int input_line_needs_record;

unsigned reset_function_line_input(void)
{
    input_line_needs_record = 1;
    input_line_cache = NULL;
    input_line_filename = NULL;
    return input_last_line;
}

C2ScopeHash *new_function_scope(void)
{
    C2ScopeHash *scope = c2_pool_allocate(1, sizeof(C2ScopeHash));
    scope->previous = &global_scope;
    return scope;
}

C2Record *new_statement_boundary(void)
{
    C2Record *record = (C2Record *)allocate_small_node(24);
    record->opcode = 0x179;
    record->line = 0;
    return record;
}

C2Function *__fastcall read_function(C2FunctionSymbol *symbol)
{
    C2Function *function;
    C2SymbolListScope *scope;
    C2Record *first, *last;
    C2InputStack *pending = NULL;
    FILE *file;

    function = c2_pool_allocate(0, sizeof(C2Function));
    input_function = function;
    function->symbol = symbol;
    function->symbols = new_function_scope();
    function->flags = (function->flags & ~0x8000U) |
                       ((symbol->additional_flags & 0x100) << 7);
    current_symbol_list_scope = c2_pool_allocate(1, sizeof(C2SymbolListScope));
    if (c2_il_extended_header)
        c2_current_il = read_pdb_symbol_il(symbol->symbol.serial, symbol->source_file);
    else {
        file = (symbol->additional_flags & 1) ? aux_symbol_file : function_symbol_input;
        c2_current_il = file;
        fseek(file, symbol->source_file, SEEK_SET);
    }
    read_symbols((C2SymbolScope *)function);
    scope = current_symbol_list_scope;
    while (scope->parent)
        scope = scope->parent;
    pending_input_records = NULL;
    current_symbol_list_scope = scope->child;
    if (c2_il_extended_header) {
        file = read_pdb_expression_symbols(symbol->symbol.serial, symbol->source_file);
        c2_current_il = file;
        if (file)
            read_initializer_records(file);
        c2_current_il = read_pdb_expression_il(symbol->symbol.serial, symbol->source_file);
    } else {
        file = (symbol->additional_flags & 1) ? aux_expression_file : expression_input;
        c2_current_il = file;
        fseek(file, symbol->input_order, SEEK_SET);
    }
    input_error_seen = 0;
    input_scope_active = 1;
    record_source_line = 0;
    record_line_delta = 0;
    function->source_line_base = reset_function_line_input();
    input_source_line_base = function->source_line_base;
    first = new_statement_boundary();
    last = new_statement_boundary();
    link_code_nodes(first, last);
    create_function_flow(function, first, last, retain_function_input_pool ? 4 : 12);
    input_value_type = 0x3004;
    read_function_code(function, &pending, first, -1, 1);
    input_value_type = 0x1004;
    record_source_line = 0;
    record_line_delta = 0;
    c2_set_argv();
    validate_function_code(first);
    finish_function_input();
    if (c2_global_optimize && pending_optimized_input) {
        pending_optimized_input = NULL;
        finish_optimized_function_input(function);
    }
    if (c2_codeview_types)
        finish_function_debug(function);
    input_inline_mark = 0;
    return function;
}

unsigned __fastcall set_input_filename(char *filename)
{
    C2InputLineMap *entry;
    if (input_line_filename && !strcmp(input_line_filename, filename)) return 0;
    for (entry = input_line_cache; entry; entry = entry->next) {
        if (!strcmp(filename, entry->filename)) {
            input_line_filename = entry->filename;
            return 0;
        }
    }
    input_line_filename = c2_pool_allocate(0, strlen(filename) + 1);
    strcpy(input_line_filename, filename);
    return 0;
}
