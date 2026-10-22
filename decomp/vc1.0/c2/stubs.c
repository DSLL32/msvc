#include "comparison-branches.h"
#include "frame-access.h"
#include "post-allocation.h"
#include "frame-symbols.h"
#include "machine-analysis.h"
#include "physical-allocation.h"
#include "physical-rewrite.h"
#include "machine-transfers.h"
#include "register-transfers.h"
#include "register-variables.h"
#include "global-registers.h"
#include "work-expressions.h"
#include "region-motion.h"
#include "flow-edges.h"
#include "work-optimizer.h"
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
void initialize_debugging(void) { __asm int 3 }
void __fastcall compiler_internal_error(char *source, unsigned code) { __asm int 3 }
void __cdecl compiler_report(unsigned code, ...) { __asm int 3 }
void __fastcall report_context_error(unsigned code, char *source, char *filename) { __asm int 3 }
int __fastcall suffix_matches(char *pattern, char *text, unsigned action) { __asm int 3 }
void __fastcall release_initializer(Initializer *initializer) { __asm int 3 }
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
char *__fastcall format_function_name(SymbolHeader *symbol) { __asm int 3 }
void __cdecl ide_message(int code, ...) { __asm int 3 }
void __fastcall set_warning_override(short number, signed char level) { __asm int 3 }
void __fastcall prepare_inline_function(Function *function) { __asm int 3 }
int __fastcall is_constant_value(Node *node, int value) { __asm int 3 }
Node *__fastcall new_storage_reference(unsigned bytes) { __asm int 3 }
int __fastcall can_repeat_tree(Node *node) { __asm int 3 }
Node *__fastcall make_truth_test(Node *node) { __asm int 3 }
Node *__fastcall negate_condition(Node *node) { __asm int 3 }
Node *__fastcall materialize_boolean(Node *node, Node *parent) { __asm int 3 }
Node *__fastcall simplify_short_circuit(Node *node, Node *parent) { __asm int 3 }
Node *__fastcall lift_left_sequence(Node *node) { __asm int 3 }
Node *__fastcall lift_right_sequence(Node *node) { __asm int 3 }
Node *__fastcall canonicalize_conditional(Node *node) { __asm int 3 }
Node *__fastcall integer_node(unsigned value, unsigned type) { __asm int 3 }
unsigned __fastcall aligned_value_size(Node *node) { __asm int 3 }
Node *__fastcall lower_intrinsic(Node *node, Node *parent) { __asm int 3 }
int __fastcall assign_parameter_register(unsigned *used, Node *argument, int convention) { __asm int 3 }
Node *__fastcall discard_sequence_tail(Node *node) { __asm int 3 }
Node *__fastcall lift_typed_sequence(Node *node, unsigned opcode) { __asm int 3 }
int __fastcall is_function_reference(Node *node) { __asm int 3 }
Node *__fastcall rewrite_short_circuit(Node *node) { __asm int 3 }
int __fastcall can_keep_conditional(Node *node) { __asm int 3 }
Node *__fastcall lower_conditional(Node *node, Node *container) { __asm int 3 }
Node *__fastcall lower_comma(Node *node, Node *container) { __asm int 3 }
Node *__fastcall materialize_expression(Node *node) { __asm int 3 }
Node *__fastcall extract_assembly_labels(Node *node) { __asm int 3 }
unsigned __fastcall map_assignment_opcode(unsigned opcode, int reverse) { __asm int 3 }
Node *__fastcall assignment_value(Node *node) { __asm int 3 }
void __fastcall lower_bitfield_reference(Node *node) { __asm int 3 }
Node *__fastcall lower_target_address(Node *node) { __asm int 3 }
void __fastcall lower_bitfield_assignment(Node *node) { __asm int 3 }
void __fastcall finish_inline_function(Function *function) { __asm int 3 }
#include "inline-calls.h"
InlineSite *__fastcall new_inline_site(Node *call, Node *parent, Expression *expression, unsigned flags) { __asm int 3 }
InlineBinding *__fastcall bind_inline_arguments(Function *function, Node *entry, Node *call) { __asm int 3 }
void __fastcall check_inline_argument_aliases(InlineBinding *bindings) { __asm int 3 }
int __fastcall analyze_inline_returns(Expression *entry, InlineBinding *bindings) { __asm int 3 }
void __fastcall merge_inline_scope(ScopeHash *outer, ScopeHash *inner) { __asm int 3 }
Node *__fastcall new_inline_storage(signed char kind, unsigned bytes) { __asm int 3 }
InlineBinding *__fastcall find_inline_binding(InlineBinding *bindings, SymbolHeader *symbol) { __asm int 3 }
Node *__fastcall inline_argument_prefix(InlineBinding *bindings) { __asm int 3 }
void __fastcall materialize_inline_arguments(Function *function, InlineSite *site, InlineBinding *bindings) { __asm int 3 }
Expression *__fastcall substitute_inline_body(Function *function, InlineBinding *bindings, Node *result,
    unsigned source_location, InlineSite *site, int hidden_result) { __asm int 3 }
