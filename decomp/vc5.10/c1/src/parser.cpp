#include <string.h>
#include <new.h>
#include "frontend.h"
#include "parser-grammar.h"
#include "keywords.h"
#pragma intrinsic(memcpy, memset, memcmp)
#include <recon.h>

/* Semantic qualifier column for the twelve kinds that can shift into the
 * qualifier reductions. These are logical relations, not the neighboring
 * mixed token-property records. Unknown kind names retain their identities. */
struct ParserTokenQualifier { BYTE kind; WORD flags; };
static const ParserTokenQualifier parser_token_qualifiers[] = {
    { 37, 0x0010 }, /* __cdecl */
    { 38, 0x0040 }, /* __fastcall */
    { 39, 0x0800 }, /* __restrict */
    { 40, 0x00b5 }, /* token kind40 */
    { 47, 0x0100 }, /* const */
    { 48, 0x0200 }, /* volatile */
    { 120, 0x00b5 }, /* token kind120 */
    { 130, 0x0060 }, /* __stdcall */
    { 191, 0x0070 }, /* __thiscall */
    { 193, 0x0400 }, /* __unaligned */
    { 222, 0x0020 }, /* __pascal */
    { 237, 0x1000 }, /* __ptr64 */
};

/* These contextual source declarations have no native default/snapshot
 * contract. Run holds every wider action before any of them is read or
 * changed. The declaration-stack bound60 is checked by the actual token
 * producer; no unknown declaration or control record is initialized here. */
struct ParserLabelStack {
    ParserLabelStack *next;
    CompilerParserNode *node;
};
#define lexer_flags c1_data.lexer_flags /* RVA5eddc, initialized/snapshotted DWORD. */
#define parser_flags c1_data.parser_flags /* RVA5ede0 */
#define parser_early_result c1_bss.parser_early_result /* RVA55068, BSS */
#define declaration_depth c1_bss.declaration_depth /* RVA5416c */
#define declaration_control c1_bss.declaration_control /* RVA54178 */
#define declaration_specs c1_bss.declaration_specs /* RVA541f0; original stride20 */
#define declaration_contexts c1_bss.declaration_contexts /* RVA54ca8 */
#define final_input_token_emitted c1_data.final_input_token_emitted /* RVA5e210, DWORD0 */
#define unevaluated_expression_depth c1_bss.unevaluated_expression_depth /* RVA53bd4 */
#define type_description_depth c1_bss.type_description_depth /* RVA54fc4 */
#define parameter_nesting c1_bss.parameter_nesting /* RVA5508c */
#define parameter_scope_value c1_bss.parameter_scope_value /* RVA5401c */

#define parameter_storage_class c1_bss.parameter_storage_class /* RVA53d78 */

#define parameter_scope_initial_value c1_bss.parameter_scope_initial_value /* RVA5509c */
#define function_storage_class c1_bss.function_storage_class /* RVA55088 */
#define function_return_type c1_data.function_return_type /* RVA5e46c */
#define function_context c1_data.function_context /* RVA5e598, initializedNULL */
#define function_reference_epoch c1_data.function_reference_epoch /* RVA5f36c */
#define function_reset_value c1_bss.function_reset_value /* RVA54dd8 */
#define function_label_depth c1_data.function_label_depth /* RVA5f388 */
#define c1_disable_inline_records c1_data.disable_inline_records /* RVA5f378 */
#define c1_nonil_type_index c1_bss.nonil_type_index /* RVA54c80 */
static int parameter_browser_suppressed;       /* RVA54de8, source-only */
static int unevaluated_type_depth;            /* RVA5f3a8 */
static unsigned parser_symbol_cache_index;    /* RVA5f494 */
int c1_constant_diagnostic_mode;          /* RVA5f35c */
static void *enumerator_context;              /* RVA5e400 */
static __int64 enumerator_value;              /* RVA5f380 */
static BYTE *c1_lexer_token_cursor;            /* RVA5f38c */
static ParserLabelStack *loop_labels;         /* RVA55090 */
static ParserLabelStack *continue_labels;     /* RVA547f0 */
static ParserLabelStack *break_labels;        /* RVA54014 */
static ParserLabelStack *switch_labels;       /* RVA54dd0 */
static ParserLabelStack *default_labels;      /* RVA54020 */
static int label_stack_count;                /* RVA550a8 */
#define case_label c1_bss.case_label /* RVA53ff8, BSS; statement entry clears it */
extern char c1_lexer_token_text[2048];         /* Existing source-only owner. */



/* Authentic52-byte stack scanner. The token and consumed-token count are
 * interpreted here. The remaining named offsets only preserve access widths
 * in source paths that stop at pending methods; they carry no wider semantic
 * or object-equality claim. The caller's whole-span zero writer is retained. */
struct CompilerDeclarationScanner {
    CompilerParserToken *token;
    CompilerType *resolved_type;
    unsigned tokens_read;
    CompilerParserToken *cached_token;
    BYTE unmodeled_10[4];
    union {
        CompilerDeclarationAttributes attributes;
        struct { DWORD declaration_flags, declaration_value; };
    };
    unsigned unmodeled_1c;
    CompilerType *type;
    void *unmodeled_24;
    DWORD unmodeled_28;
    DWORD declaration_callback; /* Original4B member-pointer identity; onlyNULL supported natively. */
    WORD base_type_flags;
    WORD type_attributes;
    CompilerParserToken *RECON_THISCALL Scan(void);
    int RECON_THISCALL ScanIdentifier(void);
    int RECON_THISCALL ScanClass27(void);
    void RECON_THISCALL ScanClass30(void);
    void RECON_THISCALL ScanTypeSpecifier(void);
    void RECON_THISCALL ScanClass32(void);
    void RECON_THISCALL ScanClass33(void);
    void RECON_THISCALL ScanClass34(void);
    int RECON_THISCALL ScanClass40(void);
    void RECON_THISCALL Normalize(void);
    CompilerParserToken *RECON_THISCALL BuildToken(unsigned);
};
typedef char CompilerDeclarationScanner_type_offset[offsetof(CompilerDeclarationScanner, type) == 32 ? 1 : -1];
typedef char CompilerDeclarationScanner_flags_offset[offsetof(CompilerDeclarationScanner, base_type_flags) == 48 ? 1 : -1];
typedef char CompilerDeclarationScanner_extent[sizeof(CompilerDeclarationScanner) == 52 ? 1 : -1];

extern "C" {
CompilerParserToken * __cdecl get_next_token(void); /* RVA02b76 */
void __cdecl emit_control_record(int); /* RVA03fc2 */
CompilerParserToken * __cdecl classify_identifier_token(CompilerParserToken *); /* RVA05ae5 */
void __cdecl declare_symbol(CompilerDeclarator *, CompilerIrSymbol *); /* RVA05f15 */
void __cdecl finish_external_declaration(int); /* RVA06928 */
int __cdecl current_recovery_shifts(void); /* RVA069d9 */
CompilerParserNode * __cdecl make_typed_constant(CompilerType *, const void *, unsigned); /* RVA072d8 */
CompilerIrSymbol * __cdecl make_declarator_symbol(CompilerNameEntry *); /* RVA074b5 */
CompilerIrSymbol * __cdecl declare_initialized_symbol(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *); /* RVA07598 */
CompilerIrSymbol * __cdecl declare_declarator(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *); /* RVA076c1 */
__int64 __cdecl constant_integer_value(CompilerParserNode *); /* RVA0b5ae */
void __cdecl finish_enum_specifier(void *, CompilerType *); /* RVA0ba0b */
void __cdecl add_record_member(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *); /* RVA0bb41 */
CompilerType * __cdecl parse_record_specifier(void); /* RVA0c0a9 */
WORD __cdecl merge_declarator_qualifiers(WORD, WORD); /* RVA0ca41 */
void __cdecl process_declaration(const CompilerParserDeclaration *); /* RVA0d706 */
BYTE __cdecl prepare_bitfield(CompilerIrSymbol *, CompilerDeclarator *, CompilerIrSymbol *); /* RVA0e4d2 */
void __cdecl add_enumerator(CompilerNameEntry *, void *, __int64); /* RVA0eed5 */
CompilerType * __cdecl parse_enum_specifier(void); /* RVA0f092 */
__int64 __cdecl next_enumerator_value(__int64, CompilerNameEntry *); /* RVA0f123 */
CompilerParserNode * __cdecl make_label_expression(CompilerIrSymbol *, int); /* RVA10519 */
CompilerParserNode * __cdecl make_internal_label(unsigned); /* RVA1066f */
void __cdecl record_label_reference(CompilerIrSymbol *); /* RVA1070a */
void __cdecl begin_compound_statement(void); /* RVA10af9 */
void __cdecl finish_compound_statement(void); /* RVA10bb3 */
void __cdecl emit_ir_label(CompilerParserNode *); /* RVA10d3c */
void __cdecl prepare_function_definition(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *); /* RVA10e70 */
void __cdecl begin_function_body(void); /* RVA10ffa */
void __cdecl finish_function_body(void); /* RVA1153d */
void __cdecl parse_inline_assembly(void); /* RVA178c6 */
CompilerParserNode * __cdecl make_binary_expression(BYTE, CompilerParserNode *, CompilerParserNode *); /* RVA197b3 */
CompilerParserNode * __cdecl make_unary_expression(BYTE, CompilerParserNode *); /* RVA1b07b */
CompilerParserNode * __cdecl make_identifier_expression(CompilerNameEntry *); /* RVA1b94e */
CompilerParserNode * __cdecl make_right_typed_expression(BYTE, CompilerParserNode *, CompilerParserNode *); /* RVA1bdf6 */
CompilerParserNode * __cdecl make_type_size_expression(CompilerType *); /* RVA1ce98 */
CompilerParserNode * __cdecl make_member_expression(BYTE, CompilerParserNode *, CompilerParserNode *); /* RVA1d38e */
void __cdecl emit_return_statement(CompilerParserNode *, int); /* RVA1d5f2 */
CompilerIrSymbol * __cdecl prepare_initializer_symbol(CompilerIrSymbol *); /* RVA1df6d */
CompilerParserNode * __cdecl make_based_expression(CompilerParserNode *); /* RVA1e277 */
CompilerParserNode * __cdecl normalize_based_expression(CompilerParserNode *); /* RVA1e3a8 */
CompilerParserNode * __cdecl make_expression_with_type(CompilerParserNode *, CompilerParserNode *, CompilerType *); /* RVA1e68f */
CompilerParserNode * __cdecl finish_argument_list(CompilerParserNode *); /* RVA1e8dc */
void __cdecl emit_expression(CompilerParserNode *); /* RVA1eba0 */
void __cdecl emit_conditional_branch(CompilerParserNode *, CompilerParserNode *, int); /* RVA1ec07 */
void __cdecl emit_loop_condition(CompilerParserNode *); /* RVA1f1ad */
void __cdecl emit_case_label(CompilerParserNode *); /* RVA1f312 */
void __cdecl finish_switch_statement(void); /* RVA1f54b */
void __cdecl emit_branch(CompilerParserNode *, CompilerParserNode *, CompilerParserNode *); /* RVA1f6d3 */
void __cdecl apply_initializer(CompilerIrSymbol *, CompilerParserNode *); /* RVA1f7d8 */
void __cdecl begin_conditional_branch(CompilerParserToken *); /* RVA1f9c4 */
void __cdecl finish_initializer(CompilerParserNode *, int); /* RVA207c2 */
void __cdecl finish_aggregate_initializer(CompilerParserNode *); /* RVA20a13 */
void __cdecl finish_initializer_element(void); /* RVA21166 */
void __cdecl begin_aggregate_initializer(CompilerParserNode *); /* RVA21208 */
CompilerParserNode * __cdecl make_sizeof_expression(CompilerParserNode *); /* RVA21bd9 */
void __cdecl begin_initializer_element(void); /* RVA21c4f */
void __cdecl begin_external_declaration(void); /* RVA21dd6 */
void __cdecl attach_declaration_context(CompilerIrSymbol *, void *); /* RVA22e59 */
void __cdecl begin_exception_handler(void); /* RVA23957 */
void __cdecl finish_exception_handler(void); /* RVA239ac */
void __cdecl set_exception_filter_mode(void); /* RVA23a50 */
void __cdecl emit_exception_filter(CompilerParserNode *); /* RVA23a5b */
void __cdecl clear_exception_filter_mode(void); /* RVA23a9b */
void __cdecl finish_finally_handler(void); /* RVA23aa6 */
void __cdecl append_literal_text(const char **, CompilerParserValue *); /* RVA24f93 */
CompilerIrSymbol * __cdecl find_statement_label(CompilerNameEntry *); /* RVA25125 */
CompilerIrSymbol * __cdecl make_statement_label(CompilerNameEntry *); /* RVA2517f */
void __cdecl define_statement_label(CompilerNameEntry *); /* RVA251d5 */
CompilerParameterDescription * __cdecl make_parameter_description(CompilerNameEntry *); /* RVA25ba5 */
void __cdecl add_old_style_parameter(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *); /* RVA25c26 */
void __cdecl prepare_for_statement(void); /* RVA25eb0 */
void __cdecl report_expected_parser_token(CompilerParserToken *, int); /* RVA3f1b4 */
CompilerParserValue * __cdecl make_recovery_parser_value(CompilerParserValue *, int); /* RVA3f281 */
void __cdecl report_parser_error(const char *); /* RVA3f2c2 */
void __cdecl skip_declaration_tokens(void); /* RVA3f393 */
CompilerParserToken * __cdecl recover_parser_token(CompilerParserToken *, int, int, int); /* RVA3f3ff */
CompilerIrSymbol * __cdecl find_based_declarator(CompilerNameEntry *); /* RVA3f4f1 */
void __cdecl process_pragma_declaration(void); /* RVA3f798 */
void __cdecl prepare_external_input(void); /* RVA3f7d4 */
void __cdecl report_type_declaration_error(int, const CompilerParserDeclaration *); /* RVA41374 */
void __cdecl emit_leave_statement(void); /* RVA41f63 */
void __cdecl begin_exception_filter(void); /* RVA41f89 */
void __cdecl finish_exception_filter(void); /* RVA41fa4 */
CompilerParserNode * __cdecl make_typed_unary_expression(CompilerType *, CompilerParserNode *); /* RVA429ae */
CompilerIrSymbol *__cdecl make_abstract_declarator(void);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
void __cdecl compiler_warning(int, ...);
void __cdecl write_ir_record(int, const void *);
void __cdecl process_version_literal(struct CompilerVersionRecord *);
}

