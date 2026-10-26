#ifndef C1XX_PARSER_STATE_H
#define C1XX_PARSER_STATE_H
#include "parser-actions.h"
#include "declaration.h"
#include <recon.h>

/* Wider action owners remain source-only. Native denies before any action
 * reads them; these views provide no native defaults, capacity or byte-equality
 * claim. Existing contextual cells below are aliases, never duplicate owners. */
extern char compiler_token_text[2048];
extern "C" void __cdecl process_version_literal(struct CompilerVersionRecord *);
#ifndef RECON_NATIVE_TEST
extern int parser_result;
extern int parser_saved_state;
extern int parser_recovery_action;
extern DWORD * parser_saved_state_top;
extern ParserSemanticView * parser_saved_value_top;
extern int label_stack_count;
extern int initializer_started;
extern ParserDeclarationView parser_special_qualifiers;
extern CompilerSymbol * current_declaration_symbol;
extern CompilerType * current_enum_type;
extern __int64 enumerator_value;
extern CompilerExpression * case_label;
extern ParserLabelStack * continue_labels;
extern ParserLabelStack * break_labels;
extern ParserLabelStack * loop_labels;
extern ParserLabelStack * switch_labels;
extern ParserLabelStack * default_labels;
extern void * allocation_context;
#else
#define parser_result (*(int *)(recon_unsupported("parser: source-only parser_result"),0))
#define parser_saved_state (*(int *)(recon_unsupported("parser: source-only parser_saved_state"),0))
#define parser_recovery_action (*(int *)(recon_unsupported("parser: source-only parser_recovery_action"),0))
#define parser_saved_state_top (*(DWORD * *)(recon_unsupported("parser: source-only parser_saved_state_top"),0))
#define parser_saved_value_top (*(ParserSemanticView * *)(recon_unsupported("parser: source-only parser_saved_value_top"),0))
#define label_stack_count (*(int *)(recon_unsupported("parser: source-only label_stack_count"),0))
#define initializer_started (*(int *)(recon_unsupported("parser: source-only initializer_started"),0))
#define parser_special_qualifiers (*(ParserDeclarationView *)(recon_unsupported("parser: source-only parser_special_qualifiers"),0))
#define current_declaration_symbol (*(CompilerSymbol * *)(recon_unsupported("parser: source-only current_declaration_symbol"),0))
#define current_enum_type (*(CompilerType * *)(recon_unsupported("parser: source-only current_enum_type"),0))
#define enumerator_value (*(__int64 *)(recon_unsupported("parser: source-only enumerator_value"),0))
#define case_label (*(CompilerExpression * *)(recon_unsupported("parser: source-only case_label"),0))
#define continue_labels (*(ParserLabelStack * *)(recon_unsupported("parser: source-only continue_labels"),0))
#define break_labels (*(ParserLabelStack * *)(recon_unsupported("parser: source-only break_labels"),0))
#define loop_labels (*(ParserLabelStack * *)(recon_unsupported("parser: source-only loop_labels"),0))
#define switch_labels (*(ParserLabelStack * *)(recon_unsupported("parser: source-only switch_labels"),0))
#define default_labels (*(ParserLabelStack * *)(recon_unsupported("parser: source-only default_labels"),0))
#define allocation_context (*(void * *)(recon_unsupported("parser: source-only allocation_context"),0))
#endif
/* The one actual BA8C8 owner. Native has honest44B capacity; literal
 * copies stay40B and declaration token/by-value transfers remain guarded. */
#define saved_parser_value (*(ParserSemanticView *)&frontend_bss.saved_token_value)
/* One actual c06d8 owner, shared with directive dispatch. */
#define current_parser_expression frontend_bss.current_parser_expression
#define suppress_parser_codegen frontend_bss.suppress_parser_codegen
#define declaration_nesting frontend_bss.declaration_nesting
#define allocation_operator frontend_data.allocation_operator
/* Encountered BF778/BF7A0/BF7C8 rows have actual declaration values. Each original
 * row has40 bytes; its native typed member pointer requires44. Other rows
 * stop before address calculation; the complete stack capacity is unclaimed. */
static RECON_FORCEINLINE ParserDeclarationView *declaration_spec_row(unsigned index)
{
    if (index >= 3) { __asm { int 3 } }
    return (ParserDeclarationView *)&frontend_bss.declaration_specs[index];
}
/* The original increments the shared nesting index, rejects depth60, and
 * addresses each actual control row at stride2. */
static RECON_FORCEINLINE ParserDeclarationControl *declaration_control_row(unsigned index)
{
    if (index >= 60) { __asm { int 3 } }
    return &frontend_bss.declaration_controls[index];
}
#define deferred_contexts ((ParserDeferredList **)frontend_bss.parameter_contexts)
#define qualification_scopes frontend_bss.parser_qualification_scopes
#define class_scopes frontend_bss.class_scope_rows
#define check_completed_class_option frontend_data.class_completion_checks
#define parser_browser_context publish_browser_local
#define unevaluated_expression_depth declaration_source_state.suppress_class_completion
#define parser_semicolon ((ParserTokenView *)frontend_data.canonical_token_records[116])
#define parser_end_marker ((ParserTokenView *)frontend_data.canonical_token_records[171])
#define parser_lookahead frontend_bss.parser_lookahead
#define parser_production frontend_bss.parser_production
#define parser_token_text_cursor frontend_data.token_text_cursor

#endif
