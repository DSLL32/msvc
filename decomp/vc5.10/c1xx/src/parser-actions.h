/* Parser action interfaces. Unimplemented callees remain named entry traps.
 * App callees remain genuine named traps when integrated. Entry/RET facts are
 * in parser-callee-abi-analysis.json; opaque pointers are retained where the
 * current Run call establishes a pointer width but no pointee interpretation.
 * Reuse existing definitions for registered functions; do not duplicate them. */
#ifndef C1XX_PARSER_ACTIONS_H
#define C1XX_PARSER_ACTIONS_H
#include <stddef.h>
#include "parser-semantics.h"
#include <recon.h>

/* The deferred queue consumes only slot0 and next at +8. Concrete action
 * classes and their other fields remain behind their actual producers. */
struct DeferredAction {
    virtual void RECON_THISCALL Execute(void) = 0;
    BYTE unmodeled_04_07[4];
    DeferredAction *next;
};


struct ParserDeferredDefinition {
    CompilerIrSymbol *symbol;
    CompilerTokenStream *stream;
    ParserDeferredDefinition *next;
};
/* Borrowed input-end state. Only the actual DWORD at+8 is interpreted. */
struct ParserInputEndState {
    BYTE unmodeled_00_07[8];
    unsigned reached_end;
    void RECON_THISCALL Finish(void); /*347b2*/
};
struct ParserClassDefinitionLink {
    CompilerSymbol *symbol;
    ParserDeferredDefinition *definitions; /* actual+4, initializedNULL */
    ParserClassDefinitionLink *next;
};
struct CompilerSavedLookup {
    CompilerScopeTable *current;
    BYTE depth;
    BYTE unmodeled_05_07[3];
    CompilerScopeTable **scopes;
    CompilerSymbol *retired;
    BYTE lookup_context;
    BYTE unmodeled_11_13[3];
    int suppress_lookup_errors;
    DWORD opaque_lookup_state;
    int lookup_override;
    CompilerSavedLookup *previous;
    CompilerSavedLookup *RECON_THISCALL Initialize(CompilerTranslationUnit *, CompilerSavedLookup *, int); /*71f0 RET12*/
};
typedef char CompilerSavedLookup_layout[(sizeof(CompilerSavedLookup) == 36 &&
    offsetof(CompilerSavedLookup, previous) == 32 &&
    offsetof(CompilerTranslationUnit, opaque_lookup_state) == 0x228) ? 1 : -1];
struct ParserTokenView : SentinelToken {
    BYTE RECON_THISCALL Kind(void); /*fdb1, BYTE[ECX]*/
};
/* The declaration variant has an authentic40B Win32 value and a real44B
 * native value. Ordinary literal/pointer tokens remain48B/40B storage. */
struct ParserDeclarationToken {
    BYTE kind;
    BYTE unmodeled_01_07[7];
    ParserDeclarationView value;
};
#ifdef __linux__
typedef char ParserDeclarationToken_layout[(sizeof(ParserDeclarationToken) == 52 &&
    offsetof(ParserDeclarationToken, value) == 8) ? 1 : -1];
#else
typedef char ParserDeclarationToken_layout[(sizeof(ParserDeclarationToken) == 48 &&
    offsetof(ParserDeclarationToken, value) == 8) ? 1 : -1];
#endif

struct ParserFrontendContextView : FrontendContextStack {
    ParserTokenView *RECON_THISCALL ReadToken(void); /*8848*/
    ParserTokenView *RECON_THISCALL PeekToken(void); /*bf6e*/
    void RECON_THISCALL UnreadToken(ParserTokenView *); /*86389, RET4*/
    int RECON_THISCALL IsComplete(void); /*86382*/
    int RECON_THISCALL HasSourceMark(void); /*36344*/
    unsigned RECON_THISCALL Position(void); /*e008: source position+4*/
};
struct ParserSymbolReference {
    CompilerSymbol *RECON_THISCALL Reference(void); /*1d81a, folded DWORD0*/
    CompilerSymbol *RECON_THISCALL ResolvedSymbol(void); /*21427, folded DWORD+8; symbol consumer2a434*/
    CompilerExpression *RECON_THISCALL Context(void); /*e008, folded DWORD+4*/
};
struct ParserNameReference {
    CompilerNameEntry *RECON_THISCALL Identifier(void); /*1d81a, separate typed alias*/
};
struct ParserQualifiedName {
    CompilerSymbol *RECON_THISCALL ResolvedSymbol(void); /*21427; symbol consumer11bf9*/
    void *RECON_THISCALL Context(void); /*e008, +4*/
    CompilerExpression *RECON_THISCALL ContextValue(void); /*864f4, +c*/
};
struct ParserRecordedContext {
    ParserTokenView *RECON_THISCALL Read(BYTE, const TokenValue *); /*2b7db RET8, authentic40B*/
    ParserDeclarationView *RECON_THISCALL GetDeclaration(void); /*2b830, this+8*/
};
struct ParserSymbolArenaView {
    unsigned RECON_THISCALL Reference(void); /*1d81a; actual existing scalar owner BA910*/
};
/* Only these stores of the literal28-byte allocation are interpreted. This
 * owner is not the interner's parameter description or a keyword record. */