int RECON_THISCALL CompilerParser::Run(void)
{
    const ParserState *state_info;
    const ParserGoto *goto_info;
    const ParserProduction *production;
    const ParserTransition *transition;
    const ParserRecovery *repair;
    CompilerParserStackValue *right;
    CompilerParserValue repair_value;
    CompilerIrSymbol *symbol, *label_symbol;
    CompilerSymbolBinding *binding;
    CompilerNameEntry *name;
    CompilerDeclarator *declarator;
    CompilerParameterDescription *parameters;
    CompilerArena *storage;
    CompilerParserToken *temporary_token;
    struct { unsigned low, high; } constant_one;
    unsigned arena_kind;
    WORD qualifiers;
    int i, terminal, rule, repair_terminal;
    for (;;) {
        ++state_top;
        /* Original permits one halfword past states[339]. Native holds that
         * overflow domain; it is not part of the owned8864-byte object. */
#ifdef RECON_NATIVE_TEST
        if (state_top >= states + 340)
            recon_unsupported("parser state stack exceeds owned allocation");
#endif
        if ((BYTE *)state_top > (BYTE *)this + 0x22a0) {
            report_parser_error("yacc stack overflow");
            return 1;
        }
        *state_top = state;
        ++value_top;
        /* States16/15/12 carry the declaration alternative from token36
         * through productions32/31. Other states retain the first-DWORD
         * projection until their own semantic consumers are recovered. */
#ifdef RECON_NATIVE_TEST
        if (state == 76) {
            value_top->value.typed_constant.type = c1_bss.parser_value.typed_constant.type;
            value_top->value.typed_constant.value = c1_bss.parser_value.typed_constant.value;
        } else if (state == 16 || state == 15 || state == 12 || state == 10)
            value_top->value.declaration = c1_bss.parser_value.declaration;
        else if (state == 27)
            value_top->value.declarator_symbol = c1_bss.parser_value.declarator_symbol;
        else value_top->value.declaration_flags = c1_bss.parser_value.declaration_flags;
#else
        value_top->value = c1_bss.parser_value;
#endif
read_action:
#ifdef RECON_NATIVE_TEST
        if ((unsigned short)state >= PARSER_STATE_COUNT)
            recon_unsupported("parser state outside recovered grammar");
#endif
        state_info = &parser_states[state];
        if (state_info->needs_token) {
            if (!lookahead)
                lookahead = get_next_token();
            terminal = PARSER_NO_TOKEN;
            if (lookahead) {
#ifdef RECON_NATIVE_TEST
                if (lookahead->kind >= PARSER_TOKEN_KIND_COUNT)
                    recon_unsupported("parser token kind outside modeled map");
#endif
                terminal = parser_token_classes[lookahead->kind];
            }
            transition = parser_shifts + state_info->first_shift;
            for (i = 0; i < state_info->shift_count; ++i, ++transition) {
                if (transition->symbol == terminal) {
                    state = transition->destination;
#ifdef RECON_NATIVE_TEST
                    if (lookahead->kind == 123 || lookahead->kind == 127) {
                        /* Semicolon has no consumed semantic payload here. */
                        c1_bss.parser_value.declaration_flags = 0;
                    } else if (lookahead->kind == 101) {
                        c1_bss.parser_value.typed_constant.type = lookahead->value.typed_constant.type;
                        c1_bss.parser_value.typed_constant.value = lookahead->value.typed_constant.value;
                    } else if (lookahead->kind == 36) {
                        if (lookahead->value.declaration.apply != 0)
                            recon_unsupported("nonNULL declaration callback is not yet modeled");
                        c1_bss.parser_value.declaration = lookahead->value.declaration;
                    } else if (lookahead->kind == 118 || lookahead->kind == 126 || lookahead->kind == 117) {
                        /* Punctuation prototypes establish only the BYTE tag.
                         * The zero extension is native representation, not a
                         * default or equality claim for the three opaque bytes. */
                        c1_bss.parser_value.declaration_flags = lookahead->value.operation;
                    } else c1_bss.parser_value.declaration_flags = lookahead->value.declaration_flags;
#else
                    c1_bss.parser_value = lookahead->value;
#endif
                    lookahead = NULL;
                    if (recovery_shifts > 0)
                        --recovery_shifts;
                    goto push_state;
                }
            }
        }
        rule = state_info->default_production;
        if (state_info->exception_count) {
            if (!lookahead)
                lookahead = get_next_token();
#ifdef RECON_NATIVE_TEST
            if (lookahead && lookahead->kind >= PARSER_TOKEN_KIND_COUNT)
                recon_unsupported("parser token kind outside modeled map");
#endif
            terminal = lookahead ? parser_token_classes[lookahead->kind] : PARSER_NO_TOKEN;
            transition = parser_exceptions + state_info->first_exception;
            for (i = 0; i < state_info->exception_count; ++i, ++transition) {
                if (transition->symbol == terminal) {
                    rule = transition->destination;
                    break;
                }
            }
            if (rule < 0)
                return 0;
        }
        if (rule == 0) {
            switch (recovery_shifts) {
            case 0:
                for (i = 0; i < 46; ++i) {
                    repair = &parser_recoveries[i];
                    if (repair->state >= state)
                        break;
                }
                if (i < 46 && repair->state == state) {
                    repair_terminal = repair->terminal;
                    if (repair_terminal < 0) {
                        ++recovery_shifts;
                        repair_terminal = -repair_terminal;
                    }
                    if (recovery_shifts <= 1)
                        report_expected_parser_token(lookahead, repair_terminal);
                    make_recovery_parser_value(&repair_value, repair_terminal);
#ifdef RECON_NATIVE_TEST
                    c1_bss.parser_value.declaration_flags = repair_value.declaration_flags;
#else
                    c1_bss.parser_value = repair_value;
#endif
                    state = repair->destination;
                    goto push_state;
                }
                report_parser_error("syntax error");
                ++syntax_errors;
                /* fall through */
            case 1:
            case 2:
                recovery_shifts = 3;
                while (state_top >= states) {
                    state_info = &parser_states[*state_top];
                    transition = parser_shifts + state_info->first_shift;
                    for (i = 0; i < state_info->shift_count; ++i, ++transition) {
                        if (transition->symbol == PARSER_ERROR) {
                            state = transition->destination;
                            goto push_state;
                        }
                    }
                    --state_top;
                    --value_top;
                }
                return 1;
            case 3:
    #ifdef RECON_NATIVE_TEST
            if (lookahead && lookahead->kind >= PARSER_TOKEN_KIND_COUNT)
                recon_unsupported("parser token kind outside modeled map");
#endif
            terminal = lookahead ? parser_token_classes[lookahead->kind] : PARSER_NO_TOKEN;
                if (terminal == 0)
                    return 1;
                lookahead = NULL;
                goto read_action;
            }
        }
        right = value_top;
        production = &parser_productions[rule];
        state_top -= production->length;
        value_top -= production->length;
#ifdef RECON_NATIVE_TEST
        if (rule == 32 || rule == 31 || rule == 30 || rule == 18) {
            if ((rule == 32 && state_top[production->length] != 16) ||
                (rule == 31 && state_top[production->length] != 15) ||
                (rule == 30 && state_top[1] != 12) ||
                (rule == 18 && state_top[1] != 10))
                recon_unsupported("declaration reduction source state is not yet modeled");
            c1_bss.parser_value.declaration = value_top[1].value.declaration;
        } else if (rule == 78) {
            if (state_top[1] != 27)
                recon_unsupported("declarator pair reduction source state is not yet modeled");
            c1_bss.parser_value.declarator_symbol = value_top[1].value.declarator_symbol;
        } else c1_bss.parser_value.declaration_flags = value_top[1].value.declaration_flags;
#else
        c1_bss.parser_value = value_top[1].value;
#endif
        goto_info = &parser_gotos[production->nonterminal];
        state = goto_info->default_state;
        transition = parser_goto_transitions + goto_info->first_transition;
        for (i = 0; i < goto_info->transition_count; ++i, ++transition) {
            if (transition->symbol == *state_top) {
                state = transition->destination;
                break;
            }
        }
        /* Semantic actions1..228 remain to be integrated from this target's
         * hot/cold body. Productions229/230 have no semantic action. */
#ifdef RECON_NATIVE_TEST
        /* Wider actions remain held. Productions31/32 additionally carry
         * the established declaration alternative, with NULL callback only. */
        switch (rule) {
        case 1: /* EOF reduction reads only the established early-result cell. */
        case 3:
        case 4:
        case 5: /* No payload consumer before external-declaration finalization. */
        case 9:
        case 10: /* Function declarator identity, then genuine finalization. */
        case 20:
        case 16:
        case 18:
        case 19:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 30:
        case 31:
        case 32:
        case 37:
        case 38:
        case 39:
        case 41:
        case 42:
        case 45:
        case 46:
        case 49:
        case 58:
        case 59:
        case 60:
        case 68:
        case 69:
        case 73:
        case 78:
        case 79:
        case 81:
        case 82:
        case 83:
        case 90:
        case 85:
        case 87:
        case 94:
        case 95:
        case 100:
        case 106:
        case 112:
        case 113:
        case 122:
        case 124:
        case 127: /* Established CLI predicates, then genuine compound entry. */
        case 128: /* Genuine compound exit, with no semantic operand. */
        case 129:
        case 130:
        case 132:
        case 135:
        case 136:
        case 137:
        case 139:
        case 155: /* Return expression, then the genuine emitter boundary. */
        case 161:
        case 168:
        case 169:
        case 179:
        case 196:
        case 208:
        case 209:
        case 218:
        case 229:
        case 230:
            break;
        default:
            recon_unsupported("parser reduction requires wider semantic state");
        }
#endif
switch (rule) {
case 1:
    if (parser_early_result)
        return parser_early_result;
    break;
case 2:
    if (c1_data.options.language_extensions)
        report_language_diagnostic(4, 206);
    else
        compiler_warning(183);
    if (parser_early_result)
        return parser_early_result;
    break;
case 5:
    finish_external_declaration(recovery_shifts);
    break;
case 6:
    prepare_external_input();
    break;
case 7:
    begin_external_declaration();
    break;
case 8:
    process_pragma_declaration();
    break;
case 9:
    parser_flags &= ~0x40;
    if (right[-1].value.declarator)
        begin_function_body();
    break;
case 10:
    if (right[-3].value.declarator)
        finish_function_body();
    parser_flags &= ~0x200;
    break;
case 11:
    if (lexer_flags & 0x1000000)
        report_language_diagnostic(4, 218);
    break;
case 12:
    if (!c1_data.options.language_extensions)
        report_language_diagnostic(4, 19);
    break;
case 13:
    if (!(lexer_flags & 0x1000000))
        process_declaration(&right[-1].value.declaration);
    break;
case 14:
    ClearRecovery();
    declaration_depth = 0;
    lexer_flags &= ~0x800000;
    break;
case 15:
    skip_declaration_tokens();
    finish_compound_statement();
    finish_function_body();
    ClearRecovery();
    lookahead = NULL;
    lexer_flags &= ~0x800000;
    declaration_depth = 0;
    break;
case 17:
    compiler_warning(449);
    skip_declaration_tokens();
    if (c1_bss.source_cursor)
        ++c1_bss.source_cursor;
    else {
        c1_bss.parser_value.words.low = 0;
        c1_bss.parser_value.words.high = 0;
    }
    ClearRecovery();
    lookahead = NULL;
    break;
case 18:
#ifdef RECON_NATIVE_TEST
    if (declaration_depth != 0 || state_top[1] != 10 || state_top[2] != 27)
        recon_unsupported("function declaration semantic source states are not yet modeled");
#endif
    declaration_control[declaration_depth].flags &= ~3;
    lexer_flags &= ~0x1000000;
    right[0].value.declarator->ApplyDeclaration(&right[-1].value.declaration);
    right[0].value.declarator->Resolve();
    if (!lookahead)
        lookahead = get_next_token();
    else if (lookahead->kind == 6)
        lookahead = classify_identifier_token(lookahead);
    switch (lookahead->kind) {
    case 34:
    case 36:
        if (right[0].value.declarator_symbol.symbol &&
            right[0].value.declarator->parameters &&
            right[0].value.declarator->parameters->kind != 2) {
            compiler_warning(444, lookahead);
            c1_bss.current_context.saved_frame->stream->Unread((unsigned)lookahead);
            lookahead = (CompilerParserToken *)c1_data.predefined_tokens[127];
            c1_bss.parser_value.symbol = NULL;
            parser_flags |= 0x200;
            break;
        }
        /* fall through */
    case 117:
        prepare_function_definition(&right[-1].value.declaration,
            right[0].value.declarator_symbol.symbol, right[0].value.declarator);
        c1_bss.parser_value.symbol = right[0].value.declarator_symbol.symbol;
        break;
    case 6:
        symbol = right[0].value.declarator_symbol.symbol;
        if (!symbol) {
            compiler_warning(61, lookahead->value.name);
            lookahead = (CompilerParserToken *)c1_data.predefined_tokens[127];
            break;
        }
        if (right[0].value.declarator->flags & 8) {
            if (right[0].value.declarator->parameters->kind == 2)
                compiler_warning(501, 73, lookahead->value.name);
            else
                compiler_warning(280, lookahead->value.name);
        } else if (right[0].value.declarator->flags & 16) {
            compiler_warning(281, symbol, lookahead->value.name);
        } else {
            compiler_warning(282, symbol, lookahead->value.name);
        }
        lookahead = recover_parser_token(lookahead, 127, 117, 1);
        if (lookahead->kind == 117) {
            skip_declaration_tokens();
            ++c1_bss.source_cursor;
            lookahead = (CompilerParserToken *)c1_data.predefined_tokens[127];
        }
        c1_bss.parser_value.symbol = NULL;
        parser_flags |= 0x200;
        break;
    default:
        report_expected_parser_token(lookahead, 117);
        c1_bss.current_context.saved_frame->stream->Unread((unsigned)lookahead);
        lookahead = (CompilerParserToken *)c1_data.predefined_tokens[127];
        c1_bss.parser_value.symbol = NULL;
        parser_flags |= 0x200;
        break;
    }
    break;
case 20:
    parser_flags |= 0x40;
    break;
case 21:
case 131:
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 27:
    compiler_warning(146, 55, right[0].value.name);
    ClearRecovery();
    lookahead = NULL;
    break;
case 28:
    right[0].value.declarator->ApplyDeclaration(&declaration_specs[declaration_depth]);
    right[0].value.declarator->Resolve();
    add_old_style_parameter(&declaration_specs[declaration_depth],
        right[0].value.declarator_symbol.symbol, right[0].value.declarator);
    break;
case 29: {
    unsigned flags = ((c1_bss.parser_value.declaration_flags & ~5) | 2) & 7;
    c1_bss.parser_value.declaration.type = c1_data.builtin_types.type_0063;
    c1_bss.parser_value.words.low = 0;
    c1_bss.parser_value.words.high = 0;
    c1_bss.parser_value.declaration_flags = flags;
    lexer_flags |= 0x1000000;
    declaration_specs[declaration_depth] = c1_bss.parser_value.declaration;
    declaration_control[declaration_depth].flags &= ~3;
    break;
}
case 31:
#ifdef RECON_NATIVE_TEST
    if (declaration_depth != 0 || (lookahead && lookahead->kind == 118))
        recon_unsupported("nested declaration specification record is not yet modeled");
#endif
    if (lookahead && lookahead->kind == 118) {
        declaration_specs[declaration_depth - 1] = right[0].value.declaration;
        declaration_control[declaration_depth - 1].flags |= 1;
    } else {
        declaration_specs[declaration_depth] = right[0].value.declaration;
        declaration_control[declaration_depth].flags |= 1;
    }
    break;
case 32:
#ifdef RECON_NATIVE_TEST
    if (right[0].value.declaration.apply != 0)
        recon_unsupported("declaration member callback invocation is not yet modeled");
#endif
    if (right[0].value.declaration.apply != 0 && right[0].value.declaration.context)
        (c1_data.translation_unit->*right[0].value.declaration.apply)(
            right[0].value.declaration.context);
    break;
case 33: {
    /* RVA518a0 is the existing literal owner, overlaid as declaration scratch.
     * The original copies20 initialized bytes into it, then24 through the
     * token. Scratch+20 has no writer here; native holds before that copy. */
#ifdef RECON_NATIVE_TEST
    recon_unsupported("identifier declaration token requires wider semantic value");
#endif
    lexer_flags |= 0x80000000U;
    memcpy(&c1_bss.literal, &c1_bss.parser_value.declaration, 20);
    if (c1_bss.symbol_arena_kind) {
        storage = &c1_bss.arenas[c1_bss.symbol_arena_kind];
        temporary_token = (CompilerParserToken *)storage->cursor;
        storage->cursor += 32;
        if (storage->cursor > storage->committed_limit)
            storage->CommitExtent();
    } else {
        CompilerPool *pool = &c1_bss.pools[6];
        if (pool->remaining < 32)
            pool->Refill(32);
        temporary_token = (CompilerParserToken *)pool->cursor;
        pool->cursor += 32;
        pool->remaining -= 32;
    }
    if (temporary_token) {
        temporary_token->kind = 34;
        memcpy(&temporary_token->value, &c1_bss.literal, 24);
    }
    temporary_token = classify_identifier_token(temporary_token);
    lexer_flags &= ~0x80000000U;
    memcpy(&c1_bss.parser_value.declaration, &temporary_token->value, 20);
    break;
}
case 34:
    c1_bss.parser_value.declaration.type = parse_record_specifier();
    break;
case 35:
    c1_bss.parser_value.declaration.type = parse_enum_specifier();
    break;
case 36:
    if (right[0].value.declaration.apply)
        (c1_data.translation_unit->*right[0].value.declaration.apply)(
            right[0].value.declaration.context);
    finish_enum_specifier(right[0].value.declaration.context,
        right[0].value.declaration.type);
    break;
case 40:
    if (right[-1].value.declaration.apply)
        (c1_data.translation_unit->*right[-1].value.declaration.apply)(
            right[-1].value.declaration.context);
    break;
case 43:
    enumerator_value = next_enumerator_value(enumerator_value, right[0].value.name);
    add_enumerator(right[0].value.name, enumerator_context, enumerator_value);
    break;
case 44:
    add_enumerator(right[-2].value.name, enumerator_context, right[0].value.integer);
    if (!unevaluated_expression_depth && !type_description_depth)
        c1_bss.arenas[3].ClearUsed();
    break;
case 47:
    if (!unevaluated_expression_depth && !type_description_depth)
        c1_bss.arenas[3].ClearUsed();
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 48:
    add_record_member(&declaration_specs[declaration_depth], NULL, NULL);
    break;
case 50:
case 51:
case 52:
    if (rule == 52)
        report_language_diagnostic(1, 228);
    right[0].value.declarator->ApplyDeclaration(&declaration_specs[declaration_depth]);
    right[0].value.declarator->Resolve();
    add_record_member(&declaration_specs[declaration_depth],
        right[0].value.declarator_symbol.symbol, right[0].value.declarator);
    break;

case 53:
case 54:
    lookahead = recover_parser_token(lookahead, 127, 123, 1);
    if (lookahead->kind != 127) {
        c1_bss.current_context.saved_frame->stream->Unread((unsigned)lookahead);
        lookahead = (CompilerParserToken *)c1_data.predefined_tokens[127];
    }
    ClearRecovery();
    break;
case 55:
    c1_bss.parser_value.symbol->kind = 3;
    break;
case 56:
    c1_bss.parser_value.symbol->kind = 2;
    c1_bss.parser_value.symbol->bit_width = prepare_bitfield(c1_bss.parser_value.symbol,
        right[0].value.declarator, right[0].value.declarator_symbol.symbol);
    break;
case 57:
    symbol = make_abstract_declarator();
    symbol->binding.name = c1_data.builtin_names.alignment_member;
    storage = &c1_bss.arenas[3];
    declarator = (CompilerDeclarator *)storage->cursor;
    storage->cursor += 48;
    if (storage->cursor > storage->committed_limit)
        storage->CommitExtent();
    c1_bss.parser_value.declarator = declarator ? declarator->InitializeSymbol(symbol) : NULL;
    break;
case 61:
    report_language_diagnostic(1, 228);
    break;
case 62:
    right[0].value.declarator->ApplyDeclaration(&declaration_specs[declaration_depth]);
    right[0].value.declarator->Resolve();
    declare_initialized_symbol(&declaration_specs[declaration_depth],
        right[0].value.declarator_symbol.symbol, right[0].value.declarator);
    break;
case 63:
    if (!c1_bss.options.prototypes_only)
        apply_initializer(right[-1].value.symbol, right[0].value.node);
    if (!unevaluated_expression_depth && !type_description_depth)
        c1_bss.arenas[3].ClearUsed();
    c1_lexer_token_cursor = (BYTE *)c1_lexer_token_text;
    break;
case 64:
    if (!c1_bss.options.prototypes_only)
        finish_initializer(right[0].value.node, 0);
    break;
case 65:
    begin_aggregate_initializer(right[-5].value.node);
    break;
case 66:
    right[-1].value.declarator->ApplyDeclaration(&declaration_specs[declaration_depth]);
    right[-1].value.declarator->Resolve();
    right[-1].value.declarator_symbol.symbol->declaration_flags |= 4;
    c1_bss.parser_value.symbol = prepare_initializer_symbol(declare_initialized_symbol(
        &declaration_specs[declaration_depth], right[-1].value.declarator_symbol.symbol,
        right[-1].value.declarator));
    break;
case 67:
    c1_bss.parser_value.node = NULL;
    report_type_declaration_error(513, &declaration_specs[declaration_depth]);
    break;
case 70:
    if (!c1_bss.options.prototypes_only)
        finish_aggregate_initializer(right[0].value.node);
    if (!unevaluated_expression_depth && !type_description_depth)
        c1_bss.arenas[3].ClearUsed();
    c1_lexer_token_cursor = (BYTE *)c1_lexer_token_text;
    break;
case 71:
    if (!c1_bss.options.prototypes_only)
        begin_initializer_element();
    break;
case 72:
    if (!c1_bss.options.prototypes_only)
        finish_initializer_element();
    break;
case 73:
case 74:
    symbol = make_declarator_symbol(right[0].value.name);
    c1_bss.parser_value.declarator_symbol.symbol = symbol;
    storage = &c1_bss.arenas[3];
    declarator = (CompilerDeclarator *)storage->cursor;
    storage->cursor += 48;
    if (storage->cursor > storage->committed_limit)
        storage->CommitExtent();
    c1_bss.parser_value.declarator = declarator ? declarator->InitializeSymbol(symbol) : NULL;
    if (rule == 74)
        c1_bss.parser_value.declarator->ApplyAttributes(&right[-1].value.attributes);
    break;
case 75:
case 118: {
    CompilerDeclarationAttributes empty;
    /* Authentic temporary writes only the lowWORD and DWORD+4. */
    empty.flags = 0;
    empty.extra = 0;
    c1_bss.parser_value.declarator = right[0].value.declarator;
    c1_bss.parser_value.declarator_symbol.symbol = right[0].value.declarator_symbol.symbol;
    c1_bss.parser_value.declarator->MergeQualifiers(&empty, &right[-1].value.attributes);
    break;
}
case 76:
case 119:
    c1_bss.parser_value.declarator = right[0].value.declarator;
    c1_bss.parser_value.declarator_symbol.symbol = right[0].value.declarator_symbol.symbol;
    c1_bss.parser_value.declarator->MergeQualifiers(&right[-3].value.attributes,
        &right[-1].value.attributes);
    break;
case 77:
case 120:
    c1_bss.parser_value.declarator->AppendArrayBound(&right[-1].value.words.low);
    break;
case 78:
case 121:
    c1_bss.parser_value.declarator->AddFunction(right[0].value.parameters);
    break;
case 80:
case 104:
case 123:
    c1_bss.parser_value.declarator = right[-1].value.declarator;
    c1_bss.parser_value.declarator_symbol.symbol = right[-1].value.declarator_symbol.symbol;
    if (declaration_depth > 0)
        --declaration_depth;
    break;
case 81:
case 90:
    ++parameter_nesting;
    ++c1_bss.declaration_nesting;
    ++unevaluated_expression_depth;
    c1_data.translation_unit->EnterParameterScope(0);
    break;
case 82:
case 91:
    c1_bss.parser_value.parameters = right[-2].value.parameters;
    if (right[-1].value.symbol)
        c1_bss.parser_value.parameters->AppendParameter(right[-1].value.symbol);
    --parameter_nesting;
    --c1_bss.declaration_nesting;
    --unevaluated_expression_depth;
    if (rule == 82) {
        if (declaration_depth > 0)
            --declaration_depth;
        declaration_control[declaration_depth].flags &= ~3;
    } else if (declaration_depth > 0) {
        --declaration_depth;
    }
    break;
case 83:
case 84:
case 92:
case 93:
    storage = &c1_bss.arenas[3];
    parameters = (CompilerParameterDescription *)storage->cursor;
    storage->cursor += 12;
    if (storage->cursor > storage->committed_limit)
        storage->CommitExtent();
    if (parameters) {
        parameters->value = c1_data.translation_unit->current->parameter_value;
        parameters->kind = rule == 83 || rule == 92 ? 1 : 6;
        parameters->scope_active = 1;
    }
    c1_bss.parser_value.parameters = parameters;
    break;
case 86:
    c1_bss.parser_value.parameters = make_parameter_description(right[0].value.name);
    break;
case 88:
case 96:
    c1_data.translation_unit->current->Insert(c1_bss.parser_value.symbol);
    break;
case 89:
case 97:
    c1_bss.parser_value.symbol = NULL;
    ClearRecovery();
    lookahead = NULL;
    break;
case 98:
    symbol = right[0].value.declarator_symbol.symbol;
    symbol->kind = 11;
    symbol->intrinsic_flags = right[-1].value.words.low;
    if ((right[-1].value.words.low & 0x1000000) && right[-1].value.words.high)
        attach_declaration_context(symbol, (void *)right[-1].value.words.high);
    else
        symbol->identifier_flags &= ~1;
    right[0].value.declarator->ApplyDeclaration(&right[-1].value.declaration);
    right[0].value.declarator->Resolve();
    c1_bss.parser_value.symbol = declare_declarator(&right[-1].value.declaration,
        symbol, right[0].value.declarator);
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 99:
    c1_bss.parser_value.symbol = right[0].value.declarator_symbol.symbol;
    c1_bss.parser_value.symbol->kind = 12;
    c1_bss.parser_value.symbol->intrinsic_flags = right[-1].value.words.low;
    if ((right[-1].value.words.low & 0x1000000) && right[-1].value.words.high)
        attach_declaration_context(c1_bss.parser_value.symbol, (void *)right[-1].value.words.high);
    else
        c1_bss.parser_value.symbol->identifier_flags &= ~1;
    c1_bss.parser_value.symbol->binding.name = c1_data.builtin_names.formal_parameter;
    right[0].value.declarator->ApplyDeclaration(&right[-1].value.declaration);
    right[0].value.declarator->Resolve();
    declare_symbol(right[0].value.declarator, c1_bss.parser_value.symbol);
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 101:
    c1_bss.parser_value.attributes.flags = merge_declarator_qualifiers(
        c1_bss.parser_value.attributes.flags, right[0].value.attributes.flags);
    if (!c1_bss.parser_value.attributes.extra)
        c1_bss.parser_value.attributes.extra = right[0].value.attributes.extra;
    break;
case 102:
    report_language_diagnostic(1, 230);
    break;
case 105:
    c1_bss.parser_value.words.low = 0;
    c1_bss.parser_value.words.high = 0;
    break;
case 103:
case 107:
case 108:
    /* The grammar reaches these reductions only through the twelve named
     * qualifier-kind shifts; the token's semantic BYTE is its kind/tag. */
    for (i = 0; i < sizeof(parser_token_qualifiers) / sizeof(parser_token_qualifiers[0]); ++i)
        if (parser_token_qualifiers[i].kind == right[0].value.operation)
            break;
#ifdef RECON_NATIVE_TEST
    if (i == sizeof(parser_token_qualifiers) / sizeof(parser_token_qualifiers[0]))
        recon_unsupported("qualifier token outside recovered relations");
#endif
    qualifiers = parser_token_qualifiers[i].flags;
    if (qualifiers == 3)
        qualifiers = 0;
    if (rule == 108)
        c1_bss.parser_value.attributes.flags |= qualifiers;
    else {
        c1_bss.parser_value.attributes.flags = qualifiers;
        c1_bss.parser_value.attributes.extra = 0;
    }
    break;
case 109:
    c1_bss.parser_value.attributes.extra = (unsigned)find_based_declarator(right[0].value.name);
    c1_bss.parser_value.attributes.flags = c1_bss.parser_value.attributes.extra ? 3 : 0;
    break;
case 110:
    c1_bss.parser_value.attributes.flags = 1;
    c1_bss.parser_value.attributes.extra = 0;
    break;
case 111:
    c1_bss.parser_value.words.low = 0;
    c1_bss.parser_value.words.high = 0;
    ClearRecovery();
    lookahead = NULL;
    break;
case 114:
case 115:
case 116:
case 117:
    symbol = make_abstract_declarator();
    storage = &c1_bss.arenas[3];
    declarator = (CompilerDeclarator *)storage->cursor;
    storage->cursor += 48;
    if (storage->cursor > storage->committed_limit)
        storage->CommitExtent();
    c1_bss.parser_value.declarator = declarator ? declarator->InitializeSymbol(symbol) : NULL;
    if (rule == 115)
        c1_bss.parser_value.declarator->ApplyAttributes(&right[0].value.attributes);
    if (rule == 116)
        c1_bss.parser_value.declarator->AddFunction(right[0].value.parameters);
    if (rule == 117) {
        c1_bss.parser_value.declarator->ApplyAttributes(&right[-1].value.attributes);
        c1_bss.parser_value.declarator->AddFunction(right[0].value.parameters);
    }
    break;

case 125:
case 126:
    if (rule == 125)
        compiler_warning(59, 76, 55);
    name = c1_data.builtin_names.ellipsis;
    arena_kind = parameter_nesting ? 2 : 0;
    symbol = NULL;
    if (!arena_kind) {
        /* Provisional access spelling only: this is the existing head-table
         * owner, with only head9 currently known. Index/size11 remain held. */
        binding = c1_data.symbol_cache.indexed[parser_symbol_cache_index];
        if (binding) {
            c1_data.symbol_cache.indexed[parser_symbol_cache_index] = binding->next;
            symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
            binding->name = NULL;
            binding->next = NULL;
            memset(symbol, 0, c1_data.record_sizes.indexed[11]);
            binding->name = name;
            symbol->kind = 11;
        }
    }
    if (!symbol) {
        storage = &c1_bss.arenas[arena_kind];
        symbol = (CompilerIrSymbol *)storage->cursor;
        storage->cursor += (c1_data.record_sizes.indexed[11] + 3) & ~3U;
        if (storage->cursor > storage->committed_limit)
            storage->CommitExtent();
        if (symbol) {
            symbol->kind = 11;
            symbol->binding.name = name;
            symbol->binding.next = NULL;
        }
    }
    c1_bss.parser_value.symbol = symbol;
    symbol->value_type = NULL;
    symbol->flags |= 2;
    break;
case 127:
    if (!c1_bss.options.prototypes_only && !(lexer_flags & 0x800000)) {
        begin_compound_statement();
#ifdef RECON_NATIVE_TEST
        if (declaration_depth < 0 || declaration_depth > 1)
            recon_unsupported("compound declaration-control index is not modeled");
#endif
        declaration_control[declaration_depth].flags &= ~3;
    } else {
        lexer_flags |= 0x800000;
        skip_declaration_tokens();
    }
    break;
case 128:
    if (!c1_bss.options.prototypes_only && !(lexer_flags & 0x800000))
        finish_compound_statement();
    else
        lexer_flags &= ~0x800000;
    break;
case 133:
    process_declaration(&right[-1].value.declaration);
    break;
case 134:
case 163:
    ClearRecovery();
    break;
case 138:
    emit_expression(right[-1].value.node);
    if (!current_recovery_shifts())
        c1_bss.arenas[3].ClearUsed();
    c1_lexer_token_cursor = (BYTE *)c1_lexer_token_text;
    break;
case 140:
case 141:
    emit_ir_label(right[-1].value.node);
    break;
case 142:
    emit_branch(NULL, right[-1].value.node, NULL);
    if (declaration_depth > 0)
        --declaration_depth;
    break;
case 143:
case 146:
    begin_conditional_branch(lookahead);
    break;
case 144:
    emit_branch(right[-4].value.node, right[-2].value.node, right[0].value.node);
    break;
case 145:
    if (declaration_depth > 0)
        --declaration_depth;
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 147:
    prepare_for_statement();
    break;
case 148:
    emit_ir_label(continue_labels->node);
    if (right[-2].value.node)
        emit_conditional_branch(right[-2].value.node, loop_labels->node, 1);
    emit_control_record(36);
    emit_ir_label(break_labels->node);
    loop_labels = loop_labels->next;
    continue_labels = continue_labels->next;
    break_labels = break_labels->next;
    label_stack_count -= 3;
    if (!label_stack_count)
        c1_bss.arenas[4].ClearUsed();
    if (!current_recovery_shifts())
        c1_bss.arenas[3].ClearUsed();
    c1_lexer_token_cursor = (BYTE *)c1_lexer_token_text;
    if (declaration_depth > 0)
        --declaration_depth;
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 149:
    emit_loop_condition(right[-1].value.node);
    if (declaration_depth > 0)
        --declaration_depth;
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 150:
    emit_control_record(36);
    write_ir_record(58, break_labels->node);
    finish_switch_statement();
    loop_labels = loop_labels->next;
    break_labels = break_labels->next;
    switch_labels = switch_labels->next;
    default_labels = default_labels->next;
    label_stack_count -= 4;
    if (!label_stack_count)
        c1_bss.arenas[4].ClearUsed();
    break;
case 151:
    emit_leave_statement();
    break;
case 152:
    if (break_labels)
        write_ir_record(58, break_labels->node);
    else
        compiler_warning(43);
    break;
case 153:
    if (continue_labels)
        write_ir_record(58, continue_labels->node);
    else
        compiler_warning(44);
    break;
case 154:
    emit_return_statement(NULL, 1);
    break;
case 155:
    emit_return_statement(right[-1].value.node, 1);
    if (!current_recovery_shifts())
        c1_bss.arenas[3].ClearUsed();
    c1_lexer_token_cursor = (BYTE *)c1_lexer_token_text;
    break;
case 156:
    label_symbol = find_statement_label(right[-1].value.name);
    if (!label_symbol) {
        label_symbol = make_statement_label(right[-1].value.name);
        record_label_reference(label_symbol);
    }
    write_ir_record(58, make_label_expression(label_symbol, 0));
    if (!current_recovery_shifts())
        c1_bss.arenas[3].ClearUsed();
    c1_lexer_token_cursor = (BYTE *)c1_lexer_token_text;
    break;
case 157:
    parse_inline_assembly();
    break;
case 158:
    finish_finally_handler();
    break;
case 159:
    begin_exception_filter();
    break;
case 160:
    finish_exception_filter();
    break;
case 162:
    compiler_warning(181);
    break;
case 164:
    begin_exception_handler();
    break;
case 165:
    finish_exception_handler();
    break;
case 166:
    set_exception_filter_mode();
    break;
case 167:
    emit_exception_filter(right[-1].value.node);
    clear_exception_filter_mode();
    if (declaration_depth > 0)
        --declaration_depth;
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 168:
    case_label = NULL;
    break;
case 170:
    define_statement_label(right[-1].value.name);
    break;
case 171:
    if (right[-1].value.node && switch_labels)
        emit_case_label(right[-1].value.node);
    else
        compiler_warning(46);
    break;
case 172:
    if (!default_labels) {
        compiler_warning(47);
    } else if (default_labels->node) {
        compiler_warning(48);
    } else if (!case_label) {
        default_labels->node = make_internal_label(20);
        case_label = default_labels->node;
        emit_ir_label(default_labels->node);
    } else {
        default_labels->node = case_label;
    }
    break;
case 173:
    c1_bss.parser_value.node = make_internal_label(30);
    if (right[-1].value.node)
        emit_conditional_branch(right[-1].value.node, c1_bss.parser_value.node, 0);
    if (declaration_depth > 0)
        --declaration_depth;
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 174:
    c1_bss.parser_value.node = make_internal_label(30);
    write_ir_record(58, c1_bss.parser_value.node);
    emit_ir_label(right[-2].value.node);
    emit_control_record(36);
    declaration_control[declaration_depth].flags &= ~3;
    break;

case 175:
    c1_bss.parser_value.words.low = 0;
    c1_bss.parser_value.words.high = 0;
    break;
case 176:
    if (c1_constant_diagnostic_mode < 0 || !c1_data.options.language_extensions) {
        if (!(right[0].value.words.low | right[0].value.words.high))
            compiler_warning(466);
    }
    break;
case 177:
    c1_bss.parser_value.integer = constant_integer_value(right[0].value.node);
    break;
case 124:
case 178:
    c1_bss.parser_value.node = NULL;
    break;
case 180:
case 181:
case 182:
case 183:
case 184:
case 185:
case 186:
case 187:
case 188:
case 189:
case 190:
case 191:
case 192:
case 193:
case 194:
    c1_bss.parser_value.node = make_binary_expression(right[-1].value.operation,
        right[-2].value.node, right[0].value.node);
    break;
case 195:
    c1_bss.parser_value.node = make_binary_expression(right[-3].value.operation,
        right[-4].value.node, make_binary_expression(right[-1].value.operation,
            right[-2].value.node, right[0].value.node));
    break;
case 197:
    constant_one.low = 1;
    constant_one.high = 0;
    c1_bss.parser_value.node = make_binary_expression(right[0].value.operation,
        right[-1].value.node, make_typed_constant(c1_data.builtin_types.type_0063,
            &constant_one, c1_data.expression_arena_kind));
    break;
case 198:
    c1_bss.parser_value.node = make_unary_expression(134, right[0].value.node);
    break;
case 199:
    c1_bss.parser_value.node = make_unary_expression(90, right[0].value.node);
    break;
case 200:
    c1_bss.parser_value.node = make_unary_expression(89, right[0].value.node);
    break;
case 201:
    c1_bss.parser_value.node = make_unary_expression(60, right[0].value.node);
    break;
case 202:
    constant_one.low = 1;
    constant_one.high = 0;
    right[-1].value.operation = right[-1].value.operation == 64 ? 85 : 84;
    c1_bss.parser_value.node = make_binary_expression(right[-1].value.operation,
        right[0].value.node, make_typed_constant(c1_data.builtin_types.type_0063,
            &constant_one, c1_data.expression_arena_kind));
    break;
case 203:
    c1_bss.parser_value.node = make_unary_expression(118, right[0].value.node);
    if (declaration_depth > 0)
        --declaration_depth;
    break;
case 204:
    c1_bss.parser_value.node = make_unary_expression(61, right[0].value.node);
    break;
case 205:
    c1_bss.parser_value.node = make_unary_expression(49, right[0].value.node);
    break;
case 206:
    if (right[0].value.node)
        c1_bss.parser_value.node = make_sizeof_expression(right[0].value.node);
    else
        c1_bss.parser_value.node = NULL;
    --unevaluated_type_depth;
    break;
case 207:
    c1_bss.parser_value.node = make_type_size_expression(right[0].value.symbol->value_type);
    --unevaluated_type_depth;
    break;
case 210:
    c1_bss.parser_value.node = make_right_typed_expression(100,
        right[-1].value.node, right[0].value.node);
    break;
case 211:
    if (right[-1].value.node->kind == 55)
        report_language_diagnostic(4, 709);
    c1_bss.parser_value.node = make_binary_expression(right[-2].value.operation,
        right[-3].value.node, right[-1].value.node);
    break;
case 212:
    c1_bss.parser_value.node = finish_argument_list(c1_bss.parser_value.node);
    if (declaration_depth > 0)
        --declaration_depth;
    break;
case 213:
    if (c1_bss.parser_value.node)
        c1_bss.parser_value.node->argument = right[-1].value.node;
    c1_bss.parser_value.node = finish_argument_list(c1_bss.parser_value.node);
    if (declaration_depth > 0)
        --declaration_depth;
    break;
case 214:
    c1_data.translation_unit->lookup_mode = 1;
    break;
case 215:
    c1_bss.parser_value.node = make_member_expression(right[-2].value.operation,
        right[-3].value.node, right[0].value.node);
    c1_data.translation_unit->lookup_mode = 0;
    break;
case 216:
    c1_bss.parser_value.node = make_typed_unary_expression(right[-2].value.type,
        right[0].value.node);
    break;
case 217:
    c1_bss.parser_value.node = make_identifier_expression(right[0].value.name);
    break;
case 218:
    c1_bss.parser_value.node = make_typed_constant(right[0].value.typed_constant.type,
        &right[0].value.typed_constant.value, c1_data.expression_arena_kind);
    break;
case 219:
    process_version_literal((CompilerVersionRecord *)right[0].value.node);
    break;
case 220: {
    CompilerArena *arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    CompilerParserNode *record = (CompilerParserNode *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    c1_bss.parser_value.node = record;
    record->flags = 5;
    record->kind = 7;
    record->text = right[0].value.string.text;
    record->length = right[0].value.string.length;
    break;
}
case 221:
    append_literal_text(&right[-1].value.node->text, &right[0].value);
    break;
case 222:
    ++unevaluated_expression_depth;
    break;
case 223:
    c1_bss.parser_value.node = right[-1].value.node;
    if (declaration_depth > 0)
        --declaration_depth;
    break;
case 224:
    c1_bss.parser_value.symbol = right[0].value.declarator_symbol.symbol;
    right[0].value.declarator->ApplyDeclaration(&right[-1].value.declaration);
    right[0].value.declarator->Resolve();
    declare_symbol(right[0].value.declarator, c1_bss.parser_value.symbol);
    --unevaluated_expression_depth;
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 225:
    c1_bss.parser_value.node = make_based_expression(right[-1].value.node);
    break;
case 226:
    c1_bss.parser_value.node = normalize_based_expression(right[-1].value.node);
    break;
case 227:
    c1_bss.parser_value.node = make_expression_with_type(NULL, right[0].value.node, NULL);
    break;
case 228:
    c1_bss.parser_value.node = make_expression_with_type(right[-2].value.node,
        right[0].value.node, NULL);
    break;

default:
    /* The remaining productions, including229/230, have no action. */
    break;
}

push_state:
        ;
    }
}


/* Genuine child boundaries. Their bodies remain pending until reached. */
extern "C" {
/* RVA02b76 */
CompilerParserToken *__cdecl resolve_identifier_type(CompilerParserToken *);
void __cdecl finish_token_input(void);
CompilerParserToken *__cdecl get_next_token(void)
{
    CompilerParserToken *token = c1_bss.current_context.ReadToken();
    CompilerIrSymbol *symbol;
    unsigned token_class;
    BYTE flags;
    for (;;) {
#ifdef RECON_NATIVE_TEST
        if (token->kind >= PARSER_TOKEN_KIND_COUNT)
            recon_unsupported("token kind is outside the known parser grammar");
#endif
        token_class = parser_token_classes[token->kind];
#ifdef RECON_NATIVE_TEST
        /* These keyword classes go directly to the genuine classifier,
         * without touching the source-only declaration stack below. */
        if (token_class != PARSER_CLASS_0 && token_class != PARSER_CLASS_123 && token_class != PARSER_CLASS_127 && token_class != PARSER_CLASS_101 && token_class != PARSER_RETURN && token_class != PARSER_CLASS_118 && token_class != PARSER_CLASS_126 &&
            token_class != PARSER_CLASS_117 &&
            token_class != PARSER_CLASS_6 &&
            token_class != PARSER_CLASS_27 && token_class != PARSER_CLASS_28 &&
            token_class != PARSER_CLASS_29 && token_class != PARSER_CLASS_31 &&
            token_class != PARSER_CLASS_32 && token_class != PARSER_CLASS_33 &&
            token_class != PARSER_CLASS_40 && token_class != PARSER_CLASS_192)
            recon_unsupported("declaration token classification state is not yet modeled");
#endif
        switch (token_class) {
        case PARSER_CLASS_126:
            lexer_flags &= ~1U;
            if (declaration_depth || (lexer_flags & 0x10000000))
                return token;
            compiler_warning(59, 76, token->kind);
            token = c1_bss.current_context.ReadToken();
            continue;
        case PARSER_CLASS_6:
            lexer_flags |= 0x400;
            token = resolve_identifier_type(token);
            lexer_flags &= ~0x400U;
            if (token->kind == 30)
                return classify_identifier_token(token);
            if (!(lexer_flags & 0x10000000)) {
#ifdef RECON_NATIVE_TEST
                if (declaration_depth != 0)
                    recon_unsupported("identifier declaration control index is not yet modeled");
#endif
                declaration_control[declaration_depth].flags |= 2;
            }
            return token;
        case PARSER_CLASS_27:
        case PARSER_CLASS_28:
        case PARSER_CLASS_29:
        case PARSER_CLASS_31:
        case PARSER_CLASS_32:
        case PARSER_CLASS_33:
        case PARSER_CLASS_40:
        case PARSER_CLASS_192:
            return classify_identifier_token(token);
        case PARSER_CLASS_36:
            flags = declaration_control[declaration_depth].flags;
            if ((flags & 2) && (lexer_flags & 0x08000000) &&
                token->value.declaration.context) {
                symbol = (CompilerIrSymbol *)token->value.declaration.context;
                c1_bss.literal.identifier = symbol->binding.name;
                if (!c1_bss.symbol_arena_kind)
                    token = (CompilerParserToken *)c1_bss.pools[6].AllocateBytes(32);
                else
                    token = (CompilerParserToken *)c1_bss.arenas[
                        c1_bss.symbol_arena_kind].AllocateBytes(32);
                if (token) {
                    token->kind = 6;
                    memcpy(&token->value, &c1_bss.literal, 24);
                }
                return token;
            }
            lexer_flags |= 1;
            if (!(lexer_flags & 0x10000000))
                declaration_control[declaration_depth].flags |= 2;
            return token;
        case PARSER_BASED:
        case PARSER_CLASS_49:
        case PARSER_CLASS_60:
        case PARSER_CLASS_70:
        case PARSER_CLASS_74:
        case PARSER_CLASS_81:
        case PARSER_CLASS_89:
        case PARSER_CLASS_101:
        case PARSER_CLASS_115:
        case PARSER_CALLING_CONVENTION:
            if (!(lexer_flags & 0x10000000)) {
#ifdef RECON_NATIVE_TEST
                if (declaration_depth < 0 || declaration_depth > 1)
                    recon_unsupported("literal declaration-control index is not modeled");
#endif
                declaration_control[declaration_depth].flags |= 2;
            }
            return token;
        case PARSER_CLASS_118:
            if (!(lexer_flags & 0x10000000)) {
#ifdef RECON_NATIVE_TEST
                if (declaration_depth != 0)
                    recon_unsupported("nested declaration entry is not yet modeled");
#endif
                if (++declaration_depth >= 60)
                    compiler_fatal(13);
                declaration_contexts[declaration_depth] = NULL;
                declaration_control[declaration_depth].flags &= 0xf0;
            }
            lexer_flags &= ~1U;
            return token;
        case PARSER_RETURN:
#ifdef RECON_NATIVE_TEST
            if (declaration_depth < 0 || declaration_depth > 1)
                recon_unsupported("return declaration-control index is not modeled");
#endif
            if (!(lexer_flags & 0x10000000))
                declaration_control[declaration_depth].flags |= 3;
            return token;
        case PARSER_SIZEOF:
            ++unevaluated_type_depth;
            if (!(lexer_flags & 0x10000000))
                declaration_control[declaration_depth].flags |= 2;
            return token;
        case PARSER_CLASS_0:
        case 109: /* Original dispatch also recognizes this unmapped class. */
            if (final_input_token_emitted) {
                finish_token_input();
                return (CompilerParserToken *)c1_data.predefined_tokens[109];
            }
            final_input_token_emitted = 1;
            return (CompilerParserToken *)c1_data.predefined_tokens[5];
        default:
            return token;
        }
    }
}
/* RVA03fc2 */

/* RVA05ae5 */
CompilerParserToken * __cdecl classify_identifier_token(CompilerParserToken *token)
{
    CompilerDeclarationScanner scanner;
    CompilerParserToken *result;
    memset(&scanner, 0, sizeof(scanner));
    scanner.token = token;
    result = scanner.Scan();
    if (result->kind == 36 && !(lexer_flags & 0x10000000)) {
#ifdef RECON_NATIVE_TEST
        if (declaration_depth != 0)
            recon_unsupported("declaration control index is not yet modeled");
#endif
        declaration_control[declaration_depth].flags |= 2;
    }
    return result;
}
/* RVA05f15 */
/* RVA06928 */
void __cdecl print_compiler_statistics(void);
int __cdecl current_recovery_shifts(void);
void __cdecl finish_external_declaration(int)
{
    if (c1_data.report_statistics) print_compiler_statistics();
    if (c1_bss.current_context.saved_frame->stream->Valid() && !current_recovery_shifts())
        c1_bss.arenas[2].ClearUsed();
    if (!current_recovery_shifts()) c1_bss.arenas[3].ClearUsed();
    lexer_flags &= ~0x1000000U;
    parser_flags &= ~0x600U;
    c1_lexer_token_cursor = (BYTE *)c1_lexer_token_text;
    c1_bss.current_scope_symbol = NULL;
    parameter_scope_initial_value = 0;
    parameter_scope_value = 0;
#ifdef RECON_NATIVE_TEST
    if (declaration_depth < 0 || declaration_depth > 1)
        recon_unsupported("external-declaration control index is not yet modeled");
#endif
    declaration_control[declaration_depth].flags &= ~3;
    parameter_storage_class = 0;
    function_storage_class = 0;
    if (parser_early_result) {
        c1_data.translation_unit->depth = 0;
        c1_data.translation_unit->current = c1_data.translation_unit->root;
        declaration_depth = 0;
        c1_bss.declaration_scope_active = 0;
        function_label_depth = 0;
    }
}
/* RVA069d9 */
int __cdecl current_recovery_shifts(void)
{
    return c1_data.active_parser->recovery_shifts;
}
/* RVA072d8 */
/* Source-only mask identities for the floating branch. Neither DWORD nor
 * any wider mask contents/default/extent is interpreted by native code. */
static unsigned single_literal_mask_identity; /* RVA5f3b8 */
static unsigned double_literal_mask_identity; /* RVA5f3c8 */
int __cdecl classify_literal_bits(unsigned, unsigned, const void *);
CompilerType *__cdecl copy_type_description(const CompilerType *);
CompilerType *__cdecl intern_type(CompilerType *);
CompilerParserNode *__cdecl make_typed_constant(CompilerType *type, const void *value, unsigned arena_kind)
{
#ifdef RECON_NATIVE_TEST
    if (arena_kind >= 6 || !(type->flags & 0x40))
        recon_unsupported("typed literal arena or noninteger payload is not yet modeled");
#endif
    CompilerArena *arena = &c1_bss.arenas[arena_kind];
    CompilerParserNode *node = (CompilerParserNode *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    node->flags = 3;
    node->kind = 101;
    if (type->flags & 0x40) {
        memcpy(&node->low, value, 8);
        if (!node->low && !node->high) node->flags = 0x83;
    } else if (type->flags & 0x100) {
        node->low = *(const unsigned *)value;
        if (classify_literal_bits(6, node->low, &single_literal_mask_identity) ||
            classify_literal_bits(6, node->low, &double_literal_mask_identity))
            node->classification_flags |= 1;
    }
    CompilerType *description = copy_type_description(type);
    description->attributes = 0x100;
    node->type = intern_type(description);
    --type_description_depth;
    return node;
}
CompilerType *__cdecl copy_type_description(const CompilerType *type)
{
#ifdef RECON_NATIVE_TEST
    if (type_description_depth != 0)
        recon_unsupported("nested type-description copy is not yet modeled");
#endif
    ++type_description_depth;
    CompilerType *description = &c1_bss.type_descriptions[type_description_depth];
#ifdef RECON_NATIVE_TEST
    /* Preserve the complete sixteen-byte storage, including the separately
     * maintained cache. Its bytes need not be initialized on this path and
     * are not part of the twelve-byte interning key. */
    memcpy(description, type, sizeof(*description));
#else
    *description = *type;
#endif
    return description;
}
int __cdecl classify_literal_bits(unsigned, unsigned, const void *) { __asm { int 3 } return 0; }
/* RVA074b5 */
CompilerIrSymbol * __cdecl make_declarator_symbol(CompilerNameEntry *name)
{
    unsigned arena_kind = parameter_nesting ? 2 : 0;
    CompilerIrSymbol *symbol = NULL;
    CompilerSymbolBinding *binding;
    CompilerArena *arena;
    BYTE *memory;
#ifdef RECON_NATIVE_TEST
    if (parameter_nesting || c1_data.declaration_symbol_cache_index != 9 ||
        c1_data.record_sizes.known.declaration_symbol_bytes != 52)
        recon_unsupported("declarator symbol size/cache/parameter domain is not yet modeled");
#endif
    if (!arena_kind) {
        binding = c1_data.symbol_cache.indexed[c1_data.declaration_symbol_cache_index];
        if (binding) {
            symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
            c1_data.symbol_cache.indexed[c1_data.declaration_symbol_cache_index] = binding->next;
            binding->name = NULL;
            binding->next = NULL;
            memset(symbol, 0, c1_data.record_sizes.known.declaration_symbol_bytes);
            binding->name = name;
            symbol->kind = 1;
        }
    }
    if (!symbol) {
        arena = &c1_bss.arenas[arena_kind];
        memory = arena->cursor;
        arena->cursor += (c1_data.record_sizes.known.declaration_symbol_bytes + 3) & ~3U;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(1, name, arena_kind) : NULL;
    }
#ifdef RECON_NATIVE_TEST
    if (c1_data.type_arena_kind >= 6)
        recon_unsupported("declarator scratch arena is not modeled");
#endif
    arena = &c1_bss.arenas[c1_data.type_arena_kind];
    memory = arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    /* Authentic16-byte scratch allocation. Its fields are not written here. */
    symbol->abstract_type = (CompilerAbstractType *)memory;
    return symbol;
}
/* RVA07598 */
CompilerIrSymbol * __cdecl declare_initialized_symbol(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *) { __asm { int 3 } return 0; }
/* RVA076c1 */
/* RVA0b5ae */
__int64 __cdecl constant_integer_value(CompilerParserNode *) { __asm { int 3 } return 0; }
/* RVA0ba0b */
void __cdecl finish_enum_specifier(void *, CompilerType *) { __asm { int 3 } }
/* RVA0bb41 */
void __cdecl add_record_member(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *) { __asm { int 3 } }
/* RVA0c0a9 */
CompilerType * __cdecl parse_record_specifier(void) { __asm { int 3 } return 0; }
/* RVA0ca41 */
WORD __cdecl merge_declarator_qualifiers(WORD, WORD) { __asm { int 3 } return 0; }
/* RVA0d706 */
void __cdecl process_declaration(const CompilerParserDeclaration *) { __asm { int 3 } }
/* RVA0e4d2 */
BYTE __cdecl prepare_bitfield(CompilerIrSymbol *, CompilerDeclarator *, CompilerIrSymbol *) { __asm { int 3 } return 0; }
/* RVA0eed5 */
void __cdecl add_enumerator(CompilerNameEntry *, void *, __int64) { __asm { int 3 } }
/* RVA0f092 */
CompilerType * __cdecl parse_enum_specifier(void) { __asm { int 3 } return 0; }
/* RVA0f123 */
__int64 __cdecl next_enumerator_value(__int64, CompilerNameEntry *) { __asm { int 3 } return 0; }
/* RVA10519 */
CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
CompilerParserNode *__cdecl make_symbol_reference(CompilerIrSymbol *, unsigned);
CompilerParserNode *__cdecl make_typed_binary_expression(unsigned, CompilerParserNode *, CompilerParserNode *, CompilerType *);
CompilerParserNode * __cdecl make_label_expression(CompilerIrSymbol *symbol, int)
{
    if (!symbol) return NULL;
#ifdef RECON_NATIVE_TEST
    if (c1_data.expression_arena_kind >= 6)
        recon_unsupported("label expression allocation arena is not yet modeled");
#endif
    CompilerArena *arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    CompilerParserNode *node = (CompilerParserNode *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    node->kind = 6;
    node->flags = 4;
    node->symbol = symbol;
    if (symbol->flags & 4) {
        compiler_warning(275, symbol);
        return NULL;
    }
    node->type = symbol->value_type;
    if (!(symbol->function_flags & 1) || symbol->kind == 7 || symbol->kind == 8 ||
        lookup_ir_symbol(symbol)->kind == 14)
        return node;
    return make_typed_binary_expression(93, node,
        make_symbol_reference(lookup_ir_symbol(symbol), 0), node->type);
}
CompilerParserNode *__cdecl make_symbol_reference(CompilerIrSymbol *, unsigned)
{ __asm { int 3 } return NULL; }
CompilerParserNode *__cdecl make_typed_binary_expression(unsigned code, CompilerParserNode *left,
    CompilerParserNode *right, CompilerType *type)
{
#ifdef RECON_NATIVE_TEST
    if (c1_data.expression_arena_kind >= 6)
        recon_unsupported("binary expression allocation arena is not yet modeled");
#endif
    CompilerArena *arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    CompilerParserNode *node = (CompilerParserNode *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    node->kind = (BYTE)code;
    node->flags = 2;
    node->type = type;
    node->left = left;
    node->right = right;
    return node;
}
/* RVA1066f */
CompilerParserNode * __cdecl make_internal_label(unsigned kind)
{
    CompilerArena *arena = &c1_bss.arenas[2];
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)arena->cursor;
    arena->cursor += (c1_data.record_sizes.known.global_symbol_bytes + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (symbol) symbol = symbol->Initialize(9, NULL, 2);
    symbol->symbol_value = kind;
    symbol->value_type = c1_data.builtin_types.type_0063;
    record_label_reference(symbol);
    CompilerParserNode *node = (CompilerParserNode *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    node->flags = 4;
    node->kind = 6;
    node->symbol = symbol;
    node->type = symbol->value_type;
    return node;
}
/* RVA1070a */
void __cdecl record_label_reference(CompilerIrSymbol *symbol)
{
    symbol->scope_bits |= 2;
    if (!(symbol->record_id & 0xffffff)) {
        if (c1_data.next_symbol_record_id >= 0x7fffffff) compiler_fatal(55);
        symbol->record_id = (symbol->record_id & 0xff000000) |
            (c1_data.next_symbol_record_id++ & 0xffffff);
    }
    symbol->binding.next = c1_data.label_symbols ? &c1_data.label_symbols->binding : NULL;
    c1_data.label_symbols = symbol;
}
/* RVA10af9 */
void __cdecl begin_compound_statement(void)
{
    c1_data.backend->Slot0c();
    if (parameter_nesting >= 249) compiler_fatal(61);
    ++parameter_nesting;
    ++function_label_depth;
    c1_bss.declaration_scope_active = parameter_nesting;
    if (c1_data.debug_module_kind != 4) {
        if (!(lexer_flags & 0x20)) c1_data.translation_unit->EnterBlockScope();
        if (!c1_bss.function_output_mode) write_ir_record(83, NULL);
    }
}
/* RVA10bb3 */
void __cdecl finish_scope_symbols(void);
CompilerIntegerConstant *__cdecl make_integer_constant(int);
void __cdecl finish_compound_statement(void)
{
    CompilerScopeTable *scope;
    if (!parameter_nesting) return;
    c1_data.backend->Slot08();
    if (parameter_nesting == 2) function_label_depth = 0;
    if (!(lexer_flags & 0x20)) {
        if (c1_data.debug_module_kind == 4) {
            if (parameter_nesting == 1) c1_data.translation_unit->UnregisterScope();
        } else {
            scope = c1_data.translation_unit->current;
            if (scope->level != parameter_nesting) compiler_fatal(506);
            c1_data.translation_unit->UnregisterScope();
            finish_scope_symbols();
        }
    }
    if (!c1_bss.function_output_mode && c1_data.debug_module_kind != 4) {
        if (!(lexer_flags & 0x20)) c1_data.translation_unit->FinishScope(scope);
        write_ir_record(84, make_integer_constant(parameter_nesting));
    }
    --parameter_nesting;
    c1_bss.declaration_scope_active = parameter_nesting;
}
/* Source-only alternating depth/symbol frames. The encountered head is
 * NULL; no record allocation, wider list or payload default is modeled. */
struct CompilerPendingScopeSymbol {
    CompilerPendingScopeSymbol *next;
    union { BYTE depth; CompilerIrSymbol *symbol; };
};
void __cdecl pop_scope_symbol_frame(CompilerPendingScopeSymbol **);
void __cdecl finish_scope_symbols(void)
{
    if (c1_bss.function_output_mode) return;
#ifdef RECON_NATIVE_TEST
    if (c1_bss.pending_scope_symbols)
        recon_unsupported("pending scope-symbol records are not yet modeled");
#endif
    while (c1_bss.pending_scope_symbols &&
           c1_bss.pending_scope_symbols->depth == parameter_nesting) {
        pop_scope_symbol_frame(&c1_bss.pending_scope_symbols);
        write_ir_record(43, make_label_expression(c1_bss.pending_scope_symbols->symbol, 0));
        pop_scope_symbol_frame(&c1_bss.pending_scope_symbols);
    }
}
void __cdecl pop_scope_symbol_frame(CompilerPendingScopeSymbol **) { __asm { int 3 } }
void RECON_THISCALL CompilerTranslationUnit::FinishScope(CompilerScopeTable *scope)
{
    if (!scope) return;
#ifdef RECON_NATIVE_TEST
    if (retired_scopes && (retired_scopes->next || retired_scopes->level != 2 ||
        scope->level != 1 || retired_scopes->outer != scope))
        recon_unsupported("wider retired scope ordering is not yet modeled");
#endif
    CompilerScopeTable *next = retired_scopes, *previous = NULL;
    while (next && next->level <= scope->level) {
        previous = next;
        next = next->next;
    }
    while (next && next->outer != scope) {
        previous = next;
        next = next->next;
    }
    if (previous) previous->next = scope;
    else retired_scopes = scope;
    scope->next = next;
}
/* RVA10d3c */
void __cdecl emit_ir_label(CompilerParserNode *node)
{
    CompilerIrSymbol *symbol = node->symbol;
#ifdef RECON_NATIVE_TEST
    if (symbol->kind != 9 || c1_data.try_depth || c1_data.finally_depth ||
        c1_data.exception_filter_depth)
        recon_unsupported("IR label symbol or exception-region domain is not yet modeled");
#endif
    symbol->flags |= 1;
    symbol->label_nesting = parameter_nesting;
    int depth = c1_data.try_depth + c1_data.finally_depth + c1_data.exception_filter_depth;
    if ((symbol->reference_flags & 3) && symbol->label_exception_depth < depth)
        compiler_warning(705, 83, symbol);
    symbol->label_exception_depth = (BYTE)depth;
    write_ir_record(41, node);
}
/* RVA10e70. Later function-state cells have no native contract yet; the
 * actual first call remains the ordinary declaration boundary. */
void __cdecl abandon_function_definition(void);
CompilerIrSymbol *__cdecl initialize_function_definition(CompilerIrSymbol *);
void __cdecl apply_function_context(CompilerIrSymbol *, void *);
unsigned __cdecl global_type_size(CompilerType *);
void __cdecl prepare_function_definition(const CompilerParserDeclaration *declaration,
    CompilerIrSymbol *symbol, CompilerDeclarator *declarator)
{
    symbol->flags |= 1;
    symbol = declare_declarator(declaration, symbol, declarator);
#ifdef RECON_NATIVE_TEST
    if (symbol->kind != 7 || !(symbol->declaration_flags & 0x80) ||
        parameter_storage_class != 1 || parameter_scope_initial_value != 0 || function_context)
        recon_unsupported("function-definition context or parameter domain is not yet modeled");
#endif
    if (symbol->kind != 7 && symbol->kind != 8) {
        if (symbol->kind == 6) compiler_warning(169, symbol);
        else compiler_warning(54, 83, symbol);
        symbol->scope_bits |= 1;
        abandon_function_definition();
        return;
    }
    if (!(symbol->declaration_flags & 0x80)) {
        compiler_warning(206, symbol);
        parameter_scope_initial_value = 0;
    } else if (parameter_storage_class == 4) compiler_warning(55);
    if ((symbol->linkage_flags & 8) && !(symbol->symbol_qualifiers & 1) &&
        !(lexer_flags & 0x200)) compiler_warning(491, symbol, 374);
    if (function_context) apply_function_context(symbol, function_context);
    c1_bss.current_scope_symbol = initialize_function_definition(symbol);
    parameter_scope_value = parameter_scope_initial_value;
    function_storage_class = parameter_storage_class;
    if (function_return_type->flags != 0x80 && !global_type_size(function_return_type))
        compiler_warning(79, c1_bss.current_scope_symbol,
            function_return_type, function_return_type->auxiliary);
}
/* The genuine later child entries remain pending. */
void __cdecl abandon_function_definition(void) { __asm { int 3 } }
void __cdecl reset_local_symbol_limit(void);
void __cdecl reset_function_label_state(void);
CompilerIrSymbol *__cdecl initialize_function_definition(CompilerIrSymbol *symbol)
{
    reset_local_symbol_limit();
    c1_data.ir_symbol_state_ready = function_reference_epoch;
    c1_data.ir_symbol_state_changed = 0;
    c1_disable_inline_records = 0;
    function_return_type = symbol->function_type->FunctionResult();
    parser_flags &= ~0x800U;
    if (!(lexer_flags & 0x20))
        c1_data.translation_unit->EnterParameterScope(parameter_scope_initial_value);
    function_reset_value = 0;
    c1_bss.declaration_scope_active = 1;
    parameter_nesting = 1;
    function_label_depth = 1;
    c1_nonil_type_index = c1_bss.current_context.value;
    reset_function_label_state();
    return symbol;
}
void __cdecl reset_local_symbol_limit(void)
{
    c1_data.local_symbol_limit = 0x3fffff;
}
void __cdecl reset_function_label_state(void)
{
    c1_data.function_label_state_first = -1;
    c1_data.function_label_state_second = -1;
}
void __cdecl apply_function_context(CompilerIrSymbol *, void *) { __asm { int 3 } }
/* RVA10ffa */
/* These contextual cells are held until their actual function-body uses. */
#define function_stack_check_reported c1_bss.function_stack_check_reported /* RVA53d68, BSS */
#define function_output_mode c1_bss.function_output_mode /* RVA54c88, predicate-only DWORD */
#define function_debug_info c1_data.options.debug_info /* RVA5ef38, existing owner */
void __cdecl initialize_function_parameters(void);
unsigned __cdecl parameter_stack_bytes(unsigned);
int __cdecl compare_debug_positions(unsigned, unsigned);
void __cdecl begin_browser_function(void);
int __cdecl compatible_parameter_types(CompilerType *, CompilerType *);
CompilerType *__cdecl replace_function_parameters(CompilerType *, unsigned, WORD, unsigned *);
void __cdecl save_type_context(void);
void __cdecl restore_type_context(void);
void __cdecl write_function_declaration(CompilerIrSymbol *, FILE *);
void __cdecl begin_function_body(void)
{
    CompilerIrSymbol *function = c1_bss.current_scope_symbol;
    CompilerType *type;
    CompilerTypeShape *shape;
    CompilerSymbolBinding *parameter;
    CompilerIrSymbol *symbol;
    unsigned parameters, result, first, second, third, bytes;
    int i;
    if (!function) return;
    if (c1_bss.current_context.saved_frame->stream->active) {
        if (!c1_bss.debug_module->Slot14(0, c1_bss.current_context.value,
                c1_bss.lexical_skip_mode, &c1_data.debug_module_kind,
                &third, &second, &first))
            compiler_error(73, ".\\funcs.c", 1215);
        c1_data.backend->SetDebugMode(!first);
        c1_data.emit_global_locations = c1_data.debug_module_kind == 4;
        if (c1_data.emit_global_locations && !compare_debug_positions(second, third))
            compiler_error(73, ".\\funcs.c", 1222);
        if (c1_bss.browser_filename) begin_browser_function();
    }
    initialize_function_parameters();
#ifdef RECON_NATIVE_TEST
    if (parameter_scope_value)
        recon_unsupported("nonempty function parameter validation is not yet modeled");
#endif
    type = function->function_type;
    shape = type->shape;
    if (!function_stack_check_reported && (type->attributes & 0x70) == 0x40 &&
        c1_data.normalized_stack_checks) {
        report_language_diagnostic(1, 124);
        function_stack_check_reported = 1;
    }
    parameters = parameter_scope_value;
    if (!function_output_mode && ((type->attributes & 0x70) == 0x60 ||
                                 (type->attributes & 0x70) == 0x40)) {
        bytes = parameter_stack_bytes(parameters);
        if ((function->qualifier_flags & 0x40) && bytes != function->symbol_value)
            compiler_warning(709, function);
        if (bytes > 0xffff) compiler_error(508, function);
        function->symbol_value = bytes & 0xffff;
        function->qualifier_flags |= 0x40;
    }
    parameter = parameters ? ((CompilerSymbolBinding **)parameters)[1] : NULL;
    if (parameter) {
        symbol = (CompilerIrSymbol *)((BYTE *)parameter - 8);
        if (shape->parameters->count > 0) {
            i = 1;
            while (symbol && symbol->kind != 11 && symbol->kind != 12) {
                parameter = parameter ? parameter->next : NULL;
                symbol = (CompilerIrSymbol *)((BYTE *)parameter - 8);
            }
            if (shape->flags & 0x4000) {
                if (symbol->parameter_type != c1_data.builtin_types.type_0080) {
                    report_language_diagnostic(1, 27);
                    goto parameters_checked;
                }
            } else if (symbol->parameter_type == c1_data.builtin_types.type_0080) {
                report_language_diagnostic(1, 26);
                goto parameters_checked;
            }
            while (symbol) {
                if (i > shape->parameters->count) break;
                if (shape->parameters->types[i - 1] != symbol->parameter_type &&
                    !compatible_parameter_types(shape->parameters->types[i - 1], symbol->parameter_type))
                    report_language_diagnostic(1, 28, i);
                ++i;
                do {
                    parameter = parameter ? parameter->next : NULL;
                    symbol = parameter ? (CompilerIrSymbol *)((BYTE *)parameter - 8) : NULL;
                } while (symbol && symbol->kind != 11 && symbol->kind != 12);
            }
            if (i != shape->parameters->count + 1 ||
                (symbol && !(shape->flags & 0x8000)))
                report_language_diagnostic(1, 29);
        } else if (parameter_storage_class != 2) {
            if (shape->flags & 0x4000) report_language_diagnostic(1, 27);
            else if ((shape->flags & 0x800) && !symbol->parameter_type) {
                if (c1_data.options.language_extensions) report_language_diagnostic(1, 52);
                else compiler_warning(142);
            }
        }
    } else if (!(shape->flags & 0x4000) && shape->parameters->count && parameter_storage_class == 1)
        report_language_diagnostic(1, 26);
parameters_checked:
    if (function_storage_class == 1) function_storage_class = 5;
    result = function_storage_class == 5 ? 0x4004 : 4;
    type = replace_function_parameters(function->function_type, result, shape->attributes, &parameters);
    if (function_storage_class == 3 || function_storage_class == 5) {
        function->function_type = type;
        if (function_output_mode) {
            save_type_context();
            write_function_declaration(function, c1_bss.preprocessor_output);
            restore_type_context();
        }
    } else if (function_output_mode) {
        CompilerType *saved = function->function_type;
        function->function_type = type;
        save_type_context();
        write_function_declaration(function, c1_bss.preprocessor_output);
        restore_type_context();
        function->function_type = saved;
    }
    if (function_debug_info && !function_output_mode && c1_data.debug_module_kind != 4)
        emit_control_record(17);
    lexer_flags |= 0x20000;
}
void __cdecl prepare_function_record(int);
CompilerParserNode *__cdecl make_ir_operation(unsigned, CompilerParserNode *, CompilerParserNode *);
void __cdecl emit_ir_node(CompilerParserNode *);
void __cdecl coerce_parameter_symbol(CompilerIrSymbol *);
#define function_entry_label c1_bss.function_entry_label /* RVA54fc8 */
void __cdecl initialize_function_parameters(void)
{
    CompilerIrSymbol *function = c1_bss.current_scope_symbol;
    function->flags |= 1;
    c1_bss.parameter_symbol_count = 0;
    function->declaration_flags &= ~0x10;
#ifdef RECON_NATIVE_TEST
    if (function_output_mode)
        recon_unsupported("nonzero function output mode is not yet supported");
#endif
    if (function_output_mode) return;
    prepare_function_record(0);
    if (c1_data.debug_module_kind != 4) {
        function_entry_label = make_internal_label(31);
        write_ir_record(83, NULL);
        write_ir_record(83, NULL);
        emit_ir_node(make_ir_operation(108, make_label_expression(function, 1), 0));
    }
#ifdef RECON_NATIVE_TEST
    if (parameter_scope_value)
        recon_unsupported("function parameter chain is not yet supported");
#endif
    CompilerSymbolBinding *binding = parameter_scope_value ?
        ((CompilerSymbolBinding **)parameter_scope_value)[1] : NULL;
    for (; binding; binding = binding->next) {
        CompilerIrSymbol *symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
        if (symbol->kind != 11 && symbol->kind != 12) continue;
        unsigned storage = symbol->storage_class & 7;
        if (!storage) symbol->storage_class = (symbol->storage_class & 0xf8) ^ 4;
        else if (storage >= 5 && storage <= 6 && c1_data.debug_module_kind != 4)
            coerce_parameter_symbol(symbol);
        if (symbol->parameter_type && symbol->parameter_type != c1_data.builtin_types.type_0080)
            ++c1_bss.parameter_symbol_count;
    }
}
static BYTE *line_output_start; /* RVA547f8, source-only */
static BYTE *line_output_cursor; /* RVA547fc, source-only */
static unsigned line_output_mode; /* RVA54f14, source-only */
void __cdecl write_global_record(unsigned, CompilerIrSymbol *);
void __cdecl prepare_function_record(int mode)
{
    c1_data.function_record_flags |= 0x1000;
    if (c1_data.c9_il) {
        c1_bss.output_buffers[3].Flush();
        c1_bss.function_expression_offset = ftell(c1_bss.output_buffers[3].stream);
        c1_bss.output_buffers[1].Flush();
        c1_bss.function_symbol_offset = ftell(c1_bss.output_buffers[1].stream);
    } else if (!(lexer_flags & 0x20)) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("non-IL line-output storage is not yet modeled");
#endif
        CompilerOutputBuffer *buffer = &c1_bss.output_buffers[4];
        unsigned used = (unsigned)(line_output_cursor - line_output_start);
        if (used) {
            int good = buffer->Ensure(used + 1);
            buffer->good &= good;
            if (buffer->good) buffer->WriteBytes(line_output_start, used);
            line_output_cursor = line_output_start;
        }
        if (line_output_mode) buffer->FlushSource();
        else buffer->Flush();
    }
    c1_bss.ir_record_count = 0;
    if (lexer_flags & 0x20) {
        int saved = c1_data.normalized_stack_checks;
        c1_data.normalized_stack_checks = 0;
        c1_data.normalized_stack_checks = saved;
    }
    if (c1_data.debug_module_kind == 4) return;
    emit_control_record(31);
    if ((unsigned)c1_data.options.inline_level > 0) emit_control_record(32);
    if (mode) {
        ++c1_bss.declaration_scope_active;
        write_global_record(6, NULL);
        --c1_bss.declaration_scope_active;
    } else {
        emit_control_record(2);
        emit_control_record(36);
        emit_control_record(35);
    }
}
CompilerParserNode *__cdecl make_ir_operation(unsigned code, CompilerParserNode *left, CompilerParserNode *right)
{
    return make_typed_binary_expression(code, left, right, left->type);
}
/* Three encountered cells of each original column. Intervening storage is
 * representation only: it has no recovered default or semantic coverage.
 * DWORD opcodes: kind6 at63e80, kind101 at63ffc, kind108 at64018.
 * BYTE property bit80 is clear at64246/642a5, and set at642ac. Other property bits and
 * adjacent columns are deliberately not represented. */
static const union {
    struct { BYTE gap0[24]; unsigned symbol; BYTE gap1[376];
             unsigned integer; BYTE gap2[24]; unsigned function; } known;
    unsigned indexed[1];
} ir_node_opcodes = { { {0}, 38, {0}, 51, {0}, 70 } };
static const union {
    struct { BYTE gap0[6], symbol, gap1[94], integer, gap2[6], function; } known;
    BYTE indexed[1];
} ir_node_properties = { { {0}, 0, {0}, 0, {0}, 0x80 } };
#ifdef RECON_NATIVE_TEST
#define REQUIRE_IR_KIND(k) do { if ((k) != 6 && (k) != 101 && (k) != 108) \
    recon_unsupported("IR node column is not modeled for this kind"); } while (0)
#else
#define REQUIRE_IR_KIND(k) ((void)0)
#endif
/* Source-only list views for the special emission paths. They establish no
 * allocation extent, initialized bytes or native support for those owners. */
struct IrInitializerPart {
    IrInitializerPart *next;
    BYTE unmodeled_04[4];
    unsigned low, high;
};
struct IrParameterReference {
    BYTE unmodeled_00[8];
    unsigned used;
    BYTE unmodeled_0c[12];
    IrParameterReference *next;
};
CompilerParserNode *__cdecl lower_ir_node(CompilerParserNode *);
int __cdecl suppress_ir_call(CompilerParserNode *);
CompilerParserNode *__cdecl make_typed_unary_operation(unsigned, CompilerType *, CompilerParserNode *);
CompilerParserNode *__cdecl make_initializer_value(unsigned, unsigned, unsigned);
extern const signed char c1_primitive_widths[16];
void __cdecl emit_ir_node(CompilerParserNode *node)
{
    if (parser_early_result) return;
    while (node) {
        REQUIRE_IR_KIND(node->kind);
        if (ir_node_properties.indexed[node->kind] & 0x80) {
            switch (node->kind) {
            case 7:
                node = lower_ir_node(node);
                REQUIRE_IR_KIND(node->kind);
                write_ir_record(ir_node_opcodes.indexed[node->kind], node);
                return;
            case 93:
                if (!node->right)
                    node->right = make_typed_unary_operation(23,
                        c1_data.builtin_types.type_4063_first, NULL);
                break;
            case 100:
                if (!node->type->shape && (node->type->flags & 0x2fcf) == 0x80) {
                    node = node->left;
                    continue;
                }
                break;
            case 108:
                {
                    CompilerIrSymbol *symbol = node->left->symbol;
                    unsigned saved = symbol->reference_flags & 3;
                    emit_ir_node(node->left);
                    symbol->reference_flags = (symbol->reference_flags & ~3) | saved;
                    write_ir_record(ir_node_opcodes.indexed[node->kind], node);
#ifdef RECON_NATIVE_TEST
                    if (parameter_scope_value)
                        recon_unsupported("IR parameter-reference list is not yet modeled");
#endif
                    IrParameterReference *reference = parameter_scope_value ?
                        *(IrParameterReference **)(parameter_scope_value + 8) : NULL;
                    while (reference) {
                        CompilerIrSymbol *parameter = (CompilerIrSymbol *)((BYTE *)reference - 8);
                        if (reference->used && !(parameter->flags & 2) &&
                            (parameter->kind == 11 || parameter->kind == 12))
                            write_ir_record(45, make_label_expression(parameter, 0));
                        reference = reference->next;
                    }
                    write_ir_record(76, NULL);
                    return;
                }
            case 119:
                if (suppress_ir_call(node)) return;
                /* The call's operand list uses the same encoding as kind112. */
            case 112:
                {
                    emit_ir_node(node->left);
                    write_ir_record(ir_node_opcodes.indexed[node->kind], node);
                    for (CompilerParserNode *part = node->right; part; part = part->right) {
                        emit_ir_node(part->left);
                        write_ir_record(85, part);
                    }
                    write_ir_record(76, NULL);
                    return;
                }
            case 132:
                {
                    emit_ir_node(node->left);
                    write_ir_record(ir_node_opcodes.indexed[node->kind], node);
                    for (IrInitializerPart *part = (IrInitializerPart *)node->right;
                         part; part = part->next) {
                        WORD flags = node->type->flags;
                        unsigned width = ((flags & 0x340) || !(flags & 0xfff0)) ?
                            c1_primitive_widths[flags & 15] : flags & 15;
                        CompilerParserNode *value = make_initializer_value(part->low, part->high, width);
                        CompilerParserNode *opcode_value = make_initializer_value(part->low, part->high, width);
                        REQUIRE_IR_KIND(opcode_value->kind);
                        write_ir_record(ir_node_opcodes.indexed[opcode_value->kind], value);
                        write_ir_record(61, (CompilerParserNode *)part);
                    }
                    write_ir_record(76, NULL);
                    return;
                }
            }
        }
        switch (node->flags & 7) {
        case 0:
            if (node->flags & 0x40) { compiler_warning(124); return; }
            break;
        case 1:
            emit_ir_node(node->left);
            break;
        case 2:
            emit_ir_node(node->left);
            emit_ir_node(node->right);
            break;
        }
        REQUIRE_IR_KIND(node->kind);
        write_ir_record(ir_node_opcodes.indexed[node->kind], node);
        return;
    }
}
#undef REQUIRE_IR_KIND
CompilerParserNode *__cdecl lower_ir_node(CompilerParserNode *) { __asm { int 3 } return NULL; }
int __cdecl suppress_ir_call(CompilerParserNode *) { __asm { int 3 } return 0; }
CompilerParserNode *__cdecl make_typed_unary_operation(unsigned, CompilerType *, CompilerParserNode *)
{ __asm { int 3 } return NULL; }
CompilerParserNode *__cdecl create_initializer_value(unsigned, unsigned, unsigned);
CompilerParserNode *__cdecl make_initializer_value(unsigned low, unsigned high, unsigned bytes)
{
    CompilerParserNode *value = create_initializer_value(low, high, bytes);
    if (!(low | high)) value->flags |= 0x80;
    return value;
}
CompilerParserNode *__cdecl create_initializer_value(unsigned low, unsigned high, unsigned bytes)
{
    CompilerArena *arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    CompilerParserNode *value = (CompilerParserNode *)arena->cursor;
#ifdef RECON_NATIVE_TEST
    if (bytes != 1 && bytes != 2 && bytes != 4 && bytes != 8 && bytes != 16)
        recon_unsupported("initializer width leaves kind and type unwritten");
#endif
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    value->flags = 3;
    switch (bytes) {
    case 1: case 2: case 4:
        value->kind = 101;
        value->type = c1_data.builtin_types.type_0063;
        break;
    case 8:
        value->kind = 101;
        value->type = c1_data.builtin_types.type_0041;
        break;
    case 16:
        value->kind = 101;
        value->type = c1_data.builtin_types.type_0042;
        break;
    }
    value->low = low;
    value->high = high;
    return value;
}
void __cdecl coerce_parameter_symbol(CompilerIrSymbol *) { __asm { int 3 } }
unsigned __cdecl parameter_stack_bytes(unsigned) { __asm { int 3 } return 0; }
int __cdecl compare_debug_positions(unsigned, unsigned) { __asm { int 3 } return 0; }
void __cdecl begin_browser_function(void) { __asm { int 3 } }
int __cdecl compatible_parameter_types(CompilerType *, CompilerType *) { __asm { int 3 } return 0; }
CompilerTypeShape *__cdecl make_type_shape(unsigned, unsigned, unsigned *);
CompilerTypeShape *__cdecl intern_type_shape(CompilerTypeShape *);
CompilerType *__cdecl intern_type(CompilerType *);
CompilerType *__cdecl replace_function_parameters(CompilerType *type, unsigned kind,
    WORD attributes, unsigned *parameters)
{
#ifdef RECON_NATIVE_TEST
    if (type_description_depth != 0)
        recon_unsupported("nested function type-description scratch is not modeled");
#endif
    ++type_description_depth;
    CompilerType *description = &c1_bss.type_descriptions[type_description_depth];
#ifdef RECON_NATIVE_TEST
    /* Only the established twelve-byte key is consumed by the interner. */
    description->flags = type->flags;
    description->attributes = type->attributes;
    description->shape = type->shape;
    description->auxiliary = type->auxiliary;
#else
    *description = *type;
#endif
    CompilerTypeShape *shape = make_type_shape(kind, attributes, parameters);
    shape->base = type->shape ? type->shape->base : NULL;
    description->shape = intern_type_shape(shape);
    CompilerType *result = intern_type(description);
    --type_description_depth;
    return result;
}
void __cdecl save_type_context(void) { __asm { int 3 } }
void __cdecl restore_type_context(void) { __asm { int 3 } }
void __cdecl write_function_declaration(CompilerIrSymbol *, FILE *) { __asm { int 3 } }
/* RVA1153d */
void __cdecl finish_function_definition(int);
void __cdecl finish_function_label_context(void);
void __cdecl reset_function_scope(void);
void __cdecl initialize_function_debug_symbol(void);
void __cdecl finish_function_body(void)
{
    unsigned saved_kind = c1_data.debug_module_kind;
    unsigned output_kind;
    int debug_state;
    int active = saved_kind == 4 || c1_bss.current_context.saved_frame->stream->active;
    if (c1_bss.current_scope_symbol) {
        if (active) {
            debug_state = c1_bss.debug_module->Slot84();
            if (!c1_bss.debug_module->Slot18(0, c1_bss.current_context.value,
                c1_bss.lexical_skip_mode, &saved_kind, &output_kind))
                compiler_error(73, ".\\funcs.c", 1881);
        }
        finish_function_definition((c1_bss.current_scope_symbol->flags >> 6) & 1);
        if (active) {
            c1_data.backend->SetDebugMode(saved_kind == 5);
            if (debug_state) initialize_function_debug_symbol();
        }
    }
    finish_function_label_context();
    parameter_scope_initial_value = 0;
    parameter_scope_value = 0;
    parameter_storage_class = 0;
    function_storage_class = 0;
    lexer_flags &= ~0x20000U;
    c1_bss.current_scope_symbol = NULL;
    reset_function_scope();
    c1_data.debug_module_kind = saved_kind;
}
/* Later cleanup cells are source-only until this owner's genuine child
 * frontiers return. Their defaults and equality are not inferred here. */


/* RVA5f2e0 is the existing c1_data.c9_il owner. */
void __cdecl check_finished_function(CompilerIrSymbol *);
void __cdecl finish_function_definition(int inline_mode)
{
    CompilerIrSymbol *function = c1_bss.current_scope_symbol;
#ifdef RECON_NATIVE_TEST
    if (inline_mode || c1_data.debug_module_kind != 2 ||
        function_output_mode || !function)
        recon_unsupported("alternate function finalization domain is not yet modeled");
#endif
    if (function) {
        if (c1_data.debug_module_kind == 4) {
            finish_compound_statement();
        } else {
            if (!function_output_mode && !(parser_flags & 0x800)) {
                CompilerType *type = function_return_type;
                if (type->flags != 0x80 || type->shape) {
                    if (!type->shape && (type->flags == 0x108 ||
                        ((function->function_type->attributes & 0x70) == 0x40 &&
                         (type->flags & 0x100))))
                        report_language_diagnostic(1, 137, function);
                    else report_language_diagnostic(3, 35, function);
                }
            }
            if (!function_output_mode) {
                if (!inline_mode) emit_control_record(36);
                emit_ir_label(function_entry_label);
                if (function_debug_info) emit_control_record(18);
                write_ir_record(71, NULL);
            }
            finish_compound_statement();
            c1_data.translation_unit->FinishFunction();
            if (!function_output_mode)
                write_ir_record(84, make_initializer_value(0, 0, 4));
        }
    }
    c1_data.translation_unit->retired_scopes = NULL;
    if (inline_mode && !c1_data.c9_il) c1_nonil_type_index = 0;
    check_finished_function(c1_bss.current_scope_symbol);
    function_return_type = NULL;
    c1_bss.pending_scope_symbols = NULL;
    c1_bss.function_cleanup_context = NULL;
    c1_bss.global_expression_scope_active = 0;
    c1_bss.declaration_scope_active = 0;
    parameter_nesting = 0;
    c1_data.function_cleanup_flags = 0;
    function_label_depth = 0;
    c1_lexer_token_cursor = (BYTE *)c1_lexer_token_text;
}
void __cdecl check_function_labels(void);
void RECON_THISCALL CompilerTranslationUnit::FinishFunction(void)
{
    CompilerScopeTable *scope;
    if (function_output_mode) return;
#ifdef RECON_NATIVE_TEST
    if (!retired_scopes)
        recon_unsupported("empty retired-scope finalization is not yet modeled");
#endif
    c1_bss.declaration_scope_active = 1;
    check_function_labels();
    if (!retired_scopes) {
        write_global_record(13, NULL);
        c1_bss.declaration_scope_active = 2;
        write_global_record(13, NULL);
        /* The original empty-list path retains its incoming cursor slot.
         * It has no defined native representation on this supported path. */
    } else {
        for (scope = retired_scopes; scope; scope = scope->next)
            scope->Check();
    }
    if (scope != root) write_global_record(6, NULL);
}
void __cdecl check_function_labels(void)
{
    if (parser_early_result) {
        c1_data.label_symbols = NULL;
        return;
    }
    CompilerIrSymbol *symbol = c1_data.label_symbols;
    while (symbol) {
#ifdef RECON_NATIVE_TEST
        if (symbol->kind != 9)
            recon_unsupported("nonlabel function label-chain entry is not yet modeled");
#endif
        if (!(symbol->flags & 1) && !symbol->symbol_value) {
            compiler_warning(94, symbol);
        } else {
            if (!(symbol->reference_flags & 3) && symbol->binding.name)
                report_language_diagnostic(1, 102, symbol);
            if (symbol->flags & 1) write_global_record(3, symbol);
        }
        symbol = symbol->binding.next ?
            (CompilerIrSymbol *)((BYTE *)symbol->binding.next - 8) : NULL;
    }
    c1_data.label_symbols = NULL;
}
/* Retained IL directory: one link followed by twenty records per allocation.
 * Flag bits outside this owner's masks are preserved without a default claim. */
/* These wider PCH-only sources remain unmodeled and guarded natively. */
int function_pch_replayed; /* RVA5f328, set by PCH loading */
static unsigned function_pch_expression_bytes, function_pch_symbol_bytes; /* RVA5f568/5f560 */
static int function_inline_output_required; /* RVA5edf4, source-only */
extern "C" unsigned __cdecl assign_symbol_output_index(CompilerIrSymbol *);
extern "C" int __cdecl uses_global_function_record(CompilerIrSymbol *);
extern "C" int __cdecl omit_inline_expression_record(void);
void RECON_THISCALL CompilerOutputBuffer::WriteFunctionIndex(unsigned index)
{
    int bytes = c1_data.c9_il ? 5 : 4;
    int available = cursor + bytes <= data + capacity || Grow(bytes) != 0;
    good &= available;
    if (!good) return;
    if (c1_data.c9_il) {
        if ((int)index <= 0x7fff) {
            *(WORD *)cursor = (WORD)index;
            cursor += 2;
        } else {
            *(DWORD *)cursor = (((index & 0xffffc000U) | 0x4000) << 1) | (index & 0x7fff);
            cursor += 4;
        }
    } else if (index >= 0x3fff80) {
        *cursor++ = (BYTE)(index + 0x80);
    } else if (index <= 0x3fff) {
        *cursor++ = (BYTE)((index & 0x3f) | 0x80);
        *cursor++ = (BYTE)(index >> 6);
    } else {
        *cursor++ = (BYTE)(index | 0xc0);
        *cursor++ = (BYTE)(index >> 6);
        *cursor++ = (BYTE)(index >> 14);
    }
}
void RECON_THISCALL CompilerOutputBuffer::AppendTo(CompilerOutputBuffer *) { __asm { int 3 } }
void __cdecl check_finished_function(CompilerIrSymbol *function)
{
    CompilerOutputBuffer *inline_records = &c1_bss.output_buffers[4]; /* RVA54df0 */
    CompilerFunctionDirectoryRecord *record;
    CompilerFunctionDirectoryBlock *block;
    unsigned index, value, mark_kind;
    int available;
    if (parser_early_result || c1_bss.options.prototypes_only || function_output_mode || !function)
        return;
#ifdef RECON_NATIVE_TEST
    if (!c1_data.c9_il || c1_data.debug_module_kind != 2 ||
        c1_data.options.pch_use || c1_data.options.pch_auto ||
        c1_data.saved_pch_mode || c1_data.function_directory_index != 19 ||
        c1_data.function_directory_head || c1_data.function_directory_current)
        recon_unsupported("alternate function-directory finalization is not yet modeled");
#endif
    if (c1_data.debug_module_kind != 4 && c1_data.c9_il &&
        !(c1_data.function_record_flags & 0x800)) {
        available = inline_records->Ensure(2);
        inline_records->good &= available;
        if (inline_records->good) *inline_records->cursor++ = 9;
        if (!c1_bss.current_scope_symbol) value = 0;
        else if (c1_data.c9_il) value = c1_bss.current_scope_symbol->record_id & 0xffffff;
        else value = (c1_bss.current_scope_symbol->reference_flags & ~7U) ?
            c1_bss.current_scope_symbol->reference_flags >> 3 :
            assign_symbol_output_index(c1_bss.current_scope_symbol);
        inline_records->WriteFunctionIndex(value);
    }
    if (!c1_bss.options.prototypes_only && !function_output_mode) {
        if (c1_data.c9_il) {
            c1_bss.output_buffers[1].Flush(); /* RVA54b40 */
            c1_bss.output_buffers[3].Flush(); /* RVA54928 */
        } else if (c1_data.debug_module_kind != 4) {
            write_ir_record(77, NULL);
            mark_kind = (function->symbol_qualifiers & 1) ? 2 : 1;
            index = c1_data.c9_il ? function->record_id & 0xffffff :
                ((function->reference_flags & ~7U) ? function->reference_flags >> 3 :
                 assign_symbol_output_index(function));
            c1_bss.output_buffers[3].MarkLocation(index, mark_kind);
            c1_bss.output_buffers[1].MarkLocation(function->reference_flags >> 3, mark_kind);
            if (!(lexer_flags & 0x20)) {
                if (uses_global_function_record(function))
                    inline_records->MarkGlobal(function->reference_flags >> 3);
                else {
                    if (!omit_inline_expression_record())
                        c1_bss.output_buffers[5].AppendTo(inline_records);
                    if (inline_records->Reserve(1)) *inline_records->cursor++ = 15;
                    inline_records->MarkLocation(function->reference_flags >> 3, mark_kind);
                }
            }
        }
    }
    c1_data.function_record_flags &= ~0x1000U;
    index = (c1_data.function_directory_index + 1) % 20;
    c1_data.function_directory_index = index;
    if (!index) {
        CompilerPool *pool = &c1_bss.pools[1];
        if (pool->remaining < 644) pool->Refill(644);
        block = (CompilerFunctionDirectoryBlock *)pool->cursor;
        pool->cursor += 644;
        pool->remaining -= 644;
        if (c1_data.function_directory_head)
            c1_data.function_directory_current->next = block;
        else c1_data.function_directory_head = block;
        c1_data.function_directory_current = block;
    }
    record = &c1_data.function_directory_current->records[index];
    record->binding = &function->binding;
    if (c1_data.saved_pch_mode && !function_pch_replayed) {
        record->flags |= 0x10;
        if (c1_data.options.pch_use) {
            c1_bss.function_expression_offset += function_pch_expression_bytes + 1132;
            c1_bss.function_symbol_offset += function_pch_symbol_bytes;
        }
        if ((function->linkage_flags & 4) && (function->storage_class & 7) != 3 &&
            (function->symbol_qualifiers & 1) && !(function->symbol_qualifiers & 0x10) &&
            !c1_data.options.pch_auto && !c1_data.options.no_expand_inline)
            function_inline_output_required = 1;
    } else record->flags &= ~0x10U;
    record->expression_position = c1_bss.function_expression_offset;
    record->symbol_position = c1_bss.function_symbol_offset;
    record->ir_records = c1_bss.ir_record_count;
    record->context_index = c1_bss.nonil_type_index;
    record->flags = (record->flags & ~7U) | (c1_data.ir_symbol_state_ready & 1) |
        ((c1_data.ir_symbol_state_changed & 1) << 1) | ((c1_data.disable_inline_records & 1) << 2);
    record->flags &= ~8U;
    record->parameter_symbols = c1_bss.parameter_symbol_count;
    record->debug_handle = c1_data.function_debug_handle;
    record->flags = (record->flags & ~0x20U) | ((c1_data.emit_global_locations & 1) << 5);
    c1_bss.ir_record_count = 0;
    c1_data.emit_global_locations = 0;
}
extern "C" int __cdecl uses_global_function_record(CompilerIrSymbol *) { __asm { int 3 } return 0; }
extern "C" int __cdecl omit_inline_expression_record(void) { __asm { int 3 } return 0; }

void __cdecl finish_function_label_context(void)
{ c1_data.debug_symbol_index = 0; }
void __cdecl reset_function_scope(void)
{ c1_data.function_scope_cache = NULL; }
void __cdecl initialize_function_debug_symbol(void) { __asm { int 3 } }
/* RVA178c6 */
void __cdecl parse_inline_assembly(void) { __asm { int 3 } }
/* RVA197b3 */
CompilerParserNode * __cdecl make_binary_expression(BYTE, CompilerParserNode *, CompilerParserNode *) { __asm { int 3 } return 0; }
/* RVA1b07b */
CompilerParserNode * __cdecl make_unary_expression(BYTE, CompilerParserNode *) { __asm { int 3 } return 0; }
/* RVA1b94e */
CompilerParserNode * __cdecl make_identifier_expression(CompilerNameEntry *) { __asm { int 3 } return 0; }
/* RVA1bdf6 */
CompilerParserNode * __cdecl make_right_typed_expression(BYTE, CompilerParserNode *, CompilerParserNode *) { __asm { int 3 } return 0; }
/* RVA1ce98 */
CompilerParserNode * __cdecl make_type_size_expression(CompilerType *) { __asm { int 3 } return 0; }
/* RVA1d38e */
CompilerParserNode * __cdecl make_member_expression(BYTE, CompilerParserNode *, CompilerParserNode *) { __asm { int 3 } return 0; }
/* RVA1d5f2 */
int __cdecl convert_expression_to_type(unsigned, CompilerType *, CompilerParserNode *);
int __cdecl returns_local_address(CompilerParserNode *);
CompilerParserNode *__cdecl convert_aggregate_return(CompilerType *, CompilerParserNode *);
void __cdecl emit_return_statement(CompilerParserNode *value, int explicit_return)
{
    CompilerType *type = function_return_type;
    CompilerIrSymbol *function;
    int no_result;
    if (!type) return;
    function = c1_bss.current_scope_symbol;
    if ((function->linkage_flags & 1) && explicit_return) {
        compiler_warning(490, 76, 125);
        return;
    }
    no_result = type->flags == 0x80 && !type->shape;
    if (value) {
        if (no_result) {
            report_language_diagnostic(1, 98, function);
            emit_expression(value);
            write_ir_record(58, function_entry_label);
            return;
        }
        convert_expression_to_type(125, type, value);
        if (returns_local_address(value))
            report_language_diagnostic(1, 172);
        if (type->shape && (type->shape->attributes & 3))
            value = convert_aggregate_return(type, value);
        emit_control_record(36);
        emit_ir_node(value);
        value->type = type;
        write_ir_record(65, value);
    } else if (!no_result && !(function->linkage_flags & 1)) {
        report_language_diagnostic(1, 33, function);
    }
    if (value || !no_result && !(function->linkage_flags & 1))
        parser_flags |= 0x800;
    write_ir_record(58, function_entry_label);
}
int __cdecl check_assignment_types(unsigned, CompilerType *, CompilerType *);
void __cdecl check_constant_conversion(unsigned, CompilerParserNode *, CompilerType *);
int __cdecl convert_expression_to_type(unsigned operation, CompilerType *type,
    CompilerParserNode *value)
{
#ifdef RECON_NATIVE_TEST
    if (operation != 125 || type->shape || value->type->shape ||
        !(type->flags & 0x40) || !(value->type->flags & 0x40))
        recon_unsupported("noninteger expression conversion is not yet modeled");
#endif
    if ((value->flags & 0x80) && type->shape && (type->shape->flags & 3))
        return 1;
    int result = check_assignment_types(operation, type, value->type);
    if (!type->shape && !value->type->shape) {
        unsigned pointer_count = ((type->flags >> 7) & 1) +
                                 ((value->type->flags >> 7) & 1);
        if ((BYTE)operation == 54) {
            if (pointer_count == 1) report_language_diagnostic(4, 53);
        } else if (pointer_count == 1 && (BYTE)operation == 125) {
            if (type->flags & 0x80) {
                if (c1_data.options.language_extensions)
                    report_language_diagnostic(1, 98, c1_bss.current_scope_symbol);
                else compiler_warning(562, c1_bss.current_scope_symbol);
            } else if (c1_data.options.language_extensions) {
                report_language_diagnostic(1, 70);
            } else if (!(c1_bss.current_scope_symbol->linkage_flags & 1)) {
                compiler_warning(561, c1_bss.current_scope_symbol);
            }
        } else if (pointer_count) {
            compiler_warning(120);
        }
        check_constant_conversion(operation, value, type);
    }
    return result;
}
int __cdecl check_type_qualifiers(unsigned, unsigned, unsigned, int, int);
int __cdecl compatible_function_conventions(unsigned, unsigned);
int __cdecl compare_parameter_types(CompilerType *, CompilerType *);
int __cdecl incompatible_shape_conversion(unsigned, CompilerType *, CompilerType *);
void __cdecl check_storage_conversion(unsigned, CompilerTypeShape *, CompilerTypeShape *);
void __cdecl warn_function_pointer_conversion(void);
int __cdecl check_assignment_types(unsigned operation, CompilerType *destination,
    CompilerType *source)
{
    CompilerTypeShape *left, *right;
    unsigned left_flags, right_flags, left_kind, right_kind;
    unsigned left_attributes, right_attributes, left_outer, right_outer;
    int first = 1, failed = 0, result, i, count;
    int left_void_pointer, right_void_pointer;
    CompilerTypeList *left_parameters, *right_parameters;
    CompilerType **left_argument, **right_argument;
#ifdef RECON_NATIVE_TEST
    if (destination->shape || source->shape ||
        !(destination->flags & 0x40) || !(source->flags & 0x40))
        recon_unsupported("structured assignment compatibility is not yet modeled");
#endif
    if (destination == source) return 1;
    left_flags = destination->flags;
    right_flags = source->flags;
    left = destination->shape;
    right = source->shape;
    left_void_pointer = (left_flags & 0x80) && left &&
        (left->flags & 7) == 1 && !left->base;
    right_void_pointer = (right_flags & 0x80) && right &&
        (right->flags & 7) == 1 && !right->base;
    if (left_void_pointer != right_void_pointer) {
        CompilerTypeShape *other = left_void_pointer ? right : left;
        unsigned kind = other ? other->flags & 7 : 0;
        if (kind == 1 || kind == 2 || kind == 3) {
            switch ((BYTE)operation) {
            case 62: case 63: case 68: case 69: case 70:
                report_language_diagnostic(3, 133, (BYTE)operation, source, destination);
                return 1;
            case 52: case 112: case 116: case 125:
                if (kind == 3) warn_function_pointer_conversion();
                result = (left_flags & 0x2fcf) == 0x41 || (left_flags & 0x80);
                if (!check_type_qualifiers(operation, destination->attributes,
                    source->attributes, 1, result)) return 0;
                break;
            case 59: case 76:
                break;
            default:
                return 1;
            }
            if ((left->attributes & 3) || (right->attributes & 3))
                check_storage_conversion(operation, left, right);
            return 1;
        }
    }
    if (left == right) {
        if (!left) {
            if ((left_flags & 0x140) && (right_flags & 0x340)) return 1;
            if (destination->auxiliary == source->auxiliary) return 1;
            if ((BYTE)operation == 54 && (left_flags & 0x200) && (right_flags & 0x200))
                return 1;
            compiler_warning(115, (BYTE)operation);
            return 0;
        }
        if (!check_type_qualifiers(operation, destination->attributes,
            source->attributes, 1, (left_flags & 0x2fcf) == 0x41)) return 0;
        goto compare_base;
    }
    while (left && right) {
        left_kind = left->flags & 7;
        right_kind = right->flags & 7;
        left_attributes = (left->base ? left->base->attributes : destination->attributes);
        right_attributes = (right->base ? right->base->attributes : source->attributes);
        left_outer = left->attributes;
        right_outer = right->attributes;
        if (left_kind == 2) left_attributes |= left_outer & 0x1f00;
        if (right_kind == 2) right_attributes |= right_outer & 0x1f00;
        if (first) {
            if (left_kind == 2) left_kind = 1;
            if (right_kind == 2) right_kind = 1;
        }
        if (left_kind != right_kind)
            return incompatible_shape_conversion(operation, destination, source);
        if (left_kind == 3 &&
            !compatible_function_conventions(left_outer, right_outer)) {
            compiler_warning(152, (BYTE)operation);
            return 0;
        }
        if ((left->attributes & 3) || (right->attributes & 3))
            check_storage_conversion(operation, left, right);
        result = check_type_qualifiers(operation, left_attributes, right_attributes,
            first, (destination->flags & 0x2fcf) == 0x41);
        if (!result) return 0;
        if (result == 2 && left_kind != 3 &&
            !(left_kind == 2 && (right->flags & 7) == 2) &&
            !compatible_function_conventions(left_outer, right_outer)) {
            report_language_diagnostic(1, 47, (BYTE)operation, destination, source);
            return 0;
        }
        if (left_kind == 2 && left->value != right->value) {
            report_language_diagnostic(1, 48, destination, source);
            return 0;
        }
        if (left_kind == 4) {
            left_parameters = left->parameters;
            right_parameters = right->parameters;
            if (left_parameters != right_parameters) {
                if (!left_parameters || !right_parameters) {
                    if (!(left->flags & 0x4000) && !(right->flags & 0x4000))
                        goto next_shape;
                    /* The original's zero-parameter special path retains
                     * its local cursor slot; this domain is unsupported
                     * natively before any such cursor is consumed. */
                    goto parameters_finished;
                }
                count = left_parameters->count;
                if (count != right_parameters->count) {
                    if ((left->flags & 0x8000) || (right->flags & 0x8000)) {
                        if (count > right_parameters->count) count = right_parameters->count;
                    } else {
                        if (count || !c1_data.options.language_extensions)
                            report_language_diagnostic(1, 113, source, destination);
                        goto next_shape;
                    }
                }
                left_argument = left_parameters->types;
                right_argument = right_parameters->types;
                for (i = 1; i <= count; ++i, ++left_argument, ++right_argument) {
                    if (*left_argument == *right_argument) continue;
                    if (!*left_argument || !*right_argument) break;
                    if (first) {
                        if (!check_assignment_types(112, *left_argument, *right_argument))
                            failed = 1;
                    } else {
                        result = compare_parameter_types(*left_argument, *right_argument);
                        if (result == 1 ||
                            (result == 2 && !c1_data.options.language_extensions)) {
                            report_language_diagnostic(1, 28, i);
                            failed = 1;
                        }
                    }
                }
parameters_finished:
                if (!failed && c1_data.options.language_extensions &&
                    ((!left_argument || !*left_argument) !=
                     (!right_argument || !*right_argument)))
                    report_language_diagnostic(4, 220);
            }
        }
next_shape:
        first = 0;
        left = left->base;
        right = right->base;
        if (left == right) { left = right = NULL; break; }
    }
    if (left != right) {
        if (first && (((right_flags & 0x80) && !right) ||
                      ((left_flags & 0x80) && !left))) {
            compiler_warning(120);
            return 0;
        }
        if ((!destination->shape && !(left_flags & 0x40)) ||
            (!source->shape && !(right_flags & 0x40))) {
            compiler_warning(115, (BYTE)operation);
            return 0;
        }
        return incompatible_shape_conversion(operation, destination, source);
    }
compare_base:
    if (left_flags == right_flags && destination->auxiliary == source->auxiliary)
        return !failed;
    if (c1_data.options.language_extensions) {
        int a = (left_flags & 0x340) || !(left_flags & 0xfff0) ?
            c1_primitive_widths[left_flags & 15] : left_flags & 15;
        int b = (right_flags & 0x340) || !(right_flags & 0xfff0) ?
            c1_primitive_widths[right_flags & 15] : right_flags & 15;
        if (a == b && !((left_flags ^ right_flags) & 0x2fc0) &&
            destination->auxiliary == source->auxiliary) {
            report_language_diagnostic(4, 57, (BYTE)operation, destination, source);
            return !failed;
        }
    }
    if ((left_flags & 0x2fcf) == 0x80) return 1;
    report_language_diagnostic(1, 133, (BYTE)operation, source, destination);
    return !failed;
}
int __cdecl check_type_qualifiers(unsigned, unsigned, unsigned, int, int)
{ __asm { int 3 } return 0; }
int __cdecl compatible_function_conventions(unsigned, unsigned) { __asm { int 3 } return 0; }
int __cdecl compare_parameter_types(CompilerType *, CompilerType *) { __asm { int 3 } return 0; }
int __cdecl incompatible_shape_conversion(unsigned, CompilerType *, CompilerType *)
{ __asm { int 3 } return 0; }
void __cdecl check_storage_conversion(unsigned, CompilerTypeShape *, CompilerTypeShape *)
{ __asm { int 3 } }
void __cdecl warn_function_pointer_conversion(void) { __asm { int 3 } }
void __cdecl check_floating_conversion(unsigned, CompilerParserNode *, CompilerType *);
void __cdecl check_integer_float_conversion(unsigned, CompilerParserNode *, CompilerType *, int, int);
int __cdecl integer_value_loses_bits(int, int, int, __int64);
void __cdecl check_constant_conversion(unsigned operation, CompilerParserNode *value,
    CompilerType *destination)
{
    CompilerType *source;
    unsigned source_flags, destination_flags, level = 0;
    int source_width, destination_width;
    if (c1_data.options.warning_level < 2) return;
#ifdef RECON_NATIVE_TEST
    recon_unsupported("conversion diagnostics at higher warning levels are not yet modeled");
#endif
    switch ((BYTE)operation) {
    case 52: case 58: case 71: case 75:
    case 82: case 112: case 116: case 125:
        break;
    default:
        return;
    }
    if (!value || !(source = value->type) || !destination ||
        source->shape || destination->shape ||
        !(source->flags & 0x140) || !(destination->flags & 0x140)) return;
    source_flags = source->flags;
    destination_flags = destination->flags;
    source_width = (source_flags & 0x340) || !(source_flags & 0xfff0) ?
        c1_primitive_widths[source_flags & 15] : source_flags & 15;
    destination_width = (destination_flags & 0x340) || !(destination_flags & 0xfff0) ?
        c1_primitive_widths[destination_flags & 15] : destination_flags & 15;
    if (source_flags & 0x100) {
        if (destination_flags & 0x40) level = 2;
        else if ((value->flags & 7) == 3)
            check_floating_conversion(operation, value, destination);
        else if (destination_width < source_width) level = 3;
        else if (destination_width == source_width &&
                 (source_flags & 0x108) == 0x108 &&
                 (destination_flags & 0x108) != 0x108) level = 4;
    } else if (destination_flags & 0x100) {
        if (destination_width <= source_width) {
            if ((value->flags & 7) == 3)
                check_integer_float_conversion(operation, value, destination, 0, 1);
            else level = 2;
        }
    } else if ((value->flags & 7) == 3) {
        __int64 integer;
        memcpy(&integer, &value->low, sizeof(integer));
        int unsigned_destination = (destination_flags & 0x4000) != 0;
        int unsigned_source = (source_flags & 0x4000) != 0;
        int unsigned_plain_char = !c1_data.options.language_extensions &&
            c1_data.options.unsigned_char &&
            (source_flags & 0x2fcf) == 0x41 && !(source_flags & 0x8000);
        if (unsigned_destination && !unsigned_source && !unsigned_plain_char && integer < 0)
            report_language_diagnostic(4, 245, (BYTE)operation, source, destination);
        if (destination_flags != source_flags &&
            integer_value_loses_bits(destination_width, unsigned_destination,
                !unsigned_source || unsigned_plain_char, integer)) {
            report_language_diagnostic(2, 305, (BYTE)operation, source, destination);
            return;
        }
    } else if (destination_width < source_width) {
        level = (source_flags & 0x6f) == 0x63 || (source_flags & 0x200) ? 4 : 3;
    }
    if (level)
        report_language_diagnostic(level, 244, (BYTE)operation, source, destination);
}
void __cdecl check_floating_conversion(unsigned, CompilerParserNode *, CompilerType *)
{ __asm { int 3 } }
void __cdecl check_integer_float_conversion(unsigned, CompilerParserNode *, CompilerType *, int, int)
{ __asm { int 3 } }
int __cdecl integer_value_loses_bits(int, int, int, __int64)
{ __asm { int 3 } return 0; }
CompilerParserNode *__cdecl strip_value_wrappers(CompilerParserNode *);
int __cdecl returns_local_address(CompilerParserNode *value)
{
    value = strip_value_wrappers(value);
    if (value && (value->flags & 7) == 4) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("address-valued return symbol is not yet modeled");
#endif
        unsigned storage = value->symbol->storage_class & 7;
        return storage >= 4 && storage <= 6;
    }
    return 0;
}
/* Source-only linkage storage for the BYTE-indexed column at RVA63c78.
 * No cell, default, original capacity or native comparison is modeled here. */
static BYTE unmodeled_expression_classes[256];
CompilerParserNode *__cdecl strip_value_wrappers(CompilerParserNode *value)
{
    for (;;) {
        if (value->kind == 100) {
            value = value->left;
            continue;
        }
        if (value->kind != 93) return value;
#ifdef RECON_NATIVE_TEST
        recon_unsupported("binary wrapper classification column is not yet modeled");
#endif
        if (!value->right) {
            value = value->left;
            continue;
        }
        CompilerParserNode *left = value->left;
        if ((left->type->shape->attributes & 3) != 3) return value;
        CompilerParserNode *right = value->right->left;
        if (unmodeled_expression_classes[value->right->kind] == 61)
            right = right->left;
        right = right->left;
        if (right->type->shape) return value;
        value = left;
    }
}
CompilerParserNode *__cdecl convert_aggregate_return(CompilerType *, CompilerParserNode *)
{ __asm { int 3 } return NULL; }
/* RVA1df6d */
CompilerIrSymbol * __cdecl prepare_initializer_symbol(CompilerIrSymbol *) { __asm { int 3 } return 0; }
/* RVA1e277 */
CompilerParserNode * __cdecl make_based_expression(CompilerParserNode *) { __asm { int 3 } return 0; }
/* RVA1e3a8 */
CompilerParserNode * __cdecl normalize_based_expression(CompilerParserNode *) { __asm { int 3 } return 0; }
/* RVA1e68f */
CompilerParserNode * __cdecl make_expression_with_type(CompilerParserNode *, CompilerParserNode *, CompilerType *) { __asm { int 3 } return 0; }
/* RVA1e8dc */
CompilerParserNode * __cdecl finish_argument_list(CompilerParserNode *) { __asm { int 3 } return 0; }
/* RVA1eba0 */
void __cdecl emit_expression(CompilerParserNode *) { __asm { int 3 } }
/* RVA1ec07 */
void __cdecl emit_conditional_branch(CompilerParserNode *, CompilerParserNode *, int) { __asm { int 3 } }
/* RVA1f1ad */
void __cdecl emit_loop_condition(CompilerParserNode *) { __asm { int 3 } }
/* RVA1f312 */
void __cdecl emit_case_label(CompilerParserNode *) { __asm { int 3 } }
/* RVA1f54b */
void __cdecl finish_switch_statement(void) { __asm { int 3 } }
/* RVA1f6d3 */
void __cdecl emit_branch(CompilerParserNode *, CompilerParserNode *, CompilerParserNode *) { __asm { int 3 } }
/* RVA1f7d8 */
void __cdecl apply_initializer(CompilerIrSymbol *, CompilerParserNode *) { __asm { int 3 } }
/* RVA1f9c4 */
void __cdecl begin_conditional_branch(CompilerParserToken *) { __asm { int 3 } }
/* RVA207c2 */
void __cdecl finish_initializer(CompilerParserNode *, int) { __asm { int 3 } }
/* RVA20a13 */
void __cdecl finish_aggregate_initializer(CompilerParserNode *) { __asm { int 3 } }
/* RVA21166 */
void __cdecl finish_initializer_element(void) { __asm { int 3 } }
/* RVA21208 */
void __cdecl begin_aggregate_initializer(CompilerParserNode *) { __asm { int 3 } }
/* RVA21bd9 */
CompilerParserNode * __cdecl make_sizeof_expression(CompilerParserNode *) { __asm { int 3 } return 0; }
/* RVA21c4f */
void __cdecl begin_initializer_element(void) { __asm { int 3 } }
/* RVA21dd6 */
void __cdecl begin_external_declaration(void) { __asm { int 3 } }
/* RVA22e59 */
void __cdecl attach_declaration_context(CompilerIrSymbol *, void *) { __asm { int 3 } }
/* RVA23957 */
void __cdecl begin_exception_handler(void) { __asm { int 3 } }
/* RVA239ac */
void __cdecl finish_exception_handler(void) { __asm { int 3 } }
/* RVA23a50 */
void __cdecl set_exception_filter_mode(void) { __asm { int 3 } }
/* RVA23a5b */
void __cdecl emit_exception_filter(CompilerParserNode *) { __asm { int 3 } }
/* RVA23a9b */
void __cdecl clear_exception_filter_mode(void) { __asm { int 3 } }
/* RVA23aa6 */
void __cdecl finish_finally_handler(void) { __asm { int 3 } }
/* RVA24f93 */
void __cdecl append_literal_text(const char **, CompilerParserValue *) { __asm { int 3 } }
/* RVA25125 */
CompilerIrSymbol * __cdecl find_statement_label(CompilerNameEntry *) { __asm { int 3 } return 0; }
/* RVA2517f */
CompilerIrSymbol * __cdecl make_statement_label(CompilerNameEntry *) { __asm { int 3 } return 0; }
/* RVA251d5 */
void __cdecl define_statement_label(CompilerNameEntry *) { __asm { int 3 } }
/* RVA25ba5 */
CompilerParameterDescription * __cdecl make_parameter_description(CompilerNameEntry *) { __asm { int 3 } return 0; }
/* RVA25c26 */
void __cdecl add_old_style_parameter(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *) { __asm { int 3 } }
/* RVA25eb0 */
void __cdecl prepare_for_statement(void) { __asm { int 3 } }
/* RVA3f1b4 */
void __cdecl report_expected_parser_token(CompilerParserToken *, int) { __asm { int 3 } }
/* RVA3f281 */
CompilerParserValue * __cdecl make_recovery_parser_value(CompilerParserValue *, int) { __asm { int 3 } return 0; }
/* RVA3f2c2 */
void __cdecl report_parser_error(const char *) { __asm { int 3 } }
/* RVA3f393 */
void __cdecl skip_declaration_tokens(void) { __asm { int 3 } }
/* RVA3f3ff */
CompilerParserToken * __cdecl recover_parser_token(CompilerParserToken *, int, int, int) { __asm { int 3 } return 0; }
/* RVA3f4f1 */
CompilerIrSymbol * __cdecl find_based_declarator(CompilerNameEntry *) { __asm { int 3 } return 0; }
/* RVA3f798 */
void __cdecl process_pragma_declaration(void) { __asm { int 3 } }
/* RVA3f7d4 */
void __cdecl prepare_external_input(void) { __asm { int 3 } }
/* RVA41374 */
void __cdecl report_type_declaration_error(int, const CompilerParserDeclaration *) { __asm { int 3 } }
/* RVA41f63 */
void __cdecl emit_leave_statement(void) { __asm { int 3 } }
/* RVA41f89 */
void __cdecl begin_exception_filter(void) { __asm { int 3 } }
/* RVA41fa4 */
void __cdecl finish_exception_filter(void) { __asm { int 3 } }
/* RVA429ae */
CompilerParserNode * __cdecl make_typed_unary_expression(CompilerType *, CompilerParserNode *) { __asm { int 3 } return 0; }
}
CompilerDeclarator *RECON_THISCALL CompilerDeclarator::InitializeSymbol(CompilerIrSymbol *declaration_symbol)
{
    memset(this, 0, 48);
    symbol = declaration_symbol ? declaration_symbol : make_abstract_declarator();
    position = c1_bss.declaration_position;
    return this;
}
void RECON_THISCALL CompilerDeclarator::ApplyDeclaration(const CompilerParserDeclaration *declaration)
{
    if (!base_type) base_type = declaration->type;
    position.first = declaration->flags;
    position.second = declaration->value;
}
void RECON_THISCALL CompilerDeclarator::AppendArrayBound(const unsigned *)
{ __asm { int 3 } }
void RECON_THISCALL CompilerParameterDescription::AppendParameter(CompilerIrSymbol *)
{ __asm { int 3 } }
void RECON_THISCALL CompilerArena::ClearUsed(void)
{
    memset(base, 0, cursor - base);
    cursor = base;
}
void RECON_THISCALL CompilerTranslationUnit::EnterParameterScope(int value)
{
    CompilerArena *arena = &c1_bss.arenas[2];
    BYTE *memory = arena->cursor;
    arena->cursor += 20;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    CompilerParameterScope *scope = memory ? new(memory) CompilerParameterScope(value) : NULL;
    if (value) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("nonzero parameter-scope input state is not yet modeled");
#endif
        parameter_scope_value = (unsigned)value;
    }
    RegisterScope(scope);
}
void RECON_THISCALL CompilerParser::ClearRecovery(void) { __asm { int 3 } }

/* New direct children remain genuine pending boundaries. */
extern "C" CompilerParserToken *__cdecl process_special_token(CompilerParserToken *);
extern "C" int __cdecl has_forced_source_input(void);
extern "C" int __cdecl restore_forced_source_input(void);

/* RVA02e2d, with the special-token continuation at270d4. The stream owns
 * the returned token. Frame+20 is used here as an optional cdecl notification;
 * Push retains its original DWORD parameter/storage spelling. */
CompilerParserToken *RECON_THISCALL CompilerContextState::ReadToken(void)
{
    CompilerParserToken *token;
    CompilerContextFrame *frame;
    CompilerKeywordEntry *name;
    CompilerArena *arena;
    CompilerPool *pool;
    unsigned category;
    for (;;) {
        token = (CompilerParserToken *)saved_frame->stream->Read();
        if (token->kind < 0xf0)
            return token;
        /* Location tokens expose only object/value; recorded-stream tokens
         * pass directly to their genuine pending consumer. Wider special
         * semantics are incomplete and trap in both builds. */
        if ((token->kind != 0xf0 && token->kind != 0xf2) ||
            (token->kind == 0xf0 && (lexer_flags & (0x200000U | 0x20000U)))) {
#ifdef RECON_NATIVE_TEST
            recon_unsupported("context reader special-token state is not yet modeled");
#else
            __asm { int 3 }
#endif
        }
        if (token->kind == 0xf0) {
            value = token->value.context.value;
            object = token->value.context.object;
            if (lexer_flags & 0x200000)
                return token;
            if ((lexer_flags & 0x20000) && !(lexer_flags & 0x20))
                emit_control_record(36);
            continue;
        }
        if (token->kind == 0xf4) {
            frame = saved_frame;
            if (frame->first != 1)
                return token;
            if (frame) {
                saved_frame = frame->next;
                object = frame->object;
                value = frame->value;
                if (frame->second)
                    ((void (__cdecl *)(void))frame->second)();
            }
            continue;
        }
        if (parser_token_classes[token->kind] == 0xf3) {
            name = (CompilerKeywordEntry *)c1_data.current_identifier;
            category = name->category;
            switch (category) {
            case 1: report_language_diagnostic(1, 226, name); continue;
            case 2: report_language_diagnostic(1, 233, name); continue;
            case 3: report_language_diagnostic(1, 234, name); continue;
            case 4: report_language_diagnostic(1, 235, name); continue;
            case 5: report_language_diagnostic(1, 236, name); continue;
            case 6:
                if (c1_data.options.language_extensions)
                    report_language_diagnostic(1, 237, name);
                else
                    compiler_warning(328, name);
                if (c1_bss.symbol_arena_kind) {
                    arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
                    token = (CompilerParserToken *)arena->cursor;
                    arena->cursor += 32;
                    if (arena->cursor > arena->committed_limit)
                        arena->CommitExtent();
                } else {
                    pool = &c1_bss.pools[6];
                    if (pool->remaining < 32)
                        pool->Refill(32);
                    token = (CompilerParserToken *)pool->cursor;
                    pool->cursor += 32;
                    pool->remaining -= 32;
                }
                if (token) {
                    token->kind = 6;
                    token->value.name = c1_data.current_identifier;
                }
                return token;
            default:
                continue;
            }
        }
        if (token->kind != 0xf2)
            return token;
        token = process_special_token(token);
        if (!token)
            continue;
        if (token->kind == 2 && c1_bss.forced_source_index &&
            has_forced_source_input())
            restore_forced_source_input();
        return token;
    }
}
extern "C" CompilerParserToken *__cdecl dispatch_recorded_pragma(void);
extern "C" int __cdecl finish_recorded_source_tokens(void);
extern "C" CompilerParserToken *__cdecl process_special_token(CompilerParserToken *token)
{
    CompilerTokenStream *stream = token->value.recorded_stream;
    stream->Reset();
    c1_bss.current_context.Push((CompilerLexer *)stream, 6, 0, 0, &c1_bss.current_context);
    CompilerParserToken *result = dispatch_recorded_pragma();
    finish_recorded_source_tokens();
    CompilerContextFrame *frame = c1_bss.current_context.saved_frame;
    if (frame) {
        c1_bss.current_context.saved_frame = frame->next;
        c1_bss.current_context.object = frame->object;
        c1_bss.current_context.value = frame->value;
        if (frame->second) {
#ifdef RECON_NATIVE_TEST
            recon_unsupported("recorded source return callback is not yet modeled");
#else
            __asm { int 3 }
#endif
            ((void (__cdecl *)(void))frame->second)();
        }
    }
    return result;
}
extern "C" CompilerParserToken *__cdecl expect_directive_token(BYTE);
extern "C" int __cdecl finish_recorded_source_tokens(void)
{
    if (expect_directive_token(45)) return 1;
    c1_bss.current_context.ReadToken();
    return 0;
}
extern "C" int __cdecl has_forced_source_input(void)
{ __asm { int 3 } return 0; }

BYTE *RECON_THISCALL CompilerPool::AllocateBytes(unsigned)
{ __asm { int 3 } return 0; }
extern "C" CompilerParserToken *__cdecl resolve_identifier_type(CompilerParserToken *token)
{
    CompilerIrSymbol *symbol;
    CompilerIrSymbol **found;
    CompilerArena *arena;
    CompilerPool *pool;
    CompilerParserToken *result;
    if (lexer_flags & 0x80000000U) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("declaration symbol input alternative is not yet modeled");
#endif
        symbol = (CompilerIrSymbol *)token;
    } else {
        if (lexer_flags & 1) {
            lexer_flags &= ~1U;
            return token;
        }
#ifdef RECON_NATIVE_TEST
        if (declaration_depth != 0)
            recon_unsupported("identifier declaration control index is not yet modeled");
#endif
        if (declaration_control[declaration_depth].flags & 2) return token;
        if (c1_data.translation_unit->lookup_mode) return token;
        found = c1_data.translation_unit->FindName(token->value.name);
        symbol = found ? *found : NULL;
        if (!symbol) return token;
    }
    if (!(symbol->flags & 4)) return token;
    if (symbol->kind == 4) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("tagged symbol declaration conflict state is not yet modeled");
#endif
        if ((declaration_control[declaration_depth].flags & 1) && !(parser_flags & 8))
            return token;
    }
    if (c1_bss.browser_filename) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("identifier browser parameter state is not yet modeled");
