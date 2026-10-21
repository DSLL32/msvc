/* Unreached graph vertices trap deliberately for the paired reconstruction workflow. */
#include "options.h"
#include "input.h"
#include "backend.h"
#include "extended.h"
#include "segments.h"
#include "initializers.h"
#include "functions.h"
#include "function-input.h"
#include "tree-input.h"
#include "expression-fields.h"
#include "control-input.h"
#include "tree-rewrite.h"
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
void __fastcall report_context_error(unsigned code, char *source, char *filename) { __asm int 3 }
int __fastcall suffix_matches(char *pattern, char *text, unsigned action) { __asm int 3 }
Initializer *__fastcall new_initializer(int kind) { __asm int 3 }
void __fastcall read_initializer_body(Initializer *initializer) { __asm int 3 }
void __fastcall insert_initializer(Initializer *initializer) { __asm int 3 }
void __fastcall release_initializer(Initializer *initializer) { __asm int 3 }
void __fastcall read_module_directive(int kind) { __asm int 3 }
short __fastcall input_counted_string(char *buffer, unsigned limit) { __asm int 3 }
int compare_extended(unsigned operation, const Extended *left, const Extended *right) { __asm int 3 }
Node *__fastcall intern_typed_label(Function *function, Node *node) { __asm int 3 }
Extended __stdcall input_extended(void) { __asm int 3 }
DoubleBits input_double_bits(void) { __asm int 3 }
int extended_format(const Extended *number) { __asm int 3 }
int extended_to_float(const Extended *number, float *result) { __asm int 3 }
int extended_to_double(const Extended *number, DoubleBits *result) { __asm int 3 }
void float_to_extended(const float *number, Extended *result) { __asm int 3 }
void double_to_extended(const DoubleBits *number, Extended *result) { __asm int 3 }
int __fastcall is_register_return_size(unsigned bytes) { __asm int 3 }
TemporarySymbol *__fastcall create_temporary(ScopeHash *scope, unsigned type, long size,
    short storage, unsigned arena, signed char subclass, unsigned short attributes) { __asm int 3 }
