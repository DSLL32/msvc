#ifndef C2_FUNCTIONS_H
#define C2_FUNCTIONS_H
#include "c2.h"
#include <recon.h>

/* The controller proves these register arguments. The pass bodies and their
 * more specific names remain pending; hexadecimal suffixes identify the
 * original application entries without guessing their algorithms. */
extern C2FunctionSymbol *pending_functions;
void initialize_backend(void);
void initialize_pass_functions(void);
void __fastcall initialize_source_output(char *source);
unsigned char get_assembly_comment_character(void);
void __fastcall write_assembly_header(char *source);
extern int assembly_output_active;
extern unsigned char assembly_comment_character;
C2DebugLink *__fastcall prepare_function_records(C2Function *function);
void __fastcall process_records(C2DebugLink *records);
C2DebugLink *prepare_global_records(void);
int __fastcall has_local_symbol(unsigned serial);
int check_extended_backend(void);
unsigned short __fastcall listing_source_index(unsigned serial, unsigned source);
int prepare_deferred_function(void);
void finish_deferred_function(void);
extern C2Function *current_function;
extern unsigned code_generation_state;
extern int function_input_flags, function_analysis_available;
void reset_register_marks(void);
void initialize_function_records(void);
typedef struct C2SmallNode C2SmallNode;
extern C2SmallNode *free_small_nodes[49];
void clear_function_input_state(void);
void reset_function_listing(void);
void begin_function_input(void);
C2Function *__fastcall read_function(C2FunctionSymbol *symbol);
void __fastcall activate_scope(C2ScopeHash *scope);
void __fastcall canonicalize_function(C2Function *function);
int __fastcall inline_function_calls(C2Function *function, int mode, int limit, int flags);
int __fastcall skip_global_optimization(C2Function *function);
void __fastcall simplify_function_expressions(C2Function *function, int mode);
void __fastcall schedule_final_function(C2Function *function, int mode);
void __fastcall function_pass_456857(C2Function *function, int mode);
void __fastcall function_pass_4570e0(C2Function *function, int mode);
void finish_function(void);
void __fastcall function_pass_463ff1(C2Function *function);
void __fastcall function_pass_456b13(C2Function *function);
void __fastcall function_pass_45729f(C2Function *function);
void __fastcall function_pass_458c9f(C2Function *function);
void __fastcall function_pass_46481c(C2Function *function);
void __fastcall function_pass_483f46(C2Function *function);
void __fastcall function_pass_4088c7(C2Function *function);
void __fastcall optimize_function_flow(C2Function *function);
void __fastcall function_pass_4828f8(C2Function *function);
unsigned __fastcall optimize_function_returns(C2Function *function);
unsigned __fastcall optimize_return_successors(C2Function *function, C2BasicBlock *block);
unsigned __fastcall fold_return_block(C2Function *function, C2BasicBlock *block);
void __fastcall prepare_function_liveness(C2Function *function);
unsigned __fastcall release_unreferenced_temporaries(C2Function *function);
void __fastcall optimize_global_flow(C2Function *function);
void __fastcall rewrite_function_records_backward(C2Function *function);
void __fastcall materialize_final_register_operands(C2Function *function);
void __fastcall rewrite_final_assignments(C2Function *function);
void __fastcall simplify_final_blocks(C2Function *function);
void __fastcall simplify_final_transfers(C2Function *function);
void __fastcall fold_final_conditional_branches(C2Function *function);
void __fastcall function_pass_464b8a(C2Function *function);
void __fastcall lower_final_constant_arithmetic(C2Function *function);
void __fastcall lower_final_operands(C2Function *function);
void __fastcall lower_final_function_records(C2Function *function);
void __fastcall assign_function_parameters(C2Function *function);
void __fastcall fold_final_integer_forms(C2Function *function);
void __fastcall refresh_final_flow_analysis(C2Function *function);
void __fastcall allocate_final_function_registers(C2Function *function);
void __fastcall plan_function_frame_pointer(C2Function *function);
void __fastcall assign_machine_registers(C2Function *function);
void __fastcall rewrite_spilled_function_registers(C2Function *function);
void __fastcall materialize_spill_scratch_operands(C2Function *function);
void __fastcall layout_function_stack_frame(C2Function *function);
void __fastcall finalize_stack_frame_size(C2Function *function);
void __fastcall function_pass_483b6f(C2Function *function);
void __fastcall build_function_prologue(C2Function *function);
void __fastcall rebuild_function_flow(C2Function *function);
void __fastcall group_symbol_records(C2Function *function);
void __fastcall optimize_final_record_sequence(C2Function *function);
void __fastcall optimize_final_instruction_forms(C2Function *function);
void __fastcall optimize_final_branch_sequences(C2Function *function);
void __fastcall simplify_symbol_records(C2Function *function);
void __fastcall prune_final_transfer_ranges(C2Function *function);
void __fastcall optimize_final_value_reuse(C2Function *function);
void __fastcall fold_scheduled_register_moves(C2Function *function);
void __fastcall initialize_final_stack_depths(C2Function *function);
void __fastcall lower_final_switch_records(C2Function *function);
void __fastcall function_pass_465486(C2Function *function);
void __fastcall function_pass_47d1d3(C2Function *function);
void __fastcall emit_final_function(C2Function *function);
void __fastcall partition_function_blocks(C2Function *function);
void __fastcall build_function_edges(C2Function *function);
void __fastcall expand_branch_edges(C2Function *function);
#endif
