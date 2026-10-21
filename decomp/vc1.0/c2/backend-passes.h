#ifndef BACKEND_PASSES_H
#define BACKEND_PASSES_H
#include "backend-flow.h"
void prepare_backend_symbols(void);
void initialize_output_callbacks(void);
void __fastcall write_module_listing(char *source);
void __fastcall prepare_machine_context(MachineInstruction *instructions);
void __fastcall sync_machine_work(WorkItem *work, MachineContext *context);
void __fastcall scan_machine_context(MachineContext *context);
void __fastcall compact_machine_context(MachineContext *context);
void optimize_machine_flow(void);
void __fastcall analyze_machine_instructions(MachineInstruction *instructions, unsigned flags);
void simplify_machine_data(void);
void simplify_machine_branches(void);
void combine_machine_instructions(void);
void finish_tree_optimization(void);
void allocate_machine_registers(void);
void __fastcall prepare_machine_blocks(WorkItem *work);
void __fastcall set_machine_frame(MachineContext *context, unsigned flags);
void __fastcall finalize_machine_context(MachineInstruction *instructions);
void __fastcall peephole_machine_33f50(MachineContext *context);
void __fastcall peephole_machine_33550(MachineContext *context);
void __fastcall peephole_machine_34520(MachineContext *context);
void __fastcall peephole_machine_34910(MachineContext *context);
void __fastcall peephole_machine_349e0(MachineContext *context);
void __fastcall schedule_machine_instructions(int first);
void finish_machine_schedule(void);
void __fastcall split_machine_instructions(MachineContext *context);
void __fastcall schedule_i586_branches(MachineContext *context);
void __fastcall finish_machine_lowering(MachineContext *context);
void lower_unoptimized_machine(void);
BackendSymbolList *__fastcall collect_machine_symbols(MachineInstruction *instructions);
void __fastcall calculate_machine_frame(MachineInstruction *instructions, unsigned flags);
void __fastcall emit_pending_symbols(BackendSymbolList *symbols);
void __fastcall write_function_listing(MachineInstruction *instructions);
void __fastcall emit_machine_instructions(MachineInstruction *instructions);
void __fastcall emit_function_exception_table(FunctionSymbol *symbol, unsigned flags);
void __fastcall emit_function_inline_data(Function *function);
void reset_machine_state(void);
#endif
