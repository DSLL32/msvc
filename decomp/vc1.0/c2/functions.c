/* Per-function compilation pipeline. */
#include "functions.h"
#include "function-input.h"
#include "input.h"
extern int optimize_global, generate_disabled;
FunctionSymbol *pending_functions, *current_symbol;
WorkItem *current_work;
Function *current_function;
int backend_stage; /* 00083174: also the canonicalizer's expression-check state */
Node *(__fastcall *tree_transform)(Node *node);
/* Original [0004c2c0,0004c4d0). */
void compile_functions(void)
{
    FunctionSymbol **link, *symbol;
    Function *function;
    WorkItem *work, *next;
    int inline_function;
    input_file = global_file;
    read_global_records(NULL);
    fclose(global_file);
    emit_backend_work();
    for (;;) {
        backend_stage = 0;
        link = &pending_functions;
        symbol = *link;
        while (symbol) {
            if ((symbol->flags52 & 0x20) && !(symbol->flags52 & 2)) {
                symbol->flags52 |= 2;
                *link = symbol->pending_next;
                break;
            }
            link = &symbol->pending_next;
            symbol = *link;
        }
        current_symbol = symbol;
        if (!symbol) return;
        function = read_function(symbol);
        current_function = function;
        if (debugger_enabled && current_symbol->symbol.name)
            ide_message(4, format_function_name(&current_symbol->symbol));
        activate_scope(function->scope);
        apply_function_options(function->options);
        apply_warning_state(function->warning_state);
        backend_stage = 1;
        inline_function = function->symbol && (function->symbol->field0e & 0x1000);
        if (inline_function) prepare_inline_function(function);
        canonicalize_function(function);
        if (inline_function) finish_inline_function(function);
        expand_inline_calls(function);
        tree_transform = optimize_global ? optimize_tree_global : optimize_tree_local;
        transform_function_trees(function);
        if (optimize_global) {
            optimize_function_flow(function);
            work = build_global_work(function->expressions);
        } else work = build_local_work(function->expressions);
        function->work = work;
        if (optimize_global && work && global_exit_work) {
            current_work = work;
            optimize_work();
            work = current_work;
        }
        if (work) {
            current_work = work;
            prepare_work(current_work);
            if (optimize_global && global_exit_work) finish_global_work();
            finalize_work_trees(current_work);
            work = current_work;
        }
        if (!generate_disabled) {
            current_work = work;
            backend_stage = 5;
            generate_work(current_work);
            backend_stage = 6;
            emit_backend_work();
            work = current_work;
        }
        current_work = work;
        while (work) {
            next = work->next;
            free_work(work);
            work = next;
        }
        reset_work();
        release_function_symbols();
    }
}

/* Original [0004c580,0004c5c0). Save the next link before rewriting. */
void __fastcall canonicalize_function(Function *function)
{
    Expression *expression = function->expressions, *next;
    while (expression) {
        next = expression->next;
        if (expression->source_location) current_source_location = expression->source_location;
        expression->tree = canonicalize_tree(function, expression->tree);
        rewrite_expression(expression);
        expression = next;
    }
}

/* Original [0004c5c0,0004c600). The selected local/global optimizer owns
   each root; list cleanup follows all root transformations. */
void __fastcall transform_function_trees(Function *function)
{
    Expression *expression;
    for (expression = function->expressions; expression; expression = expression->next) {
        if (expression->source_location) current_source_location = expression->source_location;
        expression->tree = tree_transform(expression->tree);
    }
    cleanup_expressions(&function->expressions);
}
