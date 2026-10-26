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
Node *__fastcall lower_target_address(Node *node) { __asm int 3 }
#include "inline-calls.h"
#include "algebra.h"
int __fastcall lower_far_pointer_cast(Node *node) { __asm int 3 }
#include "work-prepare.h"
#include "instruction-selection.h"
void __fastcall uppercase_name(char *name) { __asm int 3 }
#include "output.h"
unsigned __fastcall output_noop_relocation(Initializer *initializer, SymbolHeader *symbol, Segment *segment) { __asm int 3 }
__declspec(naked) void output_noop_record(void) { RECON_TRACE_NAKED_ENTRY(output_noop_record) __asm int 3 }






__declspec(naked) void omf_output_value(void) { RECON_TRACE_NAKED_ENTRY(omf_output_value) __asm int 3 }











#include "coff-output.h"
void __fastcall coff_error(unsigned code, char *source, unsigned line) { __asm int 3 }
#include "coff-writer.h"
#include "coff-segments.h"
#include "listing.h"
#include "machine.h"
#include "symbol-output.h"
#include "frame.h"



#include "register-use.h"

#include "instruction-rewrite.h"

#include "value-selection.h"
#include "bitsets.h"

#include "unoptimized.h"

#include "machine-layout.h"

#include "instruction-encoding.h"
#include "float-peepholes.h"
#include "float-machine.h"
#include <recon.h>

















































































void __fastcall redirect_pentium_branch_labels(MachineInstruction *branch, MachineInstruction *middle) { __asm int 3 }

#include "intrinsics.h"


int __fastcall ide_error(char *source, unsigned line) { __asm int 3 }
