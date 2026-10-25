#include "functions.h"
#include "function-reader.h"
#include "function-records.h"
#include <recon.h>

extern char *c2_intermediate_base;
extern int c2_volatile_mode, c2_basic_mode, c2_exception_mode;
extern int c2_global_optimize, c2_favor_speed, c2_entry_hooks;
FILE *expression_input, *function_symbol_input;
C2FunctionSymbol *listed_functions;
C2Function *current_function, *compiled_function;
int deferred_compilation, extra_function_passes, special_function_passes;
unsigned volatile_pass_limit;
unsigned short function_pass_state;
unsigned function_pass_result = 1;
unsigned code_generation_state;
int generate_frame, generate_prologue;

void compile_functions(void)
{
    C2FunctionSymbol **link, *symbol;
    C2Function *function;
    C2FunctionListing *listing;
    int deferred;

    if (!c2_il_extended_header) {
        expression_input = c2_open_intermediate(c2_intermediate_base, "ex", "r", "exp il");
        function_symbol_input = c2_open_intermediate(c2_intermediate_base, "sy", "r", "sym il");
    } else {
        if (!check_extended_backend())
            c2_report_internal("E:\\utc\\src\\\\P2\\main.c", 0x1a4);
        if (c2_internal_listing) {
            for (symbol = pending_functions; symbol; symbol = symbol->pending_next) {
                listing = c2_pool_allocate(0, sizeof(C2FunctionListing));
                symbol->listing = listing;
                listing->source_index = listing_source_index(symbol->symbol.serial, symbol->source_file);
            }
        }
    }
    initialize_backend();
    if (c2_volatile_mode <= 6)
        volatile_pass_limit = 16U << (c2_volatile_mode & 31);
    else
        volatile_pass_limit = 1000;
    for (;;) {
        deferred = 0;
        link = &pending_functions;
        symbol = *link;
        current_symbol = (C2Symbol *)symbol;
        while (symbol) {
            if ((symbol->additional_flags & 0x20) && !(symbol->additional_flags & 2)) {
                current_symbol = (C2Symbol *)symbol;
                symbol->additional_flags |= 2;
                *link = symbol->pending_next;
                if (c2_internal_listing) {
                    symbol->pending_next = listed_functions;
                    listed_functions = symbol;
                    if (deferred_compilation) {
                        listing = symbol->listing;
                        if ((symbol->additional_flags & 0x800) || !listing->available) {
                            listing->begin = listing->end = 0;
                        } else {
                            if (!prepare_deferred_function()) {
                                deferred = 1;
                            } else {
                                ((C2FunctionSymbol *)current_symbol)->listing->begin = 0;
                                ((C2FunctionSymbol *)current_symbol)->listing->end = 0;
                            }
                            symbol = (C2FunctionSymbol *)current_symbol;
                        }
                    }
                }
                break;
            }
            link = &symbol->pending_next;
            symbol = *link;
        }
        if (!symbol) {
            current_symbol = NULL;
            return;
        }
        if (deferred) {
            finish_deferred_function();
            continue;
        }
        begin_function_input();
        function = read_function((C2FunctionSymbol *)current_symbol);
        function_pass_state = 0;
        current_function = function;
        activate_scope(function->symbols);
        if (function->flags & 0x800)
            c2_global_optimize = 0;
        if (function->symbol->symbol.flags & 0x1000) {
            function_pass_463ff1(function);
            function_pass_state = 0;
        }
        if (function->symbol->additional_flags & 0x600) {
            function_pass_456857(function, 0);
            function_pass_456b13(function);
            function_pass_4570e0(function, 0);
            function_pass_state = 0;
        }
        if (c2_global_optimize && (function->flags & 1)) {
            if (skip_global_optimization(function)) {
                c2_global_optimize = 0;
                function_pass_state = 0;
            }
        }
        canonicalize_function(function);
        function_pass_state = 0;
        compiled_function = function;
        if ((function->flags & 0x8000) || (function->symbol->additional_flags & 0x200)) {
            if (c2_exception_mode)
                function_pass_45729f(function);
            if ((function->flags & 0x8000) || (function->symbol->additional_flags & 0x200))
                function_pass_458c9f(function);
            function_pass_state = 0;
        } else if ((function->symbol->symbol.flags & 0x1000) && c2_global_optimize) {
            if (!c2_basic_mode)
                function_pass_46481c(function);
            function_pass_state = 0;
        }
        if (extra_function_passes) {
            function_pass_483f46(function);
            function_pass_state = 0;
        }
        if (c2_global_optimize) {
            function_pass_4088c7(function);
            function_pass_state = 0;
            function_pass_427d89(function);
            function_pass_state = 0;
            if (c2_basic_mode && (function->symbol->symbol.flags & 0x1000)) {
                function_pass_4828f8(function);
                function_pass_state = 0;
            }
            function_pass_421bdf(function);
            function_pass_state = 0;
            function_pass_421e34(function);
            function_pass_state = 0;
            function_pass_409dac(function, 1);
            function_pass_state = 0;
            function_pass_409c5c(function);
            function_pass_42127d(function);
            function_pass_state = 0;
            function_pass_409c5c(function);
            function_pass_state = 0;
            function_pass_409dac(function, 2);
            function_pass_state = 0;
            function_pass_4180e8(function);
            function_pass_state = 0;
            function_pass_413245(function);
            function_pass_state = 0;
            function_pass_4132fa(function);
            function_pass_state = 0;
            function_pass_413369(function);
            function_pass_state = 0;
            function_pass_412292(function);
            function_pass_412412(function);
            function_pass_state = 0;
        }
        if (function->symbol->symbol.flags & 0x1000) {
            function_pass_464b8a(function);
            function_pass_state = 0;
        }
        if (c2_global_optimize) {
            function_pass_412457(function);
            function_pass_state = 0;
        }
        function_pass_4120d4(function);
        function_pass_state = 0;
        function_pass_412d5c(function);
        function_pass_state = 0;
        function_pass_4144ef(function);
        function_pass_state = 0;
        if (c2_global_optimize) {
            function_pass_414862(function);
            function_pass_state = 0;
            function_pass_4088c7(function);
            function_pass_state = 0;
            function_pass_40d254(function);
            function_pass_state = 0;
            if (function->symbol->symbol.flags & 0x1000) {
                if (special_function_passes)
                    function_pass_40d9be(function, 1);
            } else if (!(function->symbol->additional_flags & 0x200) || c2_exception_mode) {
                function_pass_40d9be(function, 1);
            } else if (special_function_passes) {
                function_pass_40d9be(function, 1);
            }
            function_pass_state = 0;
            function_pass_414a64(function);
            function_pass_state = 0;
            function_pass_41759e(function);
            function_pass_416e6a(function);
            function_pass_state = 0;
        }
        function_pass_42a8aa(function);
        function_pass_state = 0;
        if (c2_global_optimize && c2_favor_speed) {
            function_pass_42b3e2(function);
            function_pass_state = 0;
        }
        function_pass_42b750(function);
        function_pass_state = 0;
        function_pass_42e689(function);
        function_pass_state = 0;
        if (c2_basic_mode && c2_global_optimize) {
            function_pass_483b6f(function);
            function_pass_state = 0;
        }
        function_pass_42e6a6(function);
        function_pass_state = 0;
        function_pass_result = 0;
        if (c2_global_optimize) {
            rebuild_function_flow(function);
            group_symbol_records(function);
            function_pass_state = 0;
        }
        if (!(function->flags & 0x800)) {
            function_pass_42d635(function);
            function_pass_state = 0;
        }
        function_pass_42d65a(function);
        function_pass_state = 0;
        code_generation_state = 0;
        if (c2_global_optimize) {
            function_pass_42d635(function);
            function_pass_state = 0;
            function_pass_42f060(function);
            function_pass_state = 0;
            group_symbol_records(function);
            function_pass_state = 0;
            simplify_symbol_records(function);
            function_pass_42f3e4(function);
            function_pass_state = 0;
            if (c2_global_optimize) {
                function_pass_42f446(function);
                function_pass_state = 0;
                if (function->symbol->symbol.flags & 0x1000) {
                    if (special_function_passes)
                        function_pass_40d9be(function, 0);
                } else if (!(function->symbol->additional_flags & 0x200) || c2_exception_mode) {
                    function_pass_40d9be(function, 0);
                } else if (special_function_passes) {
                    function_pass_40d9be(function, 0);
                }
                function_pass_state = 0;
                if (c2_favor_speed) {
                    function_pass_4330f7(function);
                    function_pass_state = 0;
                }
            }
        }
        if (function->flags & 0x10) {
            generate_prologue = generate_frame = 1;
            function_pass_433546(function);
        } else {
            generate_prologue = 0;
            if (function->flags & 0x300000) {
                generate_frame = 1;
                function_pass_433546(function);
                function_pass_state = 0;
            } else {
                generate_frame = 0;
                if (c2_global_optimize)
                    function_pass_465486(function);
            }
        }
        function_pass_4338ab(function);
        function_pass_state = 0;
        if (c2_entry_hooks) {
            function_pass_47d1d3(function);
            function_pass_state = 0;
        }
        function_pass_433906(function);
        finish_function();
    }
}