void __fastcall recycle_inline_site(InlineSite *site) { __asm int 3 }
void __fastcall recycle_inline_binding(InlineBinding *binding) { __asm int 3 }
int __fastcall tree_contains_opcode(Node *node, unsigned opcode) { __asm int 3 }
unsigned short __fastcall value_node_type(short kind, unsigned bytes) { __asm int 3 }
void __fastcall refresh_tree_references(Node *node) { __asm int 3 }
void __fastcall release_node_children(Node *node) { __asm int 3 }
void __fastcall reassociate_right_chain(Node *node) { __asm int 3 }
int __fastcall rotate_left_chain(Node *node, unsigned right_key, unsigned prior_key, int under_load) { __asm int 3 }
unsigned __fastcall sort_commutative_operands(Node *node, unsigned left_key, unsigned right_key) { __asm int 3 }
Node *__fastcall fold_integer_comparison(Node *node) { __asm int 3 }
Node *__fastcall fold_floating_comparison(Node *node) { __asm int 3 }
Node *__fastcall fold_floating_node(Node *node) { __asm int 3 }
Node *__fastcall fold_integer_node(Node *node) { __asm int 3 }
#include "algebra.h"
int __fastcall integer_type_limit(Node *node, int limit) { __asm int 3 }
Node *__fastcall strip_integer_cast(Node *node) { __asm int 3 }
Node *__fastcall typed_integer_node(Node *model, unsigned value) { __asm int 3 }
int __fastcall rewrite_shift_as_multiply(Node *node) { __asm int 3 }
Node *__fastcall fold_absorbing_constant(Node *node, int value) { __asm int 3 }
int __fastcall reduce_constant_bits(Node *node, Node *parent) { __asm int 3 }
int __fastcall is_nonnegative_tree(Node *node) { __asm int 3 }
int __fastcall can_combine_constants(Node *node, unsigned first, unsigned second) { __asm int 3 }
int __fastcall integer_log2(unsigned value) { __asm int 3 }
int __fastcall extract_negative(Node **node) { __asm int 3 }
int __fastcall negate_tree_in_place(Node **node) { __asm int 3 }
int __fastcall constant_as_integer(Node *node, int *value) { __asm int 3 }
Node *__fastcall lift_left_unary(Node *node) { __asm int 3 }
Node *__fastcall remove_left_wrapper(Node *node) { __asm int 3 }
Node *__fastcall take_left_operand(Node *node) { __asm int 3 }
Node *__fastcall lift_unary_operand(Node *node, unsigned opcode) { __asm int 3 }
Node *__fastcall remove_right_wrapper(Node *node) { __asm int 3 }
Node *__fastcall simplify_masked_tree(Node *node, unsigned mask) { __asm int 3 }
unsigned __fastcall shift_discard_mask(Node *node) { __asm int 3 }
int __fastcall factor_common_operator(Node *node, unsigned opcode, int implicit) { __asm int 3 }
int __fastcall factor_compatible_operator(Node *node, unsigned opcode, int implicit) { __asm int 3 }
int __fastcall distribute_constant(Node *node, unsigned opcode) { __asm int 3 }
int __fastcall conversion_loses_value(unsigned short source, unsigned short target) { __asm int 3 }
int __fastcall lower_far_pointer_cast(Node *node) { __asm int 3 }
unsigned __fastcall map_comparison_opcode(unsigned opcode, int negate) { __asm int 3 }
#include "work-prepare.h"
void __fastcall prepare_generated_intrinsic(Node *node, int mode) { __asm int 3 }
void __fastcall lower_float_expression(Node *node) { __asm int 3 }
void finish_target_expression(void) { __asm int 3 }
#include "instruction-selection.h"
void __fastcall peephole_selected_block(MachineBlockHeader *block, int global) { __asm int 3 }
void __fastcall read_qualified_symbol(SymbolHeader *symbol, int arena) { __asm int 3 }
void __fastcall clone_alias_symbol(SymbolHeader *symbol) { __asm int 3 }
void __cdecl compiler_warning(int level, int code, ...) { __asm int 3 }
void __fastcall uppercase_name(char *name) { __asm int 3 }
void __fastcall peephole_machine_33550(MachineContext *context) { __asm int 3 }
void __fastcall peephole_machine_34520(MachineContext *context) { __asm int 3 }
void __fastcall peephole_machine_34910(MachineContext *context) { __asm int 3 }
void __fastcall peephole_machine_349e0(MachineContext *context) { __asm int 3 }
void __fastcall schedule_machine_instructions(int first) { __asm int 3 }
void finish_machine_schedule(void) { __asm int 3 }
void __fastcall split_machine_instructions(MachineContext *context) { __asm int 3 }
void __fastcall schedule_i586_branches(MachineContext *context) { __asm int 3 }
void __fastcall finish_machine_lowering(MachineContext *context) { __asm int 3 }
void __fastcall emit_function_exception_table(FunctionSymbol *symbol, unsigned flags) { __asm int 3 }
void __fastcall emit_function_inline_data(Function *function) { __asm int 3 }
SymbolHeader *__fastcall require_external_symbol(char *name) { __asm int 3 }
#include "output.h"
__declspec(naked) void output_noop_data(void) { __asm int 3 }
__declspec(naked) void output_noop_function(void) { __asm int 3 }
__declspec(naked) void output_noop_value(void) { __asm int 3 }
__declspec(naked) void output_noop_relocation(void) { __asm int 3 }
__declspec(naked) void output_noop_record(void) { __asm int 3 }
__declspec(naked) void mark_unwritten_symbol(void) { __asm int 3 }
__declspec(naked) void end_debug_function(void) { __asm int 3 }
__declspec(naked) void begin_debug_function(void) { __asm int 3 }
__declspec(naked) void finish_debug_module(void) { __asm int 3 }
__declspec(naked) void coff_output_debug_pair(void) { __asm int 3 }
__declspec(naked) void coff_output_debug_record(void) { __asm int 3 }
__declspec(naked) void coff_output_value(void) { __asm int 3 }
__declspec(naked) void coff_output_bytes(void) { __asm int 3 }
__declspec(naked) void coff_output_address(void) { __asm int 3 }
__declspec(naked) void coff_output_offset(void) { __asm int 3 }
__declspec(naked) void coff_output_size(void) { __asm int 3 }
__declspec(naked) void coff_output_relocation(void) { __asm int 3 }
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
void __fastcall coff_error(unsigned code, char *source, unsigned line) { __asm int 3 }
#include "coff-writer.h"
void __fastcall coff_write_comment(char *text) { __asm int 3 }
#include "coff-segments.h"
void __fastcall coff_fill_bytes(unsigned char value, unsigned bytes) { __asm int 3 }
unsigned char __fastcall coff_comdat_selection(unsigned char selection) { __asm int 3 }
unsigned short __fastcall coff_create_comdat_section(char *name, unsigned flags,
    unsigned char selection, unsigned short associated, int definition) { __asm int 3 }
