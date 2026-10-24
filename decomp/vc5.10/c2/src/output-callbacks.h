#ifndef C2_OUTPUT_CALLBACKS_H
#define C2_OUTPUT_CALLBACKS_H
#include "c2.h"
#include <recon.h>

/* Stores-only function identities until the actual consumer supplies its ABI.
 * The member deliberately has an incomplete prototype. Direct slot(...) is
 * ill-typed; invocation or escape of .target is a review hold. No accepted
 * consumer currently calls these slots. Targets remain genuine INT3 stubs.
 */
typedef struct C2PendingCallback {
    void (__cdecl *target)();
} C2PendingCallback;
typedef struct C2PendingCallbackAlignment { char byte; C2PendingCallback slot; } C2PendingCallbackAlignment;
typedef char C2PendingCallbackSizeCheck[sizeof(C2PendingCallback) == 4 ? 1 : -1];
typedef char C2PendingCallbackOffsetCheck[offsetof(C2PendingCallback, target) == 0 ? 1 : -1];
typedef char C2PendingCallbackAlignmentCheck[offsetof(C2PendingCallbackAlignment, slot) == 4 ? 1 : -1];
typedef struct C2OutputDirective C2OutputDirective;
typedef void (__fastcall *C2OutputDirectiveWriter)(C2OutputDirective *directive);
typedef struct C2DirectiveCallback { C2OutputDirectiveWriter target; } C2DirectiveCallback;
typedef char C2DirectiveCallbackSizeCheck[sizeof(C2DirectiveCallback) == 4 ? 1 : -1];
typedef char C2DirectiveCallbackOffsetCheck[offsetof(C2DirectiveCallback, target) == 0 ? 1 : -1];
/* Provisional code-address values only: this incomplete pointer view must not
 * be called. The symbol visitor remains a named trap until real signatures
 * are recovered at its consumer boundary. It is a scalar for the EDX argument.
 */
typedef void (__cdecl *C2AssemblyVisitor)();
void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor);
void assembly_callback_4894ca();
void assembly_callback_489596();
void assembly_callback_489693();
void assembly_callback_48a251();
typedef void (__fastcall *C2SymbolWriter)(C2Symbol *symbol);
typedef struct C2SymbolCallback { C2SymbolWriter target; } C2SymbolCallback;
typedef char C2SymbolCallbackSizeCheck[sizeof(C2SymbolCallback) == 4 ? 1 : -1];
typedef char C2SymbolCallbackOffsetCheck[offsetof(C2SymbolCallback, target) == 0 ? 1 : -1];
extern C2PendingCallback output_slot_497de4;
extern C2PendingCallback output_slot_499a00;
extern C2PendingCallback output_slot_497e10;
extern C2PendingCallback output_slot_497dbc;
extern C2PendingCallback output_slot_497de0;
extern C2PendingCallback output_slot_497dec;
extern C2PendingCallback output_slot_497db0;
extern C2PendingCallback output_slot_497dd4;
extern C2PendingCallback output_slot_497ce4;
extern C2PendingCallback output_slot_497ddc;
extern C2PendingCallback output_slot_497dcc;
extern C2PendingCallback output_slot_499750;
extern C2PendingCallback output_slot_491070;
extern C2PendingCallback output_slot_497db4;
extern C2PendingCallback output_slot_491078;
extern C2PendingCallback output_slot_497f60;
extern C2PendingCallback output_slot_497dfc;
extern C2PendingCallback output_slot_497f50;
extern C2DirectiveCallback output_slot_49971c;
extern C2SymbolCallback output_slot_497cec;
void output_callback_41eb89();
void __fastcall select_output_section(C2Section *section);
void output_callback_41f3f2();
void __fastcall output_callback_41fa04(C2Symbol *symbol);
void output_callback_434139();
void output_callback_4346cf();
void output_callback_435633();
void output_callback_4357b8();
void output_callback_435af6();
void output_callback_43696a();
void output_callback_442b9f();
void output_callback_442d21();
void output_callback_442d33();
void output_callback_44308f();
void output_callback_4432a1();
void output_callback_44a79d();
void __fastcall emit_output_directive(C2OutputDirective *directive);
void __fastcall emit_incremental_string(char *value);
void __fastcall write_coff_directive(char *prefix, char *value, char *suffix, int quoted);
void output_callback_4541f3();
void output_callback_454ba8();
void output_callback_4849f2();
void output_callback_4849f5();
void __fastcall output_callback_4849fa(C2Symbol *symbol);
void output_callback_4850f5();
extern char *output_group_name;
extern int output_enabled;
char *default_runtime_library(void);
struct C2OutputDirective {
    struct C2OutputDirective *next;
    int kind;
    char *value;
};
typedef char C2OutputDirectiveSizeCheck[sizeof(C2OutputDirective) == 12 ? 1 : -1];
typedef char C2OutputDirectiveKindCheck[offsetof(C2OutputDirective, kind) == 4 ? 1 : -1];
typedef char C2OutputDirectiveValueCheck[offsetof(C2OutputDirective, value) == 8 ? 1 : -1];
extern C2OutputDirective *output_directives;
C2OutputDirective *__fastcall add_output_directive(int kind, char *value);
void initialize_object_output(void);
void initialize_codeview_output(void);
#endif
