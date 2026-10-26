#ifndef C1XX_DECLARATION_H
#define C1XX_DECLARATION_H
#include "frontend-init.h"
#include <recon.h>

/* Local declaration state. The setters establish both the active flag and
 * pointer, including the two zero stores on SetFunctionContext(NULL). */
struct CompilerDeclarationState {
    CompilerDeclarationContext context; /* one actual local28-byte owner */
    void RECON_THISCALL SetReference(CompilerSymbol *);
    void RECON_THISCALL SetMetadata(CompilerDeclarationMetadata *);
    void RECON_THISCALL SetFunctionContext(CompilerFunctionContext *);
};
typedef char CompilerDeclarationState_size[sizeof(CompilerDeclarationState) == 28 ? 1 : -1];
struct DeclarationSourceState {
    int suppress_class_completion;
};
#ifndef RECON_NATIVE_TEST
extern DeclarationSourceState declaration_source_state;
extern CompilerType declaration_type_scratch;
extern int publish_browser_local;
#else
#define declaration_source_state (*(DeclarationSourceState *)(recon_unsupported("declaration: unknown class-scope owner"),0))
#define declaration_type_scratch (*(CompilerType *)(recon_unsupported("declaration: unknown type scratch owner"),0))
#define publish_browser_local (*(int *)(recon_unsupported("symbol publication: unmodeled browser-local owner"),0))
#endif
#define declaration_type_scratch_index frontend_bss.type_scratch_index

#endif