#include "listing.h"
Segment *__fastcall write_segment_listing(Segment *segment, int definition) { __asm int 3 }
Segment *__fastcall close_segment_listing(Segment *segment) { __asm int 3 }
char *__fastcall listing_segment_name(Segment *segment) { __asm int 3 }
#include "machine.h"
SymbolHeader *__fastcall lookup_alias_target(SymbolHeader *symbol) { __asm int 3 }
int __fastcall initializer_symbols_available(SymbolHeader *symbol) { __asm int 3 }
void __fastcall queue_initializer_dependencies(SymbolHeader *symbol) { __asm int 3 }
BackendSymbolList *new_symbol_list_node(void) { __asm int 3 }
#include "symbol-output.h"
void __fastcall write_public_listing(SymbolHeader *symbol) { __asm int 3 }
void __fastcall write_external_listing(SymbolHeader *symbol) { __asm int 3 }
void __fastcall write_common_listing(SymbolHeader *symbol) { __asm int 3 }
void __fastcall mark_listing_symbols(SymbolHeader *symbol) { __asm int 3 }
Initializer *__fastcall find_initializer(unsigned identifier) { __asm int 3 }
void __fastcall emit_initializer(Initializer *initializer) { __asm int 3 }
void __fastcall write_initializer_assembly(Initializer *initializer) { __asm int 3 }
#include "frame.h"


