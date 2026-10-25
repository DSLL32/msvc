#include "scheduler.h"
#include "intrinsic-selection.h"
#include "jump-threading.h"
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
#include "exception-scope.h"
#include "work-optimizer.h"
/* Unreached graph vertices trap deliberately for the paired reconstruction workflow. */
#include "options.h"
#include "input.h"
#include "backend.h"
#include "extended.h"
#include "segments.h"
#include "omf-output.h"
#include "initializers.h"
#include "functions.h"
#include "function-input.h"
#include "tree-input.h"
#include "expression-fields.h"
#include "control-input.h"
#include "tree-rewrite.h"
#include "records.h"
#include "backend-passes.h"
#include "float-peepholes.h"
#include "float-machine.h"
#include <stdarg.h>
#include <recon.h>
BOOL WINAPI compiler_control(DWORD event) { __asm int 3 }
void initialize_debugging(void) { __asm int 3 }
void __fastcall compiler_internal_error(char *source, unsigned code) { __asm int 3 }
void __cdecl compiler_report(unsigned code, ...) { __asm int 3 }
void __fastcall report_context_error(unsigned code, char *source, char *filename) { __asm int 3 }
int __fastcall suffix_matches(char *pattern, char *text, unsigned action) { __asm int 3 }
Node *__fastcall intern_typed_label(Function *function, Node *node) { __asm int 3 }
DoubleBits input_double_bits(void) { __asm int 3 }
int extended_to_float(const Extended *number, float *result) { __asm int 3 }
void float_to_extended(const float *number, Extended *result) { __asm int 3 }
char *__fastcall undecorate_function_name(char *name) { __asm int 3 }
void __cdecl ide_message(int code, ...) { __asm int 3 }
void __fastcall set_warning_override(short number, signed char level) { __asm int 3 }
Node *__fastcall simplify_short_circuit(Node *node, Node *parent) { __asm int 3 }
Node *__fastcall lift_left_sequence(Node *node) { __asm int 3 }
Node *__fastcall lift_right_sequence(Node *node) { __asm int 3 }
int __fastcall assign_parameter_register(unsigned *used, Node *argument, int convention) { __asm int 3 }
Node *__fastcall discard_sequence_tail(Node *node) { __asm int 3 }
Node *__fastcall rewrite_short_circuit(Node *node) { __asm int 3 }
Node *__fastcall materialize_expression(Node *node) { __asm int 3 }
Node *__fastcall lower_target_address(Node *node) { __asm int 3 }
#include "inline-calls.h"
int __fastcall inline_argument_has_global_load(Node *node) { __asm int 3 }
unsigned __fastcall mark_inline_assembly_parameter(SymbolHeader *symbol, void *context) { __asm int 3 }
unsigned __fastcall substitute_inline_assembly_symbol(SymbolHeader *symbol, void *context) { __asm int 3 }
unsigned __fastcall second_return_value_register(FunctionSymbol *symbol) { __asm int 3 }
Node *__fastcall fold_floating_comparison(Node *node) { __asm int 3 }
#include "algebra.h"
int __fastcall is_nonnegative_tree(Node *node) { __asm int 3 }
Node *__fastcall lift_right_unary(Node *node) { __asm int 3 }
int __fastcall lower_far_pointer_cast(Node *node) { __asm int 3 }
#include "work-prepare.h"
#include "instruction-selection.h"
void __fastcall read_qualified_symbol(SymbolHeader *symbol, int arena) { __asm int 3 }
void __fastcall clone_alias_symbol(SymbolHeader *symbol) { __asm int 3 }
void __fastcall change_warning_override(short number, signed char level) { __asm int 3 }
char *__fastcall diagnostic_format(unsigned number) { __asm int 3 }
void __cdecl emit_diagnostic(unsigned category, unsigned number, char *format, va_list arguments) { __asm int 3 }
void __fastcall uppercase_name(char *name) { __asm int 3 }
void __fastcall emit_function_inline_data(Function *function) { __asm int 3 }
#include "output.h"
__declspec(naked) void output_noop_data(void) { RECON_TRACE_NAKED_ENTRY(output_noop_data) __asm int 3 }
__declspec(naked) void output_noop_value(void) { RECON_TRACE_NAKED_ENTRY(output_noop_value) __asm int 3 }
unsigned __fastcall output_noop_relocation(Initializer *initializer, SymbolHeader *symbol, Segment *segment) { __asm int 3 }
__declspec(naked) void output_noop_record(void) { RECON_TRACE_NAKED_ENTRY(output_noop_record) __asm int 3 }
__declspec(naked) void mark_unwritten_symbol(void) { RECON_TRACE_NAKED_ENTRY(mark_unwritten_symbol) __asm int 3 }

void __fastcall write_codeview_constant(SymbolHeader *symbol) { __asm int 3 }

