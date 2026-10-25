#ifndef C1_DECLARATOR_H
#define C1_DECLARATOR_H
#include "frontend-state.h"
#include <recon.h>

/* The encoded source-position pair is copied as two DWORDs. Its component
 * encodings have not yet been interpreted by a reconstructed consumer. */
struct CompilerParameterDescription {
    unsigned value, kind;
    struct CompilerSymbolTable *scope;
    void RECON_THISCALL Normalize(void);
};
struct CompilerDeclarationAttributes { unsigned flags, extra; };
struct CompilerDeclarator {
    CompilerType *base_type;
    CompilerTypeShape *shape;
    CompilerTypeShape *last_shape;
    CompilerDeclarationAttributes attributes;
    BYTE unmodeled_14[4];
    CompilerParameterDescription *parameters;
    CompilerSourcePosition position;
    CompilerIrSymbol *symbol;
    CompilerType *resolved_type;
    unsigned flags;
    CompilerDeclarator *RECON_THISCALL Initialize(CompilerType *, const CompilerSourcePosition *);
    void RECON_THISCALL AddFunction(CompilerParameterDescription *);
    void RECON_THISCALL AppendShape(CompilerTypeShape *);
    void RECON_THISCALL ApplyAttributes(const CompilerDeclarationAttributes *);
    CompilerType *RECON_THISCALL Resolve(void);
};
typedef char CompilerDeclarator_size[sizeof(CompilerDeclarator) == 48 ? 1 : -1];
#endif