#endif
        if (!parameter_nesting || parameter_browser_suppressed)
            c1_data.backend->Slot1c(symbol);
    }
    c1_bss.literal.symbol = symbol;
    if (c1_bss.symbol_arena_kind) {
#ifdef RECON_NATIVE_TEST
        if (c1_bss.symbol_arena_kind >= 6)
            recon_unsupported("identifier token arena is not modeled");
#endif
        arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
        result = (CompilerParserToken *)arena->cursor;
        arena->cursor += 32;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &c1_bss.pools[6];
        if (pool->remaining < 32) pool->Refill(32);
        result = (CompilerParserToken *)pool->cursor;
        pool->cursor += 32;
        pool->remaining -= 32;
    }
    if (result) {
        result->kind = 30;
#ifdef RECON_NATIVE_TEST
        result->value.symbol = symbol;
#else
        memcpy(&result->value, &c1_bss.literal, 24);
#endif
    }
    return result;
}
CompilerIrSymbol **RECON_THISCALL CompilerTranslationUnit::FindName(CompilerNameEntry *)
{ __asm { int 3 } return 0; }
extern "C" void __cdecl prepare_pch_expression_input(const char *);
extern "C" void __cdecl finish_pch_expression_input(void);
extern "C" int __cdecl finish_forced_include(void);
extern "C" CompilerBackend *__cdecl acquire_initial_backend(void);
/* Source-only PCH header identity; current entry rejects PCH before this read. */
static const char *implicit_pch_header; /* RVA54170 */
extern "C" void __cdecl finish_token_input(void)
{
    if (c1_data.options.pch_create) {
#ifdef __linux__
        recon_unsupported("token finish: PCH creation state is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    if (c1_data.options.pch_create && !function_pch_replayed) {
        if (!c1_data.options.pch_create_file) {
            prepare_pch_expression_input(implicit_pch_header);
            finish_pch_expression_input();
        } else compiler_warning(857, c1_data.options.pch_create_file);
    } else if (c1_bss.forced_source_index)
        finish_forced_include();
    if (c1_bss.browser_filename) c1_data.backend->Slot14(0);
    c1_bss.token_input_finished = 1;
    c1_data.backend = acquire_initial_backend();
    parser_flags |= 0x80;
}
extern "C" void __cdecl finish_pch_expression_input(void) { __asm { int 3 } }

/* The recorded stream shares the lexer prefix through byte60, but has no
 * unread stack. The original directive path allocates exactly64 bytes. */

/* These cells are source-only. The supported native newline path does not
 * read them; its flags DWORD is modeled separately in c1_data. */
#define source_exhausted c1_data.source_exhausted /* RVA5e214, initialized0 */
#define lexical_skip_mode c1_bss.lexical_skip_mode /* BSS RVA54da0 */
/* Shared numeric-spelling workspace. Only bytes written by the active
 * producer are interpreted; no whole-buffer initial-state claim is made. */
extern "C" char c1_numeric_token_text[128]; /* Same physical numeric workspace, RVA54f38. */
#define floating_token_text c1_numeric_token_text
extern "C" {
int __cdecl read_source_chunk(void);
unsigned __cdecl read_unspliced_character(void); /* RVA0a3fe */
int __cdecl skip_line_comment(void); /* RVA04468 */
int __cdecl skip_block_comment(void); /* RVA084b5 */
int __cdecl skip_language_comment(void); /* RVA3c1c8 */
unsigned __cdecl process_special_character(void); /* RVA059d0 */
unsigned __cdecl process_trigraph(void); /* RVA0d572 */
void __cdecl discard_input_character(void); /* RVA0a430 */
int __cdecl splice_input_line(void); /* RVA097e9 */
unsigned __cdecl resolve_special_character(unsigned); /* RVA196a8 */
CompilerParserToken *__cdecl process_source_newline(void); /* RVA03b5d */
void __cdecl scan_quoted_literal(int, unsigned); /* RVA0f274 */
void __cdecl copy_quoted_literal(unsigned, int); /* RVA3dea6 */
CompilerParserToken *__cdecl make_character_constant(unsigned, const void *, unsigned); /* RVA1fa2b */
int __cdecl scan_identifier(unsigned); /* RVA03dec */
int __cdecl expand_macro(CompilerMacro *); /* RVA0629c */
CompilerParserToken *__cdecl scan_numeric_token(unsigned, int); /* RVA069e3 */
unsigned __cdecl collect_floating_token(char *); /* RVA3e1e3 */
CompilerParserToken *__cdecl scan_floating_token(char *); /* RVA23aed */
int __cdecl finish_forced_include(void); /* RVA21ddf */
CompilerParserToken *__cdecl make_arena_token(unsigned); /* RVA05721 */
}

/* Inline source operations retain the original allocation and lookahead
 * sites, without introducing helper vertices absent from the target. */
#define LEXER_PROTOTYPE(kind) ((CompilerParserToken *)c1_data.predefined_tokens[kind])
#define LEXER_ALLOCATE_TOKEN() do { \
    if (c1_bss.symbol_arena_kind) { \
        arena = &c1_bss.arenas[c1_bss.symbol_arena_kind]; \
        token = (CompilerParserToken *)arena->cursor; \
        arena->cursor += 32; \
        if (arena->cursor > arena->committed_limit) arena->CommitExtent(); \
    } else { \
        pool = &c1_bss.pools[6]; \
        if (pool->remaining < 32) pool->Refill(32); \
        token = (CompilerParserToken *)pool->cursor; \
        pool->cursor += 32; pool->remaining -= 32; \
    } \
} while (0)
#define LEXER_PEEK_CLASS() do { \
    for (;;) { \
        next = *c1_bss.source_cursor++; \
        next_class = c1_data.scanner.classes[next]; \
        if (next_class != SC_SPECIAL) break; \
        special = process_special_character(); \
        if (special & ~0x70U) continue; \
        if (special & 0x20) ++c1_bss.source_cursor; \
        next_class = SC_UNUSED2; \
        break; \
    } \
} while (0)

