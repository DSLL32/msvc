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
            optimize_function_flow(function);
            function_pass_state = 0;
            if (c2_basic_mode && (function->symbol->symbol.flags & 0x1000)) {
                function_pass_4828f8(function);
                function_pass_state = 0;
            }
            optimize_function_returns(function);
            function_pass_state = 0;
            prepare_function_liveness(function);
            function_pass_state = 0;
            simplify_function_expressions(function, 1);
            function_pass_state = 0;
            release_unreferenced_temporaries(function);
            optimize_global_flow(function);
            function_pass_state = 0;
            release_unreferenced_temporaries(function);
            function_pass_state = 0;
            simplify_function_expressions(function, 2);
            function_pass_state = 0;
            rewrite_function_records_backward(function);
            function_pass_state = 0;
            materialize_final_register_operands(function);
            function_pass_state = 0;
            rewrite_final_assignments(function);
            function_pass_state = 0;
            simplify_final_blocks(function);
            function_pass_state = 0;
            simplify_final_transfers(function);
            fold_final_conditional_branches(function);
            function_pass_state = 0;
        }
        if (function->symbol->symbol.flags & 0x1000) {
            function_pass_464b8a(function);
            function_pass_state = 0;
        }
        if (c2_global_optimize) {
            lower_final_constant_arithmetic(function);
            function_pass_state = 0;
        }
        lower_final_operands(function);
        function_pass_state = 0;
        lower_final_function_records(function);
        function_pass_state = 0;
        assign_function_parameters(function);
        function_pass_state = 0;
        if (c2_global_optimize) {
            fold_final_integer_forms(function);
            function_pass_state = 0;
            function_pass_4088c7(function);
            function_pass_state = 0;
            refresh_final_flow_analysis(function);
            function_pass_state = 0;
            if (function->symbol->symbol.flags & 0x1000) {
                if (special_function_passes)
                    schedule_final_function(function, 1);
            } else if (!(function->symbol->additional_flags & 0x200) || c2_exception_mode) {
                schedule_final_function(function, 1);
            } else if (special_function_passes) {
                schedule_final_function(function, 1);
            }
            function_pass_state = 0;
            allocate_final_function_registers(function);
            function_pass_state = 0;
            plan_function_frame_pointer(function);
            assign_machine_registers(function);
            function_pass_state = 0;
        }
        rewrite_spilled_function_registers(function);
        function_pass_state = 0;
        if (c2_global_optimize && c2_favor_speed) {
            materialize_spill_scratch_operands(function);
            function_pass_state = 0;
        }
        layout_function_stack_frame(function);
        function_pass_state = 0;
        finalize_stack_frame_size(function);
        function_pass_state = 0;
        if (c2_basic_mode && c2_global_optimize) {
            function_pass_483b6f(function);
            function_pass_state = 0;
        }
        build_function_prologue(function);
        function_pass_state = 0;
        function_pass_result = 0;
        if (c2_global_optimize) {
            rebuild_function_flow(function);
            group_symbol_records(function);
            function_pass_state = 0;
        }
        if (!(function->flags & 0x800)) {
            optimize_final_record_sequence(function);
            function_pass_state = 0;
        }
        optimize_final_instruction_forms(function);
        function_pass_state = 0;
        code_generation_state = 0;
        if (c2_global_optimize) {
            optimize_final_record_sequence(function);
            function_pass_state = 0;
            optimize_final_branch_sequences(function);
            function_pass_state = 0;
            group_symbol_records(function);
            function_pass_state = 0;
            simplify_symbol_records(function);
            prune_final_transfer_ranges(function);
            function_pass_state = 0;
            if (c2_global_optimize) {
                optimize_final_value_reuse(function);
                function_pass_state = 0;
                if (function->symbol->symbol.flags & 0x1000) {
                    if (special_function_passes)
                        schedule_final_function(function, 0);
                } else if (!(function->symbol->additional_flags & 0x200) || c2_exception_mode) {
                    schedule_final_function(function, 0);
                } else if (special_function_passes) {
                    schedule_final_function(function, 0);
                }
                function_pass_state = 0;
                if (c2_favor_speed) {
                    fold_scheduled_register_moves(function);
                    function_pass_state = 0;
                }
            }
        }
        if (function->flags & 0x10) {
            generate_prologue = generate_frame = 1;
            initialize_final_stack_depths(function);
        } else {
            generate_prologue = 0;
            if (function->flags & 0x300000) {
                generate_frame = 1;
                initialize_final_stack_depths(function);
                function_pass_state = 0;
            } else {
                generate_frame = 0;
                if (c2_global_optimize)
                    function_pass_465486(function);
            }
        }
        lower_final_switch_records(function);
        function_pass_state = 0;
        if (c2_entry_hooks) {
            function_pass_47d1d3(function);
            function_pass_state = 0;
        }
        emit_final_function(function);
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
    expand_branch_edges(function);
}
