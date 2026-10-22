/* Machine passes and the output phase. */
#include "backend-flow.h"
#include "options.h"
#include "backend-passes.h"
#include "output.h"
extern int optimize_global, optimization_schedule, optimization_schedule_model;
extern int optimization_split, optimization_jump_schedule, cpu_model;
int backend_initialized;
MachineInstruction *machine_instructions; /* 0007b940 */
MachineContext *machine_context; /* 0007b944 */
char *listing_source_filename;
char *function_source_filename; /* 0007e8a4 */
unsigned function_source_line; /* 0007e8a8 */
/* Original [00067fc0,00068300). */
void emit_backend_work(void)
{
    BackendSymbolList *symbols;
    int saved_cpu, frame_metadata;
    prepare_backend_symbols();
    if (!backend_initialized) {
        initialize_output_callbacks();
        listing_source_filename = source_filename;
        write_module_listing(source_filename);
        backend_initialized = 1;
    }
    if (current_symbol) {
        if (optimize_global) {
            prepare_machine_context(machine_instructions);
            sync_machine_work(current_work, machine_context);
            scan_machine_context(machine_context);
            compact_machine_context(machine_context);
            optimize_machine_flow();
            sync_machine_work(current_work, machine_context);
            scan_machine_context(machine_context);
            compact_machine_context(machine_context);
            analyze_machine_instructions(machine_instructions, 0);
            simplify_machine_data();
            simplify_machine_branches();
            combine_machine_instructions();
            sync_machine_work(current_work, machine_context);
            scan_machine_context(machine_context);
            compact_machine_context(machine_context);
            finish_tree_optimization();
            backend_stage = 7;
            allocate_machine_registers();
            prepare_machine_blocks(current_work);
            set_machine_frame(machine_context, (current_function->field28 >> 4) & 1);
            finalize_machine_context(machine_instructions);
            scan_machine_context(machine_context);
            peephole_machine_33f50(machine_context);
            peephole_machine_33550(machine_context);
            peephole_machine_34520(machine_context);
            peephole_machine_34910(machine_context);
            compact_machine_context(machine_context);
            peephole_machine_349e0(machine_context);
            if (optimization_schedule) {
                analyze_machine_instructions(machine_instructions, 0);
                if (optimization_schedule_model) {
                    saved_cpu = cpu_model;
                    cpu_model = optimization_schedule_model;
                }
                schedule_machine_instructions(1);
                if (optimization_schedule_model) cpu_model = saved_cpu;
                finish_machine_schedule();
                if (optimization_split) split_machine_instructions(machine_context);
                if (optimization_jump_schedule && cpu_model == 5)
                    schedule_i586_branches(machine_context);
                finish_machine_schedule();
            }
            scan_machine_context(machine_context);
            compact_machine_context(machine_context);
            finish_machine_lowering(machine_context);
        } else {
            lower_unoptimized_machine();
            finalize_machine_context(machine_instructions);
        }
    }
    symbols = collect_machine_symbols(machine_instructions);
    if (current_symbol) {
        function_source_filename = 0;
        function_source_line = 0;
        layout_machine_stream(machine_instructions, 1);
        frame_metadata = (current_function->field28 & 0x10) || (current_symbol->symbol.field0e & 0x1000);
        output_callbacks.begin_function(current_symbol, function_source_filename, frame_metadata, function_source_line);
        emit_pending_symbols(symbols);
        write_function_listing(machine_instructions);
        emit_machine_instructions(machine_instructions);
        output_callbacks.end_function(current_symbol);
        if (current_function->field28 & 0x10)
            emit_function_exception_table(current_symbol, (current_function->field28 >> 5) & 1);
        if (current_symbol->symbol.field0e & 0x1000)
            emit_function_inline_data(current_function);
    } else emit_pending_symbols(symbols);
    reset_machine_state();
}