unsigned RECON_THISCALL CompilerLexer::Read(void)
{
    BYTE character, next;
    unsigned dispatch, next_class, special, arena_kind;
    int directive, wide;
    CompilerParserToken *token;
    CompilerArena *arena;
    CompilerPool *pool;
    CompilerContextFrame *frame;
    CompilerTokenStream *recorded;
    CompilerRecordedLexer *new_stream;
    CompilerNameEntry *name;
    const char *text;
    unsigned length;
    if (unread_count) {
#ifdef RECON_NATIVE_TEST
        if (unread_count > 5) recon_unsupported("lexer unread stack exceeds known extent");
#endif
        return unread_tokens[--unread_count];
    }
    for (;;) {
        character = *c1_bss.source_cursor++;
        dispatch = c1_data.scanner.classes[character];
        /* Only the encountered token domains have complete semantic state.
         * Retain the other original branches below, behind an inline trap. */
        if ((dispatch != SC_SPECIAL && dispatch != SC_RBRACE && dispatch != SC_SEMICOLON &&
             dispatch != SC_DIGIT && dispatch != SC_NEWLINE && dispatch != SC_IDENTIFIER &&
             dispatch != SC_SPACE && dispatch != SC_LPAREN && dispatch != SC_RPAREN &&
             dispatch != SC_LBRACE) ||
            (dispatch == SC_SPECIAL && (character || lexical_skip_mode || c1_bss.options.preprocess_stdout))) {
#ifdef RECON_NATIVE_TEST
            recon_unsupported("lexer token domain is not yet modeled");
#else
            __asm { int 3 }
#endif
        }
        switch (dispatch) {
        case SC_NEWLINE:
            if (lexer_flags & 0x8000) {
                --c1_bss.source_cursor;
                return (unsigned)LEXER_PROTOTYPE(45);
            }
            token = process_source_newline();
            if (token) return (unsigned)token;
            continue;
        case SC_SPACE:
            do {
                if (c1_bss.options.preprocess_stdout && character != '\r')
                    fputc(character, c1_bss.preprocessor_output);
                character = *c1_bss.source_cursor++;
            } while (c1_data.scanner.classes[character] == SC_SPACE);
            --c1_bss.source_cursor;
            continue;
        case SC_SLASH:
            if (c1_data.options.language_extensions || c1_data.options.option_zbe) {
                if (read_unspliced_character() == '/') {
                    lexer_flags |= 0x800;
                    skip_line_comment();
                    lexer_flags &= ~0x800U;
                    continue;
                }
                --c1_bss.source_cursor;
                if (read_unspliced_character() == '*') {
                    lexer_flags |= 0x800;
                    skip_block_comment();
                    lexer_flags &= ~0x800U;
                    continue;
                }
                --c1_bss.source_cursor;
            } else if (skip_language_comment()) continue;
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(58);
            --c1_bss.source_cursor;
            return (unsigned)LEXER_PROTOTYPE(57);
        case SC_HASH:
            if (c1_bss.options.preprocess_stdout) {
                fwrite("#", 1, 1, c1_bss.preprocessor_output);
                continue;
            }
            compiler_warning(lexical_skip_mode ? 121 : 14);
            if (!lexical_skip_mode) discard_input_character();
            continue;
        case SC_SPECIAL:
            if (character == '\\') {
                if (!c1_bss.options.preprocess_stdout) {
                    if (!splice_input_line()) compiler_warning(17);
                } else {
                    fputc('\\', c1_bss.preprocessor_output);
                    next = *c1_bss.source_cursor++;
                    if (c1_data.scanner.classes[next] == SC_SPECIAL)
                        next = (BYTE)resolve_special_character(next);
                    fputc(next, c1_bss.preprocessor_output);
                }
                continue;
            }
            if (character == '?') {
                if (process_trigraph() & 0x40)
                    return (unsigned)LEXER_PROTOTYPE(86);
                continue;
            }
            if (source_exhausted) {
                --c1_bss.source_cursor;
                return (unsigned)LEXER_PROTOTYPE(109);
            }
            if (lexical_skip_mode) { process_special_character(); continue; }
            if (!read_source_chunk()) continue;
            if (c1_bss.options.preprocess_stdout) {
                if (restore_forced_source_input()) continue;
                return (unsigned)LEXER_PROTOTYPE(109);
            }
            if (!c1_data.source_include_depth && c1_bss.forced_source_index == 2 &&
                finish_forced_include() == 1) continue;
            directive = restore_forced_source_input();
            if (!directive) {
                if (lexer_flags & 0x8000) return (unsigned)LEXER_PROTOTYPE(45);
                source_exhausted = 1;
                c1_bss.source_cursor = (BYTE *)c1_empty_string;
                c1_bss.input->Mark();
            }
            if (directive == 1000) continue;
            arena_kind = c1_bss.symbol_arena_kind;
            new_stream = (CompilerRecordedLexer *)c1_bss.arenas[arena_kind].AllocateBytes(64);
            if (new_stream) new(new_stream) CompilerRecordedLexer(
                c1_bss.current_context.object, c1_bss.current_context.value, arena_kind);
            /* Push reads only the common vptr/object/value prefix. This cast
             * preserves the original shared layout, not an88-byte allocation. */
            c1_bss.context.Push((CompilerLexer *)new_stream, arena_kind, 0, 0,
                                &c1_bss.current_context);
            c1_bss.literal.integer = directive;
            LEXER_ALLOCATE_TOKEN();
            if (token) { token->kind = 45; memcpy(&token->value, &c1_bss.literal, 24); }
            c1_bss.context.saved_frame->stream->Trace((unsigned)token);
            token = (CompilerParserToken *)make_arena_token(c1_bss.symbol_arena_kind);
            c1_bss.context.saved_frame->stream->Trace((unsigned)token);
            c1_bss.context.saved_frame->stream->Trace((unsigned)LEXER_PROTOTYPE(45));
            c1_bss.context.saved_frame->stream->Trace((unsigned)LEXER_PROTOTYPE(244));
            frame = c1_bss.context.saved_frame;
            if (frame) {
                recorded = frame->stream;
                c1_bss.context.saved_frame = frame->next;
                c1_bss.context.object = frame->object;
                c1_bss.context.value = frame->value;
                if (frame->second) ((void (__cdecl *)(void))frame->second)();
            } else recorded = NULL;
            c1_bss.literal.recorded_stream = recorded;
            LEXER_ALLOCATE_TOKEN();
            if (token) { token->kind = 0xf2; memcpy(&token->value, &c1_bss.literal, 24); }
            return (unsigned)token;
        case SC_LETTER_L:
            LEXER_PEEK_CLASS();
            if (next_class == SC_SQUOTE) { wide = 1; goto character_literal; }
            if (next_class == SC_DQUOTE) { wide = 1; goto string_literal; }
            --c1_bss.source_cursor;
            c1_bss.source_cursor[-1] = character;
            /* FALL THROUGH */
        case SC_IDENTIFIER:
            if (scan_identifier(character)) {
                name = c1_data.current_identifier;
            } else {
                name = c1_data.current_identifier;
                if (name->macro && expand_macro(name->macro)) continue;
            }
            if (c1_bss.options.preprocess_stdout) {
                for (text = name->name; *text; ++text)
                    fputc(*text, c1_bss.preprocessor_output);
                return (unsigned)LEXER_PROTOTYPE(136);
            }
            if (name->flags & 0x10000)
                return (unsigned)LEXER_PROTOTYPE(((CompilerKeywordEntry *)name)->token);
            LEXER_ALLOCATE_TOKEN();
            if (token) { token->kind = 6; token->value.name = c1_data.current_identifier; }
            return (unsigned)token;
        case SC_DQUOTE:
            wide = 0;
        string_literal:
            if (c1_bss.options.preprocess_stdout) { copy_quoted_literal('"', wide); continue; }
            scan_quoted_literal(wide, '"');
            LEXER_ALLOCATE_TOKEN();
            if (token) { token->kind = 7; memcpy(&token->value, &c1_bss.literal, 24); }
            return (unsigned)token;
        case SC_SQUOTE:
            wide = 0;
        character_literal:
            if (c1_bss.options.preprocess_stdout) { copy_quoted_literal('\'', wide); continue; }
            scan_quoted_literal(wide, '\'');
            return (unsigned)make_character_constant(wide,
                c1_bss.literal.string.text, c1_bss.literal.string.length - 1);
        case SC_DOT:
            LEXER_PEEK_CLASS();
            if (next_class == SC_DIGIT) {
                floating_token_text[0] = '.';
                floating_token_text[1] = c1_bss.source_cursor[-1];
                if (!(lexer_flags & 0x4000) &&
                    (c1_bss.options.preprocess_stdout || (lexer_flags & 0x800000))) {
                    length = collect_floating_token(floating_token_text + 2) + 1;
                    if (c1_bss.options.preprocess_stdout) {
                        fwrite(floating_token_text, length, 1, c1_bss.preprocessor_output);
                        return (unsigned)LEXER_PROTOTYPE(136);
                    }
                    return (unsigned)LEXER_PROTOTYPE(136);
                }
                return (unsigned)scan_floating_token(floating_token_text + 2);
            }
            if (next_class == SC_DOT) {
                next = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[next] == SC_SPECIAL)
                    next = (BYTE)resolve_special_character(next);
                if (next == '.') {
                    LEXER_ALLOCATE_TOKEN();
                    if (token) { token->kind = 106; memcpy(&token->value, &c1_bss.literal, 24); }
                    return (unsigned)token;
                }
                --c1_bss.source_cursor;
                c1_bss.source_cursor[-1] = '.';
            }
            --c1_bss.source_cursor;
            return (unsigned)LEXER_PROTOTYPE(80);
        case SC_DIGIT:
            return (unsigned)scan_numeric_token(character,
                (lexer_flags & 0x200000) && !(lexer_flags & 0x8000));
        case SC_STAR:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(75);
            if (next_class == SC_SLASH && !c1_bss.options.preprocess_stdout)
                report_language_diagnostic(1, 138);
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(74);
        case SC_LESS:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(69);
            if (next_class == SC_LESS) {
                LEXER_PEEK_CLASS();
                if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(66);
                --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(67);
            }
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(68);
        case SC_GREATER:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(63);
            if (next_class == SC_GREATER) {
                LEXER_PEEK_CLASS();
                if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(87);
                --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(88);
            }
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(62);
        case SC_MINUS:
            LEXER_PEEK_CLASS();
            switch (next_class) {
            case SC_EQUAL: return (unsigned)LEXER_PROTOTYPE(71);
            case SC_MINUS: return (unsigned)LEXER_PROTOTYPE(56);
            case SC_GREATER: return (unsigned)LEXER_PROTOTYPE(83);
            default: --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(70);
            }
        case SC_PLUS:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(82);
            if (next_class == SC_PLUS) return (unsigned)LEXER_PROTOTYPE(64);
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(81);
        case SC_AMP:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(51);
            if (next_class == SC_AMP) return (unsigned)LEXER_PROTOTYPE(50);
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(49);
        case SC_BAR:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(78);
            if (next_class == SC_BAR) return (unsigned)LEXER_PROTOTYPE(79);
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(77);
        case SC_COLON:
            LEXER_PEEK_CLASS();
            if (next_class == SC_GREATER) return (unsigned)LEXER_PROTOTYPE(94);
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(54);
        case SC_CARET:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(92);
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(91);
        case SC_PERCENT:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(73);
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(72);
        case SC_EQUAL:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(59);
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(52);
        case SC_BANG:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(76);
            --c1_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(60);
        case SC_LPAREN: return (unsigned)LEXER_PROTOTYPE(118);
        case SC_RPAREN: return (unsigned)LEXER_PROTOTYPE(126);
        case SC_LBRACE: return (unsigned)LEXER_PROTOTYPE(117);
        case SC_RBRACE: return (unsigned)LEXER_PROTOTYPE(123);
        case SC_LBRACKET: return (unsigned)LEXER_PROTOTYPE(65);
        case SC_RBRACKET: return (unsigned)LEXER_PROTOTYPE(122);
        case SC_COMMA: return (unsigned)LEXER_PROTOTYPE(55);
        case SC_SEMICOLON: return (unsigned)LEXER_PROTOTYPE(127);
        case SC_TILDE: return (unsigned)LEXER_PROTOTYPE(89);
        case 41: ++c1_bss.source_cursor; continue;
        default:
            if (c1_bss.options.preprocess_stdout) fputc(character, c1_bss.preprocessor_output);
            else compiler_warning(18, character);
            continue;
        }
    }
}
/* RVA03b5d plus the newline/record-allocation continuation at27a8e. */
extern "C" int __cdecl at_source_buffer_start(void); /* RVA17564 */
extern "C" int __cdecl begin_first_line_forced_include(void); /* RVA1757d */
extern "C" CompilerParserToken *__cdecl read_directive_token(void); /* RVA04537 */
extern "C" int __cdecl skip_newline_comment(void); /* RVA0439a */
extern "C" void __cdecl begin_line_output(void);
extern "C" CompilerParserToken *__cdecl process_source_newline(void)
{
    BYTE character;
    unsigned category;
    int first_line;
    CompilerParserToken *token;
    CompilerArena *arena;
    CompilerPool *pool;
    for (;;) {
    repeat_line:
        if (c1_bss.options.preprocess_stdout) {
            fputc('\n', c1_bss.preprocessor_output);
            if (lexer_flags & 0x400000) {
                lexer_flags &= ~0x400000U;
                if (!c1_data.options.preprocess_no_lines) {
                    begin_line_output();
                    fputc('\n', c1_bss.preprocessor_output);
                }
            }
        }
        ++c1_bss.current_context.value;
        first_line = !c1_data.source_include_depth &&
            c1_bss.current_context.value == 1 && at_source_buffer_start();
        if (first_line) {
            if (begin_first_line_forced_include()) {
                ++c1_bss.source_cursor;
                continue;
            }
        }
        for (;;) {
            character = *c1_bss.source_cursor++;
            category = c1_data.scanner.classes[character];
            switch (category) {
            case SC_SPACE:
            whitespace:
                do {
                    if (c1_bss.options.preprocess_stdout && character != '\r')
                        fputc(character, c1_bss.preprocessor_output);
                    character = *c1_bss.source_cursor++;
                } while (c1_data.scanner.classes[character] == SC_SPACE);
                --c1_bss.source_cursor;
                continue;
            case SC_NEWLINE:
                ++c1_bss.current_context.value;
                first_line = !c1_data.source_include_depth &&
                    c1_bss.current_context.value == 1 && at_source_buffer_start();
                if (first_line && begin_first_line_forced_include()) {
                    ++c1_bss.source_cursor;
                    goto repeat_line;
                }
                /* The interior newline shares the whitespace emitter. It
                 * does not run the initial line-output-mode adjustment. */
                goto whitespace;
            case SC_HASH:
                lexer_flags |= 0x8000;
                token = read_directive_token();
                lexer_flags &= ~0x8000U;
                if (token->kind == 1) continue;
                return token;
            case SC_SLASH:
                if (c1_data.options.language_extensions || c1_data.options.option_zbe) {
                    if (skip_newline_comment()) continue;
                } else if (skip_language_comment()) continue;
                goto end_line;
            case SC_SPECIAL:
                if (character || lexical_skip_mode) {
#ifdef RECON_NATIVE_TEST
                    recon_unsupported("non-EOF newline special-token state is not yet modeled");
#else
                    __asm { int 3 }
#endif
                }
                if (!character || character == 0x1a) {
                    if (lexical_skip_mode) { process_special_character(); continue; }
                    if (!read_source_chunk()) continue;
                    c1_bss.literal.context.object = c1_bss.current_context.object;
                    c1_bss.literal.context.value = c1_bss.current_context.value;
                    /* This refill path uses the actual outlined allocator;
                     * the later line-location path advances inline. */
                    if (c1_bss.symbol_arena_kind)
                        token = (CompilerParserToken *)c1_bss.arenas[
                            c1_bss.symbol_arena_kind].AllocateBytes(32);
                    else {
                        pool = &c1_bss.pools[6];
                        if (pool->remaining < 32) pool->Refill(32);
                        token = (CompilerParserToken *)pool->cursor;
                        pool->cursor += 32;
                        pool->remaining -= 32;
                    }
                    if (token) { token->kind = 0xf0; memcpy(&token->value, &c1_bss.literal, 24); }
                    return token;
                }
                if (character == '?') {
                    if (process_trigraph() & ~0x40U) continue;
                } else if (splice_input_line()) continue;
                goto end_line;
            default:
            end_line:
                --c1_bss.source_cursor;
                if (!c1_data.source_include_depth && c1_bss.forced_source_index)
                    return LEXER_PROTOTYPE(4);
                if (lexer_flags & 0x200000) {
#ifdef RECON_NATIVE_TEST
                    recon_unsupported("newline location-token payload is not yet modeled");
#endif
                    c1_bss.literal.context.object = c1_bss.current_context.object;
                    c1_bss.literal.context.value = c1_bss.current_context.value;
                    LEXER_ALLOCATE_TOKEN();
                    if (token) { token->kind = 0xf0; memcpy(&token->value, &c1_bss.literal, 24); }
                    return token;
                }
                if ((lexer_flags & 0x20000) && !(lexer_flags & 0x20))
                    emit_control_record(36);
                return NULL;
            }
        }
    }
}