/* Source-only +14 read in guarded productions89/90. This partial view
 * establishes no allocation, default or wider parameter-scope layout. */
struct ParserParameterScopeView : CompilerScopeTable {
    CompilerSymbolLink **parameters;
};
struct ParserSavedParameters {
    CompilerSymbolLink **parameters; /*original+0, current scope buckets*/
    unsigned kind, has_parameters; /*original+10/+14; unknown intervening bytes excluded*/
};
struct ParserDeferredRecord {
    RECON_FORCEINLINE ParserDeferredRecord() {} /*no outlined construction vertex*/
    RECON_THISCALL ParserDeferredRecord(const ParserDeferredRecord *); /*8af2c*/
    ParserDeferredRecord *next, *previous; /*original+4/+8 after vptr*/
    BYTE flags; /*original+c, only encountered bits interpreted*/
    virtual void RECON_THISCALL Traverse(void); /*base C9394 ->b5cda, pending*/
};
struct ParserDeferredPointer : ParserDeferredRecord {
    void *payload; /*original+10*/
    RECON_FORCEINLINE ParserDeferredPointer() {} /*actual inline vptr store, no other stores*/
    RECON_FORCEINLINE ParserDeferredPointer(const ParserDeferredRecord *source)
        : ParserDeferredRecord(source) { payload = ((const ParserDeferredPointer *)source)->payload; }
    void RECON_THISCALL Traverse(void); /*C9360 ->b5c9f*/
    ParserDeferredPointer *RECON_THISCALL Initialize(void *, unsigned); /*864c5 RET8*/
};
struct ParserDeferredListItem : ParserDeferredRecord {
    CompilerExpression *payload; /*original+10*/
    RECON_FORCEINLINE ParserDeferredListItem() {}
    RECON_THISCALL ParserDeferredListItem(const ParserDeferredRecord *); /*8af62*/
    void RECON_THISCALL Traverse(void); /*C9324 ->b5cb7*/
    ParserDeferredListItem *RECON_THISCALL Initialize(CompilerExpression *, unsigned, unsigned); /*3637c RET12*/
};
struct ParserDeferredList {
    ParserDeferredRecord *head, *tail; /*original+0/+4*/
    BYTE flags; /*original+8, natural remaining bytes unmodeled*/
    ParserDeferredList *RECON_THISCALL Initialize(ParserDeferredRecord *); /*363bc RET4*/
    void RECON_THISCALL Prepend(ParserDeferredRecord *); /*4263f RET4*/
};
struct ParserLabelStack {
    ParserLabelStack *next;
    CompilerExpression *expression;
};
/* Wider tagged symbol view of the SAME owner. It is neither a new allocation
 * nor a replacement for small native namespace/parameter records. All reads
 * occur only after the explicit native semantic-domain guard. */
struct ParserClassDetail {
    WORD scope_number; /*original+2c*/
};
/* The same binding owners used by the declaration lookup and name reader. */
typedef CompilerSymbolBindingScope ParserBindingScope;
typedef CompilerSymbolBindingOwner ParserBindingOwner;
typedef CompilerSymbolBinding ParserBinding;
struct ParserSymbolView : CompilerNamedSymbol {
    union { CompilerType *type; void *payload; ParserBinding *binding; }; /*original+10*/
    unsigned emission_state, encoded_index; /*original+14/+18*/
    unsigned declaration_state; /*original+1c*/
    ParserClassDetail *class_detail; /*original+20, kind4 only*/
    short type_index; /*original+14, kind0f only; source-only alternate role*/
};
struct ParserBitfieldSymbol : CompilerIrSymbol { unsigned width; /*original+24*/ };
struct ParserSourceFrame { BYTE source_flags; /*actual source-context role, offset not modeled*/ };

