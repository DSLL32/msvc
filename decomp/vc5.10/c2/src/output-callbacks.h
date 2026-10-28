#ifndef C2_OUTPUT_CALLBACKS_H
#define C2_OUTPUT_CALLBACKS_H
#include "c2.h"
#include <recon.h>

/* Stores-only function identities until the actual consumer supplies its ABI.
 * The member deliberately has an incomplete prototype. Direct slot(...) is
 * ill-typed; invocation or escape of .target is a review hold. No accepted
 * consumer currently calls these slots. Unresolved targets remain INT3 stubs;
 * the known shared no-op address is the existing c2_set_argv function.
 */
typedef struct C2PendingCallback {
    void (__cdecl *target)();
} C2PendingCallback;
typedef struct C2PendingCallbackAlignment { char byte; C2PendingCallback slot; } C2PendingCallbackAlignment;
typedef char C2PendingCallbackSizeCheck[sizeof(C2PendingCallback) == 4 ? 1 : -1];
typedef char C2PendingCallbackOffsetCheck[offsetof(C2PendingCallback, target) == 0 ? 1 : -1];
typedef char C2PendingCallbackAlignmentCheck[offsetof(C2PendingCallbackAlignment, slot) == 4 ? 1 : -1];
/* Unit completion invokes both alternatives without arguments or a result.
 * Their complete original hot/cold paths use no entry inputs and RET0. */
typedef struct C2OutputFinishCallback { void (__cdecl *target)(void); } C2OutputFinishCallback;
typedef char C2OutputFinishCallbackSizeCheck[sizeof(C2OutputFinishCallback) == 4 ? 1 : -1];
typedef char C2OutputFinishCallbackOffsetCheck[offsetof(C2OutputFinishCallback, target) == 0 ? 1 : -1];
typedef struct C2OutputDirective C2OutputDirective;
typedef void (__fastcall *C2OutputDirectiveWriter)(C2OutputDirective *directive);
typedef struct C2DirectiveCallback { C2OutputDirectiveWriter target; } C2DirectiveCallback;
typedef char C2DirectiveCallbackSizeCheck[sizeof(C2DirectiveCallback) == 4 ? 1 : -1];
typedef char C2DirectiveCallbackOffsetCheck[offsetof(C2DirectiveCallback, target) == 0 ? 1 : -1];
/* The visitor calls with the symbol in ECX, no stack arguments and RET0.
 * All known assembly callbacks use that same signature; their bodies remain
 * pending. The shared prefix admits function and input symbol subtypes. */
typedef void (__fastcall *C2AssemblyVisitor)(C2Symbol *symbol);
void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor);
void __fastcall assembly_callback_4894ca(C2Symbol *symbol);
void __fastcall assembly_callback_489596(C2Symbol *symbol);
void __fastcall assembly_callback_489693(C2Symbol *symbol);
void __fastcall assembly_callback_48a251(C2Symbol *symbol);
typedef void (__fastcall *C2SymbolWriter)(C2Symbol *symbol);
typedef struct C2SymbolCallback { C2SymbolWriter target; } C2SymbolCallback;
typedef char C2SymbolCallbackSizeCheck[sizeof(C2SymbolCallback) == 4 ? 1 : -1];
typedef char C2SymbolCallbackOffsetCheck[offsetof(C2SymbolCallback, target) == 0 ? 1 : -1];
/* Reached function-output slots have independent, actual fastcall views.
 * All other pending slots retain their stores-only barrier. */
typedef void (__fastcall *C2FunctionCodeWriter)(C2Symbol *symbol,
    char *file, int unwind, unsigned line);
typedef struct C2FunctionCodeCallback { C2FunctionCodeWriter target; } C2FunctionCodeCallback;
typedef char C2FunctionCodeCallbackSize[sizeof(C2FunctionCodeCallback) == 4 ? 1 : -1];
/* These register-only views are recovered from the actual output caller and
 * complete callback bodies. The canonical no-op alternatives are code-free
 * aliases of the existing no-input RET0 CRT body, not new function vertices. */
typedef struct C2SectionCallback { void (__fastcall *target)(C2Section *section); } C2SectionCallback;
typedef struct C2BytesCallback { void (__fastcall *target)(const void *bytes, unsigned count); } C2BytesCallback;
typedef struct C2FilenameCallback { void (__fastcall *target)(char *file); } C2FilenameCallback;
typedef struct C2LineCallback { void (__fastcall *target)(unsigned line, unsigned offset); } C2LineCallback;
typedef char C2OutputViewsSizeCheck[sizeof(C2SectionCallback) == 4 &&
    sizeof(C2BytesCallback) == 4 && sizeof(C2FilenameCallback) == 4 &&
    sizeof(C2LineCallback) == 4 ? 1 : -1];