#undef LEXER_PEEK_CLASS
#undef LEXER_ALLOCATE_TOKEN
#undef LEXER_PROTOTYPE

unsigned RECON_THISCALL CompilerRecordedLexer::Read(void)
{
    CompilerContextBlock *block = read_buffer.block;
    if (read_buffer.position < 4 || read_buffer.position > 31) {
#ifdef __linux__
        recon_unsupported("recorded reader: unsupported block position");
#else
        __asm { int 3 }
#endif
    }
    if (read_buffer.position == 31) {
        if (block->next) {
            read_buffer.position = 4;
            block = block->next;
        } else block = NULL;
    }
    if (!block) return 0;
    read_buffer.block = block;
    unsigned index = read_buffer.position++;
    return block->tokens[index - 4];
}
unsigned RECON_THISCALL CompilerRecordedLexer::Peek(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerRecordedLexer::Unread(unsigned token)
{
    CompilerContextBlock *block = read_buffer.block;
    CompilerRecordedLexer *recorded;
    CompilerContextFrame *frame;
    unsigned arena_kind;
    if (!block || read_buffer.position < 5 || read_buffer.position > 31) {
#ifdef __linux__
        recon_unsupported("recorded unread: unsupported block position");
#else
        __asm { int 3 }
#endif
    }
    unsigned previous = block->tokens[read_buffer.position - 5];
    if (!token || !previous || ((CompilerParserToken *)token)->kind ==
        ((CompilerParserToken *)previous)->kind) {
        --read_buffer.position;
        block->tokens[read_buffer.position - 4] = token;
        return;
    }
    /* A different token kind is replayed in a separate real context. Its
     * initializer is still a genuine pending child; callbacks remain held. */
    arena_kind = c1_bss.symbol_arena_kind;
    if (arena_kind >= 6) {
#ifdef __linux__
        recon_unsupported("recorded unread: unsupported allocation arena");
#else
        __asm { int 3 }
#endif
    }
    recorded = (CompilerRecordedLexer *)c1_bss.arenas[arena_kind].AllocateBytes(64);
    if (recorded) recorded = recorded->Initialize(arena_kind, 1);
    c1_bss.context.Push((CompilerLexer *)recorded, arena_kind, 0, 0, &c1_bss.current_context);
    c1_bss.context.saved_frame->stream->Trace(token);
    c1_bss.context.saved_frame->stream->Trace((unsigned)c1_data.predefined_tokens[244]);
    frame = c1_bss.context.saved_frame;
    if (frame) {
        recorded = (CompilerRecordedLexer *)frame->stream;
        c1_bss.context.saved_frame = frame->next;
        c1_bss.context.object = frame->object;
        c1_bss.context.value = frame->value;
        if (frame->second) {
#ifdef __linux__
            recon_unsupported("recorded unread callback is not yet modeled");
#else
            __asm { int 3 }
#endif
            ((void (__cdecl *)(void))frame->second)();
        }
    } else recorded = NULL;
    c1_bss.current_context.Push((CompilerLexer *)recorded, 6, 1, 0, &c1_bss.current_context);
}
void RECON_THISCALL CompilerRecordedLexer::Trace(unsigned token)
{
    CompilerContextBlock *memory = start_buffer.block;
    if (!memory || start_buffer.position < 4 || start_buffer.position > 31) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("recorded token output position is outside its block");
#else
        __asm { int 3 }
#endif
    }
    if (start_buffer.position == 31) {
        if (memory->next) {
            start_buffer.position = 4;
            memory = memory->next;
        } else if (start_buffer.arena_kind == 6) {
#ifdef RECON_NATIVE_TEST
            recon_unsupported("fixed recorded-token chain has no following block");
#else
            __asm { int 3 }
#endif
            start_buffer.position = 31;
            memory = NULL;
        } else {
            if (start_buffer.arena_kind >= 6) {
#ifdef RECON_NATIVE_TEST
                recon_unsupported("recorded token allocation arena is not modeled");
#else
                __asm { int 3 }
#endif
            }
            CompilerArena *arena = &c1_bss.arenas[start_buffer.arena_kind];
            CompilerContextBlock *created = (CompilerContextBlock *)arena->cursor;
            arena->cursor += 128;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            if (created) created->next = NULL;
            memory->next = created;
            start_buffer.position = 4;
            memory = memory->next;
        }
    }
    start_buffer.block = memory;
    memory->tokens[start_buffer.position - 4] = token;
    ++start_buffer.position;
}
/* Valid shares the one canonical NullDebugModule::Slot7c physical body. */
void RECON_THISCALL CompilerRecordedLexer::Reset(void)
{ read_buffer = buffer; }
void *RECON_THISCALL CompilerRecordedLexer::Destroy(unsigned) { __asm { int 3 } return this; }
extern "C" unsigned __cdecl read_unspliced_character(void) { __asm { int 3 } return 0; }
extern "C" int __cdecl skip_line_comment(void) { __asm { int 3 } return 0; }
extern "C" int __cdecl skip_block_comment(void) { __asm { int 3 } return 0; }
extern "C" int __cdecl skip_language_comment(void) { __asm { int 3 } return 0; }
extern "C" unsigned __cdecl process_special_character(void) { __asm { int 3 } return 0; }
extern "C" unsigned __cdecl process_trigraph(void) { __asm { int 3 } return 0; }
extern "C" void __cdecl discard_input_character(void) { __asm { int 3 }  }
extern "C" int __cdecl splice_input_line(void) { __asm { int 3 } return 0; }
extern "C" unsigned __cdecl resolve_special_character(unsigned) { __asm { int 3 } return 0; }
extern "C" void __cdecl scan_quoted_literal(int, unsigned) { __asm { int 3 }  }
extern "C" void __cdecl copy_quoted_literal(unsigned, int) { __asm { int 3 }  }
extern "C" CompilerParserToken * __cdecl make_character_constant(unsigned, const void *, unsigned) { __asm { int 3 } return 0; }
extern "C" int __cdecl scan_identifier_continuation(const BYTE *, unsigned, unsigned);
/* RVA03dec. Only the low byte of the incoming character is consumed; the
 * original caller's other stack-slot bits are not identifier data. */
