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
#include "ide.h"
#include <stdarg.h>
#include <recon.h>
BOOL WINAPI compiler_control(DWORD event) { __asm int 3 }
void __fastcall compiler_internal_error(char *source, unsigned code) { __asm int 3 }
int __fastcall suffix_matches(char *pattern, char *text, unsigned action) { __asm int 3 }
Node *__fastcall lift_left_sequence(Node *node) { __asm int 3 }
Node *__fastcall lift_right_sequence(Node *node) { __asm int 3 }
Node *__fastcall discard_sequence_tail(Node *node) { __asm int 3 }
Node *__fastcall lower_target_address(Node *node) { __asm int 3 }
#include "inline-calls.h"
unsigned __fastcall second_return_value_register(FunctionSymbol *symbol) { __asm int 3 }
#include "algebra.h"
int __fastcall lower_far_pointer_cast(Node *node) { __asm int 3 }
#include "work-prepare.h"
#include "instruction-selection.h"
void __fastcall uppercase_name(char *name) { __asm int 3 }
#include "output.h"
unsigned __fastcall output_noop_relocation(Initializer *initializer, SymbolHeader *symbol, Segment *segment) { __asm int 3 }
__declspec(naked) void output_noop_record(void) { RECON_TRACE_NAKED_ENTRY(output_noop_record) __asm int 3 }


__declspec(naked) void coff_output_value(void) { RECON_TRACE_NAKED_ENTRY(coff_output_value) __asm int 3 }




__declspec(naked) void omf_output_value(void) { RECON_TRACE_NAKED_ENTRY(omf_output_value) __asm int 3 }






void __fastcall omf_emit_weak_external(FunctionSymbol *symbol) { __asm int 3 }





#include "coff-output.h"
void __fastcall coff_error(unsigned code, char *source, unsigned line) { __asm int 3 }
#include "coff-writer.h"
#include "coff-segments.h"
#include "listing.h"
#include "machine.h"
SymbolHeader *__fastcall lookup_alias_target(SymbolHeader *symbol) { __asm int 3 }
#include "symbol-output.h"
#include "frame.h"



#include "register-use.h"

#include "instruction-rewrite.h"
unsigned __fastcall integer_width_bits(unsigned bytes) { __asm int 3 }

#include "value-selection.h"
#include "bitsets.h"

#include "unoptimized.h"

#include "machine-layout.h"

#include "instruction-encoding.h"
#include "float-peepholes.h"
#include "float-machine.h"
#include <recon.h>
unsigned __fastcall coff_define_weak_symbol(char *name, unsigned short section,
    unsigned char storage, unsigned offset, unsigned short type, unsigned fallback,
    unsigned char search) { __asm int 3 }






































void __fastcall forget_exception_label(Function *function, SymbolHeader *label) { __asm int 3 }




























int __cdecl compare_allocation_load_counts(const void *left, const void *right) { __asm int 3 }

int __fastcall allocation_use_dominates_backedges(WorkItem *work) { __asm int 3 }


int __fastcall redirect_assembly_label(Node *node, SymbolHeader *old_label, SymbolHeader *new_label) { __asm int 3 }



int __fastcall retarget_machine_region_entry(LabelSymbol *label, LabelSymbol *replacement, MachineInstruction *first, MachineInstruction *last) { __asm int 3 }
int __fastcall register_spill_homes_compatible(RegisterVariable *left, RegisterVariable *right) { __asm int 3 }
int __fastcall count_shared_register_conflicts(RegisterVariable *left, RegisterVariable *right) { __asm int 3 }








void __fastcall redirect_pentium_branch_labels(MachineInstruction *branch, MachineInstruction *middle) { __asm int 3 }

#include "intrinsics.h"
void __fastcall append_intrinsic_expression(Node *expression) { __asm int 3 }
short __fastcall next_numbered_work(WorkItem *work) { __asm int 3 }
int __fastcall has_unblocked_derived_use(Node *node) { __asm int 3 }
void __fastcall transform_machine_region_blocks(MachineFlowRegion *region) { __asm int 3 }
int __fastcall pentium_flags_live_after(MachineInstruction *instruction) { __asm int 3 }
MachineInstruction *__fastcall split_machine_region_exit(MachineFlowRegion *region) { __asm int 3 }
void __fastcall mark_machine_region_split(MachineFlowRegion *region, unsigned short color, unsigned short number, int apply, MachineBlockHeader *block) { __asm int 3 }

void __fastcall include_register_block_use(unsigned short number, int block) { __asm int 3 }
void __fastcall split_float_motion_branch(MachineInstruction *instruction, MemoryMotionContext *context) { __asm int 3 }
unsigned __fastcall float_register_arithmetic_opcode(unsigned operation) { __asm int 3 }

int __fastcall ide_error(char *source, unsigned line) { __asm int 3 }