__declspec(naked) void coff_output_value(void) { RECON_TRACE_NAKED_ENTRY(coff_output_value) __asm int 3 }




__declspec(naked) void omf_output_value(void) { RECON_TRACE_NAKED_ENTRY(omf_output_value) __asm int 3 }






void __fastcall omf_emit_weak_external(FunctionSymbol *symbol) { __asm int 3 }





#include "coff-output.h"
void __fastcall coff_error(unsigned code, char *source, unsigned line) { __asm int 3 }
#include "coff-writer.h"
void __fastcall coff_write_comment(char *text) { __asm int 3 }
#include "coff-segments.h"
#include "listing.h"
#include "machine.h"
SymbolHeader *__fastcall lookup_alias_target(SymbolHeader *symbol) { __asm int 3 }
#include "symbol-output.h"
#include "frame.h"


MachineOperand *__fastcall high_dword_operand(MachineOperand *operand) { __asm int 3 }
unsigned short __fastcall select_assembly_record(Node *node) { __asm int 3 }

#include "register-use.h"

#include "instruction-rewrite.h"
int __fastcall narrow_result_safe(MachineInstruction *instruction) { __asm int 3 }
int __fastcall narrow_comparison_condition(unsigned condition) { __asm int 3 }
unsigned __fastcall integer_sign_bit(unsigned bytes) { __asm int 3 }
unsigned __fastcall integer_width_bits(unsigned bytes) { __asm int 3 }

#include "value-selection.h"
#include "bitsets.h"

#include "unoptimized.h"
unsigned __fastcall spill_lowering_register(MachineInstruction *instruction, unsigned reg) { __asm int 3 }

#include "machine-layout.h"

#include "instruction-encoding.h"
#include "float-peepholes.h"
#include "float-machine.h"
#include <recon.h>
int __fastcall add_alignment_prefix(MachineInstruction *instruction) { __asm int 3 }
unsigned __fastcall coff_define_weak_symbol(char *name, unsigned short section,
    unsigned char storage, unsigned offset, unsigned short type, unsigned fallback,
    unsigned char search) { __asm int 3 }
int __fastcall write_demangled_listing(SymbolHeader *symbol, int first) { __asm int 3 }
unsigned __fastcall write_listing_line(unsigned line) { __asm int 3 }
void __cdecl compiler_error(unsigned code, ...) { __asm int 3 }
unsigned input_float_bits(void) { __asm int 3 }


void unsigned_integer_to_extended(const unsigned *integer, Extended *number) { __asm int 3 }








Node *__fastcall push_unary_below_conversion(Node *node) { __asm int 3 }

int __fastcall extract_complement(Node **node) { __asm int 3 }


SymbolHeader *__fastcall tail_argument_address(Node *node) { __asm int 3 }
int __fastcall is_tail_call_position(Expression *expressions, Expression *call) { __asm int 3 }













void __fastcall refresh_region_entries(WorkItem *work) { __asm int 3 }








void __fastcall add_protected_flow_edge(WorkItem *work, WorkItem *successor) { __asm int 3 }




void __fastcall forget_exception_label(Function *function, SymbolHeader *label) { __asm int 3 }












int __fastcall equal_chain_value_ids(Node *left, Node *right) { __asm int 3 }










Node *__fastcall make_switch_pivot(Node *range) { __asm int 3 }
void __fastcall split_switch_ranges(Node *ranges, unsigned cases, int count, unsigned span, Node **pivot, int *left_count) { __asm int 3 }






int __cdecl compare_allocation_load_counts(const void *left, const void *right) { __asm int 3 }

int __fastcall allocation_use_dominates_backedges(WorkItem *work) { __asm int 3 }

void __fastcall restore_excluded_registers(Node *node) { __asm int 3 }

int __fastcall redirect_assembly_label(Node *node, SymbolHeader *old_label, SymbolHeader *new_label) { __asm int 3 }
void __fastcall mark_register_definition_ends(WorkItem *work, AllocationValue *value, AllocationRange *range) { __asm int 3 }

MachineInstruction *__fastcall find_transfer_call_boundary(MachineInstruction *transfer) { __asm int 3 }


int __fastcall close_machine_region(MachineFlowRegion *parent, MachineFlowRegion *region) { __asm int 3 }
void __fastcall prepare_machine_region_transform(MachineFlowRegion *region) { __asm int 3 }
void __fastcall prepare_region_insertion(MachineFlowRegion *region) { __asm int 3 }
void __fastcall replace_machine_definition(unsigned identifier, MachineInstruction *previous, MachineInstruction *replacement) { __asm int 3 }
void __fastcall record_machine_definition_range(unsigned short number, MachineInstruction *first, MachineInstruction *last) { __asm int 3 }
void __fastcall record_region_register(MachineFlowRegion *region, unsigned short number, int definition, int use) { __asm int 3 }
int __fastcall register_spill_homes_compatible(RegisterVariable *left, RegisterVariable *right) { __asm int 3 }
int __fastcall count_shared_register_conflicts(RegisterVariable *left, RegisterVariable *right) { __asm int 3 }