extern "C" int __cdecl scan_identifier(unsigned first)
{
    unsigned character = first & 0xff;
    unsigned hash = 0, folded;
    const BYTE *start = c1_bss.source_cursor - 1;
    int length;
    CompilerNameEntry **bucket, **link, *entry;
    CompilerPool *pool;
    CompilerArena *arena;
    while (c1_data.scanner.flags[character] & 3) {
        hash = (hash >> 4) + 4 * hash + character;
        character = *c1_bss.source_cursor++;
    }
    if (c1_data.scanner.classes[character] == SC_SPECIAL)
        return scan_identifier_continuation(start, hash, character);
    if (character == 4) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("encoded identifier continuation mode is not modeled");
#endif
        if (lexical_skip_mode > 0)
            return scan_identifier_continuation(start, hash, character);
    }
    --c1_bss.source_cursor;
    length = (int)(c1_bss.source_cursor - start);
    if (length >= 2048)
        compiler_fatal(64);
    folded = ((hash >> 16) ^ hash) & 0xffff;
    bucket = &c1_data.name_buckets[hash & 0x7ff];
    link = bucket;
    for (entry = *link; entry; link = &entry->next, entry = *link) {
        if ((entry->flags & 0xffff) == folded && entry->name[length] == 0 &&
            memcmp(start, entry->name, length) == 0) {
            *link = entry->next;
            entry->next = *bucket;
            *bucket = entry;
            c1_data.current_identifier = entry;
            return 0;
        }
    }
    pool = &c1_bss.pools[4];
    if (pool->remaining < 16)
        pool->Refill(16);
    entry = (CompilerNameEntry *)pool->cursor;
    pool->cursor += 16;
    pool->remaining -= 16;
    entry->next = *bucket;
    *bucket = entry;
    arena = &c1_bss.arenas[0];
    entry->name = (char *)arena->cursor;
    arena->cursor += (length + 4) & ~3U;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    memcpy(entry->name, start, length);
    entry->name[length] = 0;
    c1_data.current_identifier = entry;
    /* The upper flags and macro slot retain their allocation contents;
     * this scanner writes no invented initializer for either. */
    entry->flags = (entry->flags & 0xffff0000) | folded;
    return 0;
}
extern "C" int __cdecl scan_identifier_continuation(const BYTE *, unsigned, unsigned)
{ __asm { int 3 } return 0; }
extern "C" int __cdecl expand_macro(CompilerMacro *) { __asm { int 3 } return 0; }
extern "C" unsigned __cdecl collect_floating_token(char *) { __asm { int 3 } return 0; }
extern "C" CompilerParserToken * __cdecl scan_floating_token(char *) { __asm { int 3 } return 0; }
extern "C" int __cdecl finish_forced_include(void) { __asm { int 3 } return 0; }
extern "C" CompilerParserToken *__cdecl make_arena_token(unsigned arena_kind)
{
    CompilerParserToken *token;
    c1_bss.literal.context.object = c1_bss.current_context.object;
    c1_bss.literal.context.value = c1_bss.current_context.value;
    if (arena_kind) {
        if (arena_kind >= 6) {
#ifdef RECON_NATIVE_TEST
            recon_unsupported("location token arena is not modeled");
#else
            __asm { int 3 }
#endif
        }
        CompilerArena *arena = &c1_bss.arenas[arena_kind];
        token = (CompilerParserToken *)arena->cursor;
        arena->cursor += 32;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        CompilerPool *pool = &c1_bss.pools[6];
        if (pool->remaining < 32) pool->Refill(32);
        token = (CompilerParserToken *)pool->cursor;
        pool->cursor += 32;
        pool->remaining -= 32;
    }
    if (token) {
        token->kind = 0xf0;
        /* Authentic24-byte storage transfer. Only object/value have roles;
         * the other sixteen bytes are retained without interpretation. */
        memcpy(&token->value, &c1_bss.literal, 24);
    }
    return token;
}