MachineOperand *__fastcall duplicate_operand(MachineOperand *operand) { __asm int 3 }
MachineOperand *__fastcall high_dword_operand(MachineOperand *operand) { __asm int 3 }
int __fastcall try_select_assignment(Node *node) { __asm int 3 }
void emit_float_return(void) { __asm int 3 }
void __fastcall select_assembly_record(Node *node) { __asm int 3 }
LabelSymbol *new_internal_label(void) { __asm int 3 }

#include "register-use.h"
int __fastcall register_partial_write_safe(unsigned short reg) { __asm int 3 }

#include "instruction-rewrite.h"
void __fastcall release_machine_address(MachineAddress *address) { __asm int 3 }
MachineInstruction *__fastcall insert_machine_after(MachineInstruction *previous, MachineInstruction *instruction) { __asm int 3 }
int __fastcall narrow_result_safe(MachineInstruction *instruction) { __asm int 3 }
int __fastcall expand_constant_multiply(MachineInstruction *instruction) { __asm int 3 }
unsigned __fastcall integer_width_mask(unsigned bytes) { __asm int 3 }
int __fastcall narrow_comparison_condition(unsigned condition) { __asm int 3 }
unsigned __fastcall integer_sign_bit(unsigned bytes) { __asm int 3 }
unsigned __fastcall integer_width_bits(unsigned bytes) { __asm int 3 }
int __fastcall register_mentioned_between(MachineInstruction *first, MachineInstruction *last, unsigned short reg) { __asm int 3 }
MachineInstruction *__fastcall clone_machine_record(MachineInstruction *instruction) { __asm int 3 }
int __fastcall register_source_changed_between(MachineInstruction *first, MachineInstruction *last, unsigned short reg) { __asm int 3 }
unsigned __fastcall byte_register_available(unsigned short reg) { __asm int 3 }
void __fastcall append_embedded_record(Node *node) { __asm int 3 }

#include "value-selection.h"
void __fastcall record_x87_node(Node *node) { __asm int 3 }
unsigned __fastcall lookup_floating_opcode(unsigned opcode, const FloatingOpcode *map) { __asm int 3 }
void __fastcall arrange_x87_left(Node *node) { __asm int 3 }
MachineOperand *__fastcall emit_register_barrier(MachineOperand *operand, unsigned type) { __asm int 3 }
unsigned short __fastcall select_conditional_value(Node *node) { __asm int 3 }
void mark_x87_call_result(void) { __asm int 3 }
MachineOperand *__fastcall frame_temporary_operand(SymbolHeader *temporary, unsigned offset) { __asm int 3 }
MachineOperand *__fastcall immediate_operand(unsigned value) { __asm int 3 }
void __fastcall spill_x87_for_node(Node *node) { __asm int 3 }
unsigned short __fastcall select_intrinsic(Node *node) { __asm int 3 }
SymbolHeader *__fastcall floating_constant_symbol(Node *constant) { __asm int 3 }
#include "bitsets.h"

#include "unoptimized.h"
unsigned __fastcall assign_physical_register(MachineInstruction *instruction, unsigned reg, int last_use) { __asm int 3 }
BitSet *__fastcall bitset_remove(BitSet *set, unsigned member) { __asm int 3 }
void __fastcall advance_operand_byte(MachineOperand *operand) { __asm int 3 }
int __fastcall expand_zero_extension(MachineInstruction *instruction) { __asm int 3 }
void __fastcall optimize_sahf(MachineInstruction *instruction) { __asm int 3 }
MachineInstruction *__fastcall previous_machine_operation(MachineInstruction *instruction) { __asm int 3 }