/* The declaration dispatcher explicitly clears this entire72-byte local.
 * Only fields addressed by it or its real pending reader are named. Opaque
 * storage retains that proved zero writer, without semantic interpretations. */
struct ParserDeclarationReader {
    ParserTokenView *token;              /* +0 */
    CompilerType *resolved_type;          /* +4, FinalizeType writes it */
    unsigned tokens_read;                /* +8, incremented by Read */
    ParserTokenView *declaration_token;  /* +c, body reader rewrites token tag */
    BYTE opaque_10_13[4];
    CompilerDeclarationContext context; /* +14, existing28-byte type */
    unsigned declaration_marked;         /* +30, reader sets1 */
    CompilerType *type;                  /* +34, type-name and finalizer writers */
    CompilerSymbol *type_symbol;         /* +38, name/class-head writers */
    unsigned copied_class_head;          /* +3c, class-head writer stores1 */
    ParserDeclarationCallback declaration_callback; /* original+40/native+40 */
    WORD type_flags;                     /* +44, tag reader clears on conflict */
    WORD qualifier_flags;                /* +46, only a zero predicate here */
    ParserTokenView *RECON_THISCALL Read(void); /* d00d */
    int RECON_THISCALL ReadIdentifier(void); /*d2cf*/
    void RECON_THISCALL ReadTypeName(void); /*d38f*/
    void RECON_THISCALL ReadStorageClass(void); /*1f148*/
    void RECON_THISCALL ReadStorageModifier(void); /*28ce3*/
    int RECON_THISCALL ReadTagSpecifier(void); /*2489c*/
    int RECON_THISCALL ReadQualifier(void); /*244d0*/
    void RECON_THISCALL ReadTypeSpecifier(void); /*d555*/
    void RECON_THISCALL CopyClassHead(void); /*2b7f8*/
    void RECON_THISCALL FinalizeType(void); /*d5b4*/
    ParserTokenView *RECON_THISCALL MakeToken(unsigned); /*d416, RET4*/

    void RECON_THISCALL SaveSemantic(void); /*8059a*/
};
#ifdef __linux__
typedef char ParserDeclarationReader_extent[(sizeof(ParserDeclarationReader) == 76 &&
    offsetof(ParserDeclarationReader, declaration_callback) == 0x40 &&
    offsetof(ParserDeclarationReader, type_flags) == 0x48 &&
    offsetof(ParserDeclarationReader, qualifier_flags) == 0x4a) ? 1 : -1];
#else
typedef char ParserDeclarationReader_extent[(sizeof(ParserDeclarationReader) == 72 &&
    offsetof(ParserDeclarationReader, declaration_callback) == 0x40 &&
    offsetof(ParserDeclarationReader, type_flags) == 0x44 &&
    offsetof(ParserDeclarationReader, qualifier_flags) == 0x46) ? 1 : -1];
#endif

/* Genuine56-byte tag-parser allocation. Its constructor writes the listed
 * fields and28-byte context; the last three bytes are untouched opaque tail. */
struct ParserTagDeclaration {
    CompilerNameEntry *name;         /* +0 */
    CompilerSymbol *symbol;          /* +4 */
    ParserTokenView *name_token;     /* +8 */
    WORD tag_flags, qualifier_flags; /* +c/+e */
    CompilerDeclarationContext context; /* +10..2b */
    CompilerDeclarationContext *parent_context; /* +2c, borrowed */
    ParserDeclarationCallback callback; /* original+30; native member pointer is8B */
    BYTE initial_state;              /* +34,1 then0 on unrecognized input */
    BYTE unmodeled_35_37[3];
    int RECON_THISCALL ReadAttributes(void); /*4b680*/
    int RECON_THISCALL ReadQualifiedName(void); /*4e614*/
    int RECON_THISCALL ReadScopedName(ParserTokenView *); /*4e69f*/
    int RECON_THISCALL ReadName(ParserTokenView *); /*24d82*/
    ParserTokenView *RECON_THISCALL FinishTag(ParserTokenView *); /*24e7b*/
    ParserTokenView *RECON_THISCALL FinishDeclaration(ParserTokenView *); /*37187*/
};
#ifndef RECON_NATIVE_TEST
typedef char ParserTagDeclaration_extent[(sizeof(ParserTagDeclaration) == 56 &&
    offsetof(ParserTagDeclaration, callback) == 0x30 &&
    offsetof(ParserTagDeclaration, initial_state) == 0x34) ? 1 : -1];
