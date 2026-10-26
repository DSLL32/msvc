#ifndef C1_DECLARATOR_H
#define C1_DECLARATOR_H
#include "frontend-state.h"
#include <recon.h>

/* The encoded source-position pair is copied as two DWORDs. Its component
 * encodings have not yet been interpreted by a reconstructed consumer. */
struct CompilerParameterDescription {
    unsigned value, kind;
    unsigned scope_active; /* Run stores literal1; Normalize tests only truth. */
    void RECON_THISCALL Normalize(void);
    void RECON_THISCALL AppendParameter(CompilerIrSymbol *);
};
struct CompilerDeclarator {
    CompilerType *base_type;
    CompilerTypeShape *shape;
    CompilerTypeShape *last_shape;
    CompilerDeclarationAttributes attributes;
    unsigned shape_flags;
    CompilerParameterDescription *parameters;
    CompilerSourcePosition position;
    CompilerIrSymbol *symbol;
    CompilerType *resolved_type;
    unsigned flags;
    CompilerDeclarator *RECON_THISCALL Initialize(CompilerType *, const CompilerSourcePosition *);
    CompilerDeclarator *RECON_THISCALL InitializeSymbol(CompilerIrSymbol *);
    void RECON_THISCALL ApplyDeclaration(const CompilerParserDeclaration *);
    void RECON_THISCALL AppendArrayBound(const unsigned *);
    void RECON_THISCALL AddFunction(CompilerParameterDescription *);
    void RECON_THISCALL AppendShape(CompilerTypeShape *);
    void RECON_THISCALL MergeQualifiers(const CompilerDeclarationAttributes *,
        const CompilerDeclarationAttributes *);
    void RECON_THISCALL ApplyAttributes(const CompilerDeclarationAttributes *);
    CompilerType *RECON_THISCALL Resolve(void);
    void RECON_THISCALL FinalizeShape(void);
    void RECON_THISCALL AppendArray(CompilerTypeShape *);
    void RECON_THISCALL ApplyBaseAttributes(void);
    WORD RECON_THISCALL ApplyCallingConvention(CompilerTypeShape *, unsigned);
    WORD RECON_THISCALL ResolveFunctionAttributes(CompilerTypeShape *);
    CompilerType *RECON_THISCALL BuildType(CompilerTypeShape *);
};
typedef char CompilerDeclarator_size[sizeof(CompilerDeclarator) == 48 ? 1 : -1];
#endif