/* The first-line caller has a live48-byte source frame. Its buffer field
 * is the original+20 pointer established by push_source_context. */
extern "C" int __cdecl at_source_buffer_start(void)
{
    return c1_bss.source_cursor == c1_data.source_frames->buffer;
}
extern "C" int __cdecl read_forced_include(void);
extern "C" int __cdecl begin_first_line_forced_include(void)
{
    --c1_bss.source_cursor;
    --c1_bss.current_context.value;
    if (read_forced_include())
        return 1;
    ++c1_bss.source_cursor;
    ++c1_bss.current_context.value;
    return 0;
}
/* The included-file token stream temporarily selects arena2. Both fields
 * are established by the original8-byte frame stores; this is not a blob. */
struct CompilerSymbolArenaFrame {
    unsigned arena_kind;
    CompilerSymbolArenaFrame *next;
};
typedef char CompilerSymbolArenaFrame_extent[sizeof(CompilerSymbolArenaFrame) == 8 ? 1 : -1];
const char *current_forced_include; /* Source-only BSS cell RVA54db0. */
extern "C" int __cdecl open_forced_include(const char *, int);
extern "C" CompilerParserToken *__cdecl make_source_state_token(unsigned);
extern "C" int __cdecl read_forced_include(void)
{
    CompilerVector *files;
    CompilerArena *arena;
    CompilerSymbolArenaFrame *saved;
    CompilerRecordedLexer *stream;
    CompilerTokenStream *recorded;
    CompilerContextFrame *frame;
    CompilerParserToken *state_token;
    for (;;) {
        files = c1_bss.pending_forced_includes;
        if (!files || c1_bss.forced_include_index >= files->count)
            return 0;
#ifdef RECON_NATIVE_TEST
        /* The current list is empty. No filename slot, include effect or
         * recorded-mode object below has a native contract yet. */
        recon_unsupported("nonempty forced-include stream is not yet modeled");
#endif
        current_forced_include = (const char *)files->items[c1_bss.forced_include_index++];
        if (open_forced_include(current_forced_include, 0) <= -1)
            continue;
        if (c1_bss.options.preprocess_stdout)
            return 1;
        arena = &c1_bss.arenas[2];
        saved = (CompilerSymbolArenaFrame *)arena->cursor;
        arena->cursor += 8;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        if (saved) {
            saved->arena_kind = c1_bss.symbol_arena_kind;
            saved->next = c1_bss.saved_symbol_arenas;
        }
        c1_bss.saved_symbol_arenas = saved;
        c1_bss.symbol_arena_kind = 2;
        stream = (CompilerRecordedLexer *)arena->AllocateBytes(64);
        if (stream) stream = stream->Initialize(2, 1);
        c1_bss.context.Push((CompilerLexer *)stream, 2, 0, 0, &c1_bss.current_context);
        state_token = make_source_state_token(0);
        c1_bss.context.saved_frame->stream->Trace((unsigned)state_token);
        c1_bss.context.saved_frame->stream->Trace((unsigned)c1_data.predefined_tokens[244]);
        frame = c1_bss.context.saved_frame;
        if (frame) {
            recorded = frame->stream;
            c1_bss.context.saved_frame = frame->next;
            c1_bss.context.object = frame->object;
            c1_bss.context.value = frame->value;
            if (frame->second) ((void (__cdecl *)(void))frame->second)();
        } else recorded = NULL;
        c1_bss.current_context.Push((CompilerLexer *)recorded, 2, 1, 0,
                                    &c1_bss.current_context);
        saved = c1_bss.saved_symbol_arenas;
        c1_bss.symbol_arena_kind = saved->arena_kind;
        c1_bss.saved_symbol_arenas = saved->next;
        return 1;
    }
}
CompilerRecordedLexer *RECON_THISCALL CompilerRecordedLexer::Initialize(unsigned, int)
{ __asm { int 3 } return 0; }
extern "C" int __cdecl open_forced_include(const char *, int)
{ __asm { int 3 } return 0; }
extern "C" CompilerParserToken *__cdecl make_source_state_token(unsigned)
{ __asm { int 3 } return 0; }
extern "C" CompilerParserToken *__cdecl read_directive_token(void) { __asm { int 3 } return 0; }
extern "C" int __cdecl skip_newline_comment(void) { __asm { int 3 } return 0; }