#include "machine-layout.h"

#include "instruction-encoding.h"
int __fastcall fits_signed_byte(int value) { __asm int 3 }
unsigned __fastcall displacement_mode(MachineInstruction *instruction, MachineOperand *operand, unsigned *bytes) { __asm int 3 }
unsigned __fastcall label_has_backward_branch(MachineInstruction *instruction) { __asm int 3 }
int __fastcall alignment_lookahead(MachineInstruction *instruction) { __asm int 3 }
int __fastcall extend_previous_instruction(MachineInstruction *instruction, unsigned bytes) { __asm int 3 }
unsigned __fastcall coff_define_weak_symbol(char *name, unsigned short section,
    unsigned char storage, unsigned offset, unsigned short type, unsigned fallback,
    unsigned char search) { __asm int 3 }
unsigned __fastcall format_integer(unsigned value, char *text, unsigned radix) { __asm int 3 }
unsigned __fastcall coff_define_aux_symbol(char *name, unsigned short section,
    unsigned char storage, unsigned offset, unsigned short type, void **auxiliary) { __asm int 3 }
unsigned __fastcall coff_data_position(CoffScope *scope) { __asm int 3 }
void __fastcall prepare_source_listing(MachineInstruction *instructions) { __asm int 3 }
void __fastcall open_listing_source(char *filename) { __asm int 3 }
int __fastcall write_demangled_listing(SymbolHeader *symbol, int first) { __asm int 3 }
void __fastcall pad_listing_column(unsigned column) { __asm int 3 }
void __fastcall write_listing_symbol(SymbolHeader *symbol) { __asm int 3 }
int __fastcall write_listing_comment(int first) { __asm int 3 }
unsigned __fastcall write_listing_line(unsigned line) { __asm int 3 }
void __fastcall write_listing_source(unsigned line) { __asm int 3 }
MachineInstruction *__fastcall write_instruction_listing(MachineInstruction *instruction, unsigned *offset) { __asm int 3 }
void __fastcall write_local_listing(SymbolHeader *symbol) { __asm int 3 }
void __cdecl compiler_error(unsigned code, ...) { __asm int 3 }
void __fastcall coff_begin_lines(unsigned handle, unsigned short line,
    unsigned bytes, unsigned offset) { __asm int 3 }
void __fastcall coff_line_marker(unsigned short marker) { __asm int 3 }
void __fastcall coff_end_lines(unsigned short line, unsigned offset) { __asm int 3 }
void __fastcall coff_write_relocations(CoffContribution *contribution, unsigned base) { __asm int 3 }
void __fastcall coff_write_lines(CoffContribution *contribution, unsigned base) { __asm int 3 }
unsigned input_float_bits(void) { __asm int 3 }
void __fastcall reserve_segment_storage(SymbolHeader *symbol) { __asm int 3 }


void unsigned_integer_to_extended(const unsigned *integer, Extended *number) { __asm int 3 }
int extended_to_integer(const Extended *number, int *result) { __asm int 3 }



Node *__fastcall simplify_reciprocal(Node *node) { __asm int 3 }
Node *__fastcall simplify_global_conversion(Node *node) { __asm int 3 }
Node *__fastcall distribute_unary_tree(Node *node) { __asm int 3 }
int __fastcall is_negatable_negative_constant(Node *node) { __asm int 3 }

void __fastcall reduce_additive_chain(Node *node) { __asm int 3 }
void __fastcall reduce_multiplicative_chain(Node *node) { __asm int 3 }
void __fastcall reduce_bitwise_terms(Node *node, unsigned identity) { __asm int 3 }
void __fastcall factor_bitwise_chain(Node *node, unsigned opcode) { __asm int 3 }


Node *__fastcall fold_address_offset(Node *node) { __asm int 3 }
void __fastcall combine_complementary_masks(Node *node) { __asm int 3 }
void __fastcall cancel_unary_factors(Node *node, unsigned unary, unsigned combine) { __asm int 3 }


Node *__fastcall narrow_binary_conversion(Node *node, unsigned short type) { __asm int 3 }
Node *__fastcall push_unary_below_conversion(Node *node) { __asm int 3 }