void __fastcall output_noop_section(C2Section *section);
void __fastcall output_noop_bytes(const void *bytes, unsigned count);
void __fastcall output_noop_filename(char *file);
void __fastcall output_noop_line(unsigned line, unsigned offset);
void __fastcall output_noop_symbol(C2Symbol *symbol);
extern C2SymbolCallback output_slot_497de4;
extern C2PendingCallback output_slot_499a00;
extern C2OutputFinishCallback output_slot_497e10;
extern C2SymbolCallback output_slot_497dbc;
extern C2SymbolCallback output_slot_497de0;
extern C2FunctionCodeCallback output_slot_497dec;
extern C2BytesCallback output_slot_497db0;
extern C2BytesCallback output_slot_497dd4;
extern C2SectionCallback output_slot_497ce4;
extern C2FilenameCallback output_slot_497ddc;
extern C2LineCallback output_slot_497dcc;
typedef void (__fastcall *C2CodeRelocationWriter)(unsigned offset, C2Symbol *symbol);
typedef struct C2CodeRelocationCallback { C2CodeRelocationWriter target; } C2CodeRelocationCallback;
typedef char C2CodeRelocationCallbackSize[sizeof(C2CodeRelocationCallback) == 4 ? 1 : -1];
void __fastcall output_noop_relocation(unsigned offset, C2Symbol *symbol);
extern C2CodeRelocationCallback output_slot_499750;
extern C2CodeRelocationCallback output_slot_491070;
extern C2CodeRelocationCallback output_slot_497db4;
extern C2CodeRelocationCallback output_slot_491078;
extern C2PendingCallback output_slot_497f60;
extern C2OutputFinishCallback output_slot_497dfc;
/* The initializer callback receives ECX=value, EDX=symbol and a single
 * stack section pointer; EAX counts bytes and both alternatives use RET4. */
typedef unsigned (__fastcall *C2InitializerWriter)(C2InitRecord *value,
    C2Symbol *symbol, C2Section *section);
typedef struct C2InitializerCallback { C2InitializerWriter target; } C2InitializerCallback;
typedef char C2InitializerCallbackSize[sizeof(C2InitializerCallback) == 4 ? 1 : -1];
extern C2InitializerCallback output_slot_497f50;
extern C2DirectiveCallback output_slot_49971c;
extern C2SymbolCallback output_slot_497cec;
void __fastcall select_output_section(C2Section *section);
void __fastcall output_callback_41f3f2(const void *bytes, unsigned count);
void __fastcall output_callback_41fa04(C2Symbol *symbol);
void __fastcall output_callback_434139(C2Symbol *symbol, char *file, int unwind, unsigned line);
void __fastcall output_callback_4346cf(C2Symbol *symbol);
void __fastcall output_callback_435633(C2Symbol *symbol);
void __fastcall output_callback_4357b8(unsigned offset, C2Symbol *symbol);
void __fastcall output_callback_435af6(unsigned offset, C2Symbol *symbol);
void __fastcall output_callback_43696a(unsigned offset, C2Symbol *symbol);
void __fastcall output_callback_442b9f(unsigned line, unsigned offset);
void __fastcall output_callback_442d21(char *file);
void output_callback_442d33();
void __fastcall output_callback_44308f(C2Symbol *symbol);
void output_callback_4432a1();
unsigned __fastcall output_callback_44a79d(C2InitRecord *value, C2Symbol *symbol, C2Section *section);
void __fastcall emit_output_directive(C2OutputDirective *directive);
void __fastcall emit_incremental_string(char *value);
void __fastcall write_coff_directive(char *prefix, char *value, char *suffix, int quoted);
void output_callback_4541f3(void);
void output_callback_454ba8(void);
void __fastcall output_callback_4849f2(C2Symbol *symbol, char *file, int unwind, unsigned line);
unsigned __fastcall output_callback_4849f5(C2InitRecord *value, C2Symbol *symbol, C2Section *section);
void __fastcall output_callback_4849fa(C2Symbol *symbol);
void __fastcall output_callback_4850f5(unsigned offset, C2Symbol *symbol);
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
