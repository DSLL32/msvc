#include "parser.h"
#include "parser-actions.h"
#include "lexer.h"
#include <recon.h>

/* Genuine pending application entries. No code-body or extent credit. */












extern "C" CompilerExpression * __cdecl prepare_call_argument_types(CompilerDeclaration *, CompilerDeclaration *) { __asm { int 3 } return (CompilerExpression *)0; }

extern "C" void __cdecl check_call_expression(CompilerExpression *) { __asm { int 3 } }
extern "C" CompilerExpression * __cdecl make_call_expression(unsigned, CompilerExpression *, CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }





extern "C" void __cdecl emit_conditional_branch(CompilerExpression *, CompilerExpression *, unsigned) { __asm { int 3 } }
extern "C" CompilerExpression * __cdecl instantiate_parser_expression(CompilerExpression *, unsigned) { __asm { int 3 } return (CompilerExpression *)0; }


extern "C" unsigned __cdecl begin_statement_scope(CompilerExpression *) { __asm { int 3 } return (unsigned)0; }
extern "C" void __cdecl begin_conditional_statement(void) { __asm { int 3 } }
extern "C" CompilerExpression * __cdecl convert_condition(CompilerExpression *, unsigned) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" void __cdecl finish_condition_statement(void) { __asm { int 3 } }
extern "C" void __cdecl finish_conditional_scope(CompilerExpression *, unsigned) { __asm { int 3 } }
extern "C" void __cdecl begin_else_clause(unsigned) { __asm { int 3 } }
extern "C" void __cdecl finish_then_clause(void) { __asm { int 3 } }
extern "C" void __cdecl finish_if_statement(void) { __asm { int 3 } }
CompilerSymbol *RECON_THISCALL CompilerScopeMatch::Reference(void) { return symbol; }