Node *__fastcall lift_both_unary(Node *node) { __asm int 3 }
Node *__fastcall negate_tree(Node *node) { __asm int 3 }
Node *__fastcall fold_bitwise_and(Node *node) { __asm int 3 }
int __fastcall extract_reciprocal(Node **node) { __asm int 3 }
int __fastcall extract_complement(Node **node) { __asm int 3 }


SymbolHeader *__fastcall tail_argument_address(Node *node) { __asm int 3 }
int __fastcall is_tail_call_position(Expression *expressions, Expression *call) { __asm int 3 }












unsigned __fastcall simplify_flow_regions(FlowRegion *regions) { __asm int 3 }

void __fastcall refresh_region_entries(WorkItem *work) { __asm int 3 }

WorkItem *__fastcall reduce_flow_regions(FlowRegion *regions) { __asm int 3 }






int __fastcall is_boundary_flow_label(Node *node) { __asm int 3 }

void __fastcall add_protected_flow_edge(WorkItem *work, WorkItem *successor) { __asm int 3 }
void __fastcall visit_machine_symbols(MachineInstruction *instruction, MachineSymbolVisitor visitor, void *context) { __asm int 3 }
unsigned __fastcall reference_assembly_flow_symbol(SymbolHeader *symbol, void *context) { __asm int 3 }
ExceptionScope *__fastcall lookup_exception_scope(ExceptionScope *scopes, long identifier) { __asm int 3 }

unsigned __fastcall normalize_assembly_flow_symbol(SymbolHeader *symbol, void *context) { __asm int 3 }



void __fastcall forget_exception_label(Function *function, SymbolHeader *label) { __asm int 3 }




Node *new_flow_label_node(void) { __asm int 3 }

void __fastcall replace_flow_region_last(FlowRegion *region, WorkItem *replacement) { __asm int 3 }

FlowLink *__fastcall partition_loop_predecessors(FlowLink *predecessors, WorkItem *header) { __asm int 3 }
void __fastcall redirect_flow_label(WorkItem *work, SymbolHeader *old_label, SymbolHeader *new_label) { __asm int 3 }
void __fastcall assign_flow_region(FlowRegion *region) { __asm int 3 }

int __fastcall work_in_flow_region(WorkItem *work, FlowRegion *region) { __asm int 3 }

void __fastcall collect_region_values(FlowRegion *region) { __asm int 3 }


int __fastcall available_chain_operands(Node *node, BitSet *available) { __asm int 3 }

void __fastcall retarget_empty_work_branch(WorkItem *work, Node *node) { __asm int 3 }

int __fastcall equal_chain_value_ids(Node *left, Node *right) { __asm int 3 }




void __fastcall invalidate_work_tree(Node *node) { __asm int 3 }
int __fastcall has_no_later_work_store(Expression *expression) { __asm int 3 }

void hoist_invariant_region_stores(void) { __asm int 3 }
void collect_basic_region_inductions(void) { __asm int 3 }
int prepare_region_exit_motion(void) { __asm int 3 }
void collect_derived_region_inductions(void) { __asm int 3 }
void combine_region_inductions(void) { __asm int 3 }
void reduce_region_induction_strength(void) { __asm int 3 }
void rewrite_region_inductions(void) { __asm int 3 }
int simplify_region_exit(void) { __asm int 3 }
void finish_region_motion(void) { __asm int 3 }
void rebuild_region_header_references(void) { __asm int 3 }
void release_region_motion(void) { __asm int 3 }


Node *__fastcall materialize_numbered_tree(Node *node) { __asm int 3 }



Node *__fastcall make_switch_pivot(Node *range) { __asm int 3 }
void __fastcall split_switch_ranges(Node *ranges, unsigned cases, int count, unsigned span, Node **pivot, int *left_count) { __asm int 3 }
void __fastcall emit_switch_table(Node *ranges, int count, int option1, int option2) { __asm int 3 }

Node *__fastcall operator_identity_node(Node *operation, Node *type_source) { __asm int 3 }
int __fastcall matches_assignment_load(Node *destination, Node *operand) { __asm int 3 }
void __fastcall reduce_assignment_operator(Node *assignment, Node *operand, Node *load, Node *operation) { __asm int 3 }
int __fastcall find_assignment_operand(Node *node, unsigned has_addition) { __asm int 3 }

