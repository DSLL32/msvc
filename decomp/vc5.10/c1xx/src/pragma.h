#ifndef C1XX_PRAGMA_H
#define C1XX_PRAGMA_H
#include "frontend.h"
#include <recon.h>

/* The declaration emitter and pragma handlers share these actual three
 * segment-symbol cells. No native storage/default or broader branch claim. */
struct DeclarationReferenceDefaults {
    CompilerSymbol *fallback;     /* cc6c4: BSS segment */
    CompilerSymbol *type_context; /* cc6c8: CONST segment */
    CompilerSymbol *definition;   /* cc6cc: DATA segment */
};
#ifndef RECON_NATIVE_TEST
extern DeclarationReferenceDefaults declaration_reference_defaults;
#else
#define declaration_reference_defaults (*(DeclarationReferenceDefaults *)(recon_unsupported("symbol emission: unmodeled reference defaults"),0))
#endif
#endif