void RECON_THISCALL CompilerSymbol::SetAlias(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl prepare_qualified_scope(ParserBindingOwner *) { __asm { int 3 } }
void RECON_THISCALL CompilerSymbol::SetDeclarationQualifier(WORD) { __asm { int 3 } }



extern "C" void __cdecl set_member_access(BYTE) { __asm { int 3 } }
extern "C" void __cdecl add_base_class_name(unsigned, int, CompilerNameEntry *) { __asm { int 3 } }
extern "C" void __cdecl add_base_class_symbol(unsigned, int, CompilerSymbol *) { __asm { int 3 } }
extern "C" void * __cdecl process_type_qualifier(void) { __asm { int 3 } return (void *)0; }
extern "C" CompilerExpression * __cdecl make_sizeof_type(CompilerType *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression * __cdecl make_conditional_expression(CompilerExpression *, CompilerExpression *, CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" void __cdecl process_initializer_expression(CompilerExpression *, unsigned) { __asm { int 3 } }
extern "C" void __cdecl finish_initializer_list(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl finish_initializer_element(void) { __asm { int 3 } }
extern "C" void __cdecl begin_initializer_list(CompilerSymbol *, unsigned) { __asm { int 3 } }
extern "C" CompilerSymbol * __cdecl select_function_candidate(CompilerSymbol *) { __asm { int 3 } return (CompilerSymbol *)0; }
extern "C" void __cdecl process_parser_directive(void *) { __asm { int 3 } }
extern "C" ParserDeferredRecord * __cdecl make_deferred_declaration(ParserDeclarationView, ParserDeclarationValue *) { __asm { int 3 } return (ParserDeferredRecord *)0; }
extern "C" void __cdecl remember_parser_context(void *) { __asm { int 3 } }
extern "C" ParserDeferredRecord * __cdecl make_deferred_expression(CompilerExpression *) { __asm { int 3 } return (ParserDeferredRecord *)0; }
extern "C" void __cdecl begin_function_definition(void *) { __asm { int 3 } }
extern "C" ParserTokenView *__cdecl prepare_deferred_declaration_token(void) { __asm { int 3 } return NULL; }
extern "C" CompilerNameEntry * __cdecl make_anonymous_conversion_name(void) { __asm { int 3 } return (CompilerNameEntry *)0; }



extern "C" void __cdecl process_pch_boundary(void) { __asm { int 3 } }
extern "C" void __cdecl begin_switch_statement(void) { __asm { int 3 } }
extern "C" void __cdecl set_switch_expression(CompilerExpression *) { __asm { int 3 } }
extern "C" void __cdecl add_switch_case(CompilerExpression *) { __asm { int 3 } }
extern "C" void __cdecl finish_switch_expression(void) { __asm { int 3 } }
extern "C" void __cdecl finish_switch_scope(void) { __asm { int 3 } }
extern "C" void __cdecl prepare_break_statement(void) { __asm { int 3 } }
extern "C" void __cdecl finish_loop_statement(unsigned, void *, CompilerExpression *) { __asm { int 3 } }
extern "C" void __cdecl process_condition_lookahead(ParserTokenView *) { __asm { int 3 } }
extern "C" CompilerExpression * __cdecl make_member_pointer_expression(CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" void __cdecl begin_initializer_element(void) { __asm { int 3 } }
void RECON_THISCALL ParserTypeModifier::MemberPointer(void *, CompilerDeclaratorQualifiers *, CompilerDeclaratorQualifiers *) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarator::PrepareAllocationType(void) { __asm { int 3 } }
extern "C" int __cdecl invalid_allocation_type(CompilerSymbol *) { __asm { int 3 } return (int)0; }
extern "C" void __cdecl prepare_allocation_type(void) { __asm { int 3 } }
CompilerType * RECON_THISCALL CompilerDeclarator::ResolvedType(void) { __asm { int 3 } return (CompilerType *)0; }
extern "C" CompilerExpression * __cdecl make_new_expression(unsigned, void *, CompilerSymbol *, CompilerExpression *, CompilerType *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" void __cdecl begin_do_statement(void) { __asm { int 3 } }
extern "C" void __cdecl finish_do_statement(void) { __asm { int 3 } }
extern "C" void __cdecl finish_condition_expression(void) { __asm { int 3 } }
extern "C" void __cdecl begin_loop_body(void) { __asm { int 3 } }
extern "C" void __cdecl finish_loop_test(void) { __asm { int 3 } }
extern "C" void __cdecl finish_loop_scope(void) { __asm { int 3 } }
extern "C" void __cdecl prepare_loop_condition(void) { __asm { int 3 } }
extern "C" CompilerExpression * __cdecl make_sizeof_expression(CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" void __cdecl finish_default_label(void) { __asm { int 3 } }
extern "C" int __cdecl ignore_initialized_declaration(CompilerSymbol *) { __asm { int 3 } return (int)0; }
extern "C" void __cdecl finish_deferred_declaration(void) { __asm { int 3 } }
extern "C" __int64 __cdecl next_enumerator_value(__int64, CompilerNameEntry *) { __asm { int 3 } return (__int64)0; }
extern "C" unsigned __cdecl prepare_bitfield(CompilerSymbol *, __int64) { __asm { int 3 } return (unsigned)0; }
extern "C" CompilerExpression * __cdecl make_delete_expression(unsigned, CompilerExpression *, CompilerExpression *, unsigned) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" void __cdecl apply_conversion_type(CompilerSymbol *, CompilerType *) { __asm { int 3 } }
extern "C" void __cdecl initialize_type_specifier(CompilerDeclaration *, CompilerType *) { __asm { int 3 } }
extern "C" CompilerNameEntry * __cdecl make_conversion_name(CompilerType *) { __asm { int 3 } return (CompilerNameEntry *)0; }
void RECON_THISCALL CompilerDeclarator::ApplyParserQualifiers(CompilerDeclaration *) { __asm { int 3 } }
extern "C" void __cdecl begin_deferred_input(unsigned) { __asm { int 3 } }
extern "C" void __cdecl require_integral_constant(CompilerExpression *) { __asm { int 3 } }
void RECON_THISCALL CompilerSymbolLink::SetClassContext(CompilerClassBinding *) { __asm { int 3 } }
extern "C" void __cdecl check_completed_class(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl begin_for_statement(void) { __asm { int 3 } }
extern "C" void __cdecl begin_for_condition(void) { __asm { int 3 } }
extern "C" void __cdecl finish_for_condition(void) { __asm { int 3 } }
extern "C" void __cdecl finish_for_statement(void) { __asm { int 3 } }
extern "C" void __cdecl process_label_name(CompilerNameEntry *) { __asm { int 3 } }
extern "C" CompilerSymbol * __cdecl find_label_symbol(CompilerNameEntry *) { __asm { int 3 } return (CompilerSymbol *)0; }
extern "C" CompilerSymbol * __cdecl make_label_symbol(CompilerNameEntry *) { __asm { int 3 } return (CompilerSymbol *)0; }
extern "C" void __cdecl note_label_reference(CompilerSymbol *, unsigned) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarator::AddArrayExpression(CompilerExpression *) { __asm { int 3 } }
extern "C" void __cdecl process_empty_statement(void) { __asm { int 3 } }
extern "C" void __cdecl begin_type_declaration(void) { __asm { int 3 } }
extern "C" void * __cdecl finish_type_declaration(CompilerParameterSet *) { __asm { int 3 } return (void *)0; }
extern "C" void __cdecl process_declared_type(void *, CompilerDeclaration *) { __asm { int 3 } }
extern "C" void __cdecl process_external_recovery(unsigned) { __asm { int 3 } }
extern "C" void __cdecl process_function_type(void *, void *) { __asm { int 3 } }
extern "C" void __cdecl prepare_continue_statement(void) { __asm { int 3 } }
extern "C" void __cdecl process_end_of_input(void) { __asm { int 3 } }
extern "C" void __cdecl append_parser_string(void *, ParserSemanticView *) { __asm { int 3 } }
int RECON_THISCALL CompilerParser::Recover(void) { __asm { int 3 } return (int)0; }
void RECON_THISCALL CompilerParser::ClearRecovery(void) { __asm { int 3 } }
extern "C" void __cdecl report_expected_parser_token(ParserTokenView *, unsigned) { __asm { int 3 } }
extern "C" void __cdecl report_parser_error(const char *) { __asm { int 3 } }
extern "C" ParserTokenView * __cdecl recover_parser_construct(ParserTokenView *, ...) { __asm { int 3 } return (ParserTokenView *)0; }
extern "C" ParserTokenView * __cdecl canonical_parser_token(unsigned) { __asm { int 3 } return (ParserTokenView *)0; }
int RECON_THISCALL ParserFrontendContextView::IsComplete(void) { __asm { int 3 } return (int)0; }
void RECON_THISCALL ParserFrontendContextView::UnreadToken(ParserTokenView *) { __asm { int 3 } }
extern "C" CompilerSymbol * __cdecl note_qualified_scope(short) { __asm { int 3 } return (CompilerSymbol *)0; }
extern "C" CompilerDeclarationContext * __cdecl get_symbol_declaration_context(CompilerDeclarationContext *, CompilerSymbol *) { __asm { int 3 } return (CompilerDeclarationContext *)0; }
ParserDeferredPointer * RECON_THISCALL ParserDeferredPointer::Initialize(void *, unsigned) { __asm { int 3 } return (ParserDeferredPointer *)0; }
CompilerExpression * RECON_THISCALL ParserQualifiedName::ContextValue(void) { __asm { int 3 } return (CompilerExpression *)0; }
void RECON_THISCALL CompilerDeclaratorQualifiers::Merge(CompilerDeclaratorQualifiers *) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarator::SetType(CompilerType *) { __asm { int 3 } }
short RECON_THISCALL CompilerSymbol::DeclarationQualifier(void) { __asm { int 3 } return (short)0; }
int RECON_THISCALL CompilerSymbol::SuppressBrowser(void) { __asm { int 3 } return (int)0; }
void RECON_THISCALL CompilerSymbol::MarkDefined(void) { __asm { int 3 } }
CompilerSymbol * RECON_THISCALL CompilerSymbol::Alias(void) { __asm { int 3 } return (CompilerSymbol *)0; }
extern "C" void __cdecl discard_member_declaration(unsigned, ParserDeclarationValue *) { __asm { int 3 } }
extern "C" unsigned __cdecl extract_type_qualifier(void *) { __asm { int 3 } return (unsigned)0; }
extern "C" void __cdecl process_deferred_parser_input(void) { __asm { int 3 } }
extern "C" void __cdecl add_qualified_base_class(unsigned, int, CompilerSymbol *) { __asm { int 3 } }
extern "C" CompilerExpression * __cdecl make_qualified_allocation_expression(unsigned, void *, CompilerDeclaration *, CompilerDeclaration *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression * __cdecl make_array_allocation_expression(unsigned, void *, void *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression *__cdecl diagnose_parser_operand(unsigned, CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression * __cdecl make_cpp_cast_expression(CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression * __cdecl make_cpp_type_expression(CompilerDeclaration *, CompilerDeclaration *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" void __cdecl prepare_case_statement(void) { __asm { int 3 } }
extern "C" void __cdecl set_exception_filter(CompilerExpression *) { __asm { int 3 } }
extern "C" void __cdecl process_except_statement(void) { __asm { int 3 } }
extern "C" void __cdecl begin_exception_handler(void) { __asm { int 3 } }
extern "C" void __cdecl begin_finally_handler(void) { __asm { int 3 } }
extern "C" void __cdecl process_try_statement(void) { __asm { int 3 } }
extern "C" void __cdecl process_finally_statement(void) { __asm { int 3 } }
extern "C" void __cdecl finish_exception_handler(void) { __asm { int 3 } }
extern "C" void __cdecl finish_exception_filter(void) { __asm { int 3 } }
extern "C" CompilerExpression * __cdecl recover_new_expression(void) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression * __cdecl make_parser_comparison(CompilerExpression *, CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression * __cdecl expand_initializer_expression(unsigned, CompilerExpression *, CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression * __cdecl make_cpp_query_expression(CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression * __cdecl make_cpp_query_type(CompilerDeclaration *, CompilerDeclaration *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression * __cdecl make_unary_parser_extension(unsigned, CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" void * __cdecl make_parser_extension_parameters(CompilerParameterDescription *) { __asm { int 3 } return (void *)0; }
extern "C" void __cdecl set_parser_extension_type(CompilerType *) { __asm { int 3 } }
extern "C" void __cdecl begin_parser_extension(void) { __asm { int 3 } }
extern "C" void __cdecl process_parser_extension(void) { __asm { int 3 } }
extern "C" void __cdecl finish_parser_extension(void) { __asm { int 3 } }
extern "C" CompilerExpression * __cdecl make_parser_extension_expression(void) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerExpression * __cdecl make_parser_extension_operand(CompilerExpression *) { __asm { int 3 } return (CompilerExpression *)0; }
extern "C" CompilerSymbol * __cdecl begin_namespace_declaration(CompilerNameEntry *) { __asm { int 3 } return (CompilerSymbol *)0; }
extern "C" void __cdecl finish_namespace_declaration(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl process_using_declaration(CompilerSymbol *, unsigned, unsigned) { __asm { int 3 } }
extern "C" void __cdecl process_using_directive(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl process_using_member(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl declare_namespace_alias(CompilerNameEntry *, CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl process_member_declaration(unsigned, CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *) { __asm { int 3 } }
void RECON_THISCALL ParserDeferredPointer::Traverse(void) { __asm { int 3 } }
void RECON_THISCALL ParserDeferredListItem::Traverse(void) { __asm { int 3 } }
void RECON_THISCALL ParserDeferredRecord::Traverse(void) { __asm { int 3 } }



extern "C" void __cdecl initialize_pending_input_state(void) { __asm { int 3 } }
extern "C" void __cdecl finalize_pending_input_state(void) { __asm { int 3 } }
extern "C" void __cdecl flush_pending_translation_symbols(void) { __asm { int 3 } }
extern "C" void __cdecl resume_pending_parser_input(void) { __asm { int 3 } }
CompilerScopeMatch *RECON_THISCALL CompilerTranslationUnit::FindDirectName(CompilerNameEntry *) { __asm { int 3 } return NULL; }

CompilerScopeMatch *RECON_THISCALL CompilerTranslationUnit::FindContextName(CompilerNameEntry *) { __asm { int 3 } return NULL; }

extern "C" CompilerSymbolBinding *__cdecl extend_qualified_binding(CompilerSymbolBinding *, CompilerNameEntry *, unsigned, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl materialize_qualified_binding(CompilerSymbolBinding *, CompilerNameEntry *, unsigned) { __asm { int 3 } return NULL; }
extern "C" void __cdecl read_dependent_template_id(ParserTokenView **, CompilerSymbol *, unsigned) { __asm { int 3 } }


extern "C" CompilerSymbolBinding *__cdecl copy_symbol_binding(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl prepare_qualified_symbol(CompilerSymbol *, unsigned) { __asm { int 3 } return NULL; }
extern "C" ParserTokenView *__cdecl make_declaration_type_token(CompilerType *, BYTE, const CompilerDeclarationContext *, CompilerSymbol *) { __asm { int 3 } return NULL; }
BYTE *RECON_THISCALL CompilerPool::Allocate(unsigned) { __asm { int 3 } return NULL; }

extern "C" int __cdecl has_saved_source_state(void) { __asm { int 3 } return 0; }

void RECON_THISCALL ParserDeclarationReader::SaveSemantic(void) { __asm { int 3 } }



void RECON_THISCALL ParserDeclarationReader::ReadStorageModifier(void) { __asm { int 3 }  }

extern "C" void __cdecl read_declaration_attributes(CompilerDeclarationContext *) { __asm { int 3 } }


int RECON_THISCALL ParserTagDeclaration::ReadAttributes(void) { __asm { int 3 } return 0; }
int RECON_THISCALL ParserTagDeclaration::ReadQualifiedName(void) { __asm { int 3 } return 0; }
int RECON_THISCALL ParserTagDeclaration::ReadScopedName(ParserTokenView *) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerTranslationUnit::InsertScopedTag(CompilerSymbol *) { __asm { int 3 } }
extern "C" CompilerSymbol *__cdecl reference_tag_name(CompilerNameEntry *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl resolve_tag_template_id(ParserTokenView **, CompilerSymbol *, unsigned, unsigned) { __asm { int 3 } }

extern "C" ParserTokenView *__cdecl begin_tag_replay(ParserTokenView *, CompilerNameEntry *) { __asm { int 3 } return NULL; }

extern "C" int __cdecl should_report_diagnostic(unsigned, unsigned) { __asm { int 3 } return 0; }
extern "C" void __cdecl print_diagnostic_auxiliary(unsigned, ...) { __asm { int 3 } }

void RECON_THISCALL CompilerTranslationUnit::InsertTagScope(CompilerSymbol *) { __asm { int 3 } }
CompilerScopeTable *RECON_THISCALL CompilerTranslationUnit::FileScope(void) { __asm { int 3 } return NULL; }


void RECON_THISCALL CompilerTranslationUnit::InsertDeferredTag(CompilerSymbol *) { __asm { int 3 } }
CompilerScopeMatch *RECON_THISCALL CompilerScope::MatchTag(CompilerSymbol *, unsigned) { __asm { int 3 } return NULL; }
CompilerScopeMatch *RECON_THISCALL CompilerScopeMatch::Initialize(CompilerSymbol *, CompilerScopeTable *) { __asm { int 3 } return NULL; }


extern "C" CompilerSymbol *__cdecl instantiate_tag_definition(CompilerSymbol *) { __asm { int 3 } return NULL; }





void RECON_THISCALL CompilerTranslationUnit::SelectNamespaceSymbol(CompilerSymbol *) { __asm { int 3 } }
void RECON_THISCALL CompilerDependency::RecordClass(CompilerSymbol *) { __asm { int 3 } }

void RECON_THISCALL CompilerScope::EnsureBuckets(void) { __asm { int 3 } }

void RECON_THISCALL CompilerClassScope::Insert(CompilerSymbol *) { __asm { int 3 }  }
CompilerScopeLookup * RECON_THISCALL CompilerClassScope::Find(CompilerNameEntry *, unsigned) { __asm { int 3 } return NULL; }
CompilerScopeMatch * RECON_THISCALL CompilerClassScope::FindOrdinary(CompilerNameEntry *) { __asm { int 3 } return NULL; }
int RECON_THISCALL CompilerClassScope::FindNested(CompilerNameEntry *, CompilerSymbol **) { __asm { int 3 } return 0; }

void RECON_THISCALL CompilerClassScope::Serialize(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerClassScope::Check(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerClassScope::Link(CompilerSymbol *) { __asm { int 3 }  }

CompilerScopeMatch * RECON_THISCALL CompilerAuxiliaryScope::FindOrdinary(CompilerNameEntry *) { __asm { int 3 } return NULL; }
int RECON_THISCALL CompilerAuxiliaryScope::FindNested(CompilerNameEntry *, CompilerSymbol **) { __asm { int 3 } return 0; }
CompilerSymbolLink ** RECON_THISCALL CompilerAuxiliaryScope::FindLink(CompilerSymbolLink *) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerAuxiliaryScope::Serialize(void) { __asm { int 3 }  }
extern "C" void *__cdecl get_class_template_context(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" ParserDeferredList *__cdecl get_class_template_arguments(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" void *__cdecl get_class_context_key(void *) { __asm { int 3 } return NULL; }
extern "C" CompilerScopeTable *__cdecl make_class_lookup_scope(void *, ParserDeferredList *, CompilerSymbol *, int, int, int) { __asm { int 3 } return NULL; }

extern "C" CompilerSymbol *__cdecl complete_member_declaration(CompilerSymbol *, int *, CompilerSymbol *) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerDependency::RecordMember(CompilerSymbol *) { __asm { int 3 } }
extern "C" int __cdecl requested_member_pointer_model(unsigned *) { __asm { int 3 } return 0; }
extern "C" int __cdecl class_member_pointer_model(CompilerSymbol *) { __asm { int 3 } return 0; }

extern "C" void __cdecl initialize_pending_declaration_cursor(struct PendingDeclarationCursor *, void *) { __asm { int 3 } }
extern "C" int __cdecl next_pending_declaration(struct PendingDeclarationCursor *) { __asm { int 3 } return 0; }

void RECON_THISCALL ParserInputEndState::Finish(void) { __asm { int 3 } }

WORD RECON_THISCALL CompilerDeclarator::MergeModifierQualifiers(WORD, WORD) { __asm { int 3 } return 0; }

extern "C" CompilerType *__cdecl reference_expression_type(CompilerExpression *) { __asm { int 3 } return NULL; }

extern "C" CompilerExpression *__cdecl prepare_ir_expression(CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" int __cdecl emit_special_ir_expression(CompilerExpression *) { __asm { int 3 } return 0; }
extern "C" unsigned __cdecl expression_control_kind(CompilerExpression *) { __asm { int 3 } return 0; }

extern "C" CompilerExpression *__cdecl attach_pending_expression_work(CompilerExpression *, CompilerWorkLink *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl attach_expression_reference_work(CompilerExpression *, CompilerWorkLink *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl attach_expression_cleanup_work(CompilerExpression *, CompilerWorkLink *) { __asm { int 3 } return NULL; }
extern "C" CompilerWorkLink *__cdecl prepend_expression_work_link(CompilerWorkLink **, void *) { __asm { int 3 } return NULL; }
extern "C" int __cdecl merge_default_declaration_context(CompilerDeclarationContext *, CompilerDeclarationContext) { __asm { int 3 } return 0; }

void RECON_THISCALL CompilerRecordedLexer::RecordAssembly(void) { __asm { int 3 } }

CompilerSymbolLink *RECON_THISCALL CompilerSymbolLink::CopyClassLink(const CompilerSymbolLink *, CompilerSymbol *) { __asm { int 3 } return NULL; }
CompilerSymbolLink *RECON_THISCALL CompilerSymbolLink::CopySmallLink(const CompilerSymbolLink *, CompilerSymbol *) { __asm { int 3 } return NULL; }