void __fastcall record_float_register(unsigned short number, SymbolHeader *symbol) { __asm int 3 }




int __cdecl compare_allocation_load_counts(const void *left, const void *right) { __asm int 3 }

int __fastcall allocation_use_dominates_backedges(WorkItem *work) { __asm int 3 }
void __fastcall release_allocation_range(AllocationRange *range) { __asm int 3 }

void __fastcall restore_excluded_registers(Node *node) { __asm int 3 }

void __fastcall split_register_transfer_edge(AllocationRange *range, Node *value, WorkItem *work, FlowLink *edge) { __asm int 3 }
void __fastcall mark_register_definition_ends(WorkItem *work, AllocationValue *value, AllocationRange *range) { __asm int 3 }

MachineInstruction *__fastcall find_transfer_call_boundary(MachineInstruction *transfer) { __asm int 3 }


int __fastcall memory_displacements_overlap(MachineOperand *left, unsigned left_bytes, MachineOperand *right, unsigned right_bytes) { __asm int 3 }
void __fastcall analyze_machine_regions(MachineFlowRegion *region, unsigned flags) { __asm int 3 }
void __fastcall replace_machine_region_suffix(MachineFlowRegion *parent, MachineFlowRegion *before, MachineFlowRegion *regions) { __asm int 3 }
int __fastcall close_machine_region(MachineFlowRegion *parent, MachineFlowRegion *region) { __asm int 3 }
void __fastcall finish_closed_machine_region(MachineFlowRegion *region, MachineFlowRegion *parent) { __asm int 3 }
MachineFlowRegion *__fastcall process_special_machine_region_reference(MachineFlowRegion *region, SymbolHeader *label) { __asm int 3 }
void __fastcall prepare_float_machine_data(int has_protected_blocks) { __asm int 3 }
void rewrite_float_machine_data(void) { __asm int 3 }
void prepare_float_stream_uses(void) { __asm int 3 }
void __fastcall prepare_machine_region_transform(MachineFlowRegion *region) { __asm int 3 }
void __fastcall prepare_region_insertion(MachineFlowRegion *region) { __asm int 3 }
void __fastcall replace_machine_definition(unsigned identifier, MachineInstruction *previous, MachineInstruction *replacement) { __asm int 3 }
void __fastcall record_machine_definition_range(unsigned short number, MachineInstruction *first, MachineInstruction *last) { __asm int 3 }
void __fastcall record_region_register(MachineFlowRegion *region, unsigned short number, int definition, int use) { __asm int 3 }
void record_debug_register_locations(void) { __asm int 3 }
void __fastcall spill_register_variables(RegisterVariable *variables, int simplification) { __asm int 3 }
int __fastcall function_allows_frame_register(Function *function) { __asm int 3 }
int __fastcall machine_sequence_cost(MachineInstruction *instruction) { __asm int 3 }
int __fastcall register_precedes_lea(MachineInstruction *instruction, unsigned short number) { __asm int 3 }
int __fastcall register_follows_lea(MachineInstruction *instruction, unsigned short number) { __asm int 3 }
void __fastcall record_rematerialized_registers(WorkItem *work, MachineInstruction *instruction) { __asm int 3 }
int __fastcall register_spill_homes_compatible(RegisterVariable *left, RegisterVariable *right) { __asm int 3 }
int __fastcall count_shared_register_conflicts(RegisterVariable *left, RegisterVariable *right) { __asm int 3 }
void __fastcall remove_register_conflict(unsigned short left, unsigned short right) { __asm int 3 }



int __fastcall choose_memory_split_register(MachineInstruction *instruction, unsigned short *number) { __asm int 3 }


int __fastcall operand_value_depends_on_color(MachineOperand *operand, unsigned short color) { __asm int 3 }


int __fastcall equivalent_stack_copy_operands(MachineInstruction *first, MachineOperand *first_operand, MachineInstruction *last, MachineOperand *last_operand) { __asm int 3 }
int __fastcall stack_copy_operand_changed(MachineInstruction *first, MachineInstruction *last, MachineOperand *operand, unsigned bytes) { __asm int 3 }
void __fastcall retarget_comparison_branch(MachineInstruction *target, MachineInstruction *branch, MachineInstruction *load, int mode) { __asm int 3 }