void __fastcall decode_inline_assembly(Node *node, ScopeHash *scope) { __asm int 3 }
char *__fastcall format_function_name(FunctionSymbol *symbol) { __asm int 3 }
void __cdecl ide_message(int code, ...) { __asm int 3 }
void __fastcall set_warning_override(short number, signed char level) { __asm int 3 }
void __fastcall prepare_inline_function(Function *function) { __asm int 3 }
Node *__fastcall canonicalize_node(Function *function, Node *node, unsigned flags) { __asm int 3 }
Node *__fastcall rewrite_short_circuit(Node *node) { __asm int 3 }
void __cdecl splice_expressions(const char *order, ...) { __asm int 3 }
Node *__fastcall skip_node_conversions(Node *node) { __asm int 3 }
int __fastcall can_keep_conditional(Node *node) { __asm int 3 }
Node *__fastcall lower_conditional(Node *node, Node *container) { __asm int 3 }
Node *__fastcall lower_comma(Node *node, Node *container) { __asm int 3 }
Node *__fastcall materialize_expression(Node *node) { __asm int 3 }
Node *__fastcall extract_assembly_labels(Node *node) { __asm int 3 }
Node *__fastcall convert_node_type(Node *node, unsigned short type) { __asm int 3 }
Node *__fastcall new_temporary_reference(unsigned bytes, unsigned flags) { __asm int 3 }
Node *__cdecl build_tree(const char *format, ...) { __asm int 3 }
unsigned __fastcall map_assignment_opcode(unsigned opcode, int reverse) { __asm int 3 }
Node *__fastcall copy_tree(Node *node) { __asm int 3 }
Node *__fastcall assignment_value(Node *node) { __asm int 3 }
Node *__fastcall clone_node(Node *node) { __asm int 3 }
void __fastcall lower_bitfield_reference(Node *node) { __asm int 3 }
void __fastcall release_tree(Node *node) { __asm int 3 }
Node *__fastcall lower_target_address(Node *node) { __asm int 3 }
void __fastcall lower_bitfield_assignment(Node *node) { __asm int 3 }
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
void __fastcall queue_symbol_debug(SymbolHeader *symbol) { __asm int 3 }
void __fastcall reserve_symbol_storage(SymbolHeader *symbol) { __asm int 3 }
void __fastcall read_qualified_symbol(SymbolHeader *symbol, int arena) { __asm int 3 }
SymbolHeader *__fastcall lookup_global_symbol(unsigned identifier) { __asm int 3 }
void __fastcall clone_alias_symbol(SymbolHeader *symbol) { __asm int 3 }
void __cdecl compiler_warning(int level, int code, ...) { __asm int 3 }
void __fastcall uppercase_name(char *name) { __asm int 3 }
int __fastcall is_constant_symbol(SymbolHeader *symbol) { __asm int 3 }
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
void __fastcall calculate_machine_frame(MachineInstruction *instructions, unsigned flags) { __asm int 3 }
void __fastcall write_function_listing(MachineInstruction *instructions) { __asm int 3 }
void __fastcall emit_machine_instructions(MachineInstruction *instructions) { __asm int 3 }
void __fastcall emit_function_exception_table(FunctionSymbol *symbol, unsigned flags) { __asm int 3 }
void __fastcall emit_function_inline_data(Function *function) { __asm int 3 }
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
__declspec(naked) void coff_output_debug_pair(void) { __asm int 3 }
__declspec(naked) void coff_output_debug_record(void) { __asm int 3 }
__declspec(naked) void coff_output_value(void) { __asm int 3 }
__declspec(naked) void coff_output_bytes(void) { __asm int 3 }
__declspec(naked) void coff_output_address(void) { __asm int 3 }
__declspec(naked) void coff_output_offset(void) { __asm int 3 }
__declspec(naked) void coff_output_size(void) { __asm int 3 }
__declspec(naked) void coff_finish_module(void) { __asm int 3 }
__declspec(naked) void coff_output_relocation(void) { __asm int 3 }
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
void __fastcall coff_error(unsigned code, char *source, unsigned line) { __asm int 3 }
#include "coff-writer.h"
void __fastcall coff_write_comment(char *text) { __asm int 3 }
#include "coff-segments.h"
void __fastcall coff_fill_bytes(unsigned char value, unsigned bytes) { __asm int 3 }
unsigned char __fastcall coff_comdat_selection(unsigned char selection) { __asm int 3 }
unsigned short __fastcall coff_create_comdat_section(char *name, unsigned flags,
    unsigned char selection, unsigned short associated, int definition) { __asm int 3 }
#include "listing.h"
void __fastcall write_segment_listing(Segment *segment, int definition) { __asm int 3 }
int __fastcall close_segment_listing(Segment *segment) { __asm int 3 }
char *__fastcall listing_segment_name(Segment *segment) { __asm int 3 }
#include "machine.h"
SymbolHeader *__fastcall lookup_alias_target(SymbolHeader *symbol) { __asm int 3 }
int __fastcall initializer_symbols_available(SymbolHeader *symbol) { __asm int 3 }
void __fastcall queue_initializer_dependencies(SymbolHeader *symbol) { __asm int 3 }
BackendSymbolList *new_symbol_list_node(void) { __asm int 3 }
#include "symbol-output.h"
void __fastcall for_each_symbol(int local, SymbolVisitor visit) { __asm int 3 }
void __fastcall write_public_listing(SymbolHeader *symbol) { __asm int 3 }
void __fastcall write_external_listing(SymbolHeader *symbol) { __asm int 3 }
void __fastcall write_common_listing(SymbolHeader *symbol) { __asm int 3 }
void __fastcall mark_listing_symbols(SymbolHeader *symbol) { __asm int 3 }
Initializer *__fastcall find_initializer(unsigned identifier) { __asm int 3 }
void __fastcall emit_initializer(Initializer *initializer) { __asm int 3 }
void __fastcall write_initializer_assembly(Initializer *initializer) { __asm int 3 }
#include "frame.h"

long ftell(FILE *stream) { __asm int 3 }
