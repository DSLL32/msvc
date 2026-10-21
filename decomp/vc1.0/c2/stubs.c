#include "options.h"
#include "input.h"
#include "backend.h"
#include "extended.h"
#include "segments.h"
#include "initializers.h"
#include "functions.h"
#include "records.h"
#include "backend-passes.h"
BOOL WINAPI compiler_control(DWORD event) { __asm int 3 }
void compiler_cleanup(void) { __asm int 3 }
void initialize_debugging(void) { __asm int 3 }
void finish_output(void) { __asm int 3 }
void finish_debugging(void) { __asm int 3 }
void finish_globals(void) { __asm int 3 }
void __fastcall compiler_internal_error(char *source, unsigned code) { __asm int 3 }
void __cdecl compiler_report(unsigned code, ...) { __asm int 3 }
__declspec(naked) void report_context_error(void) { __asm int 3 }
int __fastcall suffix_matches(char *pattern, char *text, unsigned action) { __asm int 3 }
Initializer *__fastcall new_initializer(int kind) { __asm int 3 }
void __fastcall read_initializer_body(Initializer *initializer) { __asm int 3 }
void __fastcall insert_initializer(Initializer *initializer) { __asm int 3 }
void __fastcall adjust_initializer_debug(Initializer *initializer) { __asm int 3 }
void __fastcall release_initializer(Initializer *initializer) { __asm int 3 }
Function *__fastcall read_function(FunctionSymbol *symbol) { __asm int 3 }
char *__fastcall format_function_name(FunctionSymbol *symbol) { __asm int 3 }
void __cdecl ide_message(int code, ...) { __asm int 3 }
void __fastcall activate_scope(ScopeHash *scope) { __asm int 3 }
void __fastcall apply_function_options(unsigned options) { __asm int 3 }
void __fastcall initialize_function_state(unsigned state) { __asm int 3 }
void __fastcall prepare_inline_function(Function *function) { __asm int 3 }
void __fastcall canonicalize_function(Function *function) { __asm int 3 }
void __fastcall finish_inline_function(Function *function) { __asm int 3 }
void __fastcall prepare_function_codegen(Function *function) { __asm int 3 }
Node *__fastcall optimize_tree_global(Node *node) { __asm int 3 }
Node *__fastcall optimize_tree_local(Node *node) { __asm int 3 }
void __fastcall transform_function_trees(Function *function) { __asm int 3 }
void __fastcall optimize_function_flow(Function *function) { __asm int 3 }
WorkItem *__fastcall build_global_work(void *expressions) { __asm int 3 }
WorkItem *__fastcall build_local_work(void *expressions) { __asm int 3 }
void optimize_work(void) { __asm int 3 }
void prepare_work(void) { __asm int 3 }
void finish_global_work(void) { __asm int 3 }
void __fastcall finalize_work_trees(WorkItem *work) { __asm int 3 }
void generate_work(void) { __asm int 3 }
void __fastcall free_work(WorkItem *work) { __asm int 3 }
void reset_work(void) { __asm int 3 }
void release_function_symbols(void) { __asm int 3 }
SymbolHeader *__fastcall type_of_symbol(SymbolHeader *symbol, int unwrap) { __asm int 3 }
SymbolHeader *__fastcall lookup_symbol(ScopeHash *scope, unsigned identifier) { __asm int 3 }
void __fastcall queue_symbol_debug(SymbolHeader *symbol) { __asm int 3 }
void __fastcall reserve_symbol_storage(SymbolHeader *symbol) { __asm int 3 }
void __fastcall read_qualified_symbol(SymbolHeader *symbol, int arena) { __asm int 3 }
SymbolHeader *__fastcall lookup_global_symbol(unsigned identifier) { __asm int 3 }
void __fastcall clone_alias_symbol(SymbolHeader *symbol) { __asm int 3 }
void __fastcall apply_il_option(int option) { __asm int 3 }
void __cdecl compiler_warning(int level, int code, ...) { __asm int 3 }
void __fastcall uppercase_name(char *name) { __asm int 3 }
int __fastcall is_constant_symbol(SymbolHeader *symbol) { __asm int 3 }
unsigned next_symbol_identifier(void) { __asm int 3 }
void __fastcall write_module_listing(char *source) { __asm int 3 }
void __fastcall prepare_machine_context(MachineInstruction *instructions) { __asm int 3 }
void __fastcall sync_machine_work(WorkItem *work, MachineContext *context) { __asm int 3 }
void __fastcall scan_machine_context(MachineContext *context) { __asm int 3 }
void __fastcall compact_machine_context(MachineContext *context) { __asm int 3 }
void optimize_machine_flow(void) { __asm int 3 }
void __fastcall analyze_machine_instructions(MachineInstruction *instructions, unsigned flags) { __asm int 3 }
void simplify_machine_data(void) { __asm int 3 }
void simplify_machine_branches(void) { __asm int 3 }
void combine_machine_instructions(void) { __asm int 3 }
void finish_tree_optimization(void) { __asm int 3 }
void allocate_machine_registers(void) { __asm int 3 }
void __fastcall prepare_machine_blocks(WorkItem *work) { __asm int 3 }
void __fastcall set_machine_frame(MachineContext *context, unsigned flags) { __asm int 3 }
void __fastcall finalize_machine_context(MachineInstruction *instructions) { __asm int 3 }
void __fastcall peephole_machine_33f50(MachineContext *context) { __asm int 3 }
void __fastcall peephole_machine_33550(MachineContext *context) { __asm int 3 }
void __fastcall peephole_machine_34520(MachineContext *context) { __asm int 3 }
void __fastcall peephole_machine_34910(MachineContext *context) { __asm int 3 }
void __fastcall peephole_machine_349e0(MachineContext *context) { __asm int 3 }
void __fastcall schedule_machine_instructions(int first) { __asm int 3 }
void finish_machine_schedule(void) { __asm int 3 }
void __fastcall split_machine_instructions(MachineContext *context) { __asm int 3 }
void __fastcall schedule_i586_branches(MachineContext *context) { __asm int 3 }
void __fastcall finish_machine_lowering(MachineContext *context) { __asm int 3 }
void lower_unoptimized_machine(void) { __asm int 3 }
BackendSymbolList *__fastcall collect_machine_symbols(MachineInstruction *instructions) { __asm int 3 }
void __fastcall calculate_machine_frame(MachineInstruction *instructions, unsigned flags) { __asm int 3 }
void __fastcall emit_pending_symbols(BackendSymbolList *symbols) { __asm int 3 }
void __fastcall write_function_listing(MachineInstruction *instructions) { __asm int 3 }
void __fastcall emit_machine_instructions(MachineInstruction *instructions) { __asm int 3 }
void __fastcall emit_function_exception_table(FunctionSymbol *symbol, unsigned flags) { __asm int 3 }
void __fastcall emit_function_inline_data(Function *function) { __asm int 3 }
void reset_machine_state(void) { __asm int 3 }
SymbolHeader *__fastcall require_external_symbol(char *name) { __asm int 3 }
#include "output.h"
__declspec(naked) void output_noop(void) { __asm int 3 }
__declspec(naked) void output_noop_pair(void) { __asm int 3 }
__declspec(naked) void output_noop_data(void) { __asm int 3 }
__declspec(naked) void output_noop_symbol(void) { __asm int 3 }
__declspec(naked) void output_noop_function(void) { __asm int 3 }
__declspec(naked) void output_noop_value(void) { __asm int 3 }
__declspec(naked) void output_noop_debug(void) { __asm int 3 }
__declspec(naked) void output_noop_relocation(void) { __asm int 3 }
__declspec(naked) void output_noop_record(void) { __asm int 3 }
__declspec(naked) void mark_unwritten_symbol(void) { __asm int 3 }
__declspec(naked) void end_debug_function(void) { __asm int 3 }
__declspec(naked) void begin_debug_function(void) { __asm int 3 }
__declspec(naked) void finish_debug_module(void) { __asm int 3 }
__declspec(naked) void coff_output_symbol(void) { __asm int 3 }
__declspec(naked) void coff_output_function(void) { __asm int 3 }
__declspec(naked) void coff_begin_function(void) { __asm int 3 }
__declspec(naked) void coff_output_data(void) { __asm int 3 }
__declspec(naked) void coff_select_segment(void) { __asm int 3 }
__declspec(naked) void coff_output_debug_pair(void) { __asm int 3 }
__declspec(naked) void coff_output_debug_record(void) { __asm int 3 }
__declspec(naked) void coff_output_value(void) { __asm int 3 }
__declspec(naked) void coff_output_bytes(void) { __asm int 3 }
__declspec(naked) void coff_output_address(void) { __asm int 3 }
__declspec(naked) void coff_output_offset(void) { __asm int 3 }
__declspec(naked) void coff_output_size(void) { __asm int 3 }
__declspec(naked) void coff_finish_module(void) { __asm int 3 }
__declspec(naked) void coff_output_relocation(void) { __asm int 3 }
__declspec(naked) void coff_output_record(void) { __asm int 3 }
__declspec(naked) void coff_mark_symbol(void) { __asm int 3 }
__declspec(naked) void omf_output_symbol(void) { __asm int 3 }
__declspec(naked) void omf_select_segment(void) { __asm int 3 }
__declspec(naked) void omf_output_data(void) { __asm int 3 }
__declspec(naked) void omf_output_debug_pair(void) { __asm int 3 }
__declspec(naked) void omf_output_debug_record(void) { __asm int 3 }
__declspec(naked) void omf_output_value(void) { __asm int 3 }
__declspec(naked) void omf_output_bytes(void) { __asm int 3 }
__declspec(naked) void omf_output_offset(void) { __asm int 3 }
__declspec(naked) void omf_output_size(void) { __asm int 3 }
__declspec(naked) void omf_finish_module(void) { __asm int 3 }
__declspec(naked) void omf_output_relocation(void) { __asm int 3 }
__declspec(naked) void omf_output_record(void) { __asm int 3 }
__declspec(naked) void omf_mark_symbol(void) { __asm int 3 }
void omf_open_module(void) { __asm int 3 }
void initialize_codeview_output(void) { __asm int 3 }
#include "coff-output.h"
unsigned coff_timestamp(void) { __asm int 3 }
void __fastcall coff_error(unsigned code, unsigned detail, void *context) { __asm int 3 }
void write_module_directives(void) { __asm int 3 }
#include "coff-writer.h"
void coff_reset_scopes(void) { __asm int 3 }
CoffScope *__fastcall coff_new_scope(unsigned kind, unsigned value) { __asm int 3 }