/* The source-name identity is shared with the existing -f option destination. */
extern char *c2_source_name;
void initialize_backend(void)
{
    initialize_pass_functions();
    initialize_source_output(c2_source_name);
    process_records(prepare_function_records(NULL));
}

/* Count of temporary identities allocated during the canonicalization pass. */
int input_temporary_count;
extern int retain_function_input_pool;
extern unsigned input_source_line_base;

void __fastcall canonicalize_function(C2Function *function)
{
    int limit, changed;
    C2RegisterBlock *block;
    C2FunctionRegister *reg;
    reset_input_pool(4);
    input_temporary_count = function->symbol->temporary_count;
    limit = input_temporary_count * 2;
    if (limit < 1000) limit = 1000;
    else if (limit > 35000) limit = 35000;
    retain_function_input_pool = 1;
    changed = inline_function_calls(function, 1, limit, 0);
    retain_function_input_pool = 0;
    reset_input_pool(4);
    input_source_line_base = function->source_line_base;
    if (changed > 0 || c2_global_optimize)
        for (block = first_function_register_block; block; block = block->next)
            for (reg = block->records; reg < block->cursor; ++reg)
                if (reg->kind != 0x10) reg->view_flags &= 0x7d;
}

void __fastcall function_pass_4088c7(C2Function *function)
{
    rebuild_function_flow(function);
    group_symbol_records(function);
    simplify_symbol_records(function);
    partition_function_blocks(function);
    build_function_edges(function);
    function_pass_408b54(function);
}