int __fastcall stack_copy_operand_changed(MachineInstruction *first, MachineInstruction *last, MachineOperand *operand, unsigned bytes) { __asm int 3 }
void __fastcall retarget_comparison_branch(MachineInstruction *target, MachineInstruction *branch, MachineInstruction *load, int mode) { __asm int 3 }

void __fastcall redirect_pentium_branch_labels(MachineInstruction *branch, MachineInstruction *middle) { __asm int 3 }
void __fastcall coff_write_dword(unsigned value) { __asm int 3 }
void __fastcall coff_add_function_fixup(unsigned identifier, CoffRelocation *relocation) { __asm int 3 }

#include "intrinsics.h"
void __fastcall append_intrinsic_expression(Node *expression) { __asm int 3 }
Node *__fastcall push_unary_into_left(Node *node) { __asm int 3 }
short __fastcall next_numbered_work(WorkItem *work) { __asm int 3 }
void remove_region_blocks(void) { __asm int 3 }
void __fastcall remove_region_block(WorkItem *work) { __asm int 3 }
int __fastcall has_unblocked_derived_use(Node *node) { __asm int 3 }
void __fastcall insert_region_memory_operation(Expression *anchor, Node *value, Node *address,
    unsigned opcode, Node *counter, unsigned bytes, Node *assignment) { __asm int 3 }
unsigned short __fastcall select_intrinsic_a6(Node *node) { __asm int 3 }
unsigned short __fastcall select_intrinsic_aa_ab(Node *node, int mode) { __asm int 3 }
SymbolHeader *__fastcall lookup_external_name(char *name) { __asm int 3 }
void __fastcall transform_machine_region_blocks(MachineFlowRegion *region) { __asm int 3 }
int __fastcall pentium_flags_live_after(MachineInstruction *instruction) { __asm int 3 }
int __fastcall machine_region_is_ancestor(MachineFlowRegion *ancestor, MachineFlowRegion *region) { __asm int 3 }
MachineFlowRegion *__fastcall find_register_preservation_region(unsigned short number, MachineFlowRegion *region) { __asm int 3 }
MachineInstruction *__fastcall split_machine_region_exit(MachineFlowRegion *region) { __asm int 3 }
void __fastcall mark_machine_region_split(MachineFlowRegion *region, unsigned short color, unsigned short number, int apply, MachineBlockHeader *block) { __asm int 3 }
int __fastcall diamond_register_constraint(unsigned short number, MachineInstruction *first, MachineInstruction *last, unsigned bytes) { __asm int 3 }
void fill_previous_pentium_gap(void) { __asm int 3 }
int __fastcall register_has_region_uses(unsigned short number, MachineFlowRegion *regions) { __asm int 3 }
int __cdecl multiply_extended(const Extended *left, const Extended *right, Extended *result) { __asm int 3 }
int __cdecl divide_extended(const Extended *left, const Extended *right, Extended *result) { __asm int 3 }
int __cdecl minimum_extended(const Extended *left, const Extended *right, Extended *result) { __asm int 3 }
int __cdecl maximum_extended(const Extended *left, const Extended *right, Extended *result) { __asm int 3 }
int __cdecl absolute_extended(const Extended *number, Extended *result) { __asm int 3 }

void __fastcall save_float_machine_register(MachineInstruction *instruction, FloatMachineRegister *reg, unsigned short number, MachineInstruction *reload) { __asm int 3 }
MachineInstruction *__fastcall rewrite_pentium_float_store(MachineInstruction *instruction) { __asm int 3 }
void __fastcall process_float_stream_protected_block(MachineInstruction *instruction) { __asm int 3 }
void __fastcall include_register_block_use(unsigned short number, int block) { __asm int 3 }
void __fastcall split_float_motion_branch(MachineInstruction *instruction, MemoryMotionContext *context) { __asm int 3 }
void __fastcall finish_float_motion_branch(MachineInstruction *instruction, MemoryMotionContext *context) { __asm int 3 }
unsigned __fastcall float_register_arithmetic_opcode(unsigned operation) { __asm int 3 }
void __fastcall mark_float_branch_targets(FloatStreamBranch *branches) { __asm int 3 }
int __fastcall can_merge_float_branch(FloatStreamBranch *branch, MachineInstruction *previous, MachineInstruction *target) { __asm int 3 }
int __fastcall can_reuse_float_branch_target(MachineInstruction *target) { __asm int 3 }
int inherit_float_motion_uses(void) { __asm int 3 }

int __cdecl multiplication_overflows(unsigned left, unsigned right, int is_unsigned, unsigned bytes) { __asm int 3 }

Node *__fastcall new_scope_unwind(long target) { __asm int 3 }