extern "C" void __cdecl scan_class192_attributes(void *);

CompilerParserToken *RECON_THISCALL CompilerDeclarationScanner::Scan(void)
{
    int result = 0;
    do {
#ifdef RECON_NATIVE_TEST
        if (token->kind >= 245)
            recon_unsupported("declaration token outside known class map");
#endif
        switch (parser_token_classes[token->kind]) {
        case 6: result = ScanIdentifier(); break;
        case 27: case 28: case 29: result = ScanClass27(); break;
        case 30: ScanClass30(); break;
        case 31: ScanTypeSpecifier(); break;
        case 32: ScanClass32(); break;
        case 33: ScanClass33(); break;
        case 34: ScanClass34(); break;
        case 40: result = ScanClass40(); break;
        case 192:
            scan_class192_attributes(&attributes);
            unmodeled_1c = 1;
            break;
        default: result = 1; break;
        }
        ++tokens_read;
        if (!result) token = c1_bss.current_context.ReadToken();
    } while (!result);
    if (result == 2) return token;
    c1_bss.current_context.saved_frame->stream->Unread((unsigned)token);
    if (type_attributes || !type) Normalize();
    return BuildToken(36);
}
int RECON_THISCALL CompilerDeclarationScanner::ScanIdentifier(void)
{
    if (type || base_type_flags) {
        lexer_flags |= 1;
        return 1;
    }
    lexer_flags |= 0x400;
    token = resolve_identifier_type(token);
    lexer_flags &= ~0x400U;
    if (token->kind == 30) {
        ScanClass30();
        return 0;
    }
    return tokens_read ? 1 : 2;
}
int RECON_THISCALL CompilerDeclarationScanner::ScanClass27(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerDeclarationScanner::ScanClass30(void) { __asm { int 3 } }
/* Class31's encountered base-type relation. The neighboring mixed-property
 * entries and unused token kinds are deliberately not represented here. */
struct DeclarationBaseType { BYTE kind; WORD flags; const char *name; };
static const DeclarationBaseType declaration_base_types[] = {
    {16, 0x0061, "char"}, {17, 0x0063, "int"},
    {18, 0x0106, "float"}, {19, 0x0107, "double"},
    {20, 0x0062, "short"}, {21, 0x0064, "long"},
    {22, 0x0080, "void"}, {24, 0x8000, "signed"},
    {25, 0x4000, "unsigned"}, {31, 0x00b5, "<type>"},
    {212, 0x0041, "__int8"}, {213, 0x0042, "__int16"},
    {214, 0x0043, "__int32"}, {215, 0x004c, "__int64"},
    {216, 0x004d, "__int128"}, {217, 0x0045, "wchar_t"},
    {231, 0x0060, "bool"}
};
extern "C" WORD __cdecl combine_base_type_flags(WORD, WORD);
void RECON_THISCALL CompilerDeclarationScanner::ScanTypeSpecifier(void)
{
    unsigned i;
    for (i = 0; i < sizeof(declaration_base_types) / sizeof(declaration_base_types[0]); ++i)
        if (declaration_base_types[i].kind == token->kind) break;
#ifdef RECON_NATIVE_TEST
    if (i == sizeof(declaration_base_types) / sizeof(declaration_base_types[0]))
        recon_unsupported("base type token outside the recovered relation");
#endif
    if (type) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("named type conflict diagnostic owner is not modeled");
#endif
        compiler_warning(628, unmodeled_24, declaration_base_types[i].name);
    } else if (!base_type_flags) {
        base_type_flags = declaration_base_types[i].flags;
    } else {
        base_type_flags = combine_base_type_flags(base_type_flags,
                                                  declaration_base_types[i].flags);
    }
}
extern "C" WORD __cdecl combine_base_type_flags(WORD, WORD)
{ __asm { int 3 } return 0; }
void RECON_THISCALL CompilerDeclarationScanner::ScanClass32(void) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarationScanner::ScanClass33(void) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarationScanner::ScanClass34(void) { __asm { int 3 } }
int RECON_THISCALL CompilerDeclarationScanner::ScanClass40(void) { __asm { int 3 } return 0; }
extern "C" CompilerType *__cdecl finish_type_description(CompilerType *);
void RECON_THISCALL CompilerDeclarationScanner::Normalize(void)
{
    CompilerType *description;
    if (!type) {
        if (!base_type_flags && !type_attributes) {
#ifdef RECON_NATIVE_TEST
            recon_unsupported("implicit declaration attribute storage is not yet modeled");
#endif
            attributes.flags |= 0x10;
            resolved_type = c1_data.builtin_types.type_0063;
            return;
        }
        if (!base_type_flags) base_type_flags = 0x63;
#ifdef RECON_NATIVE_TEST
        if (type_description_depth != 0)
            recon_unsupported("nested type-description stack index is not yet modeled");
#endif
        ++type_description_depth;
        description = &c1_bss.type_descriptions[type_description_depth];
        memset(description, 0, 16);
        description->flags = base_type_flags | 0x1000;
        description->attributes = type_attributes;
        resolved_type = finish_type_description(description);
        return;
    }
#ifdef RECON_NATIVE_TEST
    recon_unsupported("existing-type declaration normalization is not yet modeled");
#endif
    if (!type_attributes) {
        resolved_type = type;
        return;
    }
    /* This contextual DWORD's broader role remains unknown. */
    if (!unmodeled_28 &&
        (type->GetCallingConvention() & type_attributes)) {
        if (!c1_data.options.language_extensions) compiler_warning(289);
        else report_language_diagnostic(1, 114);
    }
    if (type->shape && (type->shape->flags & 7) == 4) {
        report_language_diagnostic(1,
            type->shape->base && (type->shape->base->flags & 7) == 4 ? 180 : 181);
        type_attributes = 0;
        resolved_type = type;
    } else resolved_type = type->WithAttributes(type_attributes);
}
extern "C" CompilerType *__cdecl intern_type(CompilerType *);
extern "C" CompilerType *__cdecl finish_type_description(CompilerType *description)
{
    WORD flags, base;
    CompilerType *result;
    if (!description) return c1_data.builtin_types.type_0063;
#ifdef RECON_NATIVE_TEST
    if (type_description_depth != 1 || description != &c1_bss.type_descriptions[1])
        recon_unsupported("type-description owner or nested index is not yet modeled");
#endif
    flags = description->flags;
    base = flags & 0x2fcf;
    if (base == 0x41 && c1_data.options.language_extensions &&
        c1_data.options.unsigned_char && !(flags & 0x8000)) flags |= 0x4000;
    if (!base) flags |= 0x63;
    flags &= ~0x3000;
    if (flags & 0xc000) {
        if ((flags & 0x40) || (flags & 0x800)) {
            if (c1_data.options.language_extensions || (flags & 0x2fcf) != 0x41)
                flags &= ~0x8000;
        } else {
            report_language_diagnostic(1, 76, 0x18 | ((flags >> 14) & 1));
            flags &= ~0xc000;
        }
    }
    description->flags = flags;
    result = intern_type(description);
    --type_description_depth;
    return result;
}
CompilerType *RECON_THISCALL CompilerType::WithAttributes(unsigned)
{ __asm { int 3 } return 0; }
CompilerParserToken *RECON_THISCALL CompilerDeclarationScanner::BuildToken(unsigned kind)
{
    CompilerArena *arena;
    CompilerPool *pool;
    CompilerParserToken *result;
    if (cached_token) return cached_token;
    if (!resolved_type) resolved_type = type;
    if (!(attributes.flags & 7)) attributes.flags |= 0x20;
    c1_bss.literal.declaration.flags = declaration_flags;
    c1_bss.literal.declaration.value = declaration_value;
    c1_bss.literal.declaration.type = resolved_type;
#ifdef RECON_NATIVE_TEST
    if (declaration_callback)
        recon_unsupported("nonNULL declaration member callback is not yet modeled");
#endif
    c1_bss.literal.declaration.unmodeled_0c = unmodeled_24;
#ifndef RECON_NATIVE_TEST
    c1_bss.literal.declaration.unmodeled_10 = declaration_callback;
#endif
    if (c1_bss.symbol_arena_kind) {
#ifdef RECON_NATIVE_TEST
        if (c1_bss.symbol_arena_kind >= 6)
            recon_unsupported("declaration token arena is not modeled");
#endif
        arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
        result = (CompilerParserToken *)arena->cursor;
        arena->cursor += 32;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &c1_bss.pools[6];
        if (pool->remaining < 32) pool->Refill(32);
        result = (CompilerParserToken *)pool->cursor;
        pool->cursor += 32;
        pool->remaining -= 32;
    }
    if (result) {
        result->kind = (BYTE)kind;
#ifdef RECON_NATIVE_TEST
        result->value.declaration.flags = c1_bss.literal.declaration.flags;
        result->value.declaration.value = c1_bss.literal.declaration.value;
        result->value.declaration.type = c1_bss.literal.declaration.type;
        result->value.declaration.context = c1_bss.literal.declaration.unmodeled_0c;
        result->value.declaration.apply = 0;
#else
        memcpy(&result->value, &c1_bss.literal, 24);
#endif
    }
    cached_token = result;
    return result;
}
extern "C" void __cdecl scan_class192_attributes(void *) { __asm { int 3 } }