#else
typedef char ParserTagDeclaration_extent[(sizeof(ParserTagDeclaration) == 60 &&
    offsetof(ParserTagDeclaration, callback) == 0x30 &&
    offsetof(ParserTagDeclaration, initial_state) == 0x38) ? 1 : -1];
#endif



extern "C" {
/* Current parser control and recovery. */
void __cdecl emit_context_transition(unsigned); /*6a19*/
ParserTokenView *__cdecl process_context_token(ParserTokenView *); /*1fe6d*/
int __cdecl has_saved_source_state(void); /*80660*/
int __cdecl restore_source_input(void); /*205cf*/
int __cdecl has_pending_parser_input(void); /*db78*/
void __cdecl finish_token_input(void); /*35351*/
ParserTokenView *__cdecl read_qualified_parser_token(ParserTokenView *, unsigned); /*c2ad*/
ParserTokenView *__cdecl resolve_identifier_type(ParserTokenView *); /*bf88*/
ParserTokenView *__cdecl process_parser_declaration(void); /*31da6*/
ParserTokenView *__cdecl classify_parser_declaration(unsigned); /*299fd*/
CompilerType *__cdecl make_parser_type_scratch(CompilerType *); /*79a0*/
ParserTokenView *__cdecl get_next_token(void); /*b95c*/
void __fastcall copy_token_value(ParserSemanticView *, ParserTokenView *); /*bf5d ECX/EDX*/
void __cdecl report_parser_error(const char *); /*85ff7*/
ParserTokenView *__cdecl canonical_parser_token(unsigned); /*86371*/
ParserTokenView *__cdecl defer_parser_token(ParserTokenView *); /*2b78a*/
ParserTokenView *__cdecl recover_parser_construct(ParserTokenView *, ...); /*86173*/
void __cdecl report_expected_parser_token(ParserTokenView *, unsigned); /*85e68*/
void __cdecl compiler_warning(int, ...); /*918e0*/
void __cdecl report_language_diagnostic(unsigned, unsigned, ...); /*2088a*/
/* report_compiler_error91a45 and report_compiler_diagnostic91a95 use
 * the exact existing frontend.h declarations. Run ignores their returns. */

void __cdecl remember_parser_context(void *); /*3342d*/
int __cdecl current_recovery_shifts(void); /*db42*/
void __cdecl select_parser_arena(unsigned); /*7363*/

/* Declaration and scope actions. Opaque payloads remain borrowed. */
CompilerSymbol *__cdecl lookup_builtin(CompilerNameEntry *); /*968d existing*/
CompilerDeclaration *__cdecl current_declaration(void); /*d7ce*/
CompilerDeclaration *__cdecl default_declaration(unsigned); /*21a97*/
CompilerSymbol *__cdecl declare_builtin(CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *); /*a1f5 existing*/
CompilerSymbol *__cdecl merge_symbol_declaration(CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *); /*a4eb existing*/
CompilerSymbol *__cdecl declare_symbol(CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *); /*19920*/
CompilerSymbol *__cdecl declare_parameter_symbol(CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *); /*1d78f*/
int __cdecl declare_initialized_symbol(CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *, CompilerSymbol **); /*25ba1*/
void __cdecl finalize_symbol_declaration(CompilerDeclarator *, CompilerSymbol *); /*97ce existing*/
void __cdecl process_merged_declaration(CompilerSymbol *, unsigned); /*28e4d*/
void __cdecl process_declaration_specifiers(CompilerDeclaration *); /*d8c2*/
void __cdecl finish_declaration_specifiers(CompilerDeclaration *); /*28d18*/
void __cdecl process_parser_directive(void *); /*320b7*/
void __cdecl finish_external_declaration(void); /*d9d1*/
void __cdecl process_end_of_input(void); /*4eb57*/
void __cdecl process_deferred_parser_input(void); /*868db*/
void __cdecl process_pch_boundary(void); /*3cf00*/
void __cdecl process_external_recovery(unsigned); /*4ddd2*/
void __cdecl process_external_initializer(void); /*4141*/
void __cdecl finish_external_initializer(void); /*5962*/
void __cdecl begin_function_definition(void *); /*359b7*/
void __cdecl set_function_parameters(CompilerParameterDescription *); /*35e45*/
void *__cdecl begin_function_body(CompilerSymbol *); /*22308*/
void __cdecl pop_declaration_context(void); /*42072*/
void __cdecl process_declared_type(void *, CompilerDeclaration *); /*4d66e*/
void __cdecl process_function_type(void *, void *); /*4e35c*/
void __cdecl process_member_declaration(unsigned, CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *); /*b54b5*/
void __cdecl discard_member_declaration(unsigned, ParserDeclarationValue *); /*86557*/
void __cdecl begin_type_declaration(void); /*4d103*/
void *__cdecl finish_type_declaration(CompilerParameterSet *); /*4d439*/
void __cdecl begin_declaration_scope(void); /*36034*/
void __cdecl prepare_declaration_lookup(void); /*2200e*/
void __cdecl finish_deferred_declaration(void); /*4461f*/
ParserDeferredRecord *__cdecl make_deferred_expression(CompilerExpression *); /*3386e*/
ParserDeferredRecord *__cdecl make_deferred_declaration(ParserDeclarationView, ParserDeclarationValue *); /*33385 authentic40B byvalue*/
int __cdecl parse_linkage_specifier(CompilerDeclaration *, CompilerExpression *); /*41ec5*/
void __cdecl push_declaration_context(int); /*2b834 existing*/
void __cdecl finish_namespace_declaration(CompilerSymbol *); /*ae251*/
CompilerSymbol *__cdecl begin_namespace_declaration(CompilerNameEntry *); /*adf2e*/
void __cdecl process_using_declaration(CompilerSymbol *, unsigned, unsigned); /*ae288*/
void __cdecl process_using_directive(CompilerSymbol *); /*ae4a1*/
void __cdecl declare_namespace_alias(CompilerNameEntry *, CompilerSymbol *); /*af040*/
void *__cdecl allocate_parser_storage(unsigned, unsigned); /*2b7ac*/
ParserRecordedContext *__cdecl parse_recorded_declaration(ParserTokenView *); /*cd8f*/
CompilerType *__cdecl current_enumerator_type(void); /*3715b*/
CompilerType *__cdecl finish_class_specifier(void); /*26828*/
void __cdecl finish_class_scope(void); /*28852*/
void __cdecl restore_class_scope(void); /*28895*/
CompilerSymbol *__cdecl finish_class_declaration(CompilerSymbol *, CompilerType *); /*256c1*/
void __cdecl finish_class_definition(void); /*259dd*/
void __cdecl check_completed_class(CompilerSymbol *); /*466b7*/
void __cdecl add_base_class_name(unsigned, int, CompilerNameEntry *); /*2a404*/
void __cdecl add_base_class_symbol(unsigned, int, CompilerSymbol *); /*2a434*/
void __cdecl add_qualified_base_class(unsigned, int, CompilerSymbol *); /*88caf*/
__int64 __cdecl next_enumerator_value(__int64, CompilerNameEntry *); /*44b00*/
void __cdecl add_enumerator(CompilerNameEntry *, CompilerType *, __int64); /*36f50*/
void __cdecl set_member_access(BYTE); /*28e2b*/
void __cdecl check_qualified_declaration(CompilerSymbol *); /*1458f existing*/
int __cdecl is_current_declaration_class(CompilerSymbol *); /*d677 existing*/
CompilerSymbol *__cdecl make_abstract_declarator(void); /*692c existing*/
unsigned __cdecl prepare_bitfield(CompilerSymbol *, __int64); /*45170*/
void __cdecl process_using_member(CompilerSymbol *); /*aeb4a*/
int __cdecl is_special_member_name(CompilerNameEntry *); /*25699*/
int __cdecl ignore_initialized_declaration(CompilerSymbol *); /*44569*/
CompilerSymbol *__cdecl prepare_symbol_initializer(CompilerSymbol *); /*1bb94*/
void __cdecl begin_symbol_initializer(CompilerSymbol *); /*17d9b*/
void __cdecl initialize_symbol(CompilerSymbol *, CompilerExpression *); /*1bdf1*/
void __cdecl finish_symbol_initializer(CompilerSymbol *); /*1bcb2*/
void __cdecl begin_initializer_list(CompilerSymbol *, unsigned); /*2e127*/
void __cdecl finish_initializer_list(CompilerSymbol *); /*2df90*/
void __cdecl process_initializer_expression(CompilerExpression *, unsigned); /*2d904*/
void __cdecl begin_initializer_element(void); /*40654*/
void __cdecl finish_initializer_element(void); /*2e089*/
BYTE *__cdecl allocate_declarator(unsigned); /*d70f*/
CompilerNameEntry *__cdecl current_class_name(void); /*25b87*/
void __cdecl prepare_qualified_scope(ParserBindingOwner *); /*20e41*/
CompilerSymbol *__cdecl select_function_candidate(CompilerSymbol *); /*31d08*/
CompilerType *__cdecl get_function_return_type(CompilerType *); /*14d0 existing*/
CompilerType *__cdecl intern_type(CompilerType *); /*1000 existing*/
CompilerDeclarationContext *__cdecl get_symbol_declaration_context(CompilerDeclarationContext *, CompilerSymbol *); /*863ab*/
CompilerSymbol *__cdecl note_qualified_scope(short); /*8639b, actual result ignored by Run*/
void __cdecl apply_conversion_type(CompilerSymbol *, CompilerType *); /*4590b*/
CompilerNameEntry *__cdecl make_conversion_name(CompilerType *); /*459b1*/
CompilerNameEntry *__cdecl make_anonymous_conversion_name(void); /*36d2a*/
void __cdecl initialize_type_specifier(CompilerDeclaration *, CompilerType *); /*4594b*/
CompilerParameterDescription *__cdecl make_empty_function_parameters(void); /*1e271*/
CompilerSymbol *__cdecl allocate_parser_symbol(unsigned, CompilerNameEntry *, unsigned); /*46520, NOT existing make_parameter_symbol219a2*/
BYTE *__cdecl allocate_parameter_description(unsigned); /*fdb4*/
void __cdecl begin_deferred_input(unsigned); /*45de6*/
BYTE *__cdecl allocate_parser_record(unsigned, unsigned); /*36350*/
void __cdecl finish_deferred_input(void); /*363e4*/
void __cdecl set_symbol_declaration_context(CompilerSymbol *, CompilerDeclarationContext); /*374c1 authentic28B byvalue*/
unsigned __cdecl allocate_symbol_record_id(void); /*37497*/
unsigned __cdecl extract_type_qualifier(void *); /*86609*/
void *__cdecl process_type_qualifier(void); /*2b90d*/

/* Expression construction and conversions. */
CompilerExpression *__cdecl append_parser_expression(CompilerExpression *, CompilerExpression *); /*3220e*/
CompilerExpression *__cdecl make_binary_node(unsigned, CompilerExpression *, CompilerExpression *, CompilerType *); /*65c0*/
CompilerExpression *__cdecl symbol_expression(CompilerSymbol *, unsigned); /*47e6*/
CompilerExpression *__cdecl make_resolved_symbol_expression(CompilerSymbol *, unsigned); /*11c18*/
CompilerType *__cdecl reference_expression_type(CompilerExpression *); /*3d3d1*/
CompilerExpression *__cdecl append_expression_list(CompilerExpression *, CompilerExpression *, unsigned); /*12057*/
CompilerExpression *__cdecl resolve_parser_expression(CompilerExpression *); /*79f0*/
void __cdecl require_integral_constant(CompilerExpression *); /*45e2f, Run ignores EAX*/
CompilerExpression *__cdecl expand_initializer_expression(unsigned, CompilerExpression *, unsigned); /*9fe00*/
CompilerExpression *__cdecl instantiate_parser_expression(CompilerExpression *, unsigned); /*1a2d8*/
void __cdecl check_initializer_type(CompilerExpression *, CompilerType *); /*21b2e*/
CompilerExpression *__cdecl make_typed_expression(unsigned, CompilerType *, CompilerExpression *, unsigned); /*11ee0*/
CompilerExpression *__cdecl convert_typed_expression(CompilerType *, CompilerExpression *); /*12ca7*/
CompilerExpression *__cdecl mark_parser_expression(CompilerExpression *); /*21b51*/
CompilerExpression *__cdecl convert_parser_rvalue(CompilerExpression *); /*7bb6*/
CompilerExpression *__cdecl check_statement_expression(CompilerExpression *, unsigned); /*181d2*/
void __cdecl emit_statement_expression(CompilerExpression *); /*183d9*/
CompilerExpression *__cdecl convert_condition(CompilerExpression *, unsigned); /*1c2fa*/
void __cdecl emit_conditional_branch(CompilerExpression *, CompilerExpression *, unsigned); /*1979e*/
__int64 __cdecl constant_integer_value(CompilerExpression *); /*23433*/
CompilerExpression *__cdecl make_binary_expression(unsigned, CompilerExpression *, CompilerExpression *); /*122ba*/
CompilerExpression *__cdecl make_parser_extension_expression(void); /*ada58*/
CompilerExpression *__cdecl make_parser_extension_operand(CompilerExpression *); /*adaa7*/
CompilerExpression *__cdecl convert_assignment_operand(CompilerType *, CompilerExpression *); /*15a3f*/
void __cdecl diagnose_parser_operand(unsigned, CompilerExpression *); /*8afa4, Run ignores result*/
CompilerExpression *__cdecl make_conditional_expression(CompilerExpression *, CompilerExpression *, CompilerExpression *); /*2ccaa*/
CompilerExpression *__cdecl make_parser_comparison(CompilerExpression *, CompilerExpression *); /*8e7b6*/
CompilerExpression *__cdecl make_call_expression(unsigned, CompilerExpression *, CompilerExpression *); /*15805*/
void __cdecl check_call_expression(CompilerExpression *); /*15702*/
void *__cdecl make_integer_constant(int); /*3fc2 existing*/
CompilerExpression *__cdecl make_unary_expression(unsigned, CompilerExpression *); /*15738*/
CompilerExpression *__cdecl make_member_pointer_expression(CompilerExpression *); /*404ef*/
CompilerExpression *__cdecl make_sizeof_expression(CompilerExpression *); /*444ee*/
CompilerExpression *__cdecl make_sizeof_type(CompilerType *); /*2c3c5*/
CompilerExpression *__cdecl recover_new_expression(void); /*8e143*/
CompilerExpression *__cdecl make_subscript_expression(CompilerExpression *, CompilerExpression *); /*16611*/
CompilerExpression *__cdecl make_unary_parser_extension(unsigned, CompilerExpression *); /*a9ac9*/
CompilerExpression *__cdecl prepare_call_argument_types(CompilerDeclaration *, CompilerDeclaration *); /*12ba6*/
CompilerExpression *__cdecl make_cpp_cast_expression(CompilerExpression *); /*8b4ce*/
CompilerExpression *__cdecl make_cpp_type_expression(CompilerDeclaration *, CompilerDeclaration *); /*8b624*/
CompilerExpression *__cdecl make_cpp_query_expression(CompilerExpression *); /*a02f6*/
CompilerExpression *__cdecl make_cpp_query_type(CompilerDeclaration *, CompilerDeclaration *); /*a0716*/
CompilerExpression *__cdecl make_typed_constant(CompilerType *, const void *, unsigned); /*78e0*/
CompilerExpression *__cdecl make_cast_expression(unsigned, CompilerExpression *); /*10239*/
CompilerExpression *__cdecl make_allocation_expression(unsigned, void *, void *); /*1ad7f*/
CompilerExpression *__cdecl make_array_allocation_expression(unsigned, void *, void *); /*89279*/
CompilerExpression *__cdecl make_qualified_allocation_expression(unsigned, void *, CompilerDeclaration *, CompilerDeclaration *); /*8922b*/
CompilerExpression *__cdecl lookup_name_expression(CompilerNameEntry *); /*11b98*/
CompilerExpression *__cdecl make_symbol_expression(CompilerSymbol *); /*11bf9, target11c18 reads the symbol kind*/
void __cdecl append_parser_string(void *, ParserSemanticView *); /*4fce3*/
void __cdecl prepare_allocation_type(void); /*433ec*/
CompilerExpression *__cdecl make_new_expression(unsigned, void *, CompilerSymbol *, CompilerExpression *, CompilerType *); /*4342d*/
int __cdecl invalid_allocation_type(CompilerSymbol *); /*433c8*/
CompilerExpression *__cdecl make_delete_expression(unsigned, CompilerExpression *, CompilerExpression *, unsigned); /*4546f*/

/* Statements, exception extensions and labels. */
void __cdecl begin_compound_statement(void); /*5052*/
void __cdecl finish_compound_statement(void); /*5558*/
void __cdecl emit_statement_location(void); /*5236*/
void __cdecl finish_conditional_scope(CompilerExpression *, unsigned); /*1c4f9*/
void __cdecl emit_label(CompilerExpression *); /*5c6d*/
void __cdecl begin_else_clause(unsigned); /*1c52f*/
void __cdecl finish_if_statement(void); /*1d09d*/
void __cdecl finish_condition_expression(void); /*43804*/
void __cdecl process_condition_lookahead(ParserTokenView *); /*3f8ab*/
void __cdecl prepare_loop_condition(void); /*43fba*/
void __cdecl begin_for_statement(void); /*46909*/
void __cdecl begin_for_condition(void); /*46981*/
unsigned __cdecl begin_statement_scope(CompilerExpression *); /*1c255*/
void __cdecl finish_for_condition(void); /*469c0*/
void __cdecl finish_for_statement(void); /*46a50*/
void __cdecl write_ir_record(int, const void *); /*3296 existing*/
void __cdecl finish_switch_expression(void); /*3f4f4*/
void __cdecl finish_switch_scope(void); /*3f592*/
void __cdecl prepare_case_statement(void); /*8c9a8*/
void __cdecl prepare_break_statement(void); /*3f72d*/
void __cdecl prepare_continue_statement(void); /*4e723*/
void __cdecl prepare_return_statement(void); /*165db*/
void __cdecl emit_return_statement(CompilerExpression *, unsigned); /*5241*/
CompilerSymbol *__cdecl find_label_symbol(CompilerNameEntry *); /*46b7d*/
void __cdecl note_label_reference(CompilerSymbol *, unsigned); /*46cde*/
CompilerSymbol *__cdecl make_label_symbol(CompilerNameEntry *); /*46bd1*/
void __cdecl emit_label_reference(CompilerSymbol *); /*477e*/
void __cdecl process_empty_statement(void); /*496a0*/
void __cdecl process_try_statement(void); /*8cb02*/
void __cdecl process_except_statement(void); /*8ca23*/
void __cdecl process_finally_statement(void); /*8cb22*/
void __cdecl begin_parser_extension(void); /*ad872*/
void __cdecl process_parser_extension(void); /*ad928*/
void __cdecl finish_parser_extension(void); /*ad950*/
void __cdecl set_parser_extension_type(CompilerType *); /*ad838*/
void *__cdecl make_parser_extension_parameters(CompilerParameterDescription *); /*ad688*/
void __cdecl release_parser_parameter_description(CompilerParameterDescription *); /*1300 approved pending cross-formal alias*/
void __cdecl begin_exception_handler(void); /*8ca3e*/
void __cdecl begin_finally_handler(void); /*8cabc*/
void __cdecl finish_exception_handler(void); /*8cb42*/
void __cdecl set_exception_filter(CompilerExpression *); /*8c9ce*/
void __cdecl finish_exception_filter(void); /*8cb4d*/
void __cdecl process_label_name(CompilerNameEntry *); /*46b1d*/
void __cdecl add_switch_case(CompilerExpression *); /*3f17c*/
CompilerExpression *__cdecl make_internal_label(unsigned); /*46f3*/
void __cdecl finish_default_label(void); /*4455e*/
void __cdecl begin_conditional_statement(void); /*1c274*/
void __cdecl finish_condition_statement(void); /*1c464*/
void __cdecl finish_then_clause(void); /*1d025*/
void __cdecl begin_loop_body(void); /*43e64*/
void __cdecl finish_loop_test(void); /*43edc*/
void __cdecl finish_loop_statement(unsigned, void *, CompilerExpression *); /*3f7dd*/
void __cdecl finish_loop_scope(void); /*43f5e*/
void __cdecl begin_do_statement(void); /*4370f*/
void __cdecl finish_do_statement(void); /*43787*/
void __cdecl begin_switch_statement(void); /*3ee23*/
void __cdecl set_switch_expression(CompilerExpression *); /*3eebd*/
}
#endif
