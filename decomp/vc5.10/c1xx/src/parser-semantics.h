#ifndef C1XX_PARSER_SEMANTICS_H
#define C1XX_PARSER_SEMANTICS_H
/* Semantic views of the original parser's distinct tagged values.
 * The original slot is40 bytes. The native LR slot accommodates the actual
 * declaration member pointer in44 bytes, while ordinary literal/token
 * storage remains40 bytes. Wider semantic actions retain their guards. */
#include "frontend-init.h"
#include "global-output.h"
#include <recon.h>

struct ParserSymbolView;
struct ParserDeferredRecord;
struct ParserDeferredList;
struct ParserSavedParameters;
struct ParserSymbolReference;
struct ParserQualifiedName;
struct ParserTypeModifier;

/* An actual callback is called with ECX=the translation unit and one typed
 * pointer on the stack, followed by callee cleanup. The original completed
 * single-inheritance TU member-pointer representation is4 bytes. The native
 * member-pointer representation is not asserted equal or copied as raw4. */
typedef void (RECON_THISCALL CompilerTranslationUnit::*ParserDeclarationCallback)(CompilerSymbol *);
struct ParserDeclarationView : CompilerDeclaration {
    CompilerSymbol *object;             /* original+20 */
    ParserDeclarationCallback callback; /* original+24 */
};
#ifndef RECON_NATIVE_TEST
typedef char ParserDeclarationView_original_extent[sizeof(ParserDeclarationView) == 40 ? 1 : -1];
#endif

/* Existing qualifiers share the WORD/DWORD roles. The modifier constructors
 * establish the consumed WORDs individually; no default for the natural
 * WORD gap at+6 is invented. */
struct ParserTypeModifier {
    WORD kind, flags, qualifiers; /* original+0/+2/+4 */
    unsigned value;             /* +8 */
    void RECON_THISCALL Pointer(CompilerDeclaratorQualifiers *, CompilerDeclaratorQualifiers *); /*d590*/
    void RECON_THISCALL Reference(CompilerDeclaratorQualifiers *, CompilerDeclaratorQualifiers *); /*3d241*/
    void RECON_THISCALL MemberPointer(void *, CompilerDeclaratorQualifiers *, CompilerDeclaratorQualifiers *); /*40a73*/
};
struct ParserDeclarationValue {
    CompilerDeclarator *declarator; /* original+0 */
    CompilerSymbol *symbol;         /* original+4 */
};
struct ParserNamedToken {
    BYTE kind;
    CompilerNameEntry *name; /* original+4; intervening alignment not modeled */
};
struct ParserAccess {
    BYTE kind;
    unsigned flags; /* original+4; only actually tested bits have meaning */
};
union ParserIndirectName { CompilerNameEntry *name; CompilerSymbol *symbol; };
struct ParserPointerPair { void *first, *second; };
typedef CompilerNumericLiteral ParserLiteralView;
/* Saved parser-value arm selected by recorded token kind0x65. Actual
 * stores are type at+0, kind at+8, value at+c. This is not the declaration
 * arm's type at+1c. The intervening original DWORD has no field/default
 * meaning. Native never reads or transfers this arm before qualification. */
struct ParserRecordedTypeValue {
    CompilerType *type;
#ifndef RECON_NATIVE_TEST
    BYTE original_unmodeled_04[4];
#endif
    unsigned kind, value;
};
/* Source boundary markers carry the existing origin/position pair only. */
struct ParserSourcePosition { void *origin; unsigned position; };
union ParserSemanticView {
    ParserSourcePosition source;
    TokenValue known;
    void *pointer;
    CompilerType *type;
    CompilerNameEntry *name;
    CompilerSymbol *symbol;
    CompilerExpression *expression;
    CompilerParameterDescription *parameter_description;
    ParserSymbolReference *reference;
    ParserIndirectName *indirect_name;
    ParserQualifiedName *qualified_name;
    ParserDeferredRecord *deferred;
    ParserDeferredList *deferred_list;
    ParserSavedParameters *saved_parameters;
    ParserDeclarationValue declaration_value;
    ParserDeclarationView declaration;
    CompilerDeclaratorQualifiers qualifiers;
    ParserTypeModifier modifier;
    ParserNamedToken named_token;
    ParserAccess access;
    ParserLiteralView literal;
    CompilerQuotedValue quoted;
    ParserRecordedTypeValue recorded_type;
    ParserPointerPair pair;
    __int64 integer;
    unsigned unsigned_value, scope_open;
    WORD word_value;
    BYTE token_kind;
};
typedef char ParserSemanticView_saved_capacity[sizeof(ParserSemanticView) <= sizeof(CompilerSavedTokenValue) ? 1 : -1];
#ifndef RECON_NATIVE_TEST
typedef char ParserSemanticView_original_extent[sizeof(ParserSemanticView) == 40 ? 1 : -1];
#endif
/* The complete compatible literal arm preserves value at original/native+8;
 * the separate declaration/member-pointer transfer restrictions still apply. */
#define PARSER_LITERAL_DATA(v) ((const void *)&(v).literal.value)

#endif
