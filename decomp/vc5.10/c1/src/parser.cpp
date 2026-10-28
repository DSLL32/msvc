#include <string.h>
#include <stdarg.h>
#include <new.h>
#include "frontend.h"
#include "parser-grammar.h"
#include "keywords.h"
#include "packing.h"
#pragma intrinsic(memcpy, memset, memcmp, strcmp)
#include <recon.h>

extern "C" const signed char c1_primitive_widths[16];

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
struct CompilerSwitchCase {
    CompilerSwitchCase *next;
    BYTE unmodeled_04[4];
    union { __int64 value; struct { unsigned low, high; }; };
    unsigned label_index;
    BYTE unmodeled_14[4];
};
typedef char CompilerSwitchCase_extent[sizeof(CompilerSwitchCase) == 24 ? 1 : -1];
typedef char CompilerSwitchCase_value[offsetof(CompilerSwitchCase, value) == 8 ? 1 : -1];
struct ParserLabelStack {
    ParserLabelStack *next;
    union { CompilerParserNode *node; CompilerSwitchCase *cases; };
};
typedef char ParserLabelStack_extent[sizeof(ParserLabelStack) == 8 ? 1 : -1];
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
#define browser_parameter_references (c1_bss.options.browser_file != NULL) /* RVA54de8, -FR filename owner */
#define unevaluated_type_depth c1_data.unevaluated_type_depth /* RVA5f3a8, initialized0 */
static unsigned parser_symbol_cache_index;    /* RVA5f494 */
#define enumerator_type c1_data.enumerator_type /* RVA5e400 */
#define enumerator_value c1_data.enumerator_value /* RVA5f380 */
#define c1_lexer_token_cursor c1_data.quoted_cursor /* RVA5f38c */
#define loop_labels c1_bss.loop_labels /* RVA55090 */
#define continue_labels c1_bss.continue_labels /* RVA547f0 */
#define break_labels c1_bss.break_labels /* RVA54014 */
#define switch_labels c1_bss.switch_labels /* RVA54dd0, BSS NULL */
#define default_labels c1_bss.default_labels /* RVA54020, BSS NULL */
#define label_stack_count c1_bss.label_stack_count /* RVA550a8 */
#define case_label c1_bss.case_label /* RVA53ff8, BSS; statement entry clears it */
#define c1_lexer_token_text c1_bss.quoted_workspace /* RVA55120, one physical owner. */



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
    unsigned attributes_seen; /* Original+28: explicit storage/attribute specifier writer1. */
    CompilerType *type;
    void *declaration_context; /* +36: declaration token carries its tag/context owner. */
    DWORD named_type_seen; /* +40: ScanClass34 writes1; whole scanner clear supplies0. */
    CompilerDeclarationApply declaration_callback; /* Original4B/native8B member pointer. */
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
#ifdef RECON_NATIVE_TEST
/* The wider native member pointer consumes four representation bytes only. */
typedef char CompilerDeclarationScanner_flags_offset[offsetof(CompilerDeclarationScanner, base_type_flags) == 52 ? 1 : -1];
typedef char CompilerDeclarationScanner_extent[sizeof(CompilerDeclarationScanner) == 56 ? 1 : -1];
#else
typedef char CompilerDeclarationScanner_flags_offset[offsetof(CompilerDeclarationScanner, base_type_flags) == 48 ? 1 : -1];
typedef char CompilerDeclarationScanner_extent[sizeof(CompilerDeclarationScanner) == 52 ? 1 : -1];
#endif

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
void __cdecl begin_record_definition(CompilerIrSymbol *, CompilerType *); /* RVA0ba0b */
int __cdecl add_record_member(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *); /* RVA0bb41 */
CompilerType * __cdecl parse_record_specifier(void); /* RVA0c0a9 */
WORD __cdecl merge_declarator_qualifiers(WORD, WORD); /* RVA0ca41 */
void __cdecl process_declaration(const CompilerParserDeclaration *); /* RVA0d706 */
BYTE __cdecl prepare_bitfield(CompilerIrSymbol *, __int64); /* RVA0e4d2 */
void __cdecl add_enumerator(CompilerNameEntry *, CompilerType *, __int64); /* RVA0eed5 */
CompilerType * __cdecl parse_enum_specifier(void); /* Distinct physical record-id callback owners, separate from pointer-name callbacks. */
extern "C" unsigned __cdecl hash_record_id(void *value) { return (unsigned)value; }
extern "C" int __cdecl record_ids_equal(void *left, void *right) { return left == right; }
/* RVA0f092 */
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
CompilerParserNode * __cdecl make_right_typed_expression(BYTE, CompilerIrSymbol *, CompilerParserNode *); /* RVA1bdf6 */
CompilerParserNode * __cdecl make_type_size_expression(CompilerType *); /* RVA1ce98 */
CompilerParserNode * __cdecl make_member_expression(BYTE, CompilerParserNode *, CompilerNameEntry *); /* RVA1d38e */
void __cdecl emit_return_statement(CompilerParserNode *, int); /* RVA1d5f2 */
CompilerIrSymbol * __cdecl prepare_initializer_symbol(CompilerIrSymbol *); /* RVA1df6d */
CompilerIntegerConstant *__cdecl make_integer_constant(int);
CompilerParserNode * __cdecl make_based_expression(CompilerNameEntry *); /* RVA1e277, named call designator */
CompilerParserNode * __cdecl normalize_based_expression(CompilerParserNode *); /* RVA1e3a8 */
CompilerParserNode * __cdecl make_expression_with_type(CompilerParserNode *, CompilerParserNode *, unsigned); /* RVA1e68f */
CompilerParserNode * __cdecl finish_argument_list(CompilerParserNode *); /* RVA1e8dc */
void __cdecl emit_expression(CompilerParserNode *); /* RVA1eba0 */
void __cdecl emit_conditional_branch(CompilerParserNode *, CompilerParserNode *, int); /* RVA1ec07 */
void __cdecl begin_switch_statement(CompilerParserNode *); /* RVA1f1ad */
void __cdecl emit_case_label(CompilerParserNode *); /* RVA1f312 */
void __cdecl finish_switch_statement(void); /* RVA1f54b */
void __cdecl emit_branch(CompilerParserNode *, CompilerParserNode *, CompilerParserNode *); /* RVA1f6d3 */
void __cdecl apply_initializer(CompilerIrSymbol *, CompilerParserNode *); /* RVA1f7d8 */
void __cdecl finish_loop_statement(void); /* RVA1f9c4 */
void __cdecl finish_initializer(CompilerIrSymbol *, int); /* RVA207c2 */
void __cdecl finish_aggregate_initializer(CompilerParserNode *); /* RVA20a13 */
void __cdecl finish_initializer_element(void); /* RVA21166 */
void __cdecl begin_aggregate_initializer(CompilerIrSymbol *); /* RVA21208 */
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
void __cdecl append_literal_text(CompilerStringValue *, const CompilerStringValue *); /* RVA24f93 */
CompilerIrSymbol * __cdecl find_statement_label(CompilerNameEntry *); /* RVA25125 */
CompilerIrSymbol * __cdecl make_statement_label(CompilerNameEntry *); /* RVA2517f */
void __cdecl define_statement_label(CompilerNameEntry *); /* RVA251d5 */
CompilerIrSymbol * __cdecl make_parameter_description(CompilerNameEntry *); /* RVA25ba5 */
CompilerIrSymbol *__cdecl add_old_style_parameter(const CompilerParserDeclaration *, CompilerIrSymbol *, CompilerDeclarator *); /* RVA25c26 */
void __cdecl prepare_for_statement(void); /* RVA25eb0 */
void __cdecl report_expected_parser_token(CompilerParserToken *, int); /* RVA3f1b4 */
CompilerParserValue * __cdecl make_recovery_parser_value(CompilerParserValue *, int); /* RVA3f281 */
void __cdecl report_parser_error(const char *); /* RVA3f2c2 */
void __cdecl skip_declaration_tokens(void); /* RVA3f393 */
CompilerParserToken * __cdecl recover_parser_token(CompilerParserToken *, ...); /* RVA3f3ff */
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
void __cdecl initialize_string_literal_type(CompilerParserNode *);
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
            if (c1_bss.parser_value.typed_constant.type->flags & 0x100)
                value_top->value.real_constant.value = c1_bss.parser_value.real_constant.value;
            else value_top->value.typed_constant.value = c1_bss.parser_value.typed_constant.value;
        } /* State148 shifts a following string into production221. Its
         * WORD length/flags travel with the text pointer, just as at77. */
        else if (state == 78 || state == 77 || state == 148)
            value_top->value.string_attributes = c1_bss.parser_value.string_attributes;
        else if (state == 16 || state == 15 || state == 12 || state == 10 || state == 22 || state == 17 || state == 20 || state == 18 || state == 176 || state == 245)
            value_top->value.declaration = c1_bss.parser_value.declaration;
        else if (state == 254 || state == 31 || state == 149 || state == 220 || state == 276)
            value_top->value.attributes = c1_bss.parser_value.attributes;
        else if (state == 103 || state == 104 || state == 163 || state == 268)
            value_top->value.integer = c1_bss.parser_value.integer;
        else if (state == 234 || state == 256 || state == 98 || state == 27 || state == 101 || state == 211 || state == 212 || state == 213 || state == 150 || state == 218 || state == 222 || state == 219 || state == 277 || state == 223 || state == 205 || state == 86 || state == 33)
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
                    } else if (lookahead->kind == 7) {
                        c1_bss.parser_value.string_attributes = lookahead->value.string_attributes;
                    } else if (lookahead->kind == 101) {
                        c1_bss.parser_value.typed_constant.type = lookahead->value.typed_constant.type;
                        if (lookahead->value.typed_constant.type->flags & 0x100)
                            c1_bss.parser_value.real_constant.value = lookahead->value.real_constant.value;
                        else c1_bss.parser_value.typed_constant.value = lookahead->value.typed_constant.value;
                    } else if (lookahead->kind == 34 || lookahead->kind == 35) {
                        if (lookahead->value.declaration.apply != &CompilerTranslationUnit::InsertDeclaration ||
                            !lookahead->value.declaration.context ||
                            !lookahead->value.declaration.type ||
                            (lookahead->kind == 35 ?
                                lookahead->value.declaration.type != c1_data.builtin_types.type_0063 :
                                !(lookahead->value.declaration.type->flags & 0x400)))
                            recon_unsupported("tag declaration alternative is not yet modeled");
                        c1_bss.parser_value.declaration = lookahead->value.declaration;
                    } else if (lookahead->kind == 36) {
                        if (lookahead->value.declaration.apply != 0)
                            recon_unsupported("nonNULL declaration callback is not yet modeled");
                        c1_bss.parser_value.declaration = lookahead->value.declaration;
                    } else if (lookahead->kind == 118 || lookahead->kind == 126 || lookahead->kind == 117 ||
                               lookahead->kind == 52 || lookahead->kind == 65 || lookahead->kind == 122) {
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
        } else if (rule == 33) {
            if (state_top[1] != 17)
                recon_unsupported("declaration reclassification source state is not yet modeled");
            c1_bss.parser_value.declaration = value_top[1].value.declaration;
        } else if (rule == 37 || rule == 34) {
            if ((rule == 37 && state_top[1] != 20) || (rule == 34 && state_top[1] != 18))
                recon_unsupported("record definition reduction source state is not yet modeled");
            c1_bss.parser_value.declaration = value_top[1].value.declaration;
        } else if (rule == 40) {
            if (state_top[1] != 22 || state_top[2] != 47)
                recon_unsupported("enum opening reduction source states are not yet modeled");
            c1_bss.parser_value.declaration = value_top[1].value.declaration;
        } else if (rule == 55) {
            if (state_top[1] != 212 || !value_top[1].value.declarator_symbol.declarator ||
                !value_top[1].value.declarator_symbol.symbol)
                recon_unsupported("record member declarator source is not yet modeled");
            c1_bss.parser_value.declarator_symbol = value_top[1].value.declarator_symbol;
        } else if (rule == 56) {
            if (state_top[1] != 213 || state_top[2] != 242 || state_top[3] != 268 ||
                !value_top[1].value.declarator_symbol.symbol)
                recon_unsupported("bitfield declarator and integer source are not yet modeled");
            c1_bss.parser_value.declarator_symbol = value_top[1].value.declarator_symbol;
        } else if (rule == 78 || rule == 77) {
            if ((rule == 78 && state_top[1] != 27 && state_top[1] != 150 && state_top[1] != 101 && state_top[1] != 218) || (rule == 77 && ((state_top[1] != 27 && state_top[1] != 212 && state_top[1] != 150 && state_top[1] != 101 && state_top[1] != 218) || state_top[2] != 55 ||
                    state_top[3] != 103 || state_top[4] != 169)))
                recon_unsupported("declarator pair reduction source state is not yet modeled");
            c1_bss.parser_value.declarator_symbol = value_top[1].value.declarator_symbol;
        } else if (rule == 120) {
            if (state_top[1] != 234)
                recon_unsupported("abstract array declarator source state is not yet modeled");
            c1_bss.parser_value.declarator_symbol = value_top[1].value.declarator_symbol;
        } else if (rule == 112) {
            if (state_top[1] != 222)
                recon_unsupported("abstract declarator pair source state is not yet modeled");
            c1_bss.parser_value.declarator_symbol = value_top[1].value.declarator_symbol;
        } else if (rule == 176) {
            if (state_top[1] != 104)
                recon_unsupported("array bound integer source state is not yet modeled");
            c1_bss.parser_value.integer = value_top[1].value.integer;
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
        case 11: /* Reads only the established lexer diagnostic flag. */
        case 13: /* Existing declaration view enters its genuine standalone-declaration owner. */
        case 20:
        case 21: /* Old-style declaration-list continuation clears the existing control flags. */
        case 16:
        case 18:
        case 19:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 28: /* Existing declaration/declarator pair enters the old-style parameter owner. */
        case 30:
        case 31:
        case 32:
        case 33: /* Existing declaration alternative is reclassified through the real scanner. */
        case 34: /* Calls the genuine record finalizer before consuming its result. */
        case 35: /* Calls the genuine enum-type finalizer before consuming a payload. */
        case 36: /* Existing record token then the real definition initializer. */
        case 37:
        case 38:
        case 39:
        case 40: /* Enum declaration plus opening brace, then real member callback. */
        case 41:
        case 42:
        case 43: /* First enum identifier uses the established enum type/value owners. */
        case 44: /* Explicit enumerator: name, assignment and the established QWORD constant. */
        case 45:
        case 46:
        case 47: /* Member terminator clears the established temporary arena/control bits. */
        case 48: /* Existing completed type enters the anonymous-member owner. */
        case 49:
        case 50: /* Established record-member declarator then genuine member owner. */
        case 51: /* A comma-separated member retains the same declarator/symbol pair. */
        case 55: /* The second DWORD is the actual declaration symbol. */
        case 56: /* Existing member symbol and the established two-DWORD bit width. */
        case 58:
        case 59:
        case 60:
        case 62: /* Established declarator pair, then the genuine initialized-declaration owner. */
        case 63: /* Established declared symbol and string expression before genuine application. */
        case 64: /* Established initializer symbol, then the genuine finish boundary. */
        case 65: /* The preserved initializer symbol is passed to its genuine completion owner. */
        case 66: /* Actual declarator pair and assignment token, before initializer preparation. */
        case 68:
        case 69:
        case 70: /* Existing expression-node word, then genuine aggregate completion. */
        case 71: /* No semantic operand; enters the genuine initializer-element helper. */
        case 72: /* No semantic operand; enters genuine element completion. */
        case 73:
        case 74: /* Shifted identifier and the established convention-attribute record. */
        case 76: /* Calling-convention/pointer qualifiers and the same declarator pair. */
        case 75: /* Pointer prefix consumes the established empty qualifier record and declarator pair. */
        case 118: /* Abstract pointer uses the same established qualifier/pair roles. */
        case 77: /* Established declarator plus integer bound, then real AppendArrayBound. */
        case 80: /* Parenthesized declarator preserves its pair and closes one level. */
        case 78:
        case 79:
        case 81:
        case 82:
        case 83:
        case 84: /* Builds the actual parameter-list descriptor from the active scope. */
        case 86: /* Identifier token enters the genuine old-style parameter constructor. */
        case 90:
        case 85:
        case 87:
        case 88: /* The written abstract parameter enters the existing scope/link owners. */
        case 96:
        case 94:
        case 95:
        case 99: /* Abstract parameter uses the explicitly written declarator/symbol pair. */
        case 126: /* Ellipsis creates the actual named-parameter sentinel in arena2. */
        case 114: /* Genuine abstract symbol plus48-byte declarator constructor. */
        case 98: /* Established parameter declaration and pointer declarator. */
        case 103: /* Calling-convention BYTE uses its recovered qualifier relation. */
        case 100:
        case 105: /* Explicitly writes both qualifier DWORDs before any consumer. */
        case 106:
        case 112:
        case 113:
        case 122:
        case 120: /* Abstract array bound consumes the known declarator pair and integer. */
        case 123: /* Parenthesized abstract declarator retains its established pair. */
        case 124:
        case 127: /* Established CLI predicates, then genuine compound entry. */
        case 128: /* Genuine compound exit, with no semantic operand. */
        case 129:
        case 130:
        case 131: /* Declaration-list continuation has no semantic action. */
        case 132:
        case 135:
        case 136:
        case 134: /* Error-statement recovery enters the genuine ClearRecovery child. */
        case 138: /* Expression statement consumes its established expression node. */
        case 137:
        case 139:
        case 147: /* No semantic operand; reaches genuine loop-header preparation. */
        case 148: /* Existing loop/continue/break heads and established condition node. */
        case 170: /* Known identifier name reaches its genuine label definition. */
        case 140: /* Completes the if statement at its established semantic label. */
        case 141: /* The if/else continuation consumes the label written by rule174. */
        case 142: /* Established while condition uses the existing loop emitter. */
        case 143: /* Established loop heads enter the genuine shared completion. */
        case 146:
        case 144: /* The three established for-expression nodes enter their genuine emitter. */
        case 145: /* Ends the header's known declaration-control flags. */
        case 150: /* Existing switch heads enter the genuine completion owner. */
        case 172: /* Default label uses the established label/root owners. */
        case 149: /* Established switch operand enters its genuine setup owner. */
        case 151: /* No semantic operand before the genuine __leave emitter. */
        case 152: /* Break consumes the existing switch or loop label. */
        case 153: /* Continue uses the existing active for-loop label. */
        case 155: /* Return expression, then the genuine emitter boundary. */
        case 156: /* Known identifier name enters the genuine statement-label lookup. */
        case 157: /* No semantic operand before the genuine inline-assembly owner. */
        case 158: /* No semantic operand before genuine exception-handler teardown. */
        case 159: /* No semantic operand before the genuine finally-region entry. */
        case 160: /* No semantic operand before the genuine finally-region completion. */
        case 161:
        case 164: /* No semantic operand before the genuine exception-region entry. */
        case 165: /* No semantic operand before the genuine region completion. */
        case 166: /* No semantic operand before the genuine filter-mode entry. */
        case 167: /* Established expression node enters the genuine filter emitter. */
        case 171: /* Established case constant, then the genuine case-label owner. */
        case 168:
        case 169:
        case 173: /* Established condition node and the genuine if-label producer. */
        case 174: /* The else edge consumes rule173's label and writes its own. */
        case 175: /* Explicitly writes both DWORDs of an omitted array bound. */
        case 176: /* Propagates the written two-DWORD integer bound. */
        case 177: /* Established integer-expression node, then genuine constant-value helper. */
        case 179:
        case 180: /* Relational operation byte and two established expression nodes. */
        case 181: case 182: case 184: case 185:
        case 189: case 190: case 192:
            /* The same two-node/operation-byte alternative; child guards
             * retain unsupported operator/type domains before consumption. */
        case 186: /* Multiplicative operation and the two established expression nodes. */
        case 187: /* Equality compares the existing scalar expression nodes. */
        case 191: /* Logical conjunction combines the established scalar predicates. */
        case 183: /* Additive operation byte and the established pointer/integer nodes. */
        case 188: /* Bitwise operation byte and the established member/integer nodes. */
        case 193: /* Compound assignment uses the same established node/operation roles. */
        case 194: /* Two established operand nodes and the actual operation byte. */
        case 195: /* Conditional expression uses its three established node operands. */
        case 196:
        case 197: /* Postfix increment has the actual operand and explicitly written constant1. */
        case 199: /* Established operand node, then genuine unary-minus helper. */
        case 200: /* Complement uses the same established operand node. */
        case 201: /* Logical negation consumes the established operand node. */
        case 203: /* Parenthesized expression reuses its node and closes one control level. */
        case 204: /* Dereference retains the actual operand node and enters unary conversion. */
        case 202: /* Prefix increment/decrement writes constant1 and its operation byte. */
        case 205: /* Address-of enters the existing unary/lvalue conversion owners. */
        case 206: /* Known expression node reaches its genuine sizeof helper. */
        case 207: /* Known abstract declaration supplies its established type. */
        case 208:
        case 209:
        case 211: /* Reached subscript uses the existing node/operator/node alternative. */
        case 214: /* Member-name lookup suppresses typedef reinterpretation. */
        case 215: /* Established operation/base node/name enter the genuine member owner. */
        case 217: /* Existing identifier name, then genuine pending expression helper. */
        case 218:
        case 219: /* Existing string node passes directly to its genuine type initializer. */
        case 220: /* Writes kind, flags and the complete defined string payload. */
        case 221: /* Adjacent string payload enters its genuine concatenation owner. */
        case 222: /* Opens the existing unevaluated-expression depth counter. */
        case 223: /* Reuses the established node and closes declaration depth. */
        case 224: /* Existing declaration record and abstract declarator/symbol pair. */
        case 210: /* Two established nodes reach the genuine right-typed owner. */
        case 225: /* Established expression node enters the genuine pending conversion. */
        case 226: /* Actual call-target node enters the existing call-context setup. */
        case 227: /* First call argument enters its genuine node-list constructor. */
        case 228: /* Subsequent actual arguments retain the same list/node owners. */
        case 213: /* Complete call target plus its established argument list. */
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
    if (declaration_depth < 0 || declaration_depth > 3 || (lookahead && lookahead->kind == 118))
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
    /* RVA518a0 is the existing literal owner. Win32 transfers20 declaration
     * bytes, then24 through the token, including an unwritten final DWORD.
     * Native transfers only the known declaration fields and its genuine
     * wider member pointer; no equality/default is claimed for that DWORD. */
    lexer_flags |= 0x80000000U;
#ifdef RECON_NATIVE_TEST
    c1_bss.literal.parser_declaration = c1_bss.parser_value.declaration;
#else
    memcpy(&c1_bss.literal, &c1_bss.parser_value.declaration, 20);
#endif
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
#ifdef RECON_NATIVE_TEST
        temporary_token->value.declaration = c1_bss.literal.parser_declaration;
#else
        memcpy(&temporary_token->value, &c1_bss.literal, 24);
#endif
    }
    temporary_token = classify_identifier_token(temporary_token);
    lexer_flags &= ~0x80000000U;
#ifdef RECON_NATIVE_TEST
    c1_bss.parser_value.declaration = temporary_token->value.declaration;
#else
    memcpy(&c1_bss.parser_value.declaration, &temporary_token->value, 20);
#endif
    break;
}
case 34:
    c1_bss.parser_value.declaration.type = parse_record_specifier();
    break;
case 35:
    c1_bss.parser_value.declaration.type = parse_enum_specifier();
    break;
case 36:
#ifdef RECON_NATIVE_TEST
    if (state_top[0] != 20 || !right[0].value.declaration.context ||
        right[0].value.declaration.apply != &CompilerTranslationUnit::InsertDeclaration ||
        !right[0].value.declaration.type || !(right[0].value.declaration.type->flags & 0x400))
        recon_unsupported("record definition semantic source is not yet modeled");
#endif
    if (right[0].value.declaration.apply)
        (c1_data.translation_unit->*right[0].value.declaration.apply)(
            right[0].value.declaration.context);
    begin_record_definition((CompilerIrSymbol *)right[0].value.declaration.context,
        right[0].value.declaration.type);
    break;
case 40:
#ifdef RECON_NATIVE_TEST
    if (right[-1].value.declaration.apply != &CompilerTranslationUnit::InsertDeclaration ||
        !right[-1].value.declaration.context)
        recon_unsupported("enum opening member callback is not yet modeled");
#endif
    if (right[-1].value.declaration.apply)
        (c1_data.translation_unit->*right[-1].value.declaration.apply)(
            right[-1].value.declaration.context);
    break;
case 43:
    enumerator_value = next_enumerator_value(enumerator_value, right[0].value.name);
    add_enumerator(right[0].value.name, enumerator_type, enumerator_value);
    break;
case 44:
#ifdef RECON_NATIVE_TEST
    if (state_top[1] != 46 || state_top[2] != 92 || state_top[3] != 163)
        recon_unsupported("explicit enumerator semantic source states not yet modeled");
#endif
    add_enumerator(right[-2].value.name, enumerator_type, right[0].value.integer);
    if (!unevaluated_expression_depth && !type_description_depth)
        c1_bss.arenas[3].ClearUsed();
    break;
case 47:
#ifdef RECON_NATIVE_TEST
    if (declaration_depth != 0)
        recon_unsupported("record member declaration control index is not yet modeled");
#endif
    if (!unevaluated_expression_depth && !type_description_depth)
        c1_bss.arenas[3].ClearUsed();
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 48:
#ifdef RECON_NATIVE_TEST
    if (declaration_depth != 0 || !declaration_specs[0].type)
        recon_unsupported("anonymous member declaration source is not yet modeled");
#endif
    add_record_member(&declaration_specs[declaration_depth], NULL, NULL);
    break;
case 50:
case 51:
case 52:
    if (rule == 52)
        report_language_diagnostic(1, 228);
#ifdef RECON_NATIVE_TEST
    if ((rule != 50 && rule != 51) || declaration_depth != 0 || c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1 ||
        !right[0].value.declarator || !right[0].value.declarator_symbol.symbol)
        recon_unsupported("record member semantic alternative is not yet modeled");
#endif
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
    c1_bss.parser_value.declarator_symbol.symbol->kind = 3;
    break;
case 56:
    c1_bss.parser_value.declarator_symbol.symbol->kind = 2;
    c1_bss.parser_value.declarator_symbol.symbol->bit_width = prepare_bitfield(c1_bss.parser_value.declarator_symbol.symbol,
        right[0].value.integer);
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
    if (declaration_depth < 0 || declaration_depth > 1 ||
        !right[0].value.declarator || !right[0].value.declarator_symbol.symbol) {
#ifdef __linux__
        recon_unsupported("initialized declaration semantic source is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
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
        finish_initializer(right[0].value.symbol, 0);
    break;
case 65:
    begin_aggregate_initializer(right[-5].value.symbol);
    break;
case 66:
    if (declaration_depth < 0 || declaration_depth > 1 ||
        !right[-1].value.declarator || !right[-1].value.declarator_symbol.symbol) {
#ifdef __linux__
        recon_unsupported("aggregate initializer declarator source is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
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
#ifdef RECON_NATIVE_TEST
    if (!((rule == 75 && state_top[3] == 150 &&
           ((state_top[1] == 221 && state_top[2] == 254) ||
            (state_top[1] == 32 && state_top[2] == 83))) ||
          (rule == 118 && state_top[3] == 277 &&
           ((state_top[1] == 221 && state_top[2] == 254) ||
            (state_top[1] == 236 && state_top[2] == 264)))) ||
        !right[0].value.declarator_symbol.declarator || !right[0].value.declarator_symbol.symbol)
        recon_unsupported("pointer declarator semantic source states are not yet modeled");
#endif
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
    c1_bss.parser_value.symbol = make_parameter_description(right[0].value.name);
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
#ifdef RECON_NATIVE_TEST
    if ((declaration_depth != 1 && declaration_depth != 2) ||
        state_top[1] != 176 || state_top[2] != 218 ||
        !right[0].value.declarator_symbol.declarator || !right[0].value.declarator_symbol.symbol ||
        right[-1].value.declaration.flags != 0x20 || right[-1].value.declaration.value ||
        right[-1].value.declaration.apply)
        recon_unsupported("parameter declaration semantic source is not yet modeled");
#endif
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
    c1_bss.parser_value.declarator_symbol.symbol = symbol;
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
    if (parameter_nesting != 1 || c1_data.record_sizes.known.named_parameter_symbol_bytes != 52) {
#ifdef __linux__
        recon_unsupported("ellipsis parameter allocation outside the established arena2 domain");
#else
        __asm { int 3 }
#endif
    }
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
            memset(symbol, 0, c1_data.record_sizes.known.named_parameter_symbol_bytes);
            binding->name = name;
            symbol->kind = 11;
        }
    }
    if (!symbol) {
        storage = &c1_bss.arenas[arena_kind];
        symbol = (CompilerIrSymbol *)storage->cursor;
        storage->cursor += (c1_data.record_sizes.known.named_parameter_symbol_bytes + 3) & ~3U;
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
    finish_loop_statement();
    break;
case 144:
    emit_branch(right[-4].value.node, right[-2].value.node, right[0].value.node);
    break;
case 145:
    if (declaration_depth > 0)
        --declaration_depth;
    if (declaration_depth < 0 || declaration_depth > 1) {
#ifdef __linux__
        recon_unsupported("for-header declaration-control index is not modeled");
#else
        __asm { int 3 }
#endif
    }
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 147:
    prepare_for_statement();
    break;
case 148:
    if (declaration_depth < 0 || declaration_depth > 2) {
#ifdef __linux__
        recon_unsupported("do-loop declaration-control index is not modeled");
#else
        __asm { int 3 }
#endif
    }
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
    begin_switch_statement(right[-1].value.node);
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
    if (declaration_depth < 0 || declaration_depth > 2) {
#ifdef __linux__
        recon_unsupported("if declaration-control index is not modeled");
#else
        __asm { int 3 }
#endif
    }
    declaration_control[declaration_depth].flags &= ~3;
    break;
case 174:
    c1_bss.parser_value.node = make_internal_label(30);
    write_ir_record(58, c1_bss.parser_value.node);
    emit_ir_label(right[-2].value.node);
    emit_control_record(36);
    if (declaration_depth < 0 || declaration_depth > 2) {
#ifdef __linux__
        recon_unsupported("else declaration-control index is not modeled");
#else
        __asm { int 3 }
#endif
    }
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
    c1_bss.parser_value.node = make_unary_expression(118, right[-1].value.node);
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
        right[-1].value.symbol, right[0].value.node);
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
        right[-3].value.node, right[0].value.name);
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
    initialize_string_literal_type(right[0].value.node);
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
    record->text = right[0].value.string_attributes.text;
    record->string_length = right[0].value.string_attributes.length;
    record->string_flags = right[0].value.string_attributes.flags;
    break;
}
case 221:
    /* The node's text/length/flags suffix is the same eight-byte string view. */
    append_literal_text((CompilerStringValue *)&right[-1].value.node->text,
        &right[0].value.string_attributes);
    break;
case 222:
    ++unevaluated_expression_depth;
    break;
case 223:
    c1_bss.parser_value.symbol = right[-1].value.symbol;
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
    c1_bss.parser_value.node = make_based_expression(right[-1].value.name);
    break;
case 226:
    c1_bss.parser_value.node = normalize_based_expression(right[-1].value.node);
    break;
case 227:
    c1_bss.parser_value.node = make_expression_with_type(NULL, right[0].value.node, 0);
    break;
case 228:
    c1_bss.parser_value.node = make_expression_with_type(right[-2].value.node,
        right[0].value.node, 0);
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
        /* Ordinary grammar classes return the token without interpreting
         * payload bytes. Hold only the special declaration alternatives whose
         * consumed state has not yet been established. */
        if (token_class == PARSER_CLASS_36 || token_class == PARSER_BASED) {
#ifdef __linux__
            recon_unsupported("special declaration-token state is not yet modeled");
#else
            __asm { int 3 }
#endif
        }
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
                if (declaration_depth < 0 || declaration_depth > 4)
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
                if (declaration_depth < 0 || declaration_depth > 4)
                    recon_unsupported("literal declaration-control index is not modeled");
#endif
                declaration_control[declaration_depth].flags |= 2;
            }
            return token;
        case PARSER_CLASS_118:
            if (!(lexer_flags & 0x10000000)) {
#ifdef RECON_NATIVE_TEST
                /* This path establishes slots1..4's NULL context and flag
                 * byte. It does not initialize a declaration-spec record. */
                if (declaration_depth < 0 || declaration_depth > 3)
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
            if (declaration_depth < 0 || declaration_depth > 4)
                recon_unsupported("return declaration-control index is not modeled");
#endif
            if (!(lexer_flags & 0x10000000))
                declaration_control[declaration_depth].flags |= 3;
            return token;
        case PARSER_SIZEOF:
            if (!(lexer_flags & 0x10000000) && (declaration_depth < 0 || declaration_depth > 4)) {
#ifdef __linux__
                recon_unsupported("sizeof declaration-control index is not modeled");
#else
                __asm { int 3 }
#endif
            }
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
        if (declaration_depth < 0 || declaration_depth > 3)
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
/* Encountered extended scalars in initialized .datafe. The precision word
 * belongs to the twelve-byte value; FLD reads only the ten-byte number. */
static InitializerExtendedReal extended_zero = {0, 0, 0, 4}; /* RVA5f3b8 */
static InitializerExtendedReal extended_one = {0, 0x80000000U, 0x3fff, 8}; /* RVA5f3c8 */
int __cdecl classify_literal_bits(unsigned, const void *, const void *);
CompilerType *__cdecl copy_type_description(const CompilerType *);
CompilerType *__cdecl intern_type(CompilerType *);
CompilerParserNode *__cdecl make_typed_constant(CompilerType *type, const void *value, unsigned arena_kind)
{
#ifdef RECON_NATIVE_TEST
    if (arena_kind >= 6 || (!(type->flags & 0x40) &&
        type != c1_data.builtin_types.type_0106 && type != c1_data.builtin_types.type_0107))
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
        if (classify_literal_bits(6, (const void *)node->low, &extended_zero) ||
            classify_literal_bits(6, (const void *)node->low, &extended_one))
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
int __cdecl classify_literal_bits(unsigned operation, const void *first, const void *second)
{
    const InitializerExtendedReal *left = (const InitializerExtendedReal *)first;
    const InitializerExtendedReal *right = (const InitializerExtendedReal *)second;
    unsigned left_exponent = left->exponent & 0x7fff;
    unsigned right_exponent = right->exponent & 0x7fff;
    WORD status, relation;
    if (left_exponent == 0x7fff || right_exponent == 0x7fff ||
        (left_exponent ? !(left->significand_high & 0x80000000U) :
            (left->significand_low || left->significand_high)) ||
        (right_exponent ? !(right->significand_high & 0x80000000U) :
            (right->significand_low || right->significand_high))) {
#ifdef __linux__
        recon_unsupported("extended literal comparison outside canonical finite operands");
#else
        __asm { int 3 }
#endif
    }
    /* VC5 has no C scalar type that can load these 80-bit operands. Keep
     * only their actual x87 comparison/status transfer in the primitive. */
    __asm {
        mov esi, left
        mov edi, right
        fld tbyte ptr [esi]
        fld tbyte ptr [edi]
        fxch st(1)
        fcompp
        fstsw status
        fwait
    }
    c1_real_range_scratch.status = status;
    relation = (status & 0x4000) ? 1 : (status & 0x100) ? 4 : 2;
    return (relation & (WORD)operation) != 0;
}
/* RVA074b5 */
CompilerIrSymbol * __cdecl make_declarator_symbol(CompilerNameEntry *name)
{
    unsigned arena_kind = parameter_nesting ? 2 : 0;
    CompilerIrSymbol *symbol = NULL;
    CompilerSymbolBinding *binding;
    CompilerArena *arena;
    BYTE *memory;
#ifdef RECON_NATIVE_TEST
    if ((!arena_kind && c1_data.declaration_symbol_cache_index != 9) ||
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
void __cdecl finalize_declared_symbol(CompilerIrSymbol *);
unsigned __cdecl declared_type_flags(CompilerType *);
void __cdecl record_initialized_symbol(CompilerIrSymbol *);
CompilerIrSymbol *__cdecl declare_initialized_symbol(const CompilerParserDeclaration *declaration,
    CompilerIrSymbol *symbol, CompilerDeclarator *declarator)
{
    int had_initializer = (symbol->declaration_flags >> 2) & 1;
    unsigned storage;
    symbol = declare_declarator(declaration, symbol, declarator);
    if (!symbol) return NULL;
    if (symbol->scope_bits & 1) return NULL;
    if (symbol->kind == 4) return symbol;
    if (c1_bss.browser_filename) {
        if (!parameter_nesting || browser_parameter_references) {
            if (!(symbol->record_id & 0xffffff)) {
                if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
                symbol->record_id = (symbol->record_id & 0xff000000U) |
                                   (c1_data.next_symbol_record_id++ & 0xffffff);
            }
            c1_data.backend->Slot20(symbol);
        }
    }
    if (symbol->kind == 7 || symbol->kind == 8 || symbol->kind == 6) {
        if (had_initializer) {
            compiler_warning(72, symbol);
            return NULL;
        }
        if (symbol->declaration_flags & 0x10) compiler_warning(136, symbol);
        if (declaration->flags & 0x10000) compiler_warning(488, symbol);
        return symbol;
    }
    storage = symbol->storage_class & 7;
    if (!(symbol->linkage_flags & 8)) {
        if (!had_initializer) {
            if (storage != 1 || (!(symbol->symbol_qualifiers & 0x10) &&
                (!(symbol->qualifier_flags & 4) || !(symbol->qualifier_flags & 1))))
                symbol->flags |= 1;
        } else {
            if (parameter_nesting && storage == 1) {
                symbol->scope_bits |= 1;
                compiler_warning(205, symbol);
            } else symbol->flags |= 1;
        }
    }
    if ((symbol->linkage_flags & 8) && ((symbol->flags & 1) || had_initializer)) {
        if (!((lexer_flags & 0x20000) &&
              (c1_bss.current_scope_symbol->linkage_flags & 8) &&
              (c1_bss.current_scope_symbol->symbol_qualifiers & 1)))
            compiler_warning(491, symbol, 377);
    }
    if (storage != 0 && storage != 7 && (declaration->flags & 7) != 1 &&
        !had_initializer && symbol->kind == 10) {
        if (declared_type_flags(symbol->value_type) & 0x100)
            report_language_diagnostic(4, 132, symbol);
        if (symbol->linkage_flags & 2) record_initialized_symbol(symbol);
    }
    finalize_declared_symbol(symbol);
    c1_bss.global_expression_scope_active = 1;
    return symbol;
}
CompilerTypeShape *__cdecl type_qualifier_shape(CompilerType *);
unsigned __cdecl declared_type_flags(CompilerType *type)
{
    CompilerTypeShape *shape = type_qualifier_shape(type);
    return (shape ? shape->attributes : type->attributes) & 0x1f00;
}
CompilerTypeShape *__cdecl skip_array_function_shapes(CompilerTypeShape *);
CompilerTypeShape *__cdecl type_qualifier_shape(CompilerType *type)
{
    return skip_array_function_shapes(type->shape);
}
CompilerTypeShape *__cdecl skip_array_function_shapes(CompilerTypeShape *shape)
{
    while (shape && ((shape->flags & 7) == 2 || (shape->flags & 7) == 4))
        shape = shape->base;
    return shape;
}
void __cdecl record_initialized_symbol(CompilerIrSymbol *symbol)
{
    CompilerType *type;
    if (!(symbol->record_id & 0xffffffU)) {
        if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
        symbol->record_id = (symbol->record_id & 0xff000000U) |
            (c1_data.next_symbol_record_id++ & 0xffffffU);
    }
    symbol->declaration_flags |= 4;
    prepare_initializer_symbol(symbol);
    type = symbol->value_type;
    if (!(type->flags & 0x400) &&
        (!type->shape || (type->shape->flags & 7) != 2)) {
        apply_initializer(symbol, (CompilerParserNode *)make_integer_constant(0));
    } else {
        finish_initializer(symbol, 0);
        finish_aggregate_initializer((CompilerParserNode *)make_integer_constant(0));
        begin_aggregate_initializer(symbol);
    }
}
/* RVA076c1 */
/* RVA0b5ae */
__int64 __cdecl constant_integer_value(CompilerParserNode *node)
{
    if (!node) {
        compiler_warning(56);
        return 0;
    }
    if ((node->flags & 7) != 3) {
        compiler_warning(57);
        return 1;
    }
    if (!(node->type->flags & (0x40 | 0x200))) {
        compiler_warning(58);
        return 0;
    }
    return ((CompilerIntegerConstant *)node)->integer;
}
/* RVA0ba0b */
void __cdecl begin_record_definition(CompilerIrSymbol *symbol, CompilerType *type)
{
    CompilerRecordDefinition *frame;
    CompilerRecordMembers *members;
    CompilerPool *pool;
    int depth;
    if (c1_bss.current_context.saved_frame->stream->active &&
        !c1_bss.debug_module->Slot38(c1_bss.current_context.value, 0))
        compiler_error(73, ".\\esu.c", 306);
    if (c1_bss.browser_filename) {
        if ((!parameter_nesting || browser_parameter_references) &&
            symbol->binding.name != c1_data.builtin_names.unnamed) {
            c1_data.backend->Slot0c();
            c1_data.backend->Slot20(symbol);
            c1_data.backend->Slot30(symbol);
        }
    }
    depth = ++c1_constant_diagnostic_mode;
    if (depth == 16) compiler_error(46, symbol->value_type);
    if (depth < 0 || depth > 1) {
#ifdef __linux__
        recon_unsupported("record-definition index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    frame = &c1_bss.record_definitions[depth];
    if ((type->flags & 0x408) == 0x408 || (type->flags & 0x402) == 0x402)
        frame->flags |= 1;
    else frame->flags &= ~1;
    frame->symbol = symbol;
    frame->initialized_04 = 0;
    frame->byte_size = 0;
    frame->initialized_14 = 0;
    frame->flags &= ~4;
    frame->bitfield_type_flags = 0;
    frame->type = type;
    frame->alignment = 1;
    pool = &c1_bss.pools[7];
    if (pool->remaining < 12) pool->Refill(12);
    members = (CompilerRecordMembers *)pool->cursor;
    pool->cursor += 12;
    pool->remaining -= 12;
    if (members) {
        members->unmodeled_00 = NULL;
        members->first = NULL;
        members->last = NULL;
    }
    c1_bss.record_definitions[c1_constant_diagnostic_mode].symbol->kind4_context = members;
}
/* RVA0bb41 */
int __cdecl check_record_member_name(CompilerNameEntry *);
unsigned __cdecl layout_record_member(CompilerIrSymbol *);
void __cdecl append_record_member(CompilerIrSymbol *);
void __cdecl layout_bitfield_member(CompilerIrSymbol *, WORD);
void __cdecl report_extension_diagnostic(unsigned, unsigned, ...);
void __cdecl emit_anonymous_record_member(CompilerType *);
extern "C" CompilerIrSymbol *__cdecl lookup_cached_symbol_record(unsigned, CompilerNameEntry *, unsigned);
CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
int __cdecl add_record_member(const CompilerParserDeclaration *declaration,
    CompilerIrSymbol *symbol, CompilerDeclarator *declarator)
{
    CompilerType *type = declaration->type;
    CompilerRecordDefinition *frame;
    if (c1_constant_diagnostic_mode < 0) {
        compiler_warning(28);
        return 0;
    }
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1)) {
#ifdef __linux__
        recon_unsupported("record member frame index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    frame = &c1_bss.record_definitions[c1_constant_diagnostic_mode];
    if (!symbol) {
        if (!(type->flags & 0x400)) {
            compiler_warning(208);
            return 0;
        }
        report_extension_diagnostic(201, 467, type);
        if (!(type->symbol_owner->flags & 1)) {
            if (!c1_bss.options.prototypes_only) compiler_warning(27, type->symbol_owner);
            return 0;
        }
        emit_anonymous_record_member(type);
        return 0;
    }
    if (parameter_nesting && !(lexer_flags & 0x20000)) {
        unsigned kind = symbol->kind;
        if (kind != 1 && kind != 4 && kind != 5 && kind != 6 && kind != 9 && kind != 13) {
#ifdef __linux__
            recon_unsupported("record copy symbol-size kind is not yet recovered");
#else
            __asm { int 3 }
#endif
        }
        BYTE *memory = c1_bss.arenas[0].AllocateBytes((c1_data.record_sizes.indexed[kind] + 3) & ~3U);
        symbol = memory ? ((CompilerIrSymbol *)memory)->CopyFrom(symbol, 0) : NULL;
    }
    symbol = declare_declarator(declaration, symbol, declarator);
    if (!symbol) return 0;
    if (symbol->function_flags & 1) lookup_ir_symbol(symbol);
    if (check_record_member_name(symbol->binding.name)) return 0;
    if (declared_type_flags(symbol->value_type) & 0x100)
        frame->symbol->tag_definition_flags |= 0x400;
    if (symbol->kind == 2) layout_bitfield_member(symbol, symbol->value_type->flags);
    else {
        if (symbol->binding.name != c1_data.builtin_names.alignment_member)
            symbol->symbol_value = layout_record_member(symbol);
        append_record_member(symbol);
        frame->symbol->qualifier_flags = (frame->symbol->qualifier_flags & ~1U) |
                                         (symbol->qualifier_flags & 1);
        frame->bitfield_type_flags = 0;
    }
    if (c1_bss.browser_filename) {
        if ((!parameter_nesting || browser_parameter_references) &&
            frame->symbol->binding.name != c1_data.builtin_names.unnamed)
            c1_data.backend->Slot20(symbol);
    }
    return 0;
}
int __cdecl check_record_member_name(CompilerNameEntry *name)
{
    CompilerIrSymbol *record;
    CompilerRecordMembers *members;
    CompilerSymbolBinding *binding;
    if (c1_constant_diagnostic_mode < 0 || !name ||
        name == c1_data.builtin_names.alignment_member || !(name->flags & 0x40000)) return 0;
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1)) {
#ifdef __linux__
        recon_unsupported("duplicate member frame index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    record = c1_bss.record_definitions[c1_constant_diagnostic_mode].symbol;
    members = (CompilerRecordMembers *)record->kind4_context;
    binding = members ? members->first : NULL;
    while (binding) {
        if (binding->name == name) {
            compiler_warning(20, name, record->value_type);
            return 0;
        }
        binding = binding->next;
    }
    return 0;
}
unsigned __cdecl global_type_size(CompilerType *);
CompilerType *__cdecl get_function_return_type(CompilerType *);
void __cdecl finish_record_bitfield(void);
void __cdecl append_record_padding(unsigned);
void __cdecl align_record_member(unsigned);
int __cdecl integer_sum_out_of_range(__int64, unsigned __int64, WORD, WORD);
unsigned __cdecl record_member_offset(unsigned, unsigned);
unsigned __cdecl layout_record_member(CompilerIrSymbol *member)
{
    CompilerRecordDefinition *frame;
    CompilerType *type;
    CompilerIrSymbol *tag;
    unsigned size, alignment, offset;
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1)) {
#ifdef __linux__
        recon_unsupported("record layout frame index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    frame = &c1_bss.record_definitions[c1_constant_diagnostic_mode];
    if ((frame->flags & 1) && (frame->symbol->qualifier_flags & 1)) {
        compiler_warning(229, frame->symbol->value_type, frame->symbol);
        frame->symbol->qualifier_flags &= ~1;
    }
    size = global_type_size(member->value_type);
    if (!size) {
        type = member->value_type;
        tag = type->symbol_owner;
        if (tag && !(tag->flags & 1) &&
            !(type->shape && (type->shape->flags & 7) == 2 && !type->shape->value)) {
            compiler_warning(79, member, type, tag);
            return 0;
        }
        if (!c1_data.options.language_extensions && !(parser_flags & 0x20000)) {
            compiler_warning(133, member);
            return 0;
        }
        report_language_diagnostic(4, 200);
        size = global_type_size(get_function_return_type(member->value_type));
        member->qualifier_flags |= 1;
    }
    if ((frame->flags & 1) && frame->bitfield_type_flags) {
        finish_record_bitfield();
        frame->flags &= ~4;
        frame->initialized_14 = 0;
    }
    type = member->value_type;
    while (type->shape && (type->shape->flags & 7) == 2)
        type = get_function_return_type(type);
    if (!type->shape && (type->flags & 0x140)) {
        if ((type->flags & 0x340) || !(type->flags & ~15U))
            alignment = (unsigned)(int)c1_primitive_widths[type->flags & 15];
        else alignment = type->flags & 15;
        if (alignment < c1_data.options.packing_stack->active->alignment)
            goto have_alignment;
    }
    if (!type->shape && (type->flags & 0x400)) {
        /* FinishRecord writes this byte from its actual alignment WORD. */
        if ((type->flags != 0x402 && type->flags != 0x404) || !type->symbol_owner ||
            type->symbol_owner->kind != 4 || !(type->symbol_owner->flags & 1)) {
#ifdef __linux__
            recon_unsupported("aggregate member alignment requires a completed tag");
#else
            __asm { int 3 }
#endif
        }
        alignment = (BYTE)type->symbol_owner->tag_definition_flags;
    } else alignment = global_type_size(type);
have_alignment:
    align_record_member(alignment);
    if (frame->flags & 1) {
        offset = frame->byte_size;
        if (!(member->qualifier_flags & 1)) {
            if (size && integer_sum_out_of_range((__int64)(long)offset,
                    (unsigned __int64)size - 1, 1, (WORD)c1_primitive_widths[3])) {
                compiler_warning(89, frame->symbol, frame->symbol->value_type);
                return 0;
            }
            frame->byte_size += size;
        }
    } else {
        offset = 0;
        if (frame->byte_size < size) frame->byte_size = size;
    }
    type = member->value_type;
    if (!type->shape && (type->flags & 0x400)) {
        tag = type->symbol_owner;
        member->qualifier_flags = (member->qualifier_flags & ~1U) | (tag->qualifier_flags & 1);
        frame->symbol->qualifier_flags |= tag->qualifier_flags & 2;
        if ((type->flags & 0x404) == 0x404 || (tag->tag_definition_flags & 0x100))
            frame->symbol->tag_definition_flags |= 0x100;
    }
    return record_member_offset(offset, 0);
}
void __cdecl finish_record_bitfield(void)
{
    CompilerRecordDefinition *frame;
    unsigned flags, bytes;
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1)) {
#ifdef __linux__
        recon_unsupported("bitfield completion record index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    frame = &c1_bss.record_definitions[c1_constant_diagnostic_mode];
    flags = frame->bitfield_type_flags;
    bytes = ((flags & 0x340) || !(flags & ~15U)) ?
        (unsigned)(int)c1_primitive_widths[flags & 15] : flags & 15;
    if (frame->flags & 4) frame->byte_size += bytes;
    else append_record_padding(bytes);
}
unsigned __cdecl record_alignment_padding(unsigned);
void __cdecl append_record_padding(unsigned);
void __cdecl align_record_member(unsigned alignment)
{
    CompilerRecordDefinition *frame;
    unsigned packing, bounded, padding;
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1)) {
#ifdef __linux__
        recon_unsupported("record alignment frame index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    frame = &c1_bss.record_definitions[c1_constant_diagnostic_mode];
    packing = c1_data.options.packing_stack->active->alignment;
    bounded = alignment < packing ? alignment : packing;
    if (frame->alignment <= (WORD)bounded) frame->alignment = (WORD)bounded;
    padding = record_alignment_padding(alignment);
    if (padding) {
        if (c1_data.options.packing_stack->active->alignment < alignment)
            report_language_diagnostic(4, 121, frame->symbol);
        append_record_padding(padding);
    }
}
unsigned __cdecl calculate_alignment_padding(unsigned, unsigned);
unsigned __cdecl record_alignment_padding(unsigned alignment)
{
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1)) {
#ifdef __linux__
        recon_unsupported("record padding frame index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    return calculate_alignment_padding(alignment,
        c1_bss.record_definitions[c1_constant_diagnostic_mode].byte_size);
}
unsigned __cdecl calculate_alignment_padding(unsigned alignment, unsigned offset)
{
    unsigned packing = c1_data.options.packing_stack->active->alignment;
    unsigned bounded = alignment < packing ? alignment : packing;
    unsigned remainder = offset % bounded;
    return remainder ? bounded - remainder : 0;
}
void __cdecl append_record_padding(unsigned bytes)
{
    CompilerIrSymbol *symbol = NULL;
    CompilerSymbolBinding *binding;
    CompilerNameEntry *name = c1_data.builtin_names.alignment_member;
    unsigned arena_kind = parameter_nesting ? 2 : 0;
    unsigned size = c1_data.record_sizes.known.member_symbol_bytes;
    CompilerArena *arena;
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1) ||
        (!arena_kind && c1_data.member_symbol_cache_index != 9) || size != 52) {
#ifdef __linux__
        recon_unsupported("padding symbol cache or record frame is not modeled");
#else
        __asm { int 3 }
#endif
    }
    if (!arena_kind) {
        binding = c1_data.symbol_cache.indexed[c1_data.member_symbol_cache_index];
        if (binding) {
            c1_data.symbol_cache.indexed[c1_data.member_symbol_cache_index] = binding->next;
            symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
            binding->name = NULL;
            binding->next = NULL;
            memset(symbol, 0, size);
            binding->name = name;
            symbol->kind = 3;
        }
    }
    if (!symbol) {
        arena = &c1_bss.arenas[arena_kind];
        symbol = (CompilerIrSymbol *)arena->cursor;
        arena->cursor += (size + 3) & ~3U;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        if (symbol) symbol = symbol->Initialize(3, name, arena_kind);
    }
    symbol->flags |= 0x40;
    symbol->symbol_value = bytes;
    append_record_member(symbol);
    c1_bss.record_definitions[c1_constant_diagnostic_mode].byte_size += bytes;
}
int __cdecl integer_sum_out_of_range(__int64 first, unsigned __int64 second,
    WORD unsigned_values, WORD width)
{
    unsigned bits;
    unsigned __int64 high_mask, mask;
    __int64 delta, limit;
    if (!width || width > 8) {
#ifdef __linux__
        recon_unsupported("integer sum width is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    bits = width * 8;
    high_mask = ~(unsigned __int64)0 << (bits - 1);
    if (unsigned_values) {
        unsigned shift = 64 - bits;
        mask = ~high_mask;
        mask |= mask << 1;
        first = (__int64)(((unsigned __int64)first << shift) >> shift);
        second = (second << shift) >> shift;
        return second > mask - (unsigned __int64)first;
    }
    delta = (__int64)second;
    if ((first < 0) != (delta < 0)) return 0;
    if (first > 0) {
        limit = (__int64)(~high_mask - (unsigned __int64)first);
        return delta > limit;
    }
    limit = (__int64)(high_mask - (unsigned __int64)first);
    return delta < limit;
}
unsigned __cdecl record_member_offset(unsigned offset, unsigned extra)
{
    return offset + extra;
}
void __cdecl append_record_member(CompilerIrSymbol *symbol)
{
    CompilerSymbolBinding *binding = &symbol->binding;
    CompilerRecordMembers *members;
    if (binding->name) binding->name->flags |= 0x40000;
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1)) {
#ifdef __linux__
        recon_unsupported("record member append frame index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    members = (CompilerRecordMembers *)c1_bss.record_definitions[c1_constant_diagnostic_mode].symbol->kind4_context;
    binding->next = NULL;
    if (members->last) members->last->next = binding;
    else members->first = binding;
    members->last = binding;
}
void __cdecl layout_bitfield_member(CompilerIrSymbol *member, WORD type_flags)
{
    CompilerRecordDefinition *frame;
    unsigned flags = type_flags, width, encoding, bytes, old_flags, old_bytes;
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1)) {
#ifdef __linux__
        recon_unsupported("bitfield record-definition index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    frame = &c1_bss.record_definitions[c1_constant_diagnostic_mode];
    if ((frame->flags & 1) && (frame->symbol->qualifier_flags & 1)) {
        compiler_warning(229, frame->symbol->value_type, frame->symbol);
        frame->symbol->qualifier_flags &= ~1;
    }
    if (!(flags & 0x40) || (flags & 0x6f) != 0x63) {
        if (!c1_data.options.language_extensions && !c1_data.options.preprocess_only) {
            encoding = flags & 15;
            bytes = ((flags & 0x340) || !(flags & ~15U)) ?
                (unsigned)(int)c1_primitive_widths[encoding] : encoding;
            if (bytes < 4 || ((flags & 0x2fef) != 0x43 &&
                (flags & 0x2fef) != 0x4c && (flags & 0x2fef) != 0x4d)) {
                compiler_warning(150, member);
                member->value_type = c1_data.builtin_types.type_4063_first;
                flags = 0x4063;
            } else if (bytes > 4) report_language_diagnostic(4, 214);
        } else if (!(flags & 0x40)) {
            report_language_diagnostic(1, 15, member);
            member->value_type = c1_data.builtin_types.type_4063_first;
            flags = 0x4063;
        } else report_language_diagnostic(4, 214);
    }
    if (member->value_type->shape) compiler_warning(33, member);
    width = member->bit_width;
    if (!width) {
        if (member->binding.name != c1_data.builtin_names.alignment_member)
            compiler_warning(149, member);
        if (!frame->bitfield_type_flags) return;
    }
    encoding = flags & 15;
    bytes = ((flags & 0x340) || !(flags & ~15U)) ?
        (unsigned)(int)c1_primitive_widths[encoding] : encoding;
    if (width > bytes * 8) { compiler_warning(34, member); width = bytes * 8; }
    if (frame->flags & 1) {
        old_flags = frame->bitfield_type_flags;
        if (old_flags) {
            old_bytes = ((old_flags & 0x340) || !(old_flags & ~15U)) ?
                (unsigned)(int)c1_primitive_widths[old_flags & 15] : old_flags & 15;
            if (bytes == old_bytes && width && frame->initialized_14 + width <= bytes * 8)
                goto place;
            finish_record_bitfield();
        }
        frame->initialized_14 = 0;
        frame->flags &= ~4;
        align_record_member(bytes);
    } else if ((int)frame->byte_size < (int)bytes) frame->byte_size = bytes;
place:
    frame->bitfield_type_flags = width ? (WORD)flags : 0;
    if (member->binding.name != c1_data.builtin_names.alignment_member) {
        append_record_member(member);
        member->bit_width = (BYTE)width;
        frame->flags |= 4;
        member->bit_offset = frame->initialized_14;
    }
    if (frame->flags & 1) {
        frame->initialized_14 += (BYTE)width;
        member->symbol_value = frame->byte_size;
    } else member->symbol_value = 0;
}
void __cdecl emit_anonymous_record_member(CompilerType *type)
{
    CompilerRecordDefinition *frame;
    CompilerIrSymbol *symbol, *owner, *source;
    CompilerRecordMembers *members;
    CompilerSymbolBinding *binding;
    CompilerNameEntry *name = c1_data.builtin_names.alignment_member;
    CompilerArena *arena = &c1_bss.arenas[0];
    unsigned offset, bytes, index;
    BYTE *memory;
    if (c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1 ||
        c1_data.record_sizes.known.member_symbol_bytes != 52 ||
        c1_data.member_symbol_cache_index != 9) {
#ifdef __linux__
        recon_unsupported("anonymous member record frame or cache is not modeled");
#else
        __asm { int 3 }
#endif
    }
    frame = &c1_bss.record_definitions[c1_constant_diagnostic_mode];
    frame->symbol->qualifier_flags |= 2;
    symbol = lookup_cached_symbol_record(3, name, 0);
    if (!symbol) {
        memory = arena->AllocateBytes((c1_data.record_sizes.known.member_symbol_bytes + 3) & ~3U);
        symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(3, name, 0) : NULL;
    }
    symbol->flags |= 0x40;
    symbol->qualifier_flags |= 8;
    symbol->value_type = type;
    offset = layout_record_member(symbol);
    symbol->symbol_value = offset;
    owner = type->symbol_owner;
    frame->symbol->qualifier_flags = (frame->symbol->qualifier_flags & ~1U) |
        (owner->qualifier_flags & 1);
    frame->symbol->tag_definition_flags =
        (frame->symbol->tag_definition_flags & ~0x100U) |
        (((owner->tag_definition_flags & 0x100) || (type->flags & 0x404) == 0x404) ? 0x100 : 0);
    append_record_member(symbol);
    members = (CompilerRecordMembers *)owner->kind4_context;
    binding = members ? members->first : NULL;
    for (; binding; binding = binding->next) {
        source = (CompilerIrSymbol *)((BYTE *)binding - 8);
        unsigned kind = source->kind;
        if (kind == 15 || kind == 17 || kind == 8 || kind == 4) continue;
        if (check_record_member_name(binding->name)) continue;
        /* Only the encountered52-byte member owner is currently supported.
         * Other original symbol kinds are valid but their copies remain held. */
        if (kind != 3) {
#ifdef __linux__
            recon_unsupported("anonymous member copy kind is not modeled");
#else
            __asm { int 3 }
#endif
        }
        bytes = c1_data.record_sizes.indexed[kind];
        memory = arena->AllocateBytes((bytes + 3) & ~3U);
        symbol = memory ? ((CompilerIrSymbol *)memory)->CopyFrom(source, 0) : NULL;
        append_record_member(symbol);
        if ((frame->flags & 1) && symbol->binding.name != name &&
            !(symbol->qualifier_flags & 0x18))
            symbol->symbol_value += offset;
    }
    /* Closing marker uses the same physical cache head and member owner. */
    index = c1_data.member_symbol_cache_index;
    binding = c1_data.symbol_cache.indexed[index];
    symbol = NULL;
    if (binding) {
        c1_data.symbol_cache.indexed[index] = binding->next;
        binding->name = NULL;
        binding->next = NULL;
        symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
        memset(symbol, 0, c1_data.record_sizes.known.member_symbol_bytes);
        symbol->binding.name = name;
        symbol->kind = 3;
    }
    if (!symbol) {
        memory = arena->AllocateBytes((c1_data.record_sizes.known.member_symbol_bytes + 3) & ~3U);
        symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(3, name, 0) : NULL;
    }
    symbol->flags |= 0x40;
    symbol->qualifier_flags |= 0x10;
    symbol->value_type = type;
    append_record_member(symbol);
    frame->bitfield_type_flags = 0;
}
/* RVA0c0a9 */
CompilerType * __cdecl parse_record_specifier(void)
{
    CompilerRecordDefinition *frame;
    CompilerIrSymbol *symbol;
    CompilerType *type;
    unsigned debug_index = 0;
    if ((c1_constant_diagnostic_mode < 0 || c1_constant_diagnostic_mode > 1)) {
#ifdef __linux__
        recon_unsupported("record finalization frame index is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    frame = &c1_bss.record_definitions[c1_constant_diagnostic_mode];
    symbol = frame->symbol;
    type = frame->type;
    if (symbol->binding.name != c1_data.builtin_names.unnamed && c1_bss.browser_filename) {
        if (!parameter_nesting || browser_parameter_references) c1_data.backend->Slot08();
    }
    if (frame->bitfield_type_flags && frame->initialized_14) {
        unsigned flags = frame->bitfield_type_flags;
        unsigned width = ((flags & 0x340) || !(flags & ~15U)) ?
            (unsigned)(int)c1_primitive_widths[flags & 15] : flags & 15;
        frame->byte_size += width;
    }
    align_record_member(frame->alignment);
    symbol->enum_base_width = frame->byte_size;
    if (!symbol->enum_base_width) symbol->enum_base_width = c1_primitive_widths[3];
    symbol->tag_definition_flags = (symbol->tag_definition_flags & 0xff00) | (BYTE)frame->alignment;
    symbol->flags |= 1;
    --c1_constant_diagnostic_mode;
    c1_bss.tag_declaration_seen = 1;
    if ((c1_bss.current_context.saved_frame->stream->active & 1) &&
        !c1_bss.debug_module->Slot3c(c1_bss.current_context.value, 0, &debug_index))
        compiler_error(73, ".\\esu.c", 1053);
    return type;
}
/* RVA0ca41 */
WORD __cdecl merge_declarator_qualifiers(WORD previous, WORD added)
{
    if (previous & added & 0x1f00) {
        if (c1_data.options.language_extensions) report_language_diagnostic(1, 114);
        else compiler_warning(289);
    }
    if (((previous ^ added) & 3) && (previous & 3) && (added & 3))
        compiler_warning(289);
    if (((previous ^ added) & 0x70) && (previous & 0x70) && (added & 0x70)) {
        compiler_warning(151);
        previous &= 0xff8f;
    }
    return previous | added;
}
/* RVA0d706 */
unsigned __cdecl save_type_context(void);
void __cdecl restore_type_context(void);
extern "C" const char *__cdecl diagnostic_text_data(unsigned);
extern "C" unsigned __cdecl format_diagnostic_type(unsigned, unsigned, unsigned, ...);
extern "C" CompilerIrSymbol *__cdecl create_tag_symbol(CompilerNameEntry *, WORD, CompilerDeclarationApply *, int);
void __cdecl process_declaration(const CompilerParserDeclaration *declaration)
{
    CompilerType *type = declaration->type;
    unsigned kind = type->flags & 0x2fef;
    switch (kind) {
    case 0x41: case 0x42: case 0x44: case 0x80:
    case 0x106: case 0x107: case 0x108:
        if (parser_flags & 0x400) return;
        if (c1_data.options.language_extensions) {
            save_type_context();
            unsigned text = format_diagnostic_type(0, declaration->flags,
                declaration->value, type);
            report_language_diagnostic(2, 91, diagnostic_text_data(text), type);
            restore_type_context();
        } else compiler_warning(59, 77, 375);
        break;
    case 0x402: case 0x404: case 0x408:
        {
            CompilerNameEntry *name = type->symbol_owner->binding.name;
            if (name == c1_data.builtin_names.unnamed) {
                if (c1_data.options.language_extensions)
                    report_language_diagnostic(2, 94, type);
                else compiler_warning(59, 77, 375);
            } else if (!c1_bss.tag_declaration_seen) {
                CompilerDeclarationApply apply = &CompilerTranslationUnit::InsertDeclaration;
                CompilerIrSymbol *symbol = create_tag_symbol(name, type->flags, &apply, 0);
                if (apply) (c1_data.translation_unit->*apply)(symbol);
            }
        }
        break;
    }
}
/* RVA433a8 reads three fixed cdecl slots. The diagnostic caller supplies
 * four slots and the declaration-prefix caller supplies three; each cleans
 * its actual argument list. No surplus slot is read, and the third is an
 * unchanged carrier forwarded to the next formatter. */
extern "C" unsigned __cdecl create_diagnostic_text(void);
extern "C" void __cdecl append_diagnostic_text(unsigned, const char *);
extern "C" void __cdecl append_diagnostic_character(unsigned, int);
extern "C" unsigned __cdecl describe_diagnostic_storage(unsigned result, unsigned flags,
    unsigned context)
{
    unsigned attributes = create_diagnostic_text();
    const char *text;
    if (flags & 0x080000) append_diagnostic_text(attributes, "dllimport ");
    if (flags & 0x040000) append_diagnostic_text(attributes, "dllexport ");
    if (flags & 0x010000) append_diagnostic_text(attributes, "naked ");
    if (flags & 0x020000) append_diagnostic_text(attributes, "thread ");
    if (flags & 0x01000000) {
        append_diagnostic_text(attributes, "allocate(\"");
        append_diagnostic_text(attributes, ((CompilerIrSymbol *)context)->binding.name->name);
        append_diagnostic_text(attributes, "\") ");
    }
    if (flags & 0x400000) append_diagnostic_text(attributes, "selectany ");
    text = diagnostic_text_data(attributes);
    if (*text) {
        append_diagnostic_text(result, "__declspec");
        append_diagnostic_character(result, '(');
        append_diagnostic_text(result, text);
        append_diagnostic_text(result, ") ");
    }
    return result;
}
extern "C" unsigned __cdecl format_diagnostic_type(unsigned result, unsigned flags,
    unsigned context, ...)
{
    const char *text;
    if (!result) result = create_diagnostic_text();
    switch (flags & 7) {
    case 0: text = ""; break;
    case 1: text = "extern"; break;
    case 2: text = "/*global*/"; break;
    case 3: text = "static"; break;
    case 4: text = "auto"; break;
    case 5: text = "register"; break;
    case 6: text = "/* really */ register"; break;
    case 7: text = "typedef"; break;
    default: text = "/*CLASS?*/"; break;
    }
    append_diagnostic_text(result, text);
    append_diagnostic_character(result, ' ');
    return describe_diagnostic_storage(result, flags, context);
}
/* RVA0e4d2 */
BYTE __cdecl prepare_bitfield(CompilerIrSymbol *symbol, __int64 width)
{
    if (width < 0 || width >= 256) {
        compiler_warning(34, symbol);
        return 1;
    }
    return (BYTE)width;
}
/* RVA0eed5 */
extern "C" CompilerIrSymbol *__cdecl lookup_cached_symbol_record(unsigned, CompilerNameEntry *, unsigned);
extern "C" CompilerIrSymbol *__cdecl merge_scoped_declaration(CompilerIrSymbol *, CompilerIrSymbol *, int);
extern "C" CompilerSymbolMapEntry *__cdecl append_enum_symbol(CompilerSymbolMap *, void *, CompilerIrSymbol *);
void __cdecl add_enumerator(CompilerNameEntry *name, CompilerType *type, __int64 value)
{
    CompilerIrSymbol *symbol, *previous = NULL;
    CompilerScopeLookup *found, *merged;
    CompilerArena *arena;
    unsigned arena_kind, width;
    if (!type) return;
    arena_kind = (declaration_depth > 0 && !(lexer_flags & 0x20000)) ?
                 0 : (parameter_nesting ? 2 : 0);
    symbol = lookup_cached_symbol_record(5, name, arena_kind);
    if (!symbol) {
        if (c1_data.record_sizes.known.enumerator_symbol_bytes != 40) {
#ifdef __linux__
            recon_unsupported("enumerator symbol size is not yet modeled");
#else
            __asm { int 3 }
#endif
        }
        BYTE *memory = c1_bss.arenas[arena_kind].AllocateBytes(
            (c1_data.record_sizes.known.enumerator_symbol_bytes + 3) & ~3U);
        symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(5, name, arena_kind) : NULL;
    }
    /* Kind5 has the authentic40-byte extent; only its common prefix and
     * the integer at+36 are consumed. The kind4 tail is never accessed. */
    symbol->value_type = type;
    if ((lexer_flags & 0x20000) || c1_data.pch_separate_debug ||
        (c1_bss.current_tag_symbol->flags & 0x40)) symbol->flags &= ~2;
    else symbol->flags |= 2;
    found = (CompilerScopeLookup *)c1_data.translation_unit->current->Find(name);
    if (found) {
        previous = merge_scoped_declaration(found->symbol, symbol, 0);
        if (previous) {
            arena = &c1_bss.arenas[3];
            merged = (CompilerScopeLookup *)arena->AllocateBytes(8);
            if (merged) { merged->symbol = previous; merged->scope = NULL; }
        }
    }
    if (previous) {
        compiler_warning(86, previous);
        symbol = previous;
    } else c1_data.translation_unit->current->Insert(symbol);
    if (c1_bss.current_tag_symbol->kind4_context) {
        append_enum_symbol((CompilerSymbolMap *)c1_bss.current_tag_symbol->kind4_context,
                           name, symbol);
        if (!(symbol->record_id & 0xffffff)) {
            if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
            symbol->record_id = (symbol->record_id & 0xff000000U) |
                               (c1_data.next_symbol_record_id++ & 0xffffff);
        }
        width = value < (-2147483647 - 1) || value > 2147483647 ? 8 : 4;
        if ((int)width > c1_primitive_widths[3] && (value < 0 || value > 0xffffffffi64))
            report_language_diagnostic(c1_data.options.language_extensions ? 3 : 1, 341, symbol);
        symbol->integer_value = (int)(unsigned)value;
        enumerator_value = value;
        if (c1_bss.browser_filename && (!parameter_nesting || browser_parameter_references))
            c1_data.backend->Slot20(symbol);
    }
}
extern "C" CompilerIrSymbol *__cdecl lookup_cached_symbol_record(unsigned kind,
    CompilerNameEntry *name, unsigned arena_kind)
{
    CompilerSymbolBinding *binding;
    CompilerIrSymbol *symbol;
    if (arena_kind) return NULL;
    kind &= 0xff;
    if (!((kind == 3 && c1_data.member_symbol_cache_index == 9 &&
           c1_data.record_sizes.known.member_symbol_bytes == 52) ||
          (kind == 5 && c1_data.enumerator_symbol_cache_index == 5 &&
           c1_data.record_sizes.known.enumerator_symbol_bytes == 40) ||
          (kind == 12 && c1_data.parameter_symbol_cache_index == 9 &&
           c1_data.record_sizes.known.parameter_symbol_bytes == 52) ||
          (kind == 10 && c1_data.static_symbol_cache_index == 9 &&
           c1_data.record_sizes.known.static_symbol_bytes == 52))) {
#ifdef __linux__
        recon_unsupported("symbol cache kind/configuration is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    unsigned index = kind == 3 ? c1_data.member_symbol_cache_index :
        kind == 5 ? c1_data.enumerator_symbol_cache_index :
        kind == 10 ? c1_data.static_symbol_cache_index : c1_data.parameter_symbol_cache_index;
    binding = c1_data.symbol_cache.indexed[index];
    if (!binding) return NULL;
    c1_data.symbol_cache.indexed[index] = binding->next;
    binding->name = NULL;
    binding->next = NULL;
    symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
    memset(symbol, 0, c1_data.record_sizes.indexed[kind]);
    binding->name = name;
    symbol->kind = (BYTE)kind;
    return symbol;
}
extern "C" unsigned __cdecl enum_symbol_bucket(CompilerSymbolMap *, void *);
extern "C" CompilerSymbolMapEntry *__cdecl find_enum_entry(CompilerSymbolMap *, void *, unsigned);
extern "C" CompilerSymbolMapEntry *__cdecl append_enum_symbol(CompilerSymbolMap *context,
    void *name, CompilerIrSymbol *symbol)
{
    unsigned bucket = enum_symbol_bucket(context, name);
    CompilerSymbolMapEntry *entry = find_enum_entry(context, name, bucket);
    if (!entry) {
        if (!context->arena_kind) {
            CompilerPool *pool = &c1_bss.pools[3];
            if (pool->remaining < 12) pool->Refill(12);
            entry = (CompilerSymbolMapEntry *)pool->cursor;
            pool->cursor += 12;
            pool->remaining -= 12;
        } else {
#ifdef RECON_NATIVE_TEST
            if (context->arena_kind >= 6)
                recon_unsupported("enum entry arena is not modeled");
#endif
            CompilerArena *arena = &c1_bss.arenas[context->arena_kind];
            entry = (CompilerSymbolMapEntry *)arena->cursor;
            arena->cursor += 12;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        }
        entry->key = name;
        entry->next = context->symbols[bucket];
        context->symbols[bucket] = entry;
        ++context->count;
    }
    entry->symbol = symbol;
    return entry;
}
extern "C" unsigned __cdecl hash_enum_name(void *);
extern "C" {
/* Encountered name/static-symbol/record-id hash slots0/2/3. Slot1 is unmodeled
 * representation and guarded before lookup; it is not a NULL original row. */
unsigned __cdecl hash_static_symbol(void *);
unsigned __cdecl hash_record_id(void *);
unsigned (__cdecl *c1_enum_hashers[])(void *) = { hash_enum_name, NULL, hash_static_symbol, hash_record_id };
}
extern "C" unsigned __cdecl enum_symbol_bucket(CompilerSymbolMap *context, void *name)
{
    if (context->hash_kind != 0 && context->hash_kind != 2 && context->hash_kind != 3) {
#ifdef __linux__
        recon_unsupported("enum hash selector is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    return c1_enum_hashers[context->hash_kind](name) % context->capacity;
}
extern "C" unsigned __cdecl hash_enum_name(void *name)
{
    return ((unsigned)name >> 2) & 0xfff;
}
extern "C" int __cdecl enum_names_equal(void *, void *);
extern "C" {
int __cdecl static_symbols_equal(void *, void *);
int __cdecl record_ids_equal(void *, void *);
int (__cdecl *c1_enum_matchers[])(void *, void *) = { enum_names_equal, static_symbols_equal, record_ids_equal };
}
extern "C" CompilerSymbolMapEntry *__cdecl find_enum_entry(CompilerSymbolMap *context,
    void *name, unsigned bucket)
{
    CompilerSymbolMapEntry *entry;
    int (__cdecl *matches)(void *, void *);
    if (context->match_kind != 0 && context->match_kind != 1 && context->match_kind != 2) {
#ifdef __linux__
        recon_unsupported("enum equality selector is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    matches = c1_enum_matchers[context->match_kind];
    entry = context->symbols[bucket];
    while (entry) {
        if (matches(name, entry->key)) return entry;
        entry = entry->next;
    }
    return NULL;
}
extern "C" int __cdecl enum_names_equal(void *left, void *right)
{
    return left == right;
}
/* RVA0f092 */
extern "C" unsigned __cdecl enum_entry_count(CompilerSymbolMap *);
CompilerType *__cdecl parse_enum_specifier(void)
{
    CompilerIrSymbol *symbol = enumerator_type->symbol_owner;
    if (!c1_data.options.language_extensions &&
        !enum_entry_count((CompilerSymbolMap *)symbol->kind4_context))
        compiler_warning(138);
    if (symbol->binding.name != c1_data.builtin_names.unnamed && c1_bss.browser_filename) {
        if (!parameter_nesting || browser_parameter_references)
            c1_data.backend->Slot08();
    }
    parser_flags |= 0x400;
    return c1_data.builtin_types.type_0063;
}
extern "C" unsigned __cdecl enum_entry_count(CompilerSymbolMap *context)
{
    return context->count;
}
/* RVA0f123 */
__int64 __cdecl next_enumerator_value(__int64 value, CompilerNameEntry *name)
{
    if (value == 0x7fffffff) report_language_diagnostic(1, 340, name);
    return (__int64)((unsigned __int64)value + 1);
}
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
CompilerParserNode *__cdecl make_enumerator_expression(CompilerIrSymbol *); /* RVA1ba32 */
CompilerParserNode *__cdecl take_expression_address(CompilerParserNode *); /* RVA1d2f3 */
CompilerType *__cdecl wrap_expression_type(CompilerType *, unsigned, unsigned, unsigned *); /* RVA1e346 */
CompilerParserNode *__cdecl make_symbol_reference(CompilerIrSymbol *symbol, unsigned)
{
    CompilerParserNode *value;
    CompilerType *type;
    if (symbol->kind == 5) return make_enumerator_expression(symbol);
    if ((symbol->storage_class & 7) == 5 && (symbol->qualifier_flags & 4) &&
        !unevaluated_type_depth) compiler_warning(103, 49);
    value = make_label_expression(symbol, 0);
    if (!value) return NULL;
    if (symbol->kind == 7 || symbol->kind == 8 || symbol->kind == 6) {
        type = symbol->value_type;
        if (!type || !type->shape) return NULL;
        value->type = wrap_expression_type(type, 11, type->shape->storage & 0x73, NULL);
        return value;
    }
    if (symbol->qualifier_flags & 4) return value;
    return take_expression_address(value);
}
CompilerParserNode *__cdecl make_enumerator_expression(CompilerIrSymbol *symbol)
{
    CompilerArena *arena;
    CompilerParserNode *value;
    if (c1_data.expression_arena_kind >= 6) {
#ifdef __linux__
        recon_unsupported("enumerator expression arena is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    value = (CompilerParserNode *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    value->flags = 3;
    value->kind = 101;
    value->low = (unsigned)symbol->integer_value;
    value->high = symbol->integer_value < 0 ? ~0U : 0;
    value->type = c1_data.builtin_types.type_0063;
    return value;
}
CompilerParserNode *__cdecl make_typed_unary_operation(unsigned, CompilerType *, CompilerParserNode *);
CompilerParserNode *__cdecl finish_segment_dereference(CompilerParserNode *);
CompilerParserNode *__cdecl take_expression_address(CompilerParserNode *value)
{
    CompilerParserNode *result;
    CompilerTypeShape *shape;
    if (!value) return NULL;
    result = make_typed_unary_operation(61, value->type, value);
    if (!result->type) return result;
    shape = result->type->shape;
    if (shape && (shape->flags & 7) == 1 && (shape->attributes & 3))
        return finish_segment_dereference(result);
    return result;
}
extern "C" CompilerTypeShape *__cdecl make_type_shape(unsigned, unsigned, unsigned *);
CompilerType *__cdecl wrap_expression_type(CompilerType *type, unsigned flags,
    unsigned attributes, unsigned *value)
{
    CompilerArena *arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    CompilerType *copy = (CompilerType *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    /* The complete established16-byte type owner includes its cache bytes. */
    memcpy(copy, type, 16);
    CompilerTypeShape *shape = make_type_shape(flags, attributes, value);
    shape->base = copy->shape;
    copy->shape = shape;
    return copy;
}
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
        if (!c1_bss.options.prototypes_only) write_ir_record(83, NULL);
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
    if (!c1_bss.options.prototypes_only && c1_data.debug_module_kind != 4) {
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
    if (c1_bss.options.prototypes_only) return;
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
    if (symbol->kind != 9 || c1_data.try_depth < 0 || c1_data.finally_depth < 0 ||
        c1_data.exception_filter_depth < 0 ||
        c1_data.try_depth + c1_data.finally_depth + c1_data.exception_filter_depth > 255)
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
        (parameter_storage_class != 1 && parameter_storage_class != 3 &&
        !(parameter_storage_class == 2 && declarator->parameters &&
          declarator->parameters->kind == 2 && declarator->parameters->bindings) &&
        !(parameter_storage_class == 5 && symbol->function_type->shape &&
        (symbol->function_type->shape->flags & 0x4000) &&
        symbol->function_type->shape->parameters == c1_data.void_type_list)) ||
        (function_context && (((CompilerIrSymbol *)function_context)->kind != 14 ||
          ((CompilerIrSymbol *)function_context)->segment_kind != 0 ||
          ((CompilerIrSymbol *)function_context)->value_type != c1_data.builtin_types.type_0802)))
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
void __cdecl apply_function_context(CompilerIrSymbol *symbol, void *context)
{
    if (symbol->function_flags & 1) {
        if (lookup_ir_symbol(symbol) != context) compiler_warning(193, symbol);
        return;
    }
    if (!context) symbol->function_flags &= ~1;
    else attach_declaration_context(symbol, context);
    symbol->function_flags |= 2;
}
/* RVA10ffa */
/* These contextual cells are held until their actual function-body uses. */
#define function_stack_check_reported c1_bss.function_stack_check_reported /* RVA53d68, BSS */
#define function_output_mode c1_bss.options.prototypes_only /* RVA54c88, predicate-only DWORD */
#define function_debug_info c1_data.options.debug_info /* RVA5ef38, existing owner */
void __cdecl initialize_function_parameters(void);
unsigned __cdecl parameter_stack_bytes(CompilerParameterBindings *);
int __cdecl compare_debug_positions(unsigned, unsigned);
void __cdecl begin_browser_function(void);
int __cdecl compatible_parameter_types(CompilerType *, CompilerType *);
CompilerType *__cdecl replace_function_parameters(CompilerType *, unsigned, WORD, unsigned *);
unsigned __cdecl save_type_context(void);
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
        bytes = parameter_stack_bytes((CompilerParameterBindings *)parameters);
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
    if (function_output_mode) return;
    prepare_function_record(0);
    if (c1_data.debug_module_kind != 4) {
        function_entry_label = make_internal_label(31);
        write_ir_record(83, NULL);
        write_ir_record(83, NULL);
        emit_ir_node(make_ir_operation(108, make_label_expression(function, 1), 0));
    }
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
/* Encountered cells of the original opcode/property columns. Intervening storage is
 * representation only: it has no recovered default or semantic coverage.
 * DWORD opcodes: kind6 at63e80, kind101 at63ffc, kind108 at64018.
 * BYTE property bit80 is clear at64246/642a5, and set at642ac. Other property bits and
 * adjacent columns are deliberately not represented. Kind116 has opcode50 at
 *64038 and only the clear property bit80 at642b4 is represented. Kind52
 * uses opcode50 at63f38 and clear bit80 at64274. Prefix increment kind85
 * uses opcode15 at63fbc and clear property bit80 at64295. Relational kind68
 * uses opcode34 at63f78 and clear property bit80 at64284. Indirection kind61
 * uses opcode48 at63f5c and set property bit80 at6427d. Addition kind81
 * uses opcode2 at63fac and clear property bit80 at64291. Multiplication kind74
 * uses opcode4 at63f90 and clear property bit80 at6428a. Bitwise conjunction
 * kind49 uses opcode11 at63f2c and clear property bit80 at64271. Logical-not
 * kind60 uses opcode26 at63f58; its property cell remains unmodeled.
 * Member address kind83 uses opcode39 at63fb4 and clear bit80 at64293.
 * Greater-than kind62 uses opcode36 at63f60 and clear bit80 at6427e.
 * Compound subtraction kind71 uses opcode16 at63f84 and clear bit80 at64287.
 * Compound addition kind82 uses opcode15 at63fb0 and clear bit80 at64292.
 * Switch dispatch kind132 uses opcode60 at64078 and set bit80 at642c4.
 * Switch value kind133 uses opcode59 at6407c and clear bit80 at642c5.
 * Ordinary call112 uses opcode62 at64028 and set bit80 at642b0;
 * the other seven property bits remain unmodeled. Reached return-tree cells:
 * logical-and50=28@63f30/clear80@64272; conditional-arms54=67@63f40/clear@64276;
 * equality59=31@63f54/clear@6427b; subscript65=40@63f6c/clear@64281;
 * conditional86=66@63fc0/clear@64296; conversion100=44@63ff8/set80@642a4.
 * Bitfield53=55@63f3c/clear80@64275; dot-member80=39@63fa8/clear80@64290.
 * Subtraction70=3@63f80/clear80@64286; inequality76=32@63f98/clear80@6428c.
 * Right shift88=10@63fc8/clear80@64298.
 * Division57=5@63f4c/clear80@64279; negate90=8@63fd0/clear80@6429a.
 * Parenthesized118=89@64040/clear80@642b6. Bitwise-and assignment51=23@63f34,
 * remainder72=6@63f88, complement89=14@63fcc, xor91=13@63fd4 and xor-assignment92=24@63fd8;
 * Post-increment64=53@63f68/clear80@64280. Their property80 bit is clear at64273/64288/64299/6429b/6429c. Other bits unmodeled.
 * Left shift67=9@63f74/clear80@64283; bitwise-or77=12@63f9c/clear80@6428d.
 * Bitwise-or assignment78=25@63fa0; only clear property bit80@6428e is modeled. */
/* String-literal kind7: opcode51 at63e84, property bit80 at64247.
 * The emitter lowers it through the real string owner before writing. */
static const union CompilerIrOpcodeColumn {
    struct {
        BYTE unmodeled_000[24];
        unsigned symbol;
        unsigned string_literal;
        BYTE unmodeled_020[164];
        unsigned conjunction;
        unsigned logical_and;
        unsigned bitwise_and_assign;
        unsigned assignment;
        unsigned bitfield;
        unsigned conditional_arms;
        BYTE unmodeled_0dc[4];
        unsigned decrement;
        unsigned division;
        BYTE unmodeled_0e8[4];
        unsigned equal;
        unsigned logical_not;
        unsigned indirection;
        unsigned greater;
        BYTE unmodeled_0fc[4];
        unsigned post_increment;
        unsigned subscript;
        BYTE unmodeled_108[4];
        unsigned shift;
        unsigned less;
        BYTE unmodeled_114[4];
        unsigned subtraction;
        unsigned subtract_assign;
        unsigned remainder;
        BYTE unmodeled_124[4];
        unsigned multiplication;
        BYTE unmodeled_12c[4];
        unsigned not_equal;
        unsigned bitwise_or;
        unsigned bitwise_or_assign;
        unsigned logical_or;
        unsigned dot_member;
        unsigned addition;
        unsigned add_assign;
        unsigned member_address;
        BYTE unmodeled_150[4];
        unsigned increment;
        unsigned conditional;
        BYTE unmodeled_15c[4];
        unsigned right_shift;
        unsigned complement;
        unsigned negate;
        unsigned bitwise_xor;
        unsigned xor_assign;
        BYTE unmodeled_174[28];
        unsigned conversion;
        unsigned integer;
        BYTE unmodeled_198[24];
        unsigned function;
        BYTE unmodeled_1b4[12];
        unsigned call;
        BYTE unmodeled_1c4[12];
        unsigned initializer;
        BYTE unmodeled_1d4[4];
        unsigned parenthesized;
        unsigned intrinsic_call;
        BYTE unmodeled_1e0[48];
        unsigned switch_dispatch;
        unsigned switch_value;
    } known;
    unsigned indexed[1];
} ir_node_opcodes = { { {0}, 38, 51, {0}, 11, 28, 23, 50, 55, 67, {0}, 54, 5, {0}, 31, 26, 48, 36, {0}, 53, 40, {0}, 9, 34, {0}, 3, 16, 6, {0}, 4, {0}, 32, 12, 25, 27, 39, 2, 15, 39, {0}, 15, 66, {0}, 10, 14, 8, 13, 24, {0}, 44, 51, {0}, 70, {0}, 62, {0}, 50, {0}, 89, 64, {0}, 60, 59 } };
static const union CompilerIrPropertyColumn {
    struct {
        BYTE unmodeled_000[6];
        BYTE symbol;
        BYTE string_literal;
        BYTE unmodeled_008[41];
        BYTE conjunction;
        BYTE logical_and;
        BYTE bitwise_and_assign;
        BYTE assignment;
        BYTE bitfield;
        BYTE conditional_arms;
        BYTE unmodeled_037[1];
        BYTE decrement;
        BYTE division;
        BYTE unmodeled_03a[1];
        BYTE equal;
        BYTE logical_not;
        BYTE indirection;
        BYTE greater;
        BYTE unmodeled_03f[1];
        BYTE post_increment;
        BYTE subscript;
        BYTE unmodeled_042[1];
        BYTE shift;
        BYTE less;
        BYTE unmodeled_045[1];
        BYTE subtraction;
        BYTE subtract_assign;
        BYTE remainder;
        BYTE unmodeled_049[1];
        BYTE multiplication;
        BYTE unmodeled_04b[1];
        BYTE not_equal;
        BYTE bitwise_or;
        BYTE bitwise_or_assign;
        BYTE logical_or;
        BYTE dot_member;
        BYTE addition;
        BYTE add_assign;
        BYTE member_address;
        BYTE unmodeled_054[1];
        BYTE increment;
        BYTE conditional;
        BYTE unmodeled_057[1];
        BYTE right_shift;
        BYTE complement;
        BYTE negate;
        BYTE bitwise_xor;
        BYTE xor_assign;
        BYTE unmodeled_05d[7];
        BYTE conversion;
        BYTE integer;
        BYTE unmodeled_066[6];
        BYTE function;
        BYTE unmodeled_06d[3];
        BYTE call;
        BYTE unmodeled_071[3];
        BYTE initializer;
        BYTE unmodeled_075[1];
        BYTE parenthesized;
        BYTE intrinsic_call;
        BYTE unmodeled_078[12];
        BYTE switch_dispatch;
        BYTE switch_value;
    } known;
    BYTE indexed[1];
} ir_node_properties = { { {0}, 0, 128, {0}, 0, 0, 0, 0, 0, 0, {0}, 0, 0, {0}, 0, 0, 128, 0, {0}, 0, 0, {0}, 0, 0, {0}, 0, 0, 0, {0}, 0, {0}, 0, 0, 0, 0, 0, 0, 0, 0, {0}, 0, 0, {0}, 0, 0, 0, 0, 0, {0}, 128, 0, {0}, 128, {0}, 128, {0}, 0, {0}, 0, 128, {0}, 128, 0 } };
/* Intrinsic-call119 consumes opcode64@64044 and only property bit80@642b7. */
typedef char IrIntrinsicCallOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.intrinsic_call) == 119 * 4 ? 1 : -1];
typedef char IrIntrinsicCallPropertyOffset[offsetof(CompilerIrPropertyColumn, known.intrinsic_call) == 119 ? 1 : -1];
typedef char IrStringOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.string_literal) == 7 * 4 ? 1 : -1];
typedef char IrStringPropertyOffset[offsetof(CompilerIrPropertyColumn, known.string_literal) == 7 ? 1 : -1];
typedef char IrOrLogicalOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.logical_or) == 79 * 4 ? 1 : -1];
typedef char IrOrLogicalPropertyOffset[offsetof(CompilerIrPropertyColumn, known.logical_or) == 79 ? 1 : -1];
typedef char IrNotPropertyOffset[offsetof(CompilerIrPropertyColumn, known.logical_not) == 60 ? 1 : -1];
typedef char IrPostIncrementOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.post_increment) == 64 * 4 ? 1 : -1];
typedef char IrPostIncrementPropertyOffset[offsetof(CompilerIrPropertyColumn, known.post_increment) == 64 ? 1 : -1];
typedef char IrDecrementOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.decrement) == 56 * 4 ? 1 : -1];
typedef char IrDecrementPropertyOffset[offsetof(CompilerIrPropertyColumn, known.decrement) == 56 ? 1 : -1];
typedef char Ir_bitwise_and_assign_opcode[offsetof(CompilerIrOpcodeColumn, known.bitwise_and_assign) == 51 * 4 ? 1 : -1];
typedef char Ir_bitwise_and_assign_property[offsetof(CompilerIrPropertyColumn, known.bitwise_and_assign) == 51 ? 1 : -1];
typedef char Ir_remainder_opcode[offsetof(CompilerIrOpcodeColumn, known.remainder) == 72 * 4 ? 1 : -1];
typedef char Ir_remainder_property[offsetof(CompilerIrPropertyColumn, known.remainder) == 72 ? 1 : -1];
typedef char Ir_complement_opcode[offsetof(CompilerIrOpcodeColumn, known.complement) == 89 * 4 ? 1 : -1];
typedef char Ir_complement_property[offsetof(CompilerIrPropertyColumn, known.complement) == 89 ? 1 : -1];
typedef char Ir_bitwise_xor_opcode[offsetof(CompilerIrOpcodeColumn, known.bitwise_xor) == 91 * 4 ? 1 : -1];
typedef char Ir_bitwise_xor_property[offsetof(CompilerIrPropertyColumn, known.bitwise_xor) == 91 ? 1 : -1];
typedef char Ir_xor_assign_opcode[offsetof(CompilerIrOpcodeColumn, known.xor_assign) == 92 * 4 ? 1 : -1];
typedef char Ir_xor_assign_property[offsetof(CompilerIrPropertyColumn, known.xor_assign) == 92 ? 1 : -1];
typedef char IrParenthesizedOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.parenthesized) == 118 * 4 ? 1 : -1];
typedef char IrParenthesizedPropertyOffset[offsetof(CompilerIrPropertyColumn, known.parenthesized) == 118 ? 1 : -1];
typedef char IrNegateOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.negate) == 90 * 4 ? 1 : -1];
typedef char IrNegatePropertyOffset[offsetof(CompilerIrPropertyColumn, known.negate) == 90 ? 1 : -1];
typedef char IrDivisionOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.division) == 57 * 4 ? 1 : -1];
typedef char IrDivisionPropertyOffset[offsetof(CompilerIrPropertyColumn, known.division) == 57 ? 1 : -1];
typedef char IrShiftOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.shift) == 67 * 4 ? 1 : -1];
typedef char IrOrAssignOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.bitwise_or_assign) == 78 * 4 ? 1 : -1];
typedef char IrOrAssignPropertyOffset[offsetof(CompilerIrPropertyColumn, known.bitwise_or_assign) == 78 ? 1 : -1];
typedef char IrOrOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.bitwise_or) == 77 * 4 ? 1 : -1];
typedef char IrShiftPropertyOffset[offsetof(CompilerIrPropertyColumn, known.shift) == 67 ? 1 : -1];
typedef char IrOrPropertyOffset[offsetof(CompilerIrPropertyColumn, known.bitwise_or) == 77 ? 1 : -1];
typedef char IrBitfieldOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.bitfield) == 53 * 4 ? 1 : -1];
typedef char IrDotOpcodeOffset[offsetof(CompilerIrOpcodeColumn, known.dot_member) == 80 * 4 ? 1 : -1];
typedef char IrBitfieldPropertyOffset[offsetof(CompilerIrPropertyColumn, known.bitfield) == 53 ? 1 : -1];
typedef char IrDotPropertyOffset[offsetof(CompilerIrPropertyColumn, known.dot_member) == 80 ? 1 : -1];
#define IR_KIND_MODELED(k) ((k) == 119 || (k) == 78 || (k) == 7 || (k) == 79 || (k) == 64 || (k) == 56 || (k) == 51 || (k) == 72 || (k) == 89 || (k) == 91 || (k) == 92 || (k) == 118 || (k) == 90 || (k) == 57 || (k) == 88 || (k) == 76 || (k) == 70 || (k) == 67 || (k) == 77 || (k) == 53 || (k) == 80 || (k) == 6 || (k) == 49 || (k) == 50 || (k) == 52 || (k) == 54 || (k) == 59 || (k) == 60 || (k) == 61 || (k) == 62 || (k) == 65 || (k) == 68 || (k) == 71 || (k) == 74 || (k) == 81 || (k) == 82 || (k) == 83 || (k) == 85 || (k) == 86 || (k) == 100 || (k) == 101 || (k) == 108 || (k) == 112 || (k) == 116 || (k) == 132 || (k) == 133)
#ifdef __linux__
#define REQUIRE_IR_KIND(k) do { if (!IR_KIND_MODELED(k)) \
    recon_unsupported("IR node column is not modeled for this kind"); } while (0)
#else
#define REQUIRE_IR_KIND(k) do { if (!IR_KIND_MODELED(k)) { \
    __asm { int 3 } } } while (0)
#endif
/* Source-only list views for the special emission paths. They establish no
 * allocation extent, initialized bytes or native support for those owners. */
/* Switch operands below reuse the authentic 24-byte CompilerSwitchCase owner. */
CompilerParserNode *__cdecl lower_ir_node(CompilerParserNode *);
int __cdecl suppress_ir_call(CompilerParserNode *);
CompilerParserNode *__cdecl make_typed_unary_operation(unsigned, CompilerType *, CompilerParserNode *);
CompilerParserNode *__cdecl make_initializer_value(unsigned, unsigned, unsigned);

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
                    CompilerParameterBindings *bindings = (CompilerParameterBindings *)parameter_scope_value;
                    CompilerSymbolBinding *reference = bindings ? bindings->last : NULL;
                    while (reference) {
                        CompilerIrSymbol *parameter = (CompilerIrSymbol *)((BYTE *)reference - 8);
                        if (parameter->kind != 11 && parameter->kind != 12) {
#ifdef __linux__
                            recon_unsupported("IR parameter reverse-link alternative is not yet recovered");
#else
                            __asm { int 3 }
#endif
                        }
                        if (parameter->parameter_type && !(parameter->flags & 2))
                            write_ir_record(45, make_label_expression(parameter, 0));
                        reference = parameter->previous_parameter;
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
                    for (CompilerSwitchCase *part = (CompilerSwitchCase *)node->right;
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
extern "C" {
int __cdecl omit_inline_expression_record(void);
int __cdecl uses_global_function_record(CompilerIrSymbol *);
char *__cdecl make_string_literal_name(const CompilerStringValue *, int);
char *__cdecl make_local_string_name(void);
CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
CompilerParserNode *__cdecl create_string_literal_reference(CompilerParserNode *, CompilerNameEntry *, int, int);
void __cdecl apply_symbol_segment(CompilerIrSymbol *, CompilerIrSymbol *);
void __cdecl write_string_literal_initializer(CompilerIrSymbol *, CompilerParserNode *);
void __cdecl write_global_reference(CompilerSymbolBinding *);
}
CompilerParserNode *__cdecl lower_ir_node(CompilerParserNode *value)
{
    CompilerNameEntry *name = NULL;
    CompilerIrSymbol **found;
    CompilerIrSymbol *existing = NULL;
    const char *spelling = NULL;
    int pooled = omit_inline_expression_record();
    int global = 0;
    if (c1_bss.declaration_scope_active && c1_bss.current_scope_symbol &&
        uses_global_function_record(c1_bss.current_scope_symbol))
        global = 1;
    ++function_reset_value;
    if (pooled)
        spelling = make_string_literal_name((const CompilerStringValue *)&value->text,
            value->string_flags & 1);
    else if (!c1_data.c9_il && global)
        spelling = make_local_string_name();
    if (spelling) {
        name = find_or_create_identifier(spelling);
        if (pooled) {
            found = (CompilerIrSymbol **)c1_data.translation_unit->root->Find(name);
            if (found) existing = *found;
            if (existing) return make_label_expression(existing, 0);
        }
    }
    return create_string_literal_reference(value, name, pooled, global);
}
extern "C" char *__cdecl make_string_literal_name(const CompilerStringValue *, int)
{ __asm { int 3 } return NULL; }
extern "C" char *__cdecl make_local_string_name(void)
{ __asm { int 3 } return NULL; }
extern "C" CompilerParserNode *__cdecl create_string_literal_reference(
    CompilerParserNode *value, CompilerNameEntry *name, int pooled, int global)
{
    BYTE saved_scope = c1_bss.declaration_scope_active;
    BYTE saved_nesting = parameter_nesting;
    CompilerIrSymbol *symbol = lookup_cached_symbol_record(10, NULL, 0);
    CompilerIrSymbol *segment = NULL;
    CompilerIrSymbol *function;
    CompilerParserNode *result;
    CompilerType *type = value->type;
    unsigned storage;
    if (!symbol) {
        symbol = (CompilerIrSymbol *)c1_bss.arenas[1].AllocateBytes(
            (c1_data.record_sizes.known.static_symbol_bytes + 3) & ~3U);
        if (symbol) symbol->Initialize(10, NULL, 0);
    }
    if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
    symbol->record_id = (symbol->record_id & 0xff000000U) |
        (c1_data.next_symbol_record_id++ & 0xffffffU);
    if (type->shape && (type->shape->flags & 7) == 1)
        type = type->WithoutOuterShape();
    symbol->value_type = type;
    if ((symbol->reference_flags & 3) < 3) ++symbol->reference_flags;
    symbol->reference_flags = (symbol->reference_flags & ~4U) |
        ((c1_data.ir_reference_epoch & 1) << 2);
    symbol->declaration_flags |= 8;
    storage = symbol->storage_class & 7;
    symbol->intrinsic_flags = storage;
    symbol->qualifier_flags |= 4;
    symbol->declaration_flags |= 4;
    symbol->flags |= 1;
    if (pooled || (!c1_data.c9_il && global)) {
        symbol->storage_class = (symbol->storage_class & ~0x47U) |
            ((pooled & 1) << 6) | (pooled ? 2 : 3);
        symbol->binding.name = name;
    } else symbol->storage_class = (symbol->storage_class & ~7U) | 3;
    if (pooled || global) {
        symbol->scope_bits = (symbol->scope_bits & ~4U) | (!c1_data.c9_il << 2);
        if ((symbol->scope_bits & 4) && !c1_bss.debug_module->Slot48())
            compiler_error(73, "p1inl.inl", 648);
    }
    if (pooled || global) {
        c1_bss.declaration_scope_active = 0;
        parameter_nesting = 0;
    }
    if (c1_data.normalized_readonly_strings && pooled)
        segment = c1_data.segment_defaults.type_context;
    else segment = c1_data.segment_defaults.definition;
    if (segment) {
        if (pooled) apply_function_context(symbol, segment);
        else apply_symbol_segment(symbol, segment);
    }
    function = c1_bss.current_scope_symbol;
    if (function && (function->linkage_flags & 12) &&
        ((function->symbol_qualifiers & 1) || (lexer_flags & 0x200)) &&
        (function->linkage_flags & 4)) symbol->linkage_flags |= 4;
    if (pooled) c1_data.translation_unit->root->Insert(symbol);
    else symbol->scope_bits = (symbol->scope_bits & ~2U) | ((parameter_nesting != 0) << 1);
    result = make_label_expression(symbol, 0);
    result->type = symbol->value_type;
    write_string_literal_initializer(symbol, value);
    write_global_reference(&symbol->binding);
    result->flags = 4;
    result->kind = 6;
    c1_bss.declaration_scope_active = saved_scope;
    parameter_nesting = saved_nesting;
    return result;
}
CompilerType *RECON_THISCALL CompilerType::WithoutOuterShape(void)
{ __asm { int 3 } return NULL; }
extern "C" void __cdecl apply_symbol_segment(CompilerIrSymbol *symbol, CompilerIrSymbol *segment)
{
    if (!segment) symbol->function_flags &= ~1;
    else attach_declaration_context(symbol, segment);
}
int __cdecl suppress_ir_call(CompilerParserNode *node)
{
    unsigned operation = node->left->low, length = 0;
    CompilerParserNode *arguments, *value;
    const char *text;
    if (operation != 164 && operation != 167) return 0;
    arguments = node->right;
    if (!arguments || !arguments->left) return 0;
    value = arguments->left;
    if (value->kind == 100) value = value->left;
    if (!value || value->kind != 7 || (value->string_flags & 1)) return 0;
    text = value->text;
    while (text[length]) ++length;
    if (operation == 167) {
        emit_ir_node((CompilerParserNode *)make_integer_constant(length));
        return 1;
    }
    node->right = make_expression_with_type(arguments,
        (CompilerParserNode *)make_integer_constant(length + 1), 2);
    node->left->low = 172;
    node->left->high = 0;
    return 0;
}
CompilerParserNode *__cdecl make_typed_unary_operation(unsigned operation, CompilerType *type,
    CompilerParserNode *operand)
{
    CompilerArena *arena;
    CompilerParserNode *value;
    if (c1_data.expression_arena_kind >= 6) {
#ifdef __linux__
        recon_unsupported("typed unary expression arena is not recovered");
#else
        __asm { int 3 }
#endif
    }
    arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    value = (CompilerParserNode *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    value->kind = (BYTE)operation;
    value->flags = 1;
    value->left = operand;
    value->type = type;
    return value;
}
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
unsigned __cdecl parameter_type_size(CompilerType *, int);
unsigned __cdecl parameter_stack_bytes(CompilerParameterBindings *parameters)
{
    unsigned bytes = 0;
    CompilerSymbolBinding *binding = parameters ? parameters->first : NULL;
    while (binding) {
        CompilerIrSymbol *symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
        if (symbol && symbol->value_type)
            bytes += (parameter_type_size(symbol->value_type, 0) + 3) & ~3U;
        binding = binding ? binding->next : NULL;
    }
    return bytes;
}
unsigned __cdecl pointer_storage_size(WORD);
unsigned __cdecl parameter_type_size(CompilerType *type, int promote_float)
{
    CompilerTypeShape *shape = type->shape;
    if (shape) {
        if ((shape->flags & 7) == 2) return pointer_storage_size(shape->attributes);
    } else if (promote_float && type->flags == 0x106) {
        return c1_primitive_widths[7];
    }
    return global_type_size(type);
}
int __cdecl compare_debug_positions(unsigned, unsigned) { __asm { int 3 } return 0; }
void __cdecl begin_browser_function(void)
{
    typedef char BrowserParameterFirstOffset[offsetof(CompilerParameterBindings, first) == 4 ? 1 : -1];
    typedef char BrowserParameterSymbolOffset[offsetof(CompilerIrSymbol, binding) == 8 ? 1 : -1];
    unsigned saved_pending = c1_bss.macro_rescan_output_pending;
    CompilerIrSymbol *function;
    CompilerParameterBindings *parameters;
    CompilerSymbolBinding *binding;
    c1_bss.macro_rescan_output_pending = 0;
    c1_data.backend->Slot0c();
    function = c1_bss.current_scope_symbol;
    if (!(function->record_id & 0xffffff)) {
        if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
        function = c1_bss.current_scope_symbol;
        function->record_id = (function->record_id & 0xff000000U) |
                              (c1_data.next_symbol_record_id++ & 0xffffff);
    }
    c1_data.backend->Slot20(c1_bss.current_scope_symbol);
    c1_data.backend->Slot30(c1_bss.current_scope_symbol);
    c1_bss.macro_rescan_output_pending = saved_pending;
    parameters = (CompilerParameterBindings *)parameter_scope_value;
    for (binding = parameters ? parameters->first : NULL; binding; binding = binding->next) {
        CompilerIrSymbol *symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
        if (symbol->value_type) c1_data.backend->Slot20(symbol);
    }
}
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
unsigned __cdecl create_diagnostic_text(void);
unsigned __cdecl save_type_context(void)
{
    ++c1_data.diagnostic_context_depth;
    return create_diagnostic_text();
}
void *__cdecl resize_compiler_memory(void *, unsigned);
unsigned __cdecl create_diagnostic_text(void)
{
    CompilerSavedContextRecord *records = c1_data.saved_context_records;
    CompilerSavedContextRecord *record;
    int index;
    for (index = 0; index < c1_data.saved_context_count; ++index)
        if (!records[index].handle) break;
    if (index == c1_data.saved_context_count) {
        if (records)
            records = (CompilerSavedContextRecord *)resize_compiler_memory(records,
                (c1_data.saved_context_count + 64) * sizeof(*records));
        else records = (CompilerSavedContextRecord *)allocate_compiler_memory(1280);
        c1_data.saved_context_records = records;
        if (!records) compiler_fatal(60);
        memset(records + c1_data.saved_context_count, 0, 1280);
        index = c1_data.saved_context_count;
        c1_data.saved_context_count += 64;
    }
    record = &records[index];
    if (!record->allocation) {
        record->capacity = 128;
        record->allocation = allocate_compiler_memory(128);
    }
    record->handle = c1_data.next_diagnostic_text_handle++;
    record->prefix_offset = 64;
    record->suffix_offset = 65;
    memset(record->allocation, 0, record->capacity);
    return record->handle;
}
void *__cdecl resize_compiler_memory(void *, unsigned) { __asm { int 3 } return NULL; }
void __cdecl restore_type_context(void)
{
    int index;
    if (!--c1_data.diagnostic_context_depth)
        for (index = 0; index < c1_data.saved_context_count; ++index)
            c1_data.saved_context_records[index].handle = 0;
}
unsigned __cdecl build_diagnostic_declaration(CompilerIrSymbol *, CompilerType *, unsigned);
const char *__cdecl diagnostic_text_data(unsigned);
#ifdef _MSC_VER
#pragma intrinsic(strlen)
#endif
void __cdecl write_declaration_prefix(unsigned flags, CompilerIrSymbol *context, FILE *stream)
{
    save_type_context();
    const char *text = diagnostic_text_data(format_diagnostic_type(0, flags, (unsigned)context));
    fwrite(text, strlen(text), 1, stream);
    restore_type_context();
}
#ifdef _MSC_VER
#pragma intrinsic(strlen)
#endif
void __cdecl write_function_declaration(CompilerIrSymbol *symbol, FILE *stream)
{
    const char *text;
    CompilerIrSymbol *context = NULL;
    unsigned flags;
    if (!symbol) return;
    c1_data.declaration_format_failure = 0;
    c1_data.declaration_format_function = symbol->kind == 7 || symbol->kind == 8;
    text = diagnostic_text_data(build_diagnostic_declaration(symbol, NULL, 0));
    if (c1_data.declaration_format_failure) {
        fwrite("/* ", 3, 1, stream);
        fwrite(text, strlen(text), 1, stream);
        fwrite("; */\n", 5, 1, stream);
        report_language_diagnostic(1, 36, c1_data.declaration_format_failure);
        return;
    }
    flags = symbol->intrinsic_flags;
    if ((symbol->function_flags & 1) && (context = lookup_ir_symbol(symbol)))
        flags |= 0x1000000;
    else flags &= ~0x1000000U;
    write_declaration_prefix(flags, context, stream);
    fwrite(text, strlen(text), 1, stream);
    fwrite(";\n", 2, 1, stream);
}
#ifdef _MSC_VER
#pragma function(strlen)
#endif
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
    if (inline_mode || c1_data.debug_module_kind != 2 || !function)
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
        c1_data.saved_pch_mode ||
        !((c1_data.function_directory_index == 19 && !c1_data.function_directory_head &&
           !c1_data.function_directory_current) ||
          (c1_data.function_directory_index < 20 && c1_data.function_directory_head &&
           c1_data.function_directory_current == c1_data.function_directory_head)))
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
extern "C" int __cdecl uses_global_function_record(CompilerIrSymbol *function)
{
    if ((function->symbol_qualifiers & 1) || (lexer_flags & 0x20)) return 1;
    return c1_data.options.inline_level == 2 && c1_data.ir_symbol_state_ready;
}
extern "C" int __cdecl omit_inline_expression_record(void)
{
    CompilerIrSymbol *function = c1_bss.current_scope_symbol;
    /* The original -cbstring option row addresses the same RVA5ef80 cell. */
    if (c1_data.normalized_pool_strings || c1_data.normalized_readonly_strings) {
        if (!function || c1_data.options.code_based_strings) return 1;
    }
    return function && (function->symbol_qualifiers & 1) &&
        (function->storage_class & 7) != 3;
}

void __cdecl finish_function_label_context(void)
{ c1_data.debug_symbol_index = 0; }
void __cdecl reset_function_scope(void)
{ c1_data.function_scope_cache = NULL; }
void __cdecl initialize_function_debug_symbol(void) { __asm { int 3 } }
/* RVA178c6 */

/* RVA197b3 */
/* Operand preparation and dispatch below express the encountered operator
 * relations directly. They do not expose the mixed classification column or
 * give its other rows defaults. All real conversion/folding calls stay outlined. */
void __cdecl normalize_binary_operands(CompilerParserNode **, CompilerParserNode **);
CompilerParserNode *__cdecl fold_binary_expression(CompilerParserNode *, CompilerParserNode *, CompilerParserNode *);
CompilerType *__cdecl binary_expression_type(BYTE, CompilerType *, CompilerType *);
CompilerParserNode *__cdecl simplify_binary_expression(CompilerParserNode *);
CompilerParserNode *__cdecl convert_binary_rhs(BYTE, CompilerType *, CompilerParserNode *);
CompilerParserNode *__cdecl validate_assignment_target(CompilerParserNode *, int);
void __cdecl check_binary_types(unsigned, CompilerParserNode *, CompilerParserNode *);
CompilerParserNode *__cdecl convert_binary_operand(CompilerType *, CompilerParserNode *);
void __cdecl report_binary_operand_error(unsigned, unsigned);
int __cdecl check_unary_operand(unsigned, int, CompilerParserNode *, CompilerParserNode *);
CompilerParserNode *__cdecl promote_scalar_expression(CompilerParserNode *);
CompilerParserNode *__cdecl finish_segment_dereference(CompilerParserNode *);
CompilerParserNode *__cdecl convert_aggregate_return(CompilerType *, CompilerParserNode *);
CompilerParserNode *__cdecl convert_scalar_expression(unsigned, CompilerType *, CompilerParserNode *);
CompilerType *__cdecl decay_initializer_array(CompilerParserNode *);
unsigned __cdecl require_complete_type_size(CompilerType *);
CompilerParserNode *__cdecl make_binary_expression(BYTE operation,
    CompilerParserNode *left, CompilerParserNode *right)
{
    CompilerParserNode *result, *temporary, *constant;
    CompilerType *type, *left_type, *right_type;
    CompilerTypeShape *shape;
    WORD left_shape, right_shape, qualifiers;
    unsigned size, exponent;
    int swapped = 0;
    if (!left || !right) return NULL;
    if (operation < 49 || operation > 116) {
#ifdef __linux__
        recon_unsupported("binary operator preparation outside recovered relations");
#else
        __asm { int 3 }
#endif
    }
    switch (operation) {
    case 50: case 52: case 55: case 56: case 64: case 79:
    case 80: case 83: case 84: case 85: case 115: case 116:
        break;
    case 51: case 58: case 66: case 71: case 73: case 75:
    case 78: case 82: case 87: case 92: case 97:
        if ((operation == 51 || operation == 73 || operation == 78 || operation == 92) &&
            (left->type->flags & 0x100))
            right = promote_scalar_expression(right);
        else right = convert_binary_operand(left->type, right);
        break;
    case 65:
        right = promote_scalar_expression(right);
        break;
    case 86:
        left = promote_scalar_expression(left);
        right = promote_scalar_expression(right);
        break;
    case 49: case 72: case 77: case 91:
        if (left->type->flags & 0x100) right = promote_scalar_expression(right);
        else if (right->type->flags & 0x100) left = promote_scalar_expression(left);
        else normalize_binary_operands(&left, &right);
        break;
    default:
        normalize_binary_operands(&left, &right);
        break;
    }
    result = make_typed_binary_expression(operation, left, right, NULL);
    if (!check_unary_operand(operation, 2, left, right)) return NULL;
    switch (operation) {
    case 56: case 64: case 84: case 85: /* Increment/decrement. */
        result->type = left->type;
        if (!result->type) return NULL;
        left = validate_assignment_target(left, 1);
        if (!left) { compiler_warning(105, operation); return result; }
        result->left = left;
        shape = result->type->shape;
        left_shape = shape ? shape->flags : 0;
        if (!left_shape) {
            result->right = convert_binary_rhs(operation, result->type, result->right);
            return result;
        }
        if ((left_shape & 7) == 1) {
            size = require_complete_type_size(result->type);
            c1_bss.integer_work_value = (unsigned __int64)size;
            type = c1_data.builtin_types.type_0044;
            if ((unsigned __int64)size <=
                ~((unsigned __int64)-1 << (c1_primitive_widths[3] * 8 - 1)))
                type = c1_data.builtin_types.type_0063;
            result->right = make_typed_constant(type, &c1_bss.integer_work_value,
                c1_data.expression_arena_kind);
        } else if ((left_shape & 7) == 3) compiler_warning(171, operation);
        if (shape->attributes & 3) return finish_segment_dereference(result);
        return result;
    case 81: /* Addition; either operand may carry the pointer. */
    case 70: /* Subtraction also computes a pointer difference. */
    case 65: /* Subscript. */
    case 54: /* Conditional alternatives. */
        if (left->type && left->type->shape && (left->type->shape->flags & 7) == 2)
            left->type = decay_initializer_array(left);
        if (right->type && right->type->shape && (right->type->shape->flags & 7) == 2)
            right->type = decay_initializer_array(right);
        result->type = binary_expression_type(operation, left->type,
            operation == 65 ? NULL : right->type);
        if (!result->type) return NULL;
        left_type = left->type;
        right_type = right->type;
        left_shape = left_type->shape ? left_type->shape->flags : 0;
        right_shape = right_type->shape ? right_type->shape->flags : 0;
        if (operation == 54) {
            if ((left_type->attributes ^ right_type->attributes) & 0x1f00) {
                qualifiers = left_type->attributes |
                    ((left_type->attributes | right_type->attributes) & 0x1f00);
                if (c1_data.type_arena_kind >= 6) {
#ifdef __linux__
                    recon_unsupported("conditional type allocation arena is not modeled");
#else
                    __asm { int 3 }
#endif
                }
                CompilerArena *arena = &c1_bss.arenas[c1_data.type_arena_kind];
                type = (CompilerType *)arena->cursor;
                arena->cursor += 16;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
                memcpy(type, result->type, 16); /* Retain the separate cache bytes. */
                type->attributes = qualifiers;
                result->type = type;
            }
            check_binary_types(operation, left, right);
            shape = result->type->shape;
            if (shape && (shape->flags & 1)) {
                if (left->flags & 0x80) {
                    if (right->flags & 0x80) result->flags |= 0x80;
                    else result->type = right->type;
                } else if (right->flags & 0x80) result->type = left->type;
            }
            return result;
        }
        if (operation == 81) {
            if (!(left_shape & 3) && (right_shape & 3)) {
                temporary = left; left = right; right = temporary;
                result->left = left; result->right = right;
                result->type = left->type;
                qualifiers = left_shape; left_shape = right_shape; right_shape = qualifiers;
                swapped = 1;
            }
            if (left_shape & 3) {
                if ((left_shape & 7) == 3) {
                    report_binary_operand_error(operation, swapped ? 2 : 1); return NULL;
                }
                if (right_shape & 3) { compiler_warning(110); return NULL; }
                if (!(right->type->flags & 0x240)) { compiler_warning(111); return NULL; }
                size = require_complete_type_size(left->type);
                c1_bss.integer_work_value = (unsigned __int64)size;
                constant = make_typed_constant(c1_data.builtin_types.type_0044,
                    &c1_bss.integer_work_value, c1_data.expression_arena_kind);
                result->right = make_binary_expression(74, right, constant);
                return result;
            }
            if (((left_type->flags & 0x80) && !left_shape) ||
                ((right_type->flags & 0x80) && !right_shape)) compiler_warning(186, operation);
            return fold_binary_expression(result, left, right);
        }
        if (operation == 70) {
            if (left_shape && (left_shape & 7) <= 2) {
                size = require_complete_type_size(left_type);
                if (right_shape && (right_shape & 7) <= 2) {
                    check_binary_types(70, left, right);
                    result->type = c1_data.builtin_types.type_0063;
                    result = fold_binary_expression(result, left, right);
                    if (size == 1) return result;
                    if (size & (size - 1))
                        return make_binary_expression(57, result,
                            (CompilerParserNode *)make_integer_constant(size));
                    size >>= 2;
                    exponent = 1;
                    while (size) { ++exponent; size >>= 1; }
                    return make_binary_expression(88, result,
                        (CompilerParserNode *)make_integer_constant(exponent));
                }
                if ((right_shape & 7) == 3) {
                    report_binary_operand_error(operation, 2); return NULL;
                }
                if (!(right_type->flags & 0x240)) { compiler_warning(112); return result; }
                c1_bss.integer_work_value = (__int64)(int)size;
                constant = make_typed_constant(c1_data.builtin_types.type_0044,
                    &c1_bss.integer_work_value, c1_data.expression_arena_kind);
                result->right = make_binary_expression(74, right, constant);
                return result;
            }
            if ((left_shape & 7) == 3) {
                report_binary_operand_error(operation, 1); return NULL;
            }
            if (right_shape & 3) { compiler_warning(113); return NULL; }
            if (((left_type->flags & 0x80) && !left_shape) ||
                ((right_type->flags & 0x80) && !right_shape)) compiler_warning(186, operation);
            return fold_binary_expression(result, left, right);
        }
        /* Subscript chooses its pointer operand before scaling the index. */
        if (!left_shape || (left_shape & 7) > 2) {
            if (!right_shape || (right_shape & 7) > 2) { compiler_warning(109); return result; }
            temporary = left; left = right; right = temporary;
            result->left = left; result->right = right; result->type = left->type;
            right_shape = left_shape;
        }
        if ((left->type->shape->attributes & 3) == 1) { compiler_warning(214); return result; }
        if (right_shape) { compiler_warning(107); return result; }
        if (!(right->type->flags & 0x240)) { compiler_warning(108); return result; }
        size = require_complete_type_size(left->type);
        c1_bss.integer_work_value = (unsigned __int64)size;
        constant = make_typed_constant(c1_data.builtin_types.type_0044,
            &c1_bss.integer_work_value, c1_data.expression_arena_kind);
        right = make_binary_expression(74, right, constant);
        result->right = right;
        if ((right->flags & 7) == 3 && left->kind == 65 && (left->right->flags & 7) == 3) {
            result->left = left->left;
            unsigned old_low = right->low;
            right->low += left->right->low;
            right->high += left->right->high + (right->low < old_low);
        }
        qualifiers = left->type->shape->attributes & 0x1f00;
        result->type = intern_type(get_function_return_type(left->type));
        if (right->type->shape && (right->type->shape->flags & 7) == 2 && qualifiers) {
            shape = result->type->shape;
            if (shape) {
                if ((shape->attributes & 0x1f00) != qualifiers) {
                    unsigned value = shape->value;
                    result->type = replace_function_parameters(result->type, shape->flags,
                        shape->attributes | qualifiers, &value);
                }
            } else result->type->attributes |= qualifiers;
        }
        shape = result->type->shape;
        if (shape && (shape->flags & 7) == 2) return result;
        return take_expression_address(result);
    case 67: case 88: /* Shifts retain the left operand type. */
        result->left = left;
        result->type = left->type;
        if (!result->type) return NULL;
        return fold_binary_expression(simplify_binary_expression(result), left, right);
    case 49: case 57: case 72: case 74: case 77: case 91:
        result->type = binary_expression_type(operation, left->type, right->type);
        if (!result->type) return NULL;
        return fold_binary_expression(simplify_binary_expression(result), left, right);
    case 51: case 52: case 58: case 66: case 71: case 73:
    case 75: case 78: case 82: case 87: case 92: case 116:
        result = simplify_binary_expression(result);
        if (!result) return NULL;
        result->type = left->type;
        result->left = validate_assignment_target(left, operation != 116);
        if (!result->left) { compiler_warning(106, operation); return result; }
        shape = result->type->shape;
        if (shape && (shape->attributes & 3)) {
            if (operation == 116 || operation == 52)
                result->right = convert_aggregate_return(result->type, result->right);
            return finish_segment_dereference(result);
        }
        return result;
    case 59: case 62: case 63: case 68: case 69: case 76:
        if ((left->flags & 7) == 5 || (right->flags & 7) == 5)
            report_language_diagnostic(4, 130, operation);
        /* Fall through to the integer truth-value result. */
    case 50: case 79:
        result = simplify_binary_expression(result);
        if (!result) return NULL;
        result->type = c1_data.builtin_types.type_0063;
        return fold_binary_expression(result, left, right);
    case 86:
        shape = left->type->shape;
        if (!(left->type->flags & 0x340) && !(shape && (shape->flags & 3))) {
            compiler_warning(155); return NULL;
        }
        if ((left->flags & 7) == 3)
            result = convert_scalar_expression(1, right->type,
                (left->low || left->high) ? right->left : right->right);
        /* Fall through: the chosen expression retains the right-hand type. */
    case 55:
        result->flags |= 0x20;
        result->type = right->type;
        if (right->flags & 0x80) result->flags |= 0x80;
        if (result->type && result->type->shape && (result->type->shape->flags & 7) == 2)
            result->type = decay_initializer_array(result);
        return result;
    default:
        return result;
    }
}

/* These are real, separately outlined App owners, still masked until reached. */
CompilerType *__cdecl choose_binary_operand_type(CompilerType *, CompilerParserNode **);
void __cdecl normalize_binary_operands(CompilerParserNode **left, CompilerParserNode **right)
{
    CompilerType *type;
    if (*right) {
        if (!(*right)->type) *right = NULL;
        else if (!(*right)->type->shape) {
            type = choose_binary_operand_type((*left)->type, right);
            if (type) *right = convert_scalar_expression(100, type, *right);
        }
    }
    if (*left) {
        if (!(*left)->type) *left = NULL;
        else if (!(*left)->type->shape) {
            type = choose_binary_operand_type((*right)->type, left);
            if (type) *left = convert_scalar_expression(100, type, *left);
        }
    }
}
CompilerType *__cdecl choose_binary_operand_type(CompilerType *other, CompilerParserNode **operand)
{
    CompilerParserNode *value = *operand;
    WORD flags = value->type->flags, other_flags = other->flags;
    int width, other_width, integer_width = c1_primitive_widths[3];
    if (flags & 0x240) {
        width = ((flags & 0x340) || !(flags & 0xfff0)) ?
            c1_primitive_widths[flags & 15] : flags & 15;
        if ((other_flags & 0x240) || other->shape) {
            other_width = other->shape ? integer_width :
                ((other_flags & 0x340) || !(other_flags & 0xfff0)) ?
                c1_primitive_widths[other_flags & 15] : other_flags & 15;
            if (other_width <= integer_width && width < integer_width)
                return c1_data.builtin_types.type_0063;
            if (other_width > width) return other;
            return (flags & 0x200) ? c1_data.builtin_types.type_0063 : NULL;
        }
        if (other_flags & 0x100) {
            if (width < integer_width)
                *operand = convert_scalar_expression(100, c1_data.builtin_types.type_0063, value);
            return other;
        }
        return width < integer_width ? c1_data.builtin_types.type_0063 : NULL;
    }
    if ((flags & 0x100) && (other_flags & 0x100)) {
        other_width = ((other_flags & 0x340) || !(other_flags & 0xfff0)) ?
            c1_primitive_widths[other_flags & 15] : other_flags & 15;
        width = ((flags & 0x340) || !(flags & 0xfff0)) ?
            c1_primitive_widths[flags & 15] : flags & 15;
        if (other_width > width) return other;
    }
    return NULL;
}
struct InitializerExtendedReal;
const char *__cdecl format_initializer_integer(const unsigned __int64 *, char *, unsigned, int);
int __stdcall parse_initializer_real(const char *, InitializerExtendedReal *);
CompilerParserNode *__cdecl fold_floating_binary_expression(CompilerParserNode *,
    const InitializerExtendedReal *, const InitializerExtendedReal *);
int __cdecl integer_difference_out_of_range(__int64, unsigned __int64, WORD, WORD);
int __cdecl integer_product_out_of_range(__int64, unsigned __int64, WORD, WORD);
CompilerParserNode *__cdecl fold_binary_expression(CompilerParserNode *node,
    CompilerParserNode *left, CompilerParserNode *right)
{
    WORD left_flags = left->type->flags, right_flags = right->type->flags;
    unsigned operation;
    CompilerType *result_type = NULL;
    int left_width, right_width, width, unsigned_values, wide_result = 0;
    unsigned __int64 first, second, value;
    BYTE left_real[12], right_real[12];
    char integer_text[40];
    if (!node) return NULL;
    operation = node->kind;
    if ((left->flags & 7) != 3) return node;
    first = ((unsigned __int64)left->high << 32) | left->low;
    if ((right->flags & 7) != 3) {
        if (operation != 50 && operation != 79) return node;
        if (left_flags & 0x100) {
            /* Preserve the complete extended value, including its precision. */
            memcpy(left_real, (const void *)left->low, 12);
            value = (__int64)classify_literal_bits(6, left_real,
                &extended_zero);
        } else value = first != 0;
        unsigned_values = (left_flags & 0x4000) ||
            (!c1_data.options.language_extensions && c1_data.options.unsigned_char &&
             (left_flags & 0x2fcf) == 0x41 && !(left_flags & 0x8000));
        /* Preserve this dispatch's actual predicate and carried truth value. */
        if (!((operation == 50) ^ (value != 0))) return node;
        c1_bss.integer_work_value = value;
        return make_typed_constant(unsigned_values ? c1_data.builtin_types.type_4063_first :
            c1_data.builtin_types.type_0063, &c1_bss.integer_work_value,
            c1_data.expression_arena_kind);
    }
    second = ((unsigned __int64)right->high << 32) | right->low;
    if ((left_flags | right_flags) & 0x100) {
        /* Real pending format/parse/fold children bound the floating domain.
         * Copy the existing extended object without interpreting its tail. */
        if (left_flags & 0x100) memcpy(left_real, (const void *)left->low, 12);
        else if (parse_initializer_real(format_initializer_integer(&first, integer_text, 10,
            left_flags & 0x4000), (InitializerExtendedReal *)left_real))
            report_language_diagnostic(2, 56);
        if (right_flags & 0x100) memcpy(right_real, (const void *)right->low, 12);
        else if (parse_initializer_real(format_initializer_integer(&second, integer_text, 10,
            right_flags & 0x4000), (InitializerExtendedReal *)right_real))
            report_language_diagnostic(2, 56);
        return fold_floating_binary_expression(node,
            (const InitializerExtendedReal *)left_real, (const InitializerExtendedReal *)right_real);
    }
    left_width = ((left_flags & 0x340) || !(left_flags & 0xfff0)) ?
        c1_primitive_widths[left_flags & 15] : left_flags & 15;
    right_width = ((right_flags & 0x340) || !(right_flags & 0xfff0)) ?
        c1_primitive_widths[right_flags & 15] : right_flags & 15;
    if (left->type->shape || right->type->shape) {
        unsigned_values = 1;
        WORD flags = node->type->flags;
        width = ((flags & 0x340) || !(flags & 0xfff0)) ?
            c1_primitive_widths[flags & 15] : flags & 15;
    } else if (left_width == right_width) {
        unsigned_values = left_width == 1 ? 0 : (left_flags | right_flags) & 0x4000;
        width = left_width < c1_primitive_widths[3] ? c1_primitive_widths[3] : left_width;
    } else if (left_width > right_width) {
        unsigned_values = left_flags & 0x4000;
        width = left_width;
    } else {
        unsigned_values = right_flags & 0x4000;
        width = right_width;
    }
    if (width == c1_primitive_widths[4] &&
        ((left_width && (left_flags & 0x6f) != 0x63) ||
         (right_width && (right_flags & 0x6f) != 0x63))) wide_result = 1;
    if (width <= 0 || width > 8) {
#ifdef __linux__
        recon_unsupported("binary constant width is outside the recovered integer domain");
#else
        __asm { int 3 }
#endif
    }
    if (operation == 50 || operation == 67 || operation == 79 || operation == 88) {
        if (left_width <= c1_primitive_widths[3]) {
            left->high = unsigned_values ? 0 : ((int)left->low < 0 ? ~0U : 0);
            first = ((unsigned __int64)left->high << 32) | left->low;
        }
    } else if (unsigned_values && !left->type->shape && !right->type->shape &&
        (((left_flags & 0x4000) == 0 && (__int64)first < 0) ||
         ((right_flags & 0x4000) == 0 && (__int64)second < 0)) &&
        operation != 49 && operation != 77 && operation != 91)
        report_language_diagnostic(2, 308);
    if (left_width <= c1_primitive_widths[3]) {
        left->high = unsigned_values ? 0 : ((int)left->low < 0 ? ~0U : 0);
        first = ((unsigned __int64)left->high << 32) | left->low;
    }
    if (right_width <= c1_primitive_widths[3]) {
        right->high = unsigned_values ? 0 : ((int)right->low < 0 ? ~0U : 0);
        second = ((unsigned __int64)right->high << 32) | right->low;
    }
    switch (operation) {
    case 49: value = first & second; break;
    case 50: value = first && second; goto boolean_result;
    case 57:
        if (!second) { right->flags = 0x40; return right; }
        value = unsigned_values ? first / second : (__int64)first / (__int64)second;
        break;
    case 59: value = first == second; goto boolean_result;
    case 62:
        value = unsigned_values ? first > second : (__int64)first > (__int64)second;
        goto boolean_result;
    case 63:
        value = unsigned_values ? first >= second : (__int64)first >= (__int64)second;
        goto boolean_result;
    case 67:
        value = (BYTE)right->low >= 64 ? 0 : first << (BYTE)right->low;
        result_type = node->type;
        break;
    case 68:
        value = unsigned_values ? first < second : (__int64)first < (__int64)second;
        goto boolean_result;
    case 69:
        value = unsigned_values ? first <= second : (__int64)first <= (__int64)second;
        goto boolean_result;
    case 70:
        value = first - second;
        if (integer_difference_out_of_range((__int64)first, second,
            (WORD)unsigned_values, (WORD)width))
            report_language_diagnostic(2, 307, operation);
        break;
    case 72:
        if (!second) { right->flags = 0x40; return right; }
        value = unsigned_values ? first % second : (__int64)first % (__int64)second;
        break;
    case 74:
        value = first * second;
        if (integer_product_out_of_range((__int64)first, second,
            (WORD)unsigned_values, (WORD)width))
            report_language_diagnostic(2, 307, operation);
        break;
    case 76: value = first != second; goto boolean_result;
    case 77: value = first | second; break;
    case 79: value = first || second; goto boolean_result;
    case 81: case 83:
        value = first + second;
        if (integer_sum_out_of_range((__int64)first, second,
            (WORD)unsigned_values, (WORD)width))
            report_language_diagnostic(2, 307, operation);
        break;
    case 88:
        if (unsigned_values) value = (BYTE)right->low >= 64 ? 0 : first >> (BYTE)right->low;
        else value = (BYTE)right->low >= 64 ? ((__int64)first < 0 ? (unsigned __int64)-1 : 0) :
            (__int64)first >> (BYTE)right->low;
        result_type = node->type;
        break;
    case 91: value = first ^ second; break;
    default: return node;
    }
    goto finish_integer;
boolean_result:
    result_type = c1_data.builtin_types.type_0063;
    unsigned_values = 0;
finish_integer:
    if ((operation == 81 || operation == 70) && node->type->shape &&
        (node->type->shape->flags & 1)) {
        left->low = (unsigned)value;
        left->high = (unsigned)(value >> 32);
        return left;
    }
    if (result_type) {
        WORD flags = result_type->flags;
        width = ((flags & 0x340) || !(flags & 0xfff0)) ?
            c1_primitive_widths[flags & 15] : flags & 15;
    }
    if (width == 8) {
        c1_bss.integer_work_value = value;
        return make_typed_constant(unsigned_values ? c1_data.builtin_types.type_404c :
            c1_data.builtin_types.type_004c, &c1_bss.integer_work_value,
            c1_data.expression_arena_kind);
    }
    value = unsigned_values ? (unsigned)value : (__int64)(int)value;
    if (result_type)
        wide_result = width == c1_primitive_widths[4] && (result_type->flags & 0x6f) != 0x63;
    if (c1_primitive_widths[3] == c1_primitive_widths[2] && !wide_result)
        value = unsigned_values ? (WORD)value : (__int64)(short)value;
    c1_bss.integer_work_value = value;
    CompilerType *type = wide_result ?
        (unsigned_values ? c1_data.builtin_types.type_4044 : c1_data.builtin_types.type_0044) :
        (unsigned_values ? c1_data.builtin_types.type_4063_first : c1_data.builtin_types.type_0063);
    return make_typed_constant(type, &c1_bss.integer_work_value, c1_data.expression_arena_kind);
}
int __cdecl add_extended_constants(const InitializerExtendedReal *, const InitializerExtendedReal *, InitializerExtendedReal *);
int __cdecl subtract_extended_constants(const InitializerExtendedReal *, const InitializerExtendedReal *, InitializerExtendedReal *);
int __cdecl multiply_extended_constants(const InitializerExtendedReal *, const InitializerExtendedReal *, InitializerExtendedReal *);
int __cdecl divide_extended_constants(const InitializerExtendedReal *, const InitializerExtendedReal *, InitializerExtendedReal *);
void __cdecl round_extended_constant(BYTE *, unsigned);
BYTE *__cdecl copy_extended_constant(const BYTE *, unsigned);
CompilerParserNode *__cdecl fold_floating_binary_expression(CompilerParserNode *node,
    const InitializerExtendedReal *left, const InitializerExtendedReal *right)
{
    InitializerExtendedReal value;
    unsigned relation = 0;
    int result;
    CompilerType *type;
    switch (node->kind) {
    case 68: relation = 4; break;
    case 69: relation = 5; break;
    case 63: relation = 3; break;
    case 62: relation = 2; break;
    case 59: relation = 1; break;
    case 76: relation = 6; break;
    case 50:
        result = classify_literal_bits(6, left, &extended_zero) &&
            classify_literal_bits(6, right, &extended_zero);
        goto integer_result;
    case 79:
        result = classify_literal_bits(6, left, &extended_zero) ||
            classify_literal_bits(6, right, &extended_zero);
        goto integer_result;
    case 81: result = add_extended_constants(left, right, &value); break;
    case 70: result = subtract_extended_constants(left, right, &value); break;
    case 74: result = multiply_extended_constants(left, right, &value); break;
    case 57:
        if (!classify_literal_bits(6, right, &extended_zero)) {
            node->flags = 0x40;
            return node;
        }
        result = divide_extended_constants(left, right, &value);
        break;
    default: return node;
    }
    if (relation) {
        result = classify_literal_bits(relation, left, right);
integer_result:
        c1_bss.integer_work_value = (__int64)result;
        return make_typed_constant(c1_data.builtin_types.type_0063,
            &c1_bss.integer_work_value, c1_data.expression_arena_kind);
    }
    if (result) report_language_diagnostic(2, 56);
    if (c1_data.options.precise_float)
        round_extended_constant((BYTE *)&value, (unsigned)(int)(short)value.precision);
    *(unsigned *)&c1_bss.integer_work_value = (unsigned)copy_extended_constant((const BYTE *)&value,
        c1_data.expression_arena_kind);
    switch (node->type->flags & 0xefcf) {
    case 0x106: type = c1_data.builtin_types.type_0106; break;
    case 0x107: type = c1_data.builtin_types.type_0107; break;
    case 0x108: type = c1_data.builtin_types.type_0108; break;
    default: return NULL;
    }
    return make_typed_constant(type, &c1_bss.integer_work_value, c1_data.expression_arena_kind);
}
int __cdecl integer_difference_out_of_range(__int64 first, unsigned __int64 second,
    WORD unsigned_values, WORD width)
{
    if (!unsigned_values && ((first >= 0) != ((__int64)second >= 0)))
        return integer_sum_out_of_range(first, (unsigned __int64)0 - second, 0, width);
    return 0;
}
int __cdecl integer_product_out_of_range(__int64 first, unsigned __int64 second,
    WORD unsigned_values, WORD width)
{
    unsigned __int64 magnitude, other, limit;
    if (width != 1 && width != 2 && width != 4 && width != 8) {
#ifdef __linux__
        recon_unsupported("integer product width is not yet qualified");
#else
        __asm { int 3 }
#endif
    }
    limit = ~((unsigned __int64)-1 << (width * 8 - 1));
    if (unsigned_values) {
        limit |= limit << 1;
        unsigned shift = (8 - width) * 8;
        magnitude = ((unsigned __int64)first << shift) >> shift;
        other = (second << shift) >> shift;
    } else {
        magnitude = first < 0 ? 0 - (unsigned __int64)first : (unsigned __int64)first;
        other = (__int64)second < 0 ? 0 - second : second;
        if ((first > 0) != ((__int64)second > 0)) ++limit;
    }
    return magnitude && other > limit / magnitude;
}

CompilerType *__cdecl binary_expression_type(BYTE operation, CompilerType *left, CompilerType *right)
{
    CompilerType *result = left;
    WORD left_flags, right_flags;
    int left_width, right_width;
    (void)operation;
    if (!left) result = right;
    else if (!right) result = left;
    else if (left->shape) {
        if (right->shape && (right->flags & 0x80)) result = right;
    } else if (right->shape) result = right;
    else {
        left_flags = left->flags;
        right_flags = right->flags;
        if (left_flags & 0x100) {
            if (right_flags & 0x100) {
                left_width = ((left_flags & 0x340) || !(left_flags & 0xfff0)) ?
                    c1_primitive_widths[left_flags & 15] : left_flags & 15;
                right_width = ((right_flags & 0x340) || !(right_flags & 0xfff0)) ?
                    c1_primitive_widths[right_flags & 15] : right_flags & 15;
                if (left_width < right_width) result = right;
            }
        } else if (right_flags & 0x100) result = right;
        else if ((left_flags & 0x240) && (right_flags & 0x240)) {
            left_width = ((left_flags & 0x340) || !(left_flags & 0xfff0)) ?
                c1_primitive_widths[left_flags & 15] : left_flags & 15;
            right_width = ((right_flags & 0x340) || !(right_flags & 0xfff0)) ?
                c1_primitive_widths[right_flags & 15] : right_flags & 15;
            if (left_width == right_width) result = (left_flags & 0x4000) ? left : right;
            else result = left_width > right_width ? left : right;
        }
    }
    if (result->GetCallingConvention() & 0x0f00) result = result->WithoutTopQualifiers();
    return result;
}
CompilerType *RECON_THISCALL CompilerType::WithoutTopQualifiers(void)
{
    if (!shape) {
        if (!(attributes & 0x1f00)) return this;
        CompilerType description;
        /* Keep the complete16-byte storage, including the opaque cache. */
        memcpy(&description, this, 16);
        description.attributes &= 0xf0ff;
        return intern_type(&description);
    }
    if (!(shape->attributes & 0x1f00)) return this;
    CompilerTypeShape replacement = *shape;
    replacement.attributes &= 0xf0ff;
    return WithShape(intern_type_shape(&replacement));
}
CompilerParserNode *__cdecl simplify_binary_expression(CompilerParserNode *node)
{
    CompilerParserNode *left = node->left, *right = node->right, *constant;
    CompilerType *left_type = left->type, *right_type = right->type;
    CompilerTypeShape *left_shape = left_type->shape, *right_shape = right_type->shape;
    WORD left_flags = left_shape ? left_shape->flags : 0;
    WORD right_flags = right_shape ? right_shape->flags : 0;
    unsigned operation = node->kind, mask;
    int bad = 0;
    switch (operation) {
    case 49: case 51: case 66: case 67: case 72: case 73:
    case 77: case 78: case 87: case 88: case 91: case 92:
        mask = 0x240;
        goto check_scalars;
    case 57: case 74:
        mask = 0x340;
check_scalars:
        if (!(left_type->flags & mask) || left_flags) {
            report_binary_operand_error(operation, 1); bad = 1;
        }
        if (!(right_type->flags & mask) || right_flags) {
            report_binary_operand_error(operation, 2); bad = 1;
        }
        return bad ? NULL : node;
    case 58: case 75:
        if (!(left_type->flags & 0x140) || left_flags) {
            report_binary_operand_error(operation, 1); bad = 1;
        }
        if (!(right_type->flags & 0x340) || right_flags) {
            report_binary_operand_error(operation, 2); bad = 1;
        }
        if (bad) return NULL;
        check_binary_types(operation, left, right);
        return node;
    case 50: case 79:
        if (!(left_type->flags & 0x340) && !(left_flags & 3)) {
            report_binary_operand_error(operation, 1); bad = 1;
        }
        if (!(right_type->flags & 0x340) && !(right_flags & 3)) {
            report_binary_operand_error(operation, 2); bad = 1;
        }
        return bad ? NULL : node;
    case 59: case 62: case 63: case 68: case 69: case 76:
        if ((left->flags & 7) != 3 && (right->flags & 7) != 3 &&
            (left_type->flags & 0x40) && !left_flags &&
            (right_type->flags & 0x40) && !right_flags &&
            ((left_type->flags ^ right_type->flags) & 0x4000)) {
            if (!(left->kind == 100 && left_type == c1_data.builtin_types.type_0063 &&
                  (left->left->type->flags & 0x4000)) &&
                !(right->kind == 100 && right_type == c1_data.builtin_types.type_0063 &&
                  (right->left->type->flags & 0x4000)))
                report_language_diagnostic(3, 18, operation);
        }
        check_binary_types(operation, left, right);
        return node;
    case 71: case 82:
        if (right_shape || !(right_type->flags & 0x340)) {
            report_binary_operand_error(operation, 2); return NULL;
        }
        if ((left_type->flags & 0x200) && !left_shape) {
            report_binary_operand_error(operation, 1); return NULL;
        }
        if (!left_shape && (left_type->flags & 0x140)) {
            check_binary_types(operation, left, right);
            return node;
        }
        switch (left_flags & 7) {
        case 1:
            if (!(right_type->flags & 0x240)) { compiler_warning(114, operation); return NULL; }
            c1_bss.integer_work_value = (unsigned __int64)require_complete_type_size(left_type);
            constant = make_typed_constant(c1_data.builtin_types.type_0044,
                &c1_bss.integer_work_value, c1_data.expression_arena_kind);
            node->right = make_binary_expression(74, right, constant);
            return node;
        case 3:
            report_binary_operand_error(operation, 1);
            return NULL;
        default:
            return node;
        }
    case 52: case 116:
        if (right_shape && (right_shape->flags & 7) == 2)
            right->type = decay_initializer_array(right);
        check_binary_types(operation, left, right);
        return node;
    default:
        return NULL;
    }
}

__int64 __cdecl convert_integer_constant(BYTE, CompilerParserNode *, WORD, WORD, WORD);
int __cdecl initializer_real_to_integer(const InitializerExtendedReal *, unsigned __int64 *);
void __cdecl round_extended_constant(BYTE *, unsigned);
BYTE *__cdecl copy_extended_constant(const BYTE *, unsigned);
/* The -Op row, default and PCH snapshot already own this exact DWORD. */
#define round_floating_constants c1_data.options.precise_float /* RVA5efbc */
CompilerParserNode *__cdecl convert_binary_rhs(BYTE operation, CompilerType *type,
    CompilerParserNode *value)
{
    CompilerType *source;
    WORD flags, source_flags, shape_flags, shape_attributes, source_shape;
    BYTE floating[12];
    char integer_text[40];
    unsigned __int64 bits;
    unsigned width;
    if ((value->type->flags & 0x200) && !(type->flags & 0x200))
        value->type = c1_data.builtin_types.type_0063;
    source = value->type;
    flags = type->flags;
    source_flags = source->flags;
    shape_flags = type->shape ? type->shape->flags : 0;
    shape_attributes = type->shape ? type->shape->attributes : 0;
    source_shape = source->shape ? source->shape->flags : 0;
    if ((source_flags & 0x40) || source_shape) {
        if ((flags & 0x40) || shape_flags) {
            bits = convert_integer_constant(operation, value, shape_flags,
                shape_attributes, flags);
            memcpy(&value->low, &bits, 8);
            if ((type->flags & 0x4000) && !(value->type->flags & 0x4000)) {
                CompilerType *description = copy_type_description(value->type);
                description->flags |= 0x4000;
                value->type = intern_type(description);
                --type_description_depth;
            }
        } else {
            if (!(flags & 0x100)) goto finish;
            if (parse_initializer_real(format_initializer_integer(
                (const unsigned __int64 *)&value->low, integer_text, 10,
                source->flags & 0x4000), (InitializerExtendedReal *)floating))
                report_language_diagnostic(2, 56);
        }
    } else if (source_flags & 0x100) {
        if (flags & 0x40) {
            if (initializer_real_to_integer((InitializerExtendedReal *)value->low, &bits))
                report_language_diagnostic(2, 56);
            memcpy(&value->low, &bits, 8);
            value->kind = 101;
        } else {
            if (!(flags & 0x100)) goto finish;
            if (shape_flags) goto finish;
            memcpy(floating, (const void *)value->low, 12);
        }
    }
    if ((flags & 0x100) && !shape_flags) {
        width = ((flags & 0x340) || !(flags & 0xfff0)) ?
            c1_primitive_widths[flags & 15] : flags & 15;
        if (round_floating_constants) round_extended_constant(floating, width);
        ((WORD *)floating)[5] = (WORD)width;
        value->kind = 101;
        value->low = (unsigned)copy_extended_constant(floating, c1_data.expression_arena_kind);
    }
finish:
    value->type = type;
    return value;
}
int __cdecl integer_value_loses_bits(int, int, int, __int64);
__int64 __cdecl convert_integer_constant(BYTE operation, CompilerParserNode *node,
    WORD shape_flags, WORD shape_attributes, WORD target_flags)
{
    unsigned __int64 value = ((unsigned __int64)node->high << 32) | node->low;
    unsigned __int64 before, normalized;
    WORD source_flags;
    int destination_width = shape_flags, source_width, shift, unsigned_source;
    (void)shape_attributes;
    if (shape_flags & 1) return (__int64)value;
    if (target_flags & 0x40)
        destination_width = ((target_flags & 0x340) || !(target_flags & 0xfff0)) ?
            c1_primitive_widths[target_flags & 15] : target_flags & 15;
    else if (target_flags & 0x200) destination_width = c1_primitive_widths[3];
    else {
        /* This machine path reuses the incoming shape-flags slot as width.
         * Only its integer/enum overwrite is supported at current callers. */
#ifdef __linux__
        recon_unsupported("constant conversion without a recovered destination width");
#else
        __asm { int 3 }
#endif
    }
    source_flags = node->type->flags;
    if (!node->type->shape) {
        if (source_flags & 0x40)
            source_width = ((source_flags & 0x340) || !(source_flags & 0xfff0)) ?
                c1_primitive_widths[source_flags & 15] : source_flags & 15;
        else source_width = (source_flags & 0x200) ?
            c1_primitive_widths[3] : destination_width;
        if (source_width < destination_width) {
            before = value;
            switch (source_width) {
            case 2:
                value = (source_flags & 0x4000) ? (unsigned __int64)(WORD)value :
                    (unsigned __int64)(__int64)(short)value;
                break;
            case 4:
                value = (source_flags & 0x4000) ? (unsigned __int64)(unsigned)value :
                    (unsigned __int64)(__int64)(int)value;
                break;
            case 8: case 16:
                break;
            default:
                value = ((source_flags & 0x4000) ||
                    (!c1_data.options.language_extensions && c1_data.options.unsigned_char &&
                     !(source_flags & 0x8000))) ? (unsigned __int64)(BYTE)value :
                    (unsigned __int64)(__int64)(signed char)value;
                break;
            }
            if (value != before) {
                if (operation == 100) report_language_diagnostic(4, 310);
                else report_language_diagnostic(2, 309, operation);
            }
        }
    }
    source_width = ((source_flags & 0x340) || !(source_flags & 0xfff0)) ?
        c1_primitive_widths[source_flags & 15] : source_flags & 15;
    if (source_width < 0 || source_width > 8) {
#ifdef __linux__
        recon_unsupported("constant source width exceeds the recovered integer domain");
#else
        __asm { int 3 }
#endif
    }
    unsigned_source = (source_flags & 0x4000) != 0;
    if (!source_width) normalized = 0;
    else {
        shift = (8 - source_width) * 8;
        normalized = unsigned_source ? (value << shift) >> shift :
            (unsigned __int64)((__int64)(value << shift) >> shift);
    }
    if (integer_value_loses_bits(destination_width, target_flags & 0x4000,
        unsigned_source, (__int64)normalized)) {
        if (operation == 100) report_language_diagnostic(4, 310);
        else report_language_diagnostic(2, 309, operation);
    }
    switch (destination_width) {
    case 2:
        return (target_flags & 0x4000) ? (__int64)(WORD)value : (__int64)(short)value;
    case 4:
        return (target_flags & 0x4000) ? (__int64)(unsigned)value : (__int64)(int)value;
    case 8: case 16:
        return (__int64)value;
    default:
        if ((target_flags & 0x4000) || (!c1_data.options.language_extensions &&
            c1_data.options.unsigned_char && !(target_flags & 0x8000)))
            return (__int64)(BYTE)value;
        return (__int64)(signed char)value;
    }
}
void __cdecl round_extended_constant(BYTE *, unsigned) { __asm { int 3 } }

CompilerParserNode *__cdecl find_expression_reference(CompilerParserNode *);
unsigned __cdecl global_type_size(CompilerType *);
CompilerParserNode *__cdecl validate_assignment_target(CompilerParserNode *value, int mode)
{
    CompilerParserNode *reference;
    unsigned constant, source_size;
    if (!value) return NULL;
    if (value->flags & 0x20) {
        if (!(mode & 10)) return NULL;
        if ((mode & 10) != 2) compiler_error(1, ".\\trees.c", 6362);
    }
    while (value->kind == 118) value = value->left;
    if (value->kind == 100) {
        if (!c1_data.options.language_extensions) return NULL;
        source_size = global_type_size(value->left->type);
        if (global_type_size(value->type) > source_size) return NULL;
        value = value->left;
        while (value->kind == 100) {
            source_size = global_type_size(value->left->type);
            if (global_type_size(value->type) > source_size) return NULL;
            value = value->left;
        }
        if (mode & 1) report_language_diagnostic(4, 213);
        if ((value->flags & 7) == 4) value->symbol->qualifier_flags |= 8;
        else {
            if (value->kind != 61 && value->kind != 101) {
#ifdef __linux__
                recon_unsupported("cast lvalue classification is not yet recovered");
#else
                __asm { int 3 }
#endif
            }
            reference = find_expression_reference(
                c1_expression_classes.indexed[value->kind] == 61 ? value->left : value);
            if (reference && (reference->flags & 7) == 4) reference->symbol->qualifier_flags |= 8;
        }
    }
    if (value->kind == 93) value = value->left;
    if (value->kind == 94) return NULL;
    if (value->kind != 6 && value->kind != 61 && value->kind != 101) {
#ifdef __linux__
        recon_unsupported("assignment lvalue classification is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    if (c1_expression_classes.indexed[value->kind] != 61) return NULL;
    constant = (declared_type_flags(value->type) >> 8) & 1;
    if ((mode & 1) && (value->type->flags & 0x400) && !value->type->shape) {
        /* Complete record definitions initialize this flag word. Other owner
         * shapes remain unsupported before the mode1 diagnostic reads it. */
        if (value->type->flags != 0x402 || value->type->attributes ||
            !value->type->symbol_owner || value->type->symbol_owner->kind != 4 ||
            !(value->type->symbol_owner->flags & 1)) {
#ifdef __linux__
            recon_unsupported("aggregate assignment type-owner flags are not yet supported");
#else
            __asm { int 3 }
#endif
        }
        constant |= (value->type->symbol_owner->tag_definition_flags >> 10) & 1;
    }
    value = value->left;
    if (value->kind == 80 && value->left->kind == 112 && !value->left->type->shape) return NULL;
    if ((mode & 1) && constant) compiler_warning(166);
    return value;
}
CompilerParserNode *__cdecl find_expression_reference(CompilerParserNode *value)
{
    for (;;) {
        switch (value->kind) {
        case 6: case 100: return value;
        case 54:
            value = value->type == value->right->type ? value->right : value->left;
            break;
        case 65: case 80: case 118:
            value = value->left;
            break;
        case 83: return value->left;
        case 93:
            value = value->left;
            if (value->kind == 61) value = value->left;
            break;
        default: return NULL;
        }
    }
}

int __cdecl convert_expression_to_type(unsigned, CompilerType *, CompilerParserNode *);
/* All actual callers discard incidental EAX; this is an operand compatibility check. */
void __cdecl check_binary_types(unsigned operation, CompilerParserNode *left, CompilerParserNode *right)
{
    if (!left || !right) return;
    if ((left->flags & 0x80) && right->type->shape && (right->type->shape->flags & 3)) return;
    convert_expression_to_type(operation, left->type, right);
}
CompilerParserNode *__cdecl convert_binary_operand(CompilerType *other, CompilerParserNode *value)
{
    CompilerType *type;
    if (!value) return value;
    if (!value->type) return NULL;
    if (value->type->shape) return value;
    type = choose_binary_operand_type(other, &value);
    return type ? convert_scalar_expression(100, type, value) : value;
}
void __cdecl report_binary_operand_error(unsigned, unsigned) { __asm { int 3 } }

/* RVA1b07b */
int __cdecl check_unary_operand(unsigned, int, CompilerParserNode *, CompilerParserNode *);
CompilerParserNode *__cdecl promote_scalar_expression(CompilerParserNode *);
CompilerType *__cdecl dereference_expression_type(CompilerType *);
CompilerType *__cdecl adjust_reference_type(CompilerType *, unsigned);
CompilerParserNode *__cdecl convert_unary_operand(CompilerParserNode *, unsigned);
CompilerParserNode *__cdecl finish_segment_dereference(CompilerParserNode *);
extern "C" int __cdecl subtract_extended_constants(const InitializerExtendedReal *,
    const InitializerExtendedReal *, InitializerExtendedReal *);
BYTE *__cdecl copy_extended_constant(const BYTE *, unsigned);
CompilerParserNode * __cdecl make_unary_expression(BYTE operation, CompilerParserNode *value)
{
    CompilerParserNode *result, *left;
    CompilerType *type;
    CompilerTypeShape *shape;
    WORD shape_flags, flags;
    unsigned width, shift;
    __int64 integer; /* Default dispatch in the original reads unwritten stack words. */
    BYTE floating[12];
    if (!value) return NULL;
    shape = value->type->shape;
    shape_flags = shape ? shape->flags : 0;
    result = make_typed_unary_operation(operation, value->type, value);
    if (!check_unary_operand(operation, 1, NULL, value)) return NULL;
    switch (operation) {
    case 49: /* Address-of uses the actual lvalue conversion helper. */
        return convert_unary_operand(value, 12);
    case 61: /* Indirection. */
        switch (shape_flags & 7) {
        case 3:
            if (shape_flags & 8) return value;
            value->type = adjust_reference_type(value->type, 1);
            return value;
        case 1:
            result->type = dereference_expression_type(value->type);
            type = result->type;
            shape = type->shape;
            if (!shape) {
                if ((type->flags & 0x2fcf) == 0x80) {
                    compiler_warning(100);
                    result = result->left;
                }
            } else if ((shape->flags & 7) == 2) {
                value->flags |= 0x20;
                value->type = replace_function_parameters(type, 2,
                    (WORD)((shape->attributes & 0x1f00) |
                    (value->type->shape->storage & 0x73)), &shape->value);
                return value;
            } else if ((shape->flags & 7) == 1 && (shape->attributes & 3))
                return finish_segment_dereference(result);
            if ((value->flags & 7) == 4 && value->kind == 6 &&
                (result->flags & 7) == 1 && result->kind == 61) {
                left = result->left;
                if ((left->flags & 7) == 4 && left->kind == 6) left->flags &= ~0x20;
            }
            return result;
        case 2:
            return make_binary_expression(65, value,
                (CompilerParserNode *)make_integer_constant(0));
        case 4:
            return value;
        default:
            compiler_warning(100);
            result = result->left;
            if ((value->flags & 7) == 4 && value->kind == 6 &&
                (result->flags & 7) == 1 && result->kind == 61) {
                left = result->left;
                if ((left->flags & 7) == 4 && left->kind == 6) left->flags &= ~0x20;
            }
            return result;
        }
    case 90: /* Unary minus, including the original constant folding. */
        if (shape_flags || !(value->type->flags & 0x340)) {
            compiler_warning(171, 90);
            return result;
        }
        value = promote_scalar_expression(value);
        result->type = value->type;
        flags = value->type->flags;
        if ((flags & 0x4000) || (!c1_data.options.language_extensions &&
            c1_data.options.unsigned_char && (flags & 0x2fcf) == 0x41 && !(flags & 0x8000)))
            report_language_diagnostic(2, 146);
        if ((value->flags & 7) != 3) return result;
        flags = value->type->flags;
        if (flags & 0x100) {
            if (subtract_extended_constants(&extended_zero,
                (const InitializerExtendedReal *)value->low, (InitializerExtendedReal *)floating))
                report_language_diagnostic(2, 56);
            value->low = (unsigned)copy_extended_constant(floating, c1_data.expression_arena_kind);
            return value;
        }
        integer = (__int64)(0 - (((unsigned __int64)value->high << 32) | value->low));
        if (!(flags & 0x4000)) {
            width = ((flags & 0x340) || !(flags & 0xfff0)) ?
                (unsigned)(int)c1_primitive_widths[flags & 15] : flags & 15;
            if (width > 8) {
#ifdef __linux__
                recon_unsupported("unary integer width exceeds the recovered scalar domain");
#else
                __asm { int 3 }
#endif
            }
            if (!width) integer = 0;
            else {
                shift = (8 - width) * 8;
                integer = (__int64)((unsigned __int64)integer << shift) >> shift;
            }
        }
        break;
    case 60: /* Logical negation. */
        if (!(value->type->flags & 0x340) && !(shape_flags & 3)) {
            compiler_warning(171, 60);
            return result;
        }
        result->type = c1_data.builtin_types.type_0063;
        if ((value->flags & 7) != 3) return result;
        if (!(value->type->flags & 0x240) && !(shape_flags & 1))
            return make_binary_expression(59, value,
                (CompilerParserNode *)make_integer_constant(0));
        integer = !(value->low | value->high);
        break;
    case 89: /* Bitwise complement. */
        if (!(value->type->flags & 0x240) || shape_flags) {
            compiler_warning(171, 89);
            return result;
        }
        value = promote_scalar_expression(value);
        result->type = value->type;
        if ((value->flags & 7) != 3) return result;
        integer = (__int64)~(((unsigned __int64)value->high << 32) | value->low);
        break;
    case 118: /* Parenthesized expression. */
        if (!(result->type->flags & 0x100) || (value->flags & 7) == 3 || shape_flags)
            return value;
        return result;
    case 134: /* Unary plus. */
        if (shape_flags || !(value->type->flags & 0x340)) compiler_warning(171, 134);
        value = promote_scalar_expression(value);
        result->type = value->type;
        if ((value->flags & 7) != 3)
            return result->type == value->type ? value : result;
        if (!shape_flags && (value->type->flags & 0x100)) return value;
        integer = (__int64)(((unsigned __int64)value->high << 32) | value->low);
        break;
    default:
        /* These operation bytes are not established unary alternatives.
         * The original carries private, unwritten stack words to its common
         * fold path; do not manufacture a value for that unsupported domain. */
#ifdef __linux__
        recon_unsupported("unrecovered unary operation and carried stack value");
#else
        __asm { int 3 }
#endif
        break;
    }
    flags = value->type->flags;
    if (!(flags & 0x80) && ((flags & 0x4000) || integer > 0)) {
        width = ((flags & 0x340) || !(flags & 0xfff0)) ?
            (unsigned)(int)c1_primitive_widths[flags & 15] : flags & 15;
        if (width > 8) {
#ifdef __linux__
            recon_unsupported("unary unsigned width exceeds the recovered scalar domain");
#else
            __asm { int 3 }
#endif
        }
        if (!width) integer = 0;
        else {
            shift = (8 - width) * 8;
            integer = (__int64)(((unsigned __int64)integer << shift) >> shift);
        }
    }
    value->low = (unsigned)integer;
    value->high = (unsigned)((unsigned __int64)integer >> 32);
    value->type = result->type;
    return value;
}
int __cdecl permits_incomplete_operand(BYTE, BYTE, int); /* RVA1f5e9 */
int __cdecl check_unary_operand(unsigned operation, int arity,
    CompilerParserNode *left, CompilerParserNode *right)
{
    int incomplete_left = left && left->type && (left->type->flags & 0x400) &&
        !left->type->shape;
    int incomplete_right = right && right->type && (right->type->flags & 0x400) &&
        !right->type->shape;
    if (!incomplete_left && !incomplete_right) return 1;
    if (permits_incomplete_operand((BYTE)operation, (BYTE)arity, incomplete_left)) return 1;
    compiler_warning(88, (BYTE)operation, incomplete_left ? left->type : right->type);
    return 0;
}
int __cdecl permits_incomplete_operand(BYTE operation, BYTE arity, int incomplete_left)
{
    if (arity == 1) {
        if (operation == 49 || operation == 118) return 1;
        if (operation != 6 && operation != 45 && operation != 50 &&
            operation != 59 && operation != 61 && operation != 65 &&
            operation != 101 && operation != 112) {
#ifdef __linux__
            recon_unsupported("incomplete unary operand classification is not yet recovered");
#else
            __asm { int 3 }
#endif
        }
        return c1_expression_classes.indexed[operation] == 61;
    }
    if (arity != 2) return 1;
    switch (operation) {
    case 52: case 54: case 55: case 116:
        return 1;
    case 86:
        return !incomplete_left;
    default:
        return 0;
    }
}
CompilerType *__cdecl promoted_integer_type(CompilerType *); /* RVA0f0d3 */
CompilerParserNode *__cdecl convert_scalar_expression(unsigned, CompilerType *, CompilerParserNode *); /* RVA1c582 */
CompilerParserNode *__cdecl promote_scalar_expression(CompilerParserNode *value)
{
    CompilerType *type;
    if (!value) return NULL;
    if (!value->type) return value;
    type = promoted_integer_type(value->type);
    if (!type) return value;
    return convert_scalar_expression(100, type, value);
}
CompilerType *__cdecl promoted_integer_type(CompilerType *type)
{
    unsigned width;
    WORD flags = type->flags;
    if (!(flags & 0x240) || type->shape) return NULL;
    if (flags & 0x200) return c1_data.builtin_types.type_0063;
    width = ((flags & 0x340) || !(flags & 0xfff0)) ?
        (unsigned)(int)c1_primitive_widths[flags & 15] : flags & 15;
    return width < (unsigned)(int)c1_primitive_widths[3] ?
        c1_data.builtin_types.type_0063 : NULL;
}
extern "C" CompilerParserNode *__cdecl make_scalar_conversion(unsigned, CompilerType *, CompilerParserNode *);
extern "C" CompilerParserNode *__cdecl make_shaped_conversion(unsigned, CompilerType *, CompilerParserNode *);
CompilerParserNode *__cdecl convert_scalar_expression(unsigned operation, CompilerType *type,
    CompilerParserNode *value)
{
    if (!value) return NULL;
    CompilerType *original = value->type;
    if (type->shape || original->shape) {
        if (type == original) return value;
        return make_shaped_conversion(operation, type, value);
    }
    if (type != original) {
        if (((type->flags ^ original->flags) & 0xefcf) ||
            type->auxiliary != original->auxiliary)
            return make_scalar_conversion(operation, type, value);
        if (original->attributes & ~type->attributes & 0x1f00) {
            CompilerType *description = copy_type_description(type);
            description->attributes |= original->attributes & 0x1f00;
            type = intern_type(description);
            --type_description_depth;
        }
        value->type = type;
    }
    value->flags &= 0x7f;
    return value;
}
extern "C" void __cdecl check_constant_conversion(unsigned, CompilerParserNode *, CompilerType *);
extern "C" CompilerParserNode *__cdecl make_conversion_expression(CompilerType *, CompilerParserNode *);
extern "C" CompilerParserNode *__cdecl make_scalar_conversion(unsigned operation,
    CompilerType *type, CompilerParserNode *value)
{
    if (!value) return NULL;
    if (type->shape && (type->shape->attributes & 3))
        return finish_segment_dereference(convert_aggregate_return(type, value));
    if ((value->flags & 7) == 3) {
        check_constant_conversion(operation, value, type);
        return convert_binary_rhs((BYTE)operation, type, value);
    }
    if ((BYTE)operation != 100 && (BYTE)operation != 1)
        convert_expression_to_type((BYTE)operation, type, value);
    return make_conversion_expression(type, value);
}
extern "C" CompilerParserNode *__cdecl make_conversion_expression(CompilerType *type,
    CompilerParserNode *value)
{
    return make_typed_unary_operation(100, type, value);
}
extern "C" CompilerParserNode *__cdecl make_shaped_conversion(unsigned, CompilerType *, CompilerParserNode *)
{ __asm { int 3 } return NULL; }
CompilerType *__cdecl dereference_expression_type(CompilerType *type)
{
    unsigned attributes = type->shape->attributes & 3;
    CompilerType *result;
    if (attributes == 1) compiler_warning(214);
    c1_bss.dereferenced_shape_value = type->shape->value;
    result = get_function_return_type(type);
    result->attributes = (WORD)((result->attributes & ~3) | attributes);
    return intern_type(result);
}
CompilerType *__cdecl adjust_reference_type(CompilerType *type, unsigned mode)
{
    CompilerTypeShape *shape = make_type_shape(mode == 1 ? 11 : 0x403,
        type->shape->attributes, NULL);
    CompilerType *copy;
    CompilerArena *arena;
    shape->base = type->shape->base;
    shape = intern_type_shape(shape);
    if (c1_data.expression_arena_kind >= 6) {
#ifdef __linux__
        recon_unsupported("reference type arena is outside encountered storage");
#else
        __asm { int 3 }
#endif
    }
    arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    copy = (CompilerType *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    /* Preserve the full type storage, including its uninterpreted cache. */
    memcpy(copy, type, 16);
    copy->shape = shape;
    return copy;
}
CompilerIrSymbol *__cdecl find_referenced_symbol(CompilerParserNode *);
CompilerParserNode *__cdecl convert_unary_operand(CompilerParserNode *value, unsigned mode)
{
    CompilerType *type;
    CompilerTypeShape *shape;
    CompilerParserNode *result;
    CompilerIrSymbol *symbol;
    WORD shape_flags;
    if ((value->flags & 7) == 3) {
        compiler_warning(101, 49);
        return value;
    }
    type = value->type;
    if (type->shape && (type->shape->flags & 7) == 4)
        type = type->DecayFunction();
    shape = type->shape;
    if (shape && (shape->flags & 7) == 3 && (shape->flags & 8)) {
        value->type = adjust_reference_type(type, 2);
        shape = value->type->shape;
        if (shape && shape->base &&
            ((value->type->attributes & 0x70) == 0x60 ||
             (value->type->attributes & 0x70) == 0x40) &&
            (shape->base->attributes & 0x800) && !c1_bss.debug_module->Slot50())
            compiler_error(73, ".\\trees.c", 4468);
        return value;
    }
    shape_flags = value->type->shape ? value->type->shape->flags : 0;
    result = validate_assignment_target(value, mode);
    if (!result) {
        if ((shape_flags & 7) == 2) {
            if ((value->flags & 7) == 5) return value;
            result = value;
        } else if ((shape_flags & 7) == 3) {
            if (shape_flags & 0x400) compiler_warning(102, 49);
            return value;
        } else {
            if (mode & 2) return value;
            compiler_warning(102, 49);
            return NULL;
        }
    }
    if ((result->flags & 7) == 4) {
        symbol = result->symbol;
        symbol->qualifier_flags |= 8;
        if ((mode & 4) && (symbol->storage_class & 7) >= 5 &&
            (symbol->storage_class & 7) <= 6) {
            compiler_warning(103, 49);
            return result;
        }
    } else {
        if (result->kind == 53 && (mode & 4)) compiler_warning(104, 49);
        if (result->kind == 83 && (result->left->flags & 7) == 3) {
            CompilerParserNode *constant = result->right;
            unsigned previous = constant->low;
            constant->low += result->left->low;
            constant->high += result->left->high + (constant->low < previous);
            result = constant;
        } else {
            symbol = find_referenced_symbol(result);
            if (symbol) symbol->qualifier_flags |= 8;
            else find_expression_reference(result);
        }
    }
    result->type = type->AddressType(0);
    return result;
}
CompilerIrSymbol *__cdecl find_referenced_symbol(CompilerParserNode *value)
{
    for (;;) {
        switch (value->kind) {
        case 6: return value->symbol;
        case 54:
            value = value->type == value->right->type ? value->right : value->left;
            break;
        case 65: case 80: case 93:
            value = value->left;
            break;
        default: return NULL;
        }
    }
}
CompilerType *RECON_THISCALL CompilerType::AddressType(unsigned attributes)
{
    CompilerTypeShape address;
    address.flags = shape && (shape->flags & 7) == 4 ? 3 : 1;
    address.attributes = (WORD)attributes;
    if (address.flags == 3) address.attributes |= shape->attributes & 0x70;
    address.base = shape;
    address.value = 0;
    CompilerType description = *this;
    description.shape = intern_type_shape(&address);
    description.attributes &= 0xff8f;
    return intern_type(&description);
}
CompilerParserNode *__cdecl finish_segment_dereference(CompilerParserNode *) { __asm { int 3 } return NULL; }
InitializerExtendedReal *__cdecl allocate_extended_constant(InitializerExtendedReal, unsigned);
BYTE *__cdecl copy_extended_constant(const BYTE *value, unsigned arena_kind)
{
    return (BYTE *)allocate_extended_constant(*(const InitializerExtendedReal *)value, arena_kind);
}
InitializerExtendedReal *__cdecl allocate_extended_constant(InitializerExtendedReal value, unsigned arena_kind)
{
    if (arena_kind >= 6) {
#ifdef __linux__
        recon_unsupported("extended constant arena exceeds the encountered owners");
#else
        __asm { int 3 }
#endif
    }
    CompilerArena *arena = &c1_bss.arenas[arena_kind];
    InitializerExtendedReal *stored = (InitializerExtendedReal *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    *stored = value;
    return stored;
}

/* RVA1b94e */
CompilerIrSymbol *__cdecl resolve_expression_name(CompilerNameEntry *); /* RVA415c4 */
CompilerIrSymbol *__cdecl declare_implicit_function(CompilerNameEntry *, BYTE); /* RVA41483 */
CompilerParserNode *__cdecl make_symbol_expression(CompilerIrSymbol *); /* RVA1b97c */
CompilerParserNode * __cdecl make_identifier_expression(CompilerNameEntry *name)
{
    CompilerIrSymbol **found = c1_data.translation_unit->FindName(name);
    CompilerIrSymbol *symbol = found ? *found : NULL;
    if (!symbol) {
        symbol = resolve_expression_name(name);
        if (!symbol) symbol = declare_implicit_function(name, 0);
    }
    return make_symbol_expression(symbol);
}
CompilerIrSymbol *__cdecl resolve_expression_name(CompilerNameEntry *) { __asm { int 3 } return NULL; }
CompilerIrSymbol *__cdecl declare_implicit_function(CompilerNameEntry *name, BYTE local)
{
    unsigned arena = local ? 2 : 0;
    CompilerIrSymbol *symbol = lookup_cached_symbol_record(10, name, arena);
    if (!symbol) {
        BYTE *memory = c1_bss.arenas[arena].AllocateBytes(
            (c1_data.record_sizes.known.static_symbol_bytes + 3) & ~3U);
        symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(10, name, arena) : NULL;
    }
    /* The original clears the complete intrinsic word after selecting its
     * low storage-class bits, retaining only class1/global or class4/local. */
    symbol->intrinsic_flags = local ? 4 : 1;
    if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
    symbol->record_id = (symbol->record_id & 0xff000000U) |
        (c1_data.next_symbol_record_id++ & 0xffffffU);
    symbol->reference_flags &= ~7U;
    symbol->value_type = c1_data.builtin_types.type_0063;
    c1_bss.browser_name_deferred = local;
    if (local) c1_data.translation_unit->current->Insert(symbol);
    else c1_data.translation_unit->root->Insert(symbol);
    return symbol;
}
CompilerParserNode *__cdecl make_symbol_expression(CompilerIrSymbol *symbol)
{
    if (c1_bss.browser_filename) {
        if (!c1_bss.browser_name_deferred || browser_parameter_references)
            c1_data.backend->Slot1c(symbol);
    }
    return make_symbol_reference(symbol, 0);
}
/* RVA1bdf6 */
CompilerParserNode *__cdecl convert_typed_expression(unsigned, CompilerType *, CompilerParserNode *);
CompilerParserNode *__cdecl make_right_typed_expression(BYTE operation,
    CompilerIrSymbol *declaration, CompilerParserNode *value)
{
    if (!declaration || !value) return NULL;
    return convert_typed_expression(operation, declaration->value_type, value);
}
unsigned __cdecl classify_type_conversion(CompilerType *, CompilerParserNode *, unsigned);
extern "C" unsigned __cdecl pointer_storage_size(WORD);
CompilerParserNode *__cdecl convert_typed_expression(unsigned operation, CompilerType *type,
    CompilerParserNode *value)
{
    CompilerTypeShape *target_shape = type->shape;
    CompilerType *source = value->type;
    WORD target_shape_flags = target_shape ? target_shape->flags : 0;
    WORD target_attributes = target_shape ? target_shape->attributes : 0;
    WORD source_shape_flags, source_attributes;
    WORD target_flags, source_flags;
    CompilerTypeShape *source_shape;
    unsigned relation;
    int source_width, target_width;
    if ((target_shape_flags & 7) == 4) { compiler_warning(66); return NULL; }
    if ((target_shape_flags & 7) == 2) { compiler_warning(67); return NULL; }
    relation = classify_type_conversion(type, value, 0);
    if (relation < 0x400) return convert_scalar_expression(operation, type, value);
    if (relation & 0x1000) { compiler_warning(585, type); return NULL; }
    source_shape = source->shape;
    source_shape_flags = source_shape ? source_shape->flags : 0;
    source_attributes = source_shape ? source_shape->attributes : 0;
    /* Array conversion uses the corresponding pointer category for these
     * compatibility/width diagnostics; the original type itself is retained. */
    if ((source_shape_flags & 7) == 2)
        source_shape_flags = (source_shape_flags & 0xfff9) | 1;
    if (type == c1_data.builtin_types.type_0080) goto convert;
    target_flags = type->flags;
    source_flags = source->flags;
    if ((source_flags & 0x80) && !source_shape) {
        compiler_warning(69);
        goto convert;
    }
    if (!source_shape_flags) {
        if (!target_shape_flags) {
            if ((target_flags | source_flags) & 0x400) {
                compiler_warning(440, (BYTE)operation, source, type);
                return NULL;
            }
            goto convert;
        }
        if (!(source_flags & 0x240)) {
            compiler_warning(440, (BYTE)operation, source, type);
            return NULL;
        }
        target_width = pointer_storage_size(target_attributes);
        source_width = (source_flags & 0x340) || !(source_flags & ~15U) ?
            c1_primitive_widths[source_flags & 15] : source_flags & 15;
        if (source_width > target_width)
            report_language_diagnostic(4, 305, (BYTE)operation, source, type);
        else if (source_width < target_width)
            report_language_diagnostic(4, 306, (BYTE)operation, source, type);
        goto convert;
    }
    if (!target_shape_flags) {
        if (!(target_flags & 0x40))
            compiler_warning(440, (BYTE)operation, source, type);
        target_width = (target_flags & 0x340) || !(target_flags & ~15U) ?
            c1_primitive_widths[target_flags & 15] : target_flags & 15;
        if (pointer_storage_size(source_attributes) > target_width)
            report_language_diagnostic(4, 305, (BYTE)operation, source, type);
        goto convert;
    }
    if ((source_shape_flags & 7) == 3) {
        if ((target_shape_flags & 7) != 3) {
            if ((target_shape_flags & 7) != 1) goto convert;
            report_language_diagnostic(c1_data.options.language_extensions ? 4 : 1,
                54, (BYTE)operation, source, type);
        }
        source_width = pointer_storage_size(source_attributes);
        target_width = pointer_storage_size(target_attributes);
        if (source_width > target_width)
            report_language_diagnostic(4, 305, (BYTE)operation, source, type);
    } else if ((source_shape_flags & 7) == 1) {
        if ((target_shape_flags & 7) == 3) {
            report_language_diagnostic(c1_data.options.language_extensions ? 4 : 1,
                55, (BYTE)operation, source, type);
            source_width = pointer_storage_size(source_attributes);
            target_width = pointer_storage_size(target_attributes);
            if (source_width > target_width)
                report_language_diagnostic(4, 305, (BYTE)operation, source, type);
        } else if ((target_shape_flags & 7) == 1) {
            source_width = pointer_storage_size(source_attributes);
            target_width = pointer_storage_size(target_attributes);
            if (source_width > target_width)
                report_language_diagnostic(4, 305, (BYTE)operation, source, type);
            else if ((target_attributes & 3) == 3 && (value->flags & 7) == 5)
                report_language_diagnostic(1, 120);
        }
    }
convert:
    return make_scalar_conversion(operation, type, value);
}
unsigned __cdecl classify_type_relation(CompilerType *, CompilerType *, unsigned);
unsigned __cdecl classify_type_conversion(CompilerType *destination,
    CompilerParserNode *value, unsigned mode)
{
    CompilerType *source;
    if (!value) return 0x2000;
    if (!destination) return 0x400;
    source = value->type;
    if (!source) return 0x2000;
    if ((source->flags & 0x400) && !source->shape) return 0x2000;
    if ((destination->flags & 0x400) && !destination->shape) return 0x2000;
    if ((value->flags & 0x80) && destination->shape) return 1;
    if (destination->shape && (destination->shape->flags & 7) == 1 &&
        source->shape && (source->shape->flags & 7) == 2)
        decay_initializer_array(value);
    return classify_type_relation(destination, source, mode);
}
unsigned __cdecl classify_type_shapes(CompilerType *, CompilerType *, unsigned);
unsigned __cdecl classify_type_relation(CompilerType *destination, CompilerType *source,
    unsigned mode)
{
    unsigned result;
    WORD destination_flags, source_flags, difference;
    int destination_numeric, source_width, destination_width;
    if (destination == source) return 1;
    result = classify_type_shapes(destination, source, mode);
    if (result) return result;
    destination_flags = destination->flags;
    source_flags = source->flags;
    destination_numeric = (destination_flags & 0x140) != 0;
    if ((!destination_numeric && !(destination_flags & 0x80) &&
         !(destination_flags & 0x800) && !destination->shape) ||
        (!(source_flags & 0x940) && !(source_flags & 0x80) && !source->shape)) {
        if (destination_flags & 0x200) {
            if (source_flags & 0x200)
                return destination->auxiliary == source->auxiliary ? 1 : 0x2000;
        } else if ((source_flags & 0x200) && destination_numeric) {
            return (destination_flags & 0x4f) == 0x43 ? 0x10 : 0x20;
        }
        return !((destination_flags ^ source_flags) & 0xefcf) &&
            destination->auxiliary == source->auxiliary ? 1 : 0x2000;
    }
    if (source_flags == destination_flags) return 1;
    if (source_flags & 0x80) return 0x2000;
    if ((source_flags & 0x40) && (destination_flags & 0x40)) {
        difference = source_flags ^ destination_flags;
        if (!(difference & 0xefcf)) return 1;
        if ((destination_flags & 0x4f) != 0x43 || (destination_flags & 0x4000))
            return 0x20;
        source_width = (source_flags & 0x340) || !(source_flags & ~15U) ?
            c1_primitive_widths[source_flags & 15] : source_flags & 15;
        destination_width = (destination_flags & 0x340) || !(destination_flags & ~15U) ?
            c1_primitive_widths[destination_flags & 15] : destination_flags & 15;
        if (source_width == destination_width && !(difference & 0xc000)) return 0x10;
        return source_width < 4 ? 0x10 : 0x20;
    }
    if (!(source_flags & 0x100) || !(destination_flags & 0x100)) return 0x20;
    source_width = (source_flags & 0x340) || !(source_flags & ~15U) ?
        c1_primitive_widths[source_flags & 15] : source_flags & 15;
    destination_width = (destination_flags & 0x340) || !(destination_flags & ~15U) ?
        c1_primitive_widths[destination_flags & 15] : destination_flags & 15;
    if (source_width == destination_width &&
        ((source_flags & 0x108) == 0x108) == ((destination_flags & 0x108) == 0x108))
        return 1;
    return source_width < destination_width && destination_width <= c1_primitive_widths[7] ?
        0x10 : 0x20;
}
unsigned __cdecl classify_shape_pair(unsigned, CompilerTypeShape *, CompilerTypeShape *);
unsigned __cdecl classify_qualifier_conversion(unsigned, unsigned, unsigned, unsigned);
unsigned __cdecl classify_pointer_attributes(unsigned, unsigned, unsigned, CompilerTypeShape *);
unsigned __cdecl classify_type_shapes(CompilerType *destination, CompilerType *source,
    unsigned mode)
{
    CompilerTypeShape *left = destination->shape, *right = source->shape;
    WORD left_flags, right_flags;
    unsigned result = 1, source_attributes;
    unsigned left_kind, right_kind;
    if (!left && !right) return 0;
    if (!left) return 0x2000;
    left_flags = left->flags;
    if ((destination->flags & 0x80) && (left_flags & 7) != 4)
        result = classify_shape_pair(result, left, right);
    if (!right) return 0x2000;
    right_flags = right->flags;
    if (result != 0x100 && left->base != right->base) return 0x2000;
    if (left->base) {
        if ((destination->attributes ^ source->attributes) & 0x1f00) return 0x2000;
    } else {
        source_attributes = source->attributes;
        if (result == 0x100 && right->base)
            source_attributes = right->base->attributes;
        result = classify_qualifier_conversion(mode, result,
            destination->attributes, source_attributes);
        if (result & 0x2000) {
            if ((parser_flags & 1) && !(left_flags & 1) &&
                ((result & 0x1fff) == 5 || (result & 0x1fff) == 6)) result = 1;
            else return 0x2000;
        }
    }
    result = classify_pointer_attributes(result, left->attributes, right->attributes, right);
    if (result & 0x2000) return 0x2000;
    left_kind = left_flags & 7;
    right_kind = right_flags & 7;
    if (left_kind != right_kind) {
        if ((left_kind == 1 && right_kind == 2) || (left_kind == 2 && right_kind == 1))
            result = result == 1 ? 2 : result | 2;
        else return 0x2000;
    } else if (left_kind == 2 && left->value && left->value != right->value)
        return 0x2000;
    if ((left_kind == 3 || left_kind == 4) && (right_kind == 3 || right_kind == 4)) {
        if (left_kind == 3) left = left->base;
        if (right_kind == 3) right = right->base;
        /* The original compares the function-parameter identity at payload+8;
         * it does not read that payload here. Equal owners have equal+8. */
        if (left->value != right->value || left->attributes != right->attributes)
            return 0x2000;
    }
    if (!((destination->flags ^ source->flags) & 0xefcf) &&
        destination->auxiliary == source->auxiliary) return result;
    if (result & 0x100) return result;
    return 0x2000;
}
unsigned __cdecl classify_shape_pair(unsigned, CompilerTypeShape *, CompilerTypeShape *)
{ __asm { int 3 } return 0; }
unsigned __cdecl classify_qualifier_conversion(unsigned, unsigned result,
    unsigned destination, unsigned source)
{
    if ((WORD)destination == (WORD)source) return result;
    if (destination & 0x100) {
        if (!(source & 0x100)) result |= 8;
    } else if (source & 0x100) return 0x2005;
    if (destination & 0x200) {
        if (!(source & 0x200)) result |= 8;
    } else if (source & 0x200) return 0x2006;
    return result;
}
unsigned __cdecl classify_pointer_attributes(unsigned result, unsigned destination,
    unsigned source, CompilerTypeShape *)
{
    if (!((destination ^ source) & 3)) return result;
    if ((destination & 1) || (source & 3) == 1) return 0x2000;
    return result | 0x10;
}
/* RVA1ce98 */
CompilerParserNode *__cdecl make_type_size_expression(CompilerType *type)
{
    unsigned size = global_type_size(type);
    if (!size) {
        if (function_output_mode) return NULL;
        if ((type->flags & 0x400) && !type->shape)
            compiler_warning(27, type->symbol_owner);
        else report_language_diagnostic(1, 34);
    }
    CompilerParserNode *value = (CompilerParserNode *)make_integer_constant((int)size);
    value->type = c1_data.builtin_types.type_4063_first;
    value->kind = 101;
    return value;
}
/* RVA1d38e */
CompilerParserNode *__cdecl strip_member_base(CompilerParserNode *);
unsigned __cdecl member_base_qualifiers(CompilerType *);
CompilerType *__cdecl merge_member_storage(CompilerType *, WORD);
CompilerParserNode * __cdecl make_member_expression(BYTE operation, CompilerParserNode *base,
    CompilerNameEntry *name)
{
    CompilerType *type;
    CompilerIrSymbol *owner, *member = NULL;
    CompilerRecordMembers *members;
    CompilerSymbolBinding *binding;
    CompilerParserNode *result;
    CompilerArena *arena;
    unsigned qualifiers, retain_temporary = 0;
    int offset;
    if (!base) return NULL;
    type = base->type;
    switch (type->flags & 0x2fcf) {
    case 0x402: case 0x404: case 0x408:
        break;
    default:
        compiler_warning(operation == 83 ? 223 : 224, name);
        return NULL;
    }
    owner = type->symbol_owner;
    members = (CompilerRecordMembers *)owner->kind4_context;
    if (!members) { compiler_warning(37, name, owner); return NULL; }
    for (binding = members->first; binding; binding = binding->next) {
        member = (CompilerIrSymbol *)((BYTE *)binding - 8);
        if (binding->name == name) break;
    }
    if (!binding) { compiler_warning(39, 73, name, owner); return NULL; }
    offset = (int)member->symbol_value;
    if (operation == 83) {
        if (!type->shape) { compiler_warning(232, 73, name, type); return NULL; }
        if (type->shape->base || !(type->shape->flags & 3)) {
            compiler_warning(223, member->binding.name);
            return NULL;
        }
    } else if (type->shape) {
        if ((type->shape->flags & 3) && !type->shape->base)
            compiler_warning(231, 73, name, type);
        else compiler_warning(224, member->binding.name);
    } else {
        retain_temporary = base->flags & 0x20;
        base = strip_member_base(base);
        if (base->kind == 52 && base->type->shape && !base->type->shape->base)
            retain_temporary = 1;
    }
    qualifiers = member_base_qualifiers(type);
    if (base->kind != 6 && base->kind != 61 && base->kind != 65 && base->kind != 101 && base->kind != 83) {
#ifdef __linux__
        recon_unsupported("member address classification is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    if (c1_expression_classes.indexed[base->kind] == 131) {
        CompilerParserNode *constant = base->right;
        unsigned prior = constant->low;
        constant->low += (unsigned)offset;
        constant->high += (offset < 0 ? ~0U : 0) + (constant->low < prior);
        result = base;
    } else {
        if (c1_data.expression_arena_kind >= 6) {
#ifdef __linux__
            recon_unsupported("member expression arena is outside modeled owners");
#else
            __asm { int 3 }
#endif
        }
        arena = &c1_bss.arenas[c1_data.expression_arena_kind];
        result = (CompilerParserNode *)arena->cursor;
        arena->cursor += 24;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        result->flags = 2;
        result->kind = operation;
        result->left = base;
        result->right = (CompilerParserNode *)make_integer_constant(offset);
    }
    WORD storage_attributes = operation == 83 ? type->shape->attributes : type->attributes;
    result->type = merge_member_storage(member->value_type, storage_attributes & 3);
    if (qualifiers) result->type = result->type->WithAttributes(qualifiers);
    if (member->qualifier_flags & 4) return result;
    if (member->kind == 2) {
        /* layout_bitfield_member writes the offset/width bytes at+24/+25. */
        result = make_ir_operation(53, result,
            make_ir_operation(54, (CompilerParserNode *)make_integer_constant(member->bit_offset),
                (CompilerParserNode *)make_integer_constant(member->bit_width)));
        if (qualifiers) {
            CompilerType *description = copy_type_description(result->type);
            description->attributes |= (WORD)qualifiers;
            result->type = intern_type(description);
            --type_description_depth;
        }
    }
    result = take_expression_address(result);
    if (retain_temporary) result->flags |= 0x20;
    return result;
}
CompilerParserNode *__cdecl strip_member_base(CompilerParserNode *value)
{
    while (value->kind == 100 || value->kind == 118)
        value = value->left;
    if (value->kind == 55) {
        value->right = strip_member_base(value->right);
        value->type = value->right->type;
        return value;
    }
    return convert_unary_operand(value, 2);
}
CompilerTypeShape *__cdecl type_qualifier_shape(CompilerType *);
unsigned __cdecl member_base_qualifiers(CompilerType *type)
{
    CompilerTypeShape *shape = type_qualifier_shape(type);
    if (shape && shape->base) return shape->base->attributes & 0x1f00;
    return type->attributes & 0x1f00;
}
CompilerType *__cdecl merge_member_storage(CompilerType *type, WORD attributes)
{
    if ((attributes & ~type->attributes) & 3) {
        CompilerType *description = copy_type_description(type);
        description->attributes = (WORD)((description->attributes & ~3U) | (attributes & 3));
        type = intern_type(description);
        --type_description_depth;
    }
    return type;
}
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
#define CDECL_CALLBACK_ASSIGNMENT(d,s) ((d)->flags == 0x63 && (s)->flags == 0x63 && \
    !(d)->attributes && (s)->attributes == 0x10 && (d)->shape && (s)->shape && \
    (d)->shape->flags == 3 && (s)->shape->flags == 11 && \
    (d)->shape->attributes == 0x10 && (s)->shape->attributes == 0x10 && \
    (d)->shape->base && (s)->shape->base && \
    ((d)->shape->base->flags & ~0x400U) == 4 && \
    ((s)->shape->base->flags & ~0x400U) == 4 && \
    (d)->shape->base->attributes == 0x10 && (s)->shape->base->attributes == 0x10 && \
    (d)->shape->base->parameters == (s)->shape->base->parameters && \
    !(d)->shape->base->base && !(s)->shape->base->base)
#define WIN32_CALLBACK_ARGUMENT(d,s) ((d)->flags == 0x63 && (s)->flags == 0x63 && \
    !(d)->attributes && ((s)->attributes == 0x40 || (s)->attributes == 0x60) && (d)->shape && (s)->shape && \
    (d)->shape->flags == 3 && (s)->shape->flags == 11 && \
    (d)->shape->attributes == (s)->attributes && (s)->shape->attributes == (s)->attributes && \
    (d)->shape->base && (s)->shape->base && \
    ((d)->shape->base->flags & ~0x400U) == 4 && \
    ((s)->shape->base->flags & ~0x400U) == 4 && \
    (d)->shape->base->attributes == (s)->attributes && (s)->shape->base->attributes == (s)->attributes && \
    (d)->shape->base->parameters == (s)->shape->base->parameters && \
    !(d)->shape->base->base && !(s)->shape->base->base)
#define VOLATILE_SHORT_ARGUMENT(d,s) ((d)->flags == 0x62 && (s)->flags == 0x62 && \
    (d)->attributes == 0x200 && (s)->attributes == 0x200 && (d)->shape && (s)->shape && \
    (d)->shape->flags == 1 && ((s)->shape->flags == 1 || (s)->shape->flags == 2) && \
    !(d)->shape->attributes && !(s)->shape->attributes && !(d)->shape->base && !(s)->shape->base)
#define CHAR_POINTER_ASSIGNMENT(d,s) ((d)->flags == 0x61 && (s)->flags == 0x61 && \
    (d)->attributes == 0x100 && !(s)->attributes && (d)->shape && (s)->shape && \
    (d)->shape->flags == 1 && (s)->shape->flags == 2 && \
    !(d)->shape->attributes && !(s)->shape->attributes && \
    !(d)->shape->base && !(s)->shape->base)
/* The ordinary array argument preserves its element type and adds const
 * qualification through the existing pointer/array compatibility walk. */
#define CONST_INT_ARRAY_ARGUMENT(d,s) ((d)->flags == 0x63 && (s)->flags == 0x63 && \
    (d)->attributes == 0x100 && !(s)->attributes && (d)->shape && (s)->shape && \
    (d)->shape->flags == 1 && (s)->shape->flags == 2 && \
    !(d)->shape->attributes && !(s)->shape->attributes && \
    !(d)->shape->base && !(s)->shape->base)
/* The encountered matrix argument decays only its outer array dimension;
 * the canonical inner row shape, bound and const-qualified element remain. */
#define CONST_INT_MATRIX_ARGUMENT(d,s) ((d)->flags == 0x63 && (s)->flags == 0x63 && \
    (d)->attributes == 0x100 && (s)->attributes == 0x100 && \
    !(d)->auxiliary && !(s)->auxiliary && (d)->shape && (s)->shape && \
    (d)->shape->flags == 1 && (s)->shape->flags == 2 && (s)->shape->value && \
    !(d)->shape->attributes && !(s)->shape->attributes && \
    (d)->shape->base && (d)->shape->base == (s)->shape->base && \
    (d)->shape->base->flags == 2 && !(d)->shape->base->attributes && \
    !(d)->shape->base->base && (d)->shape->base->value)
#define RECORD_POINTER_ASSIGNMENT(d,s) ((d)->flags == 0x402 && (s)->flags == 0x402 && \
    (d)->attributes == 0x100 && !(s)->attributes && (d)->shape && (s)->shape && \
    (d)->shape->flags == 1 && (s)->shape->flags == 1 && !(d)->shape->attributes && \
    (s)->shape->attributes == 0x100 && !(d)->shape->base && !(s)->shape->base && \
    (d)->symbol_owner && (d)->symbol_owner == (s)->symbol_owner && \
    (d)->symbol_owner->kind == 4 && ((d)->symbol_owner->flags & 1))
#define VOID_FROM_RECORD_POINTER(d,s) ((d)->flags == 0x80 && (s)->flags == 0x402 && \
    !(d)->attributes && !(s)->attributes && (d)->shape && (d)->shape == (s)->shape && \
    (d)->shape->flags == 1 && !(d)->shape->attributes && !(d)->shape->base && \
    (s)->symbol_owner && (s)->symbol_owner->kind == 4 && ((s)->symbol_owner->flags & 1))
#define RECORD_VALUE_ASSIGNMENT(d,s) ((d)->flags == 0x402 && (s)->flags == 0x402 && \
    !(d)->attributes && !((s)->attributes & ~0x10U) && !(d)->shape && !(s)->shape && \
    (d)->symbol_owner && (d)->symbol_owner == (s)->symbol_owner && \
    (d)->symbol_owner->kind == 4 && ((d)->symbol_owner->flags & 1))
/* This known incompatible alternative takes the real C2115 path before any
 * structured conversion. Only the complete record's established owner is read. */
#define RECORD_TO_INT_DIAGNOSTIC(d,s) (!(d)->shape && (d)->flags == 0x63 && \
    !(d)->attributes && !(d)->auxiliary && !(s)->shape && (s)->flags == 0x402 && \
    !((s)->attributes & ~0x110U) && (s)->symbol_owner && \
    (s)->symbol_owner->kind == 4 && ((s)->symbol_owner->flags & 1))
int __cdecl convert_expression_to_type(unsigned operation, CompilerType *type,
    CompilerParserNode *value)
{
    /* The genuine NULL-constant fast path consumes no operand type data. */
    if ((value->flags & 0x80) && type->shape && (type->shape->flags & 3))
        return 1;
    /* Scalar integers use the authentic diagnostics below. Identical shaped
     * types take the genuine compatibility fast path and skip scalar checks. */
    if (!(!type->shape && !value->type->shape &&
          (type->flags == 0x106 || type->flags == 0x107) &&
          (value->type->flags == 0x106 || value->type->flags == 0x107) &&
          !(type->attributes & ~0x100U) && !(value->type->attributes & ~0x110U)) &&
        !(type == value->type && type->shape) &&
        !((operation == 125 || operation == 112) && type == value->type && !type->shape &&
          type->flags == 0x402 && !type->attributes && type->symbol_owner &&
          type->symbol_owner->kind == 4 && (type->symbol_owner->flags & 1)) &&
        !(operation == 116 && RECORD_TO_INT_DIAGNOSTIC(type, value->type)) &&
        !((operation == 112 || operation == 116) && CDECL_CALLBACK_ASSIGNMENT(type, value->type)) &&
        !(operation == 112 && VOLATILE_SHORT_ARGUMENT(type, value->type)) &&
        !(operation == 112 && CONST_INT_ARRAY_ARGUMENT(type, value->type)) &&
        !(operation == 112 && CONST_INT_MATRIX_ARGUMENT(type, value->type)) &&
        !(operation == 112 && WIN32_CALLBACK_ARGUMENT(type, value->type)) &&
        !((operation == 52 || operation == 112 || operation == 116) && CHAR_POINTER_ASSIGNMENT(type, value->type)) &&
        !(operation == 52 && RECORD_POINTER_ASSIGNMENT(type, value->type)) &&
        !((operation == 52 || operation == 116) && RECORD_VALUE_ASSIGNMENT(type, value->type)) &&
        !(operation == 116 && VOID_FROM_RECORD_POINTER(type, value->type)) &&
        !(operation == 116 && type->flags == 0x402 && value->type->flags == 0x402 &&
          !type->attributes && !value->type->attributes &&
          type->shape && type->shape == value->type->shape &&
          type->shape->flags == 1 && !type->shape->attributes && !type->shape->base &&
          type->symbol_owner && type->symbol_owner == value->type->symbol_owner &&
          type->symbol_owner->kind == 4) &&
        !(operation == 112 && type->flags == 0x402 && value->type->flags == 0x402 &&
          type->attributes == 0x100 && !(value->type->attributes & ~0x100U) &&
          type->symbol_owner == value->type->symbol_owner && type->symbol_owner &&
          type->symbol_owner->kind == 4 && type->shape && value->type->shape &&
          type->shape->flags == 1 && (value->type->shape->flags == 1 || value->type->shape->flags == 2) &&
          !type->shape->base && !value->type->shape->base &&
          !type->shape->attributes && !value->type->shape->attributes) &&
        (type->shape || value->type->shape ||
         !(type->flags & 0x40) || !(value->type->flags & 0x40))) {
#ifdef __linux__
        recon_unsupported("noninteger expression conversion is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
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
    if (destination == source) return 1;
#ifdef RECON_NATIVE_TEST
    if (!(!destination->shape && !source->shape &&
          (destination->flags == 0x106 || destination->flags == 0x107) &&
          (source->flags == 0x106 || source->flags == 0x107) &&
          !(destination->attributes & ~0x100U) && !(source->attributes & ~0x110U)) &&
        !(operation == 116 && RECORD_TO_INT_DIAGNOSTIC(destination, source)) &&
        !((operation == 112 || operation == 116) && CDECL_CALLBACK_ASSIGNMENT(destination, source)) &&
        !(operation == 112 && VOLATILE_SHORT_ARGUMENT(destination, source)) &&
        !(operation == 112 && CONST_INT_ARRAY_ARGUMENT(destination, source)) &&
        !(operation == 112 && CONST_INT_MATRIX_ARGUMENT(destination, source)) &&
        !(operation == 112 && WIN32_CALLBACK_ARGUMENT(destination, source)) &&
        !((operation == 52 || operation == 112 || operation == 116) && CHAR_POINTER_ASSIGNMENT(destination, source)) &&
        !(operation == 52 && RECORD_POINTER_ASSIGNMENT(destination, source)) &&
        !((operation == 52 || operation == 116) && RECORD_VALUE_ASSIGNMENT(destination, source)) &&
        !(operation == 116 && VOID_FROM_RECORD_POINTER(destination, source)) &&
        !(operation == 116 && destination->flags == 0x402 && source->flags == 0x402 &&
          !destination->attributes && !source->attributes && destination->shape &&
          destination->shape == source->shape && destination->shape->flags == 1 &&
          !destination->shape->attributes && !destination->shape->base &&
          destination->symbol_owner && destination->symbol_owner == source->symbol_owner &&
          destination->symbol_owner->kind == 4) &&
        !(operation == 112 && destination->flags == 0x402 && source->flags == 0x402 &&
          destination->attributes == 0x100 && !(source->attributes & ~0x100U) &&
          destination->symbol_owner == source->symbol_owner && destination->symbol_owner &&
          destination->symbol_owner->kind == 4 && destination->shape && source->shape &&
          destination->shape->flags == 1 && (source->shape->flags == 1 || source->shape->flags == 2) &&
          !destination->shape->base && !source->shape->base &&
          !destination->shape->attributes && !source->shape->attributes) &&
        (destination->shape || source->shape ||
        !(destination->flags & 0x40) || !(source->flags & 0x40))
       )
        recon_unsupported("structured assignment compatibility is not yet modeled");
#endif
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
int __cdecl check_type_qualifiers(unsigned operation, unsigned destination,
    unsigned source, int first, int character_type)
{
    operation &= 255;
    if (operation != 112 && operation != 116 && operation != 52) {
#ifdef __linux__
        recon_unsupported("conversion qualifier operation column is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    switch (c1_expression_classes.indexed[operation]) {
    case 54: case 110: case 124:
        return 1;
    case 52: case 112: case 116: case 125:
        if (((source ^ destination) & 0x1f00) && (source & 0x1f00)) {
            int failed = 0;
            if (!(destination & 0x200) && (source & 0x200)) {
                report_language_diagnostic(1, 90, operation, 48);
                failed = 1;
            }
            if (!(destination & 0x100) && (source & 0x100)) {
                report_language_diagnostic(1, 90, operation, 47);
                failed = 1;
            }
            if (failed) return 0;
        }
        return first ? 1 : 2;
    default:
        return 2;
    }
}
int __cdecl compatible_function_conventions(unsigned left, unsigned right)
{
    unsigned difference;
    if (function_output_mode) return 1;
    difference = left ^ right;
    if (difference & 3) return 0;
    return (difference & 0x70) == 0;
}
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
    if (!value || !value->type || !destination || value->type->shape || destination->shape ||
        value->type->flags != 0x63 || destination->flags != 0x63 ||
        (value->type->attributes & ~0x110U) || (destination->attributes & ~0x100U))
        recon_unsupported("conversion diagnostics outside the same-int domain are not yet modeled");
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
int __cdecl integer_value_loses_bits(int width, int unsigned_destination,
    int unsigned_source, __int64 value)
{
    /* The original compiler's shift helper consumes CL. Keep the bound
     * calculation defined for its complete incoming WORD width domain. */
    unsigned shift = (BYTE)((WORD)width * 8 - 1);
    unsigned __int64 minimum = shift >= 64 ? 0 : (unsigned __int64)-1 << shift;
    unsigned __int64 maximum = ~minimum;
    unsigned __int64 bits = (unsigned __int64)value;
    if ((WORD)unsigned_destination || (WORD)unsigned_source) {
        unsigned __int64 unsigned_maximum = (maximum << 1) | maximum;
        if ((WORD)unsigned_destination && !(WORD)unsigned_source && value < 0)
            bits = 0 - bits;
        return bits > unsigned_maximum;
    }
    return value > (__int64)maximum || value < (__int64)minimum;
}
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
/* Encountered semantic cells of shared BYTE column63c78; gaps remain unmodeled. */
CompilerExpressionClasses c1_expression_classes = { { {0}, 6, 7, {0}, 1, {0}, 50, {0}, 52, {0}, 110, 60, 61, 124, {0}, 65, {0}, 128, 124, {0}, 70, {0}, 74, {0}, 79, {0}, 81, {0}, 131, {0}, 101, {0}, 112, {0}, 116, {0}, 118, {0}, 126, 127 } };
CompilerParserNode *__cdecl strip_value_wrappers(CompilerParserNode *value)
{
    for (;;) {
        if (value->kind == 100) {
            value = value->left;
            continue;
        }
        if (value->kind != 93) return value;
#ifdef __linux__
        recon_unsupported("binary wrapper classification column is not yet modeled");
#else
        __asm { int 3 }
#endif
        if (!value->right) {
            value = value->left;
            continue;
        }
        CompilerParserNode *left = value->left;
        if ((left->type->shape->attributes & 3) != 3) return value;
        CompilerParserNode *right = value->right->left;
        if (c1_expression_classes.indexed[value->right->kind] == 61)
            right = right->left;
        right = right->left;
        if (right->type->shape) return value;
        value = left;
    }
}
CompilerParserNode *__cdecl convert_aggregate_return(CompilerType *, CompilerParserNode *)
{ __asm { int 3 } return NULL; }
/* RVA1df6d */
extern "C" CompilerIrSymbol *__cdecl begin_initializer_symbol(CompilerIrSymbol *);
CompilerIrSymbol * __cdecl prepare_initializer_symbol(CompilerIrSymbol *symbol)
{
    if (function_output_mode || !symbol || (symbol->scope_bits & 1)) return symbol;
    unsigned storage = symbol->storage_class & 7;
    if (storage == 7) {
        compiler_warning(71, symbol);
        return NULL;
    }
    if (storage == 1) {
        if (parameter_nesting) compiler_warning(205, symbol);
        symbol->storage_class = (symbol->storage_class & ~7) | 2;
    }
    if (symbol->linkage_flags & 8) c1_data.function_record_flags |= 0x800;
    return begin_initializer_symbol(symbol);
}
/* RVA1e277 */
extern "C" CompilerIrSymbol *__cdecl declare_undeclared_function(CompilerNameEntry *);
CompilerParserNode * __cdecl make_based_expression(CompilerNameEntry *name)
{
    CompilerIrSymbol **found = c1_data.translation_unit->FindName(name);
    CompilerIrSymbol *symbol = found ? *found : NULL;
    if (!symbol) {
        report_language_diagnostic(3, 13, name);
        symbol = declare_undeclared_function(name);
        symbol->qualifier_flags |= 8;
        if (c1_bss.browser_filename) {
            if (!c1_bss.browser_name_deferred || browser_parameter_references)
                c1_data.backend->Slot20(symbol);
        }
    }
    if (c1_bss.browser_filename) {
        if (!c1_bss.browser_name_deferred || browser_parameter_references)
            c1_data.backend->Slot1c(symbol);
    }
    CompilerTypeShape *shape = symbol->value_type->shape;
    if (!shape || ((shape->flags & 7) != 4 && (shape->flags & 7) != 3)) {
        compiler_warning(63, symbol);
        return NULL;
    }
    CompilerParserNode *value = make_label_expression(symbol, 0);
    if (symbol->qualifier_flags & 0x20)
        report_language_diagnostic(4, 217);
    if ((shape->flags & 7) == 3) value = take_expression_address(value);
    else value->type = wrap_expression_type(value->type,
        (shape->flags & ~4U) | 3, shape->storage & 0x73, NULL);
    return normalize_based_expression(value);
}
extern "C" CompilerIrSymbol *__cdecl declare_undeclared_function(CompilerNameEntry *)
{ __asm { int 3 } return NULL; }
/* RVA1e3a8 */
/* Call analysis owns one36-byte linked context. These source declarations
 * describe later fields; the current prefix stops at PushCallContext before
 * any new context read or mutation. */
struct CompilerCallContext {
    CompilerCallContext *outer;
    CompilerType **next_parameter;
    CompilerIrSymbol *callee;
    const char *diagnostic_name;
    CompilerType *function_type;
    unsigned remaining_parameters;
    unsigned supplied_arguments;
    unsigned reference_depth;
    BYTE flags;
    BYTE unmodeled_21[3];
};
typedef char CompilerCallContext_size[sizeof(CompilerCallContext) == 36 ? 1 : -1];
#define call_context c1_data.call_context /* RVA5e3e4, original NULL; saved/restored. */
#define call_reference_depth c1_bss.call_reference_depth /* RVA53d6c; actual classifier writer sets1. */
static CompilerNameEntry *special_call_symbols[2]; /* RVA5e458/5e45c, source-only. */
#define exception_code_enabled c1_data.finally_depth /* RVA5f3a0, shared authoritative counter. */
#define abnormal_termination_enabled c1_data.exception_filter_depth /* RVA5f3a4. */
extern "C" {
CompilerType *__cdecl decay_call_function_type(CompilerType *); /* RVA42ad4 */
void __cdecl push_call_context(CompilerType *); /* RVA1e54f */
CompilerIrSymbol *__cdecl find_call_symbol(CompilerParserNode *); /* RVA1e5d4 */
const char *__cdecl get_compiler_message(unsigned);
}
CompilerParserNode * __cdecl normalize_based_expression(CompilerParserNode *value)
{
    CompilerParserNode *result = NULL, *unwrapped;
    CompilerIrSymbol *symbol;
    if (!value) return NULL;
    CompilerTypeShape *shape = value->type->shape;
    if (!shape || (shape->flags & 7) != 3) {
        if (shape && (shape->flags & 7) == 4)
            value->type = decay_call_function_type(value->type);
        else {
            compiler_warning(64);
            return NULL;
        }
    }
    push_call_context(get_function_return_type(value->type));
    unwrapped = value;
    for (;;) {
        /* Only the established indirection relation is currently consumed.
         * Other original kinds remain valid but unsupported in both builds. */
        if (unwrapped->kind != 6 && unwrapped->kind != 61 && unwrapped->kind != 101 && unwrapped->kind != 112 && unwrapped->kind != 65) {
#ifdef __linux__
            recon_unsupported("call wrapper classification is not yet recovered");
#else
            __asm { int 3 }
#endif
        }
        if (c1_expression_classes.indexed[unwrapped->kind] != 61) break;
        unwrapped = unwrapped->left;
    }
    symbol = find_call_symbol(unwrapped);
    if (symbol) {
        call_context->callee = symbol;
        if (symbol->kind == 6) {
#ifdef __linux__
            if (symbol->intrinsic_opcode != 0x0f || symbol->intrinsic_traits != 1)
                recon_unsupported("intrinsic call context is outside the integer-absolute domain");
#endif
            unsigned opcode = symbol->intrinsic_opcode;
            if ((opcode == 0xd9 && !(lexer_flags & 0x2000000) && !exception_code_enabled) ||
                (opcode == 0xda && !(lexer_flags & 0x2000000)) ||
                (opcode == 0xd7 && !abnormal_termination_enabled))
                compiler_warning(707, symbol);
            if (opcode == 0xd8 && (c1_bss.current_scope_symbol->linkage_flags & 1))
                compiler_warning(490, 83, symbol);
            result = make_ir_operation(119,
                (CompilerParserNode *)make_integer_constant(symbol->intrinsic_opcode), NULL);
            call_context->flags |= 1;
        }
        call_context->diagnostic_name = symbol->binding.name->name;
        call_context->reference_depth = call_reference_depth;
#ifdef __linux__
        /* These two special callee cells are outside the supported native
         * ordinary-call owner domain. Their Win32 declarations retain identity. */
        if (special_call_symbols[0] || special_call_symbols[1])
            recon_unsupported("special call symbol identity is not yet qualified");
#endif
        if (symbol->binding.name == special_call_symbols[0] ||
            symbol->binding.name == special_call_symbols[1]) {
            if (c1_bss.current_scope_symbol->linkage_flags & 1)
                compiler_warning(490, 83, symbol);
            else c1_bss.current_scope_symbol->qualifier_flags |= 2;
        }
    } else call_context->diagnostic_name = get_compiler_message(378);
    if (!call_context->next_parameter && !(call_context->flags & 2) &&
        (!symbol || !(symbol->declaration_flags & 1))) {
        if ((call_context->function_type->shape->storage & 0x70) == 0x40)
            report_language_diagnostic(1, 72, call_context->diagnostic_name, 38);
        else report_language_diagnostic(2, 71, call_context->diagnostic_name);
        if (symbol) symbol->declaration_flags |= 1;
    }
    if (!result) result = make_ir_operation(112, value, NULL);
    result->type = get_function_return_type(call_context->function_type);
    return result;
}
extern "C" CompilerType *__cdecl decay_call_function_type(CompilerType *)
{ __asm { int 3 } return NULL; }
extern "C" void __cdecl push_call_context(CompilerType *function_type)
{
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerCallContext *frame = (CompilerCallContext *)arena->cursor;
    arena->cursor += 36;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    frame->outer = call_context;
    frame->function_type = function_type;
    CompilerTypeList *parameters = function_type->shape->parameters;
    frame->remaining_parameters = parameters->count;
    frame->next_parameter = parameters->types;
    if (frame->remaining_parameters) frame->flags |= 8;
    else frame->flags &= ~8;
    if (function_type->shape->flags & 0x4000) frame->flags |= 2;
    else if (function_type->shape->flags & 0x8000) frame->flags |= 4;
    call_context = frame;
}
extern "C" CompilerIrSymbol *__cdecl find_call_symbol(CompilerParserNode *value)
{
    call_reference_depth = 1;
    for (;;) {
        switch (value->kind) {
        case 6:
            return value->symbol;
        case 54:
            if (value->type == value->right->type) {
                value = value->right;
                continue;
            }
            value = value->left;
            break;
        case 112:
            ++call_reference_depth;
            /* fall through */
        case 65:
        case 80:
        case 93:
            value = value->left;
            break;
        default:
            return NULL;
        }
    }
}

/* RVA1e68f */
extern "C" CompilerParserNode *__cdecl make_call_argument(CompilerParserNode *, unsigned);
CompilerParserNode * __cdecl make_expression_with_type(CompilerParserNode *list,
    CompilerParserNode *value, unsigned mode)
{
    if (!value) return list;
    CompilerParserNode *argument = make_call_argument(value, mode);
    if (!argument) return NULL;
    argument->right = list;
    if (list) {
        argument->argument_head = list->argument_head;
        list->argument_head = argument;
    } else argument->argument_head = argument;
    return argument;
}
extern "C" int __cdecl is_void_parameter_type(CompilerType *);
extern "C" unsigned __cdecl pointer_storage_size(WORD);
extern "C" CompilerParserNode *__cdecl make_call_argument(CompilerParserNode *value, unsigned mode)
{
    CompilerArena *arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    CompilerParserNode *argument = (CompilerParserNode *)arena->cursor;
    CompilerType *parameter_type, *type;
    CompilerTypeShape *shape;
    unsigned kind;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    argument->flags = 2;
    argument->kind = 96;
    argument->left = value;
    if (mode) {
        argument->type = mode == 1 ? c1_data.builtin_types.type_0000 : value->type;
        return argument;
    }
    if (!call_context) return argument;
    ++call_context->supplied_arguments;
    type = value->type;
    if (type->flags == 0x80 && (!type->shape || !(type->shape->flags & 3))) {
        if (call_context->reference_depth <= 1)
            compiler_warning(95, call_context->diagnostic_name, call_context->supplied_arguments);
        else compiler_warning(174, call_context->diagnostic_name, call_context->supplied_arguments,
            call_context->reference_depth);
        argument->type = c1_data.builtin_types.type_0063;
        return argument;
    }
    if ((int)call_context->remaining_parameters > 0 &&
        (parameter_type = *call_context->next_parameter) != NULL && !(call_context->flags & 2)) {
        shape = parameter_type->shape;
        if (shape && (shape->storage & 1))
            argument->left = convert_aggregate_return(parameter_type, argument->left);
        if (!is_void_parameter_type(*call_context->next_parameter)) {
            if (!convert_expression_to_type(112, *call_context->next_parameter, value)) {
                if (call_context->reference_depth > 1)
                    report_language_diagnostic(1, 89, call_context->diagnostic_name,
                        call_context->supplied_arguments, call_context->reference_depth);
                else report_language_diagnostic(1, 24, call_context->diagnostic_name,
                    call_context->supplied_arguments);
            }
        } else {
            if (value->type->shape) {
                if (!convert_expression_to_type(112, *call_context->next_parameter, value))
                    goto incompatible_argument;
            } else if (!(value->flags & 0x80)) {
                if (*(BYTE *)value->type & 0x40) {
incompatible_argument:
                    if (call_context->reference_depth > 1)
                        report_language_diagnostic(1, 88, call_context->diagnostic_name,
                            call_context->supplied_arguments, call_context->reference_depth);
                    else report_language_diagnostic(1, 22, call_context->diagnostic_name,
                        call_context->supplied_arguments);
                } else if (call_context->reference_depth > 1)
                    compiler_warning(173, call_context->diagnostic_name,
                        call_context->supplied_arguments, call_context->reference_depth);
                else compiler_warning(172, call_context->diagnostic_name,
                    call_context->supplied_arguments);
            }
        }
        argument->type = *call_context->next_parameter;
        if (*call_context->next_parameter) {
            ++call_context->next_parameter;
            --call_context->remaining_parameters;
        }
        return argument;
    }
    if ((call_context->flags & 1) && call_context->callee && !call_context->next_parameter)
        compiler_warning(294, call_context->diagnostic_name, call_context->supplied_arguments);
    if ((call_context->flags & 2) && (call_context->flags & 8)) {
        if (!(call_context->flags & 0x10)) {
            report_language_diagnostic(1, 87, call_context->diagnostic_name);
            call_context->flags |= 0x10;
        }
    } else if ((int)call_context->remaining_parameters <= 0 &&
               !(call_context->flags & 4) && (call_context->flags & 8) &&
               !(call_context->flags & 0x20)) {
        if (c1_data.options.language_extensions)
            report_language_diagnostic(1, 20, call_context->diagnostic_name);
        else compiler_warning(197, 115, call_context->diagnostic_name);
        call_context->flags |= 0x20;
    }
    type = value->type;
    shape = type->shape;
    if (shape) {
        kind = shape->flags & 7;
        if (kind == 3) {
            argument->type = type;
            if (pointer_storage_size(shape->attributes) >= 4) return argument;
            CompilerArena *shape_arena = &c1_bss.arenas[3];
            CompilerTypeShape *copy = (CompilerTypeShape *)shape_arena->cursor;
            shape_arena->cursor += 12;
            if (shape_arena->cursor > shape_arena->committed_limit) shape_arena->CommitExtent();
            *copy = *shape;
            copy = intern_type_shape(copy);
#ifdef RECON_NATIVE_TEST
            if (type_description_depth != 0)
                recon_unsupported("nested call type scratch is not yet qualified");
#endif
            ++type_description_depth;
            CompilerType *description = &c1_bss.type_descriptions[type_description_depth];
            memcpy(description, type, 16);
            description->shape = copy;
            argument->type = intern_type(description);
            --type_description_depth;
            return argument;
        }
        if ((kind == 1 || kind == 2) && (shape->storage & 1)) {
            if (!(call_context->flags & 8))
                report_language_diagnostic(1, 23, call_context->diagnostic_name,
                    call_context->supplied_arguments);
            else if (call_context->flags & 4)
                report_language_diagnostic(1, 25, call_context->diagnostic_name,
                    call_context->supplied_arguments);
        }
    }
    argument->type = c1_data.builtin_types.type_0000;
    return argument;
}
extern "C" int __cdecl is_void_parameter_type(CompilerType *type)
{
    /* Void-derived parameters allow the permissive pointer conversion path;
     * a terminal function shape and unshaped void do not select it. */
    if (type->flags != 0x80) return 0;
    CompilerTypeShape *shape = type->shape;
    if (!shape) return 0;
    do {
        if ((shape->flags & 7) == 4 && !shape->base) return 0;
        shape = shape->base;
    } while (shape);
    return 1;
}

/* RVA1e8dc */
extern "C" unsigned __cdecl parameter_list_stack_bytes(CompilerTypeList *);
extern "C" unsigned __cdecl call_argument_stack_bytes(CompilerParserNode *);
CompilerParserNode * __cdecl finish_argument_list(CompilerParserNode *value)
{
    CompilerTypeShape *function_shape = NULL;
    CompilerIrSymbol *symbol;
    unsigned bytes;
    if (!value) return NULL;
    if (!call_context) return value;
    if (call_context->flags & 1) {
        int expected = call_context->callee->intrinsic_traits & 7;
        if ((int)call_context->supplied_arguments < expected)
            compiler_warning(168, call_context->diagnostic_name);
        else if ((int)call_context->supplied_arguments > expected)
            compiler_warning(167, call_context->diagnostic_name);
    } else if ((int)call_context->remaining_parameters > 0 &&
               *call_context->next_parameter && !(call_context->flags & 2))
        compiler_warning(198, 115, call_context->diagnostic_name);
    if (value->type && value->type->shape &&
        (value->type->shape->flags & 7) == 1 &&
        (value->type->shape->attributes & 3))
        value = finish_segment_dereference(value);
    symbol = call_context->callee;
    if (symbol) function_shape = symbol->value_type->shape;
    if (function_shape && (function_shape->flags & 7) == 4 &&
        ((function_shape->attributes & 0x70) == 0x60 ||
         (function_shape->attributes & 0x70) == 0x40)) {
        if ((symbol->qualifier_flags & 8) || (function_shape->flags & 0x800))
            bytes = call_argument_stack_bytes(value);
        else bytes = parameter_list_stack_bytes(function_shape->parameters);
        symbol = call_context->callee;
        if (symbol->qualifier_flags & 0x40) {
            if (((symbol->qualifier_flags & 8) || (function_shape->flags & 0x800)) &&
                bytes != symbol->symbol_value) {
                compiler_warning(708, symbol);
                call_context = call_context->outer;
                return value;
            }
        } else {
            symbol->symbol_value = bytes;
            symbol->qualifier_flags |= 0x40;
            c1_bss.debug_module->Slot64();
        }
    }
    call_context = call_context->outer;
    return value;
}
extern "C" unsigned __cdecl parameter_list_stack_bytes(CompilerTypeList *parameters)
{
    unsigned bytes = 0;
    if (parameters->count < 0) {
#ifdef __linux__
        recon_unsupported("parameter list stack size requires an established nonnegative count");
#else
        __asm { int 3 }
#endif
    }
    for (unsigned index = 0; index < (unsigned)parameters->count; ++index)
        bytes += (parameter_type_size(parameters->types[index], 0) + 3) & ~3U;
    return bytes;
}
extern "C" unsigned __cdecl call_argument_stack_bytes(CompilerParserNode *)
{ __asm { int 3 } return 0; }

/* RVA1eba0 */
void __cdecl emit_expression(CompilerParserNode *value)
{
    if (parser_early_result) return;
    emit_control_record(36);
    emit_ir_node(value);
    write_ir_record(75, NULL);
}
/* RVA1ec07 */
void __cdecl emit_conditional_branch(CompilerParserNode *value, CompilerParserNode *target, int when_true)
{
    CompilerParserNode *intermediate;
    unsigned opcode;
    for (;;) {
        if (value->type->flags == 0x80 && !value->type->shape) compiler_warning(180);
        if ((value->flags & 7) == 3 || (value->flags & 7) == 5)
            report_language_diagnostic(4, 127);
        if (!IR_KIND_MODELED(value->kind)) {
#ifdef __linux__
            recon_unsupported("conditional expression opcode is outside the recovered column");
#else
            __asm { int 3 }
#endif
        }
        opcode = ir_node_opcodes.indexed[value->kind];
        switch (opcode) {
        case 26: /* Logical negation reverses the branch sense. */
            value = value->left;
            when_true = !when_true;
            continue;
        case 27: /* Short-circuit disjunction. */
            intermediate = when_true ? target : make_internal_label(target->symbol->symbol_value);
            emit_conditional_branch(value->left, intermediate, 1);
            emit_conditional_branch(value->right, target, when_true);
            if (!when_true) emit_ir_label(intermediate);
            return;
        case 28: /* Short-circuit conjunction. */
            intermediate = when_true ? make_internal_label(target->symbol->symbol_value) : target;
            emit_conditional_branch(value->left, intermediate, 0);
            emit_conditional_branch(value->right, target, when_true);
            if (when_true) emit_ir_label(intermediate);
            return;
        }
        if (value && value->type && (value->type->flags & 0x400) && !value->type->shape) {
            compiler_warning(83, value->type);
            return;
        }
        emit_control_record(36);
        emit_ir_node(value);
        write_ir_record(when_true ? 57 : 56, target);
        return;
    }
}
void __cdecl push_parser_label(ParserLabelStack **, unsigned);
/* RVA1f1ad */
void __cdecl begin_switch_statement(CompilerParserNode *value)
{
    push_parser_label(&loop_labels, 17);
    push_parser_label(&break_labels, 18);
    push_parser_label(&switch_labels, 19);
    switch_labels->node = NULL;
    push_parser_label(&default_labels, 20);
    default_labels->node = NULL;
    if (!value) {
        loop_labels->node->type = c1_data.builtin_types.type_0063;
        return;
    }
    if ((value->flags & 7) == 1 && value->kind == 60)
        report_language_diagnostic(1, 144, 60);
    else if ((value->flags & 7) == 2) {
        switch (value->kind) {
        case 50: report_language_diagnostic(1, 145, 50, 49); break;
        case 59: report_language_diagnostic(1, 145, 59, 52); break;
        case 79: report_language_diagnostic(1, 145, 79, 77); break;
        case 62: case 63: case 68: case 69:
            report_language_diagnostic(1, 144, (unsigned)value->kind);
            break;
        }
    }
    if (!value->type || value->type->shape || !(value->type->flags & (0x40 | 0x200))) {
        compiler_warning(50);
        loop_labels->node->type = c1_data.builtin_types.type_0063;
    } else {
        value = promote_scalar_expression(value);
        loop_labels->node->type = intern_type(value->type);
    }
    value = make_typed_unary_operation(133, value->type, value);
    c1_bss.switch_control_symbol = loop_labels->node->symbol;
    emit_control_record(36);
    emit_ir_node(value);
}
/* RVA1f312 */
void __cdecl emit_case_label(CompilerParserNode *value)
{
    WORD flags;
    int bytes;
    unsigned shift, references;
    unsigned __int64 magnitude, maximum;
    __int64 converted;
    CompilerArena *arena = &c1_bss.arenas[4];
    CompilerSwitchCase *entry, *current, *previous;
    CompilerIrSymbol *symbol;
    union { __int64 integer; struct { unsigned low, high; } words; } duplicate;
    int unsigned_order;
    if ((value->flags & 7) != 3) {
        compiler_warning(51);
        return;
    }
    if (!(value->type->flags & (0x40 | 0x200))) {
        compiler_warning(52);
        return;
    }
    magnitude = ((CompilerIntegerConstant *)value)->integer;
    value->type = loop_labels->node->type;
    flags = value->type->flags;
    if ((__int64)magnitude < 0) magnitude = 0 - magnitude;
    bytes = (flags & 0x340) || !(flags & ~15) ? c1_primitive_widths[flags & 15] : flags & 15;
    shift = (BYTE)(bytes * 8 - 1);
    maximum = ~(shift >= 64 ? 0 : (unsigned __int64)-1 << shift);
    maximum = (maximum << 1) | maximum;
    if (magnitude > maximum)
        report_language_diagnostic(1, 178, value->low, value->high);
    converted = convert_integer_constant(99, value, 0, 0, flags);
    entry = (CompilerSwitchCase *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    entry->value = converted;
    if (!case_label) {
        case_label = make_internal_label(19);
        emit_ir_label(case_label);
    }
    symbol = case_label->symbol;
    if (c1_data.c9_il) entry->label_index = symbol->record_id & 0xffffff;
    else {
        references = symbol->reference_flags;
        entry->label_index = references & ~7U ? references >> 3 : assign_symbol_output_index(symbol);
    }
    references = symbol->reference_flags;
    if ((references & 3) < 3) references = (references & ~3U) | ((references + 1) & 3);
    symbol->reference_flags = (references & ~4U) | ((c1_data.ir_reference_epoch & 1) << 2);
    unsigned_order = (flags & 0x4000) || (!c1_data.options.language_extensions &&
        c1_data.options.unsigned_char && (flags & 0x2fcf) == 0x41 && !(flags & 0x8000));
    current = switch_labels->cases;
    previous = NULL;
    while (current && (unsigned_order ?
        (unsigned __int64)converted > (unsigned __int64)current->value : converted > current->value)) {
        previous = current;
        current = current->next;
    }
    if (current && converted == current->value) {
        duplicate.integer = converted;
        compiler_warning(196, duplicate.words.low, duplicate.words.high);
        return;
    }
    entry->next = current;
    if (previous) previous->next = entry;
    else switch_labels->cases = entry;
}
/* RVA1f54b */
void __cdecl finish_switch_statement(void)
{
    CompilerParserNode *dispatch = make_ir_operation(132, loop_labels->node,
        (CompilerParserNode *)switch_labels->cases);
    dispatch->type = loop_labels->node->type;
    loop_labels->node->symbol->flags |= 1;
    loop_labels->node->symbol->label_nesting = parameter_nesting;
    c1_bss.switch_control_symbol = default_labels->node ?
        default_labels->node->symbol : break_labels->node->symbol;
    emit_ir_node(dispatch);
    emit_ir_label(break_labels->node);
}
/* RVA1f6d3 */
void __cdecl push_parser_label(ParserLabelStack **, unsigned);
void __cdecl emit_branch(CompilerParserNode *initialize, CompilerParserNode *condition,
    CompilerParserNode *advance)
{
    push_parser_label(&loop_labels, 8);
    push_parser_label(&continue_labels, 12);
    push_parser_label(&break_labels, 11);
    if (initialize) emit_expression(initialize);
    if (advance) {
        write_ir_record(58, loop_labels->node);
        emit_ir_label(continue_labels->node);
        emit_expression(advance);
    } else loop_labels->node = continue_labels->node;
    emit_ir_label(loop_labels->node);
    if (condition) emit_conditional_branch(condition, break_labels->node, 0);
}
ParserLabelStack *__cdecl push_parser_label_link(ParserLabelStack **, unsigned);
void __cdecl push_parser_label(ParserLabelStack **head, unsigned kind)
{
    ParserLabelStack *entry = push_parser_label_link(head, 4);
    entry->node = make_internal_label(kind);
    ++label_stack_count;
}
ParserLabelStack *__cdecl push_parser_label_link(ParserLabelStack **head, unsigned arena_kind)
{
    if (arena_kind >= 6) {
#ifdef __linux__
        recon_unsupported("label-link arena is outside the modeled owners");
#else
        __asm { int 3 }
#endif
    }
    CompilerArena *arena = &c1_bss.arenas[arena_kind];
    ParserLabelStack *entry = (ParserLabelStack *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    entry->next = *head;
    *head = entry;
    return entry;
}
/* RVA1f7d8 */
void __cdecl initialize_symbol_value(CompilerIrSymbol *, CompilerParserNode *); /* RVA1f7fb */
void __cdecl apply_initializer(CompilerIrSymbol *symbol, CompilerParserNode *value)
{
    initialize_symbol_value(symbol, value);
    if (c1_data.function_record_flags & 0x800) {
        c1_data.function_record_flags &= ~0x800U;
        symbol->declaration_flags &= ~4;
    }
}

/* RVA1f9c4 */
void __cdecl finish_loop_statement(void)
{
    write_ir_record(58, continue_labels->node);
    emit_ir_label(break_labels->node);
    loop_labels = loop_labels->next;
    continue_labels = continue_labels->next;
    break_labels = break_labels->next;
    label_stack_count -= 3;
    if (!label_stack_count) c1_bss.arenas[4].ClearUsed();
}
/* RVA21166 */

/* RVA21208 */
void __cdecl complete_initializer_symbol(CompilerIrSymbol *); /* RVA21226 */
void __cdecl begin_aggregate_initializer(CompilerIrSymbol *symbol)
{
    complete_initializer_symbol(symbol);
    if (c1_data.function_record_flags & 0x800) {
        c1_data.function_record_flags &= ~0x800U;
        symbol->declaration_flags &= ~4;
    }
}
unsigned __cdecl initializer_array_count(void); /* RVA2506b */
void __cdecl finalize_initializer_symbol(CompilerIrSymbol *); /* RVA1e045 */
void __cdecl complete_initializer_symbol(CompilerIrSymbol *symbol)
{
    CompilerType *type;
    CompilerTypeShape *shape;
    unsigned count;
    int unbounded_array, incomplete_record;
    if (function_output_mode || !symbol || c1_data.initializer_depth == -1) return;
    if (c1_data.initializer_depth < 0 || c1_data.initializer_depth > C1_INITIALIZER_LAST) {
#ifdef __linux__
        recon_unsupported("initializer symbol completion beyond encountered frame records");
#else
        __asm { int 3 }
#endif
    }
    if (c1_bss.initializer_frame->flags & 0x20) {
        --c1_data.initializer_depth;
        c1_bss.initializer_frame = (CompilerInitializerFrame *)
            ((BYTE *)c1_bss.initializer_frames + 20 * c1_data.initializer_depth);
        return;
    }
    finish_initializer_element();
    unbounded_array = (symbol->qualifier_flags & 5) == 5;
    type = symbol->value_type;
    shape = type->shape;
    incomplete_record = !shape && (type->flags & 0x400) &&
        (type->symbol_owner->qualifier_flags & 1);
    if (unbounded_array) {
        symbol->qualifier_flags &= ~1U;
        count = initializer_array_count();
        type = symbol->value_type;
        shape = type->shape;
        symbol->value_type = replace_function_parameters(type, shape->flags,
            shape->attributes, &count);
        finalize_initializer_symbol(symbol);
        return;
    }
    if (incomplete_record) {
        if (c1_bss.initializer_saved_type) {
            count = global_type_size(c1_bss.initializer_saved_type);
            count *= initializer_array_count();
            symbol->symbol_value = count;
        } else symbol->symbol_value = 0;
        c1_bss.initializer_saved_type = NULL;
    }
    finalize_initializer_symbol(symbol);
}
unsigned __cdecl initializer_array_count(void)
{
    unsigned i = 0;
    while (!c1_bss.initializer_frames[i].offset) {
        if (++i >= 4) {
#ifdef __linux__
            recon_unsupported("initializer count search beyond encountered frame records");
#else
            __asm { int 3 }
#endif
        }
    }
    return c1_bss.initializer_frames[i].offset;
}
void __cdecl finish_initializer_record(void); /* RVA2076b */
void __cdecl finalize_initializer_symbol(CompilerIrSymbol *)
{
    if (c1_bss.initializer_resume_label) {
        emit_ir_label(c1_bss.initializer_resume_label);
        c1_bss.initializer_resume_label = NULL;
    }
    c1_data.last_initializer_symbol = NULL;
    if (c1_data.function_record_flags & 0x4000) {
        c1_data.backend->Slot08();
        c1_data.function_record_flags &= ~0x4000U;
    }
    if (!(lexer_flags & 0x100000) || (lexer_flags & 0x40000))
        finish_initializer_record();
}
void __cdecl finish_initializer_record(void)
{
    CompilerOutputBuffer *output;
    int ready;
    if (c1_data.function_record_flags & 0x800) return;
    output = &c1_bss.output_buffers[4];
    ready = output->cursor + 2 <= output->data + output->capacity ? 1 : output->Grow(2);
    output->good &= ready;
    if (output->good) *output->cursor++ = 7;
}
/* RVA21bd9 */
CompilerParserNode *__cdecl make_sizeof_expression(CompilerParserNode *value)
{
    CompilerType *type = value->type;
    CompilerTypeShape *shape;
    if (!type) { compiler_warning(70); return NULL; }
    if ((value->flags & 7) == 4) {
        CompilerType *declared = value->symbol->value_type;
        shape = declared->shape;
        if (shape) {
            if ((shape->flags & 7) == 2)
                return make_type_size_expression(declared);
            if ((shape->flags & 7) == 4) {
                if (type->shape->flags & 8) {
                    compiler_warning(70);
                    return NULL;
                }
                return make_type_size_expression(type);
            }
        }
    } else {
        /* Only these encountered classification cells are initialized. */
        if (value->kind != 6 && value->kind != 45 && value->kind != 50 &&
            value->kind != 59 && value->kind != 61 && value->kind != 65 &&
            value->kind != 101 && value->kind != 112) {
#ifdef __linux__
            recon_unsupported("sizeof expression classification is not yet recovered");
#else
            __asm { int 3 }
#endif
        }
        if (c1_expression_classes.indexed[value->kind] == 61 && value->left->kind == 53) {
            compiler_warning(70);
            return NULL;
        }
    }
    if (type->shape && (type->shape->flags & 8)) {
        compiler_warning(70);
        return NULL;
    }
    return make_type_size_expression(type);
}
/* RVA21dd6 */
void __cdecl begin_external_declaration(void) { __asm { int 3 } }
/* RVA22e59 */
extern "C" void *__cdecl create_enum_context(unsigned, unsigned, unsigned, unsigned, int, int);
void __cdecl attach_declaration_context(CompilerIrSymbol *symbol, void *context)
{
    if (!(symbol->record_id & 0xffffff)) {
        if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
        symbol->record_id = (symbol->record_id & 0xff000000U) |
            (c1_data.next_symbol_record_id++ & 0xffffff);
    }
    if (!c1_data.declaration_context_map)
        c1_data.declaration_context_map = (CompilerSymbolMap *)create_enum_context(31, 0, 3, 2, 0, 1);
    append_enum_symbol(c1_data.declaration_context_map, (void *)(symbol->record_id & 0xffffff),
        (CompilerIrSymbol *)context);
    symbol->function_flags |= 1;
}
/* RVA23957 */
void __cdecl begin_exception_handler(void)
{
    if (c1_bss.current_scope_symbol->linkage_flags & 1)
        compiler_warning(490, 76, 167);
    write_ir_record(143, NULL);
    c1_bss.current_scope_symbol->qualifier_flags |= 0x10;
    c1_bss.current_scope_symbol->qualifier_flags |= 1;
    ++c1_data.try_depth;
    push_parser_label(&c1_bss.exception_labels, 26);
}
/* RVA239ac */
void __cdecl update_exception_label_depths(void);
void __cdecl finish_exception_handler(void)
{
    update_exception_label_depths();
    --c1_data.try_depth;
    emit_ir_label(c1_bss.exception_labels->node);
    c1_bss.exception_labels = c1_bss.exception_labels->next;
    if (!--label_stack_count) c1_bss.arenas[4].ClearUsed();
}
/* RVA239f5 */
void __cdecl update_exception_label_depths(void)
{
    for (CompilerIrSymbol *symbol = c1_data.label_symbols; symbol;
         symbol = symbol->binding.next ?
             (CompilerIrSymbol *)((BYTE *)symbol->binding.next - 8) : NULL) {
        if (symbol->kind != 9) {
#ifdef __linux__
            recon_unsupported("exception label list contains an unrecovered symbol kind");
#else
            __asm { int 3 }
#endif
        }
        int depth = c1_data.try_depth + c1_data.finally_depth +
            c1_data.exception_filter_depth;
        if ((int)symbol->label_exception_depth >= depth) {
            if (symbol->flags & 1) symbol->qualifier_flags |= 2;
            else --symbol->label_exception_depth;
        }
    }
}
/* RVA23a50 */
void __cdecl set_exception_filter_mode(void)
{
    lexer_flags |= 0x2000000;
}
/* RVA23a5b */
void __cdecl emit_exception_filter(CompilerParserNode *condition)
{
    if (c1_data.exception_filter_depth > 0) {
        compiler_warning(702);
    } else {
        emit_control_record(36);
        emit_ir_node(condition);
        write_ir_record(141, condition);
    }
    ++c1_data.finally_depth;
}
/* RVA23a9b */
void __cdecl clear_exception_filter_mode(void)
{
    lexer_flags &= ~0x2000000U;
}
/* RVA23aa6 */
void __cdecl finish_finally_handler(void)
{
    update_exception_label_depths();
    --c1_data.finally_depth;
    write_ir_record(144, NULL);
}
/* RVA24f93 */
/* RVA25125 */
int __cdecl compare_label_spelling(const char *, const char *);
CompilerIrSymbol *__cdecl find_statement_label(CompilerNameEntry *name)
{
    CompilerIrSymbol *symbol;
    if (parser_early_result) return NULL;
    for (symbol = c1_data.label_symbols; symbol; symbol = symbol->binding.next ?
        (CompilerIrSymbol *)((BYTE *)symbol->binding.next - 8) : NULL) {
        if (symbol->binding.name == name) return symbol;
        if (!symbol->symbol_value && (symbol->qualifier_flags & 0x10) &&
            !compare_label_spelling(name->name, symbol->binding.name->name)) return symbol;
    }
    return NULL;
}

/* RVA2517f */
CompilerIrSymbol *__cdecl make_statement_label(CompilerNameEntry *name)
{
    CompilerArena *arena = &c1_bss.arenas[2];
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)arena->cursor;
    arena->cursor += (c1_data.record_sizes.known.global_symbol_bytes + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (symbol) {
        symbol->kind = 9;
        symbol->binding.name = name;
        symbol->binding.next = NULL;
    }
    symbol->symbol_value = 0;
    symbol->value_type = c1_data.builtin_types.type_0063;
    return symbol;
}
/* RVA251d5 */
CompilerParserNode *__cdecl make_symbol_node(BYTE, CompilerIrSymbol *, CompilerType *);
void __cdecl define_statement_label(CompilerNameEntry *name)
{
    CompilerIrSymbol *symbol = find_statement_label(name);
    if (symbol) {
        if (symbol->flags & 1) compiler_warning(45, name);
    } else {
        symbol = make_statement_label(name);
        symbol->declaration_flags |= 0x20;
        record_label_reference(symbol);
    }
    emit_ir_label(make_symbol_node(6, symbol, NULL));
}
CompilerParserNode *__cdecl make_symbol_node(BYTE kind, CompilerIrSymbol *symbol,
    CompilerType *type)
{
    if (c1_data.expression_arena_kind >= 6) {
#ifdef __linux__
        recon_unsupported("symbol-node arena is outside the encountered owners");
#else
        __asm { int 3 }
#endif
    }
    CompilerArena *arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    CompilerParserNode *node = (CompilerParserNode *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    node->kind = kind;
    node->flags = 4;
    node->symbol = symbol;
    node->type = type;
    return node;
}
/* RVA25ba5 */
CompilerIrSymbol * __cdecl make_parameter_description(CompilerNameEntry *name)
{
    CompilerIrSymbol *symbol;
    unsigned index;
    if (c1_data.record_sizes.known.named_parameter_symbol_bytes != 52) {
#ifdef __linux__
        recon_unsupported("old-style parameter symbol size is not recovered");
#else
        __asm { int 3 }
#endif
    }
    symbol = (CompilerIrSymbol *)c1_bss.arenas[2].AllocateBytes(
        (c1_data.record_sizes.known.named_parameter_symbol_bytes + 3) & ~3U);
    if (symbol) symbol = symbol->Initialize(11, name, 2);
    symbol->value_type = c1_data.builtin_types.type_0063;
    if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
    index = c1_data.next_symbol_record_id++;
    symbol->record_id = (symbol->record_id & 0xff000000U) | (index & 0xffffffU);
    symbol->declaration_flags |= 0x10;
    c1_data.translation_unit->current->Insert(symbol);
    return symbol;
}
/* RVA25c26 */
CompilerIrSymbol *__cdecl add_old_style_parameter(const CompilerParserDeclaration *declaration,
    CompilerIrSymbol *symbol, CompilerDeclarator *declarator)
{
    CompilerIrSymbol **found = c1_data.translation_unit->FindName(symbol->binding.name);
    CompilerIrSymbol *parameter = found ? *found : NULL;
    CompilerType *type;
    if (!parameter) { compiler_warning(85, symbol); return NULL; }
    if (parameter->storage_class & 7) { compiler_warning(82, symbol); return NULL; }
    symbol->kind = 11;
    symbol->intrinsic_flags = declaration->flags;
    if ((declaration->flags & 0x1000000U) && declaration->value)
        attach_declaration_context(symbol, (void *)declaration->value);
    else symbol->function_flags &= ~1U;
    parameter->intrinsic_flags = declaration->flags;
    if ((declaration->flags & 0x1000000U) && declaration->value)
        attach_declaration_context(parameter, (void *)declaration->value);
    else parameter->function_flags &= ~1U;
    declare_symbol(declarator, symbol);
    type = symbol->value_type;
    if (type->flags == 0x106 && !type->shape)
        type = c1_data.builtin_types.type_0107;
    parameter->value_type = type;
    return parameter;
}
/* RVA25eb0 */
void __cdecl prepare_for_statement(void)
{
    push_parser_label(&loop_labels, 5);
    push_parser_label(&continue_labels, 7);
    push_parser_label(&break_labels, 6);
    emit_ir_label(loop_labels->node);
}
/* RVA3f1b4 */
void __cdecl report_expected_parser_token(CompilerParserToken *, int) { __asm { int 3 } }
/* RVA3f281 */
CompilerParserValue * __cdecl make_recovery_parser_value(CompilerParserValue *, int) { __asm { int 3 } return 0; }
/* RVA3f2c2 */
CompilerParserToken *__cdecl current_parser_lookahead(void)
{
    return c1_data.active_parser->lookahead;
}
void __cdecl report_parser_error(const char *message)
{
    CompilerParserToken *token;
    if (parser_flags & 0x200) return;
    if (*message == 'y') compiler_fatal(26);
    token = current_parser_lookahead();
    if (token->kind != 6 && token->kind != 127) {
#ifdef __linux__
        recon_unsupported("parser diagnostic token classification is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    switch (c1_expression_classes.indexed[token->kind]) {
    case 0: case 5:
        compiler_fatal(4);
        break;
    case 6:
        if (current_parser_lookahead()->value.name)
            compiler_warning(61, current_parser_lookahead()->value.name);
        else compiler_warning(132);
        break;
    case 31:
        /* These actual incoming token words are only diagnostic operands;
         * their wider literal roles remain behind the classification guard. */
        if (current_parser_lookahead()->value.typed_constant.high)
            compiler_warning(226, current_parser_lookahead()->value.typed_constant.high);
        else compiler_warning(62, current_parser_lookahead()->value.typed_constant.low);
        break;
    default:
        compiler_warning(59, 76, current_parser_lookahead()->kind);
        break;
    }
}
/* RVA3f393 */
void __cdecl skip_declaration_tokens(void)
{
    unsigned depth = 0;
    for (;;) {
        CompilerParserToken *token = c1_bss.current_context.ReadToken();
        switch (token->kind) {
        case 0:
            return;
        case 7:
            c1_lexer_token_cursor = c1_bss.quoted_workspace;
            break;
        case 109:
            compiler_fatal(4);
            break;
        case 117:
            ++depth;
            break;
        case 123:
            if (depth) --depth;
            else {
                c1_bss.current_context.saved_frame->stream->Unread((unsigned)token);
                return;
            }
            break;
        case 127:
            c1_bss.arenas[3].ClearUsed();
            c1_lexer_token_cursor = c1_bss.quoted_workspace;
            break;
        }
    }
}
/* RVA3f3ff */
CompilerParserToken * __cdecl recover_parser_token(CompilerParserToken *, ...) { __asm { int 3 } return 0; }
/* RVA3f4f1 */
CompilerIrSymbol * __cdecl find_based_declarator(CompilerNameEntry *) { __asm { int 3 } return 0; }
/* RVA3f798 */
void __cdecl process_pragma_declaration(void) { __asm { int 3 } }
/* RVA3f7d4 */
void __cdecl prepare_external_input(void) { __asm { int 3 } }
/* RVA41374 */
void __cdecl report_type_declaration_error(int, const CompilerParserDeclaration *) { __asm { int 3 } }
/* RVA41f63 */
void __cdecl emit_leave_statement(void)
{
    if (c1_bss.exception_labels)
        write_ir_record(58, c1_bss.exception_labels->node);
    else
        compiler_warning(703);
}
/* RVA41f89 */
void __cdecl begin_exception_filter(void)
{
    write_ir_record(142, NULL);
    ++c1_data.exception_filter_depth;
}
/* RVA41fa4 */
void __cdecl finish_exception_filter(void)
{
    update_exception_label_depths();
    --c1_data.exception_filter_depth;
    write_ir_record(144, NULL);
}
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
extern "C" CompilerTypeShape *__cdecl create_type_shape(unsigned, unsigned, const unsigned *);
void RECON_THISCALL CompilerDeclarator::AppendArrayBound(const unsigned *bound)
{
    unsigned count = *bound;
    if ((int)count < 0) {
        compiler_warning(118);
        count = 1;
    }
    CompilerTypeShape *array = create_type_shape(2, 0, &count);
    AppendArray(array);
    if (!last_shape) {
        last_shape = array;
        shape = array;
    } else {
        last_shape->base = array;
        last_shape = array;
    }
}
extern "C" CompilerIrSymbol *__cdecl create_parameter_sentinel(void);
void RECON_THISCALL CompilerParameterDescription::AppendParameter(CompilerIrSymbol *symbol)
{
    CompilerParameterBindings *list = bindings;
    CompilerSymbolBinding *entry = &symbol->binding;
    if (!list) {
        CompilerArena *arena = &c1_bss.arenas[2];
        list = (CompilerParameterBindings *)arena->cursor;
        arena->cursor += 12;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        if (list) {
            list->arena_kind = 2;
            list->first = NULL;
            list->last = NULL;
        }
        bindings = list;
    }
    if (entry) {
        entry->name = symbol->binding.name;
        entry->next = NULL;
        symbol->previous_parameter = list->last;
    }
    if (list->last) {
        if (!c1_data.parameter_sentinel)
            c1_data.parameter_sentinel = create_parameter_sentinel();
        CompilerIrSymbol *previous = (CompilerIrSymbol *)((BYTE *)list->last - 8);
        if (previous != c1_data.parameter_sentinel) list->last->next = entry;
        else list->first = entry;
    } else list->first = entry;
    list->last = entry;
}
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
        /* The value is the already constructed parameter-binding owner. This
         * method stores its identity; later list consumers qualify its fields. */
        parameter_scope_value = (unsigned)value;
    }
    RegisterScope(scope);
}
void RECON_THISCALL CompilerParser::ClearRecovery(void)
{
    recovery_shifts = 0;
}

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
        if ((token->kind != 0xf0 && token->kind != 0xf2 && token->kind != 0xf4) ||
            (token->kind == 0xf0 && !(lexer_flags & 0x200000U) && (lexer_flags & 0x20000U))) {
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
                if (frame->second) {
#ifdef __linux__
                    recon_unsupported("context-end callback ABI has not been encountered");
#else
                    __asm { int 3 }
#endif
                    ((void (__cdecl *)(void))frame->second)();
                }
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
        if (declaration_depth < 0 || declaration_depth > 4)
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
        if (!parameter_nesting || browser_parameter_references)
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
CompilerIrSymbol **RECON_THISCALL CompilerTranslationUnit::FindName(CompilerNameEntry *name)
{
    BYTE index;
    CompilerIrSymbol **result;
    if (!name) return NULL;
    index = depth;
    do {
        if (index >= 128) {
#ifdef __linux__
            recon_unsupported("ordinary name lookup exceeds the actual scope stack");
#else
            __asm { int 3 }
#endif
        }
        result = (CompilerIrSymbol **)scopes[index]->FindOrdinary(name);
        if (result) return result;
    } while (index-- != 0);
    return NULL;
}
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
void __cdecl skip_line_comment(void); /* RVA04468 */
int __cdecl skip_block_comment(void); /* RVA084b5 */
int __cdecl skip_language_comment(void); /* RVA3c1c8 */
void __cdecl skip_macro_block_comment(void);
void __cdecl skip_preserved_macro_comment(void);
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
unsigned char __cdecl read_numeric_character(void);
void __cdecl copy_numeric_spelling(CompilerValue *, const char *, unsigned, unsigned);
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
             dispatch != SC_DIGIT && dispatch != SC_NEWLINE && dispatch != SC_IDENTIFIER && dispatch != SC_LETTER_L &&
             dispatch != SC_SPACE && dispatch != SC_LPAREN && dispatch != SC_RPAREN &&
             dispatch != SC_LBRACE && dispatch != SC_DQUOTE && dispatch != SC_SQUOTE && dispatch != SC_COMMA &&
             dispatch != SC_CARET && dispatch != SC_PERCENT && dispatch != SC_TILDE &&
             dispatch != SC_SLASH && dispatch != SC_BAR && dispatch != SC_DOT && dispatch != SC_GREATER && dispatch != SC_COLON && dispatch != SC_AMP && dispatch != SC_BANG && dispatch != SC_PLUS && dispatch != SC_LESS && dispatch != SC_STAR && dispatch != SC_MINUS && dispatch != SC_LBRACKET && dispatch != SC_RBRACKET && dispatch != SC_EQUAL) ||
            (dispatch == SC_SPECIAL && character != '?' && (character ||
                (lexical_skip_mode && (lexical_skip_mode > 1 || *c1_bss.source_cursor != 'R'))))) {
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
            if (c1_data.options.language_extensions || c1_data.options.preprocess_only) {
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
                if (c1_data.options.language_extensions || c1_data.options.preprocess_only) {
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
                    /* Inline assembly consumes this existing location-token
                     * alternative. Preserve the authentic24-byte transfer;
                     * only its context object/value and later token tag are
                     * interpreted, not the untouched union tail. */
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
extern "C" unsigned __cdecl read_unspliced_character(void)
{
    BYTE character = *c1_bss.source_cursor++;
    if (c1_data.scanner.classes[character] == SC_SPECIAL)
        return (BYTE)resolve_special_character(character);
    return character;
}
extern "C" unsigned char __cdecl read_multibyte_character(void);
extern "C" void __cdecl skip_preserved_line_comment(void)
{
    unsigned character, category;
    fwrite("//", 2, 1, c1_bss.preprocessor_output);
    for (;;) {
        character = *c1_bss.source_cursor++;
        category = c1_data.scanner.classes[character];
        if (category == SC_NEWLINE) { --c1_bss.source_cursor; return; }
        if (category == SC_SPACE) {
            if (character == '\r') continue;
        } else if (category == SC_SPECIAL) {
            if (character == '?') {
                if (!(process_trigraph() & 0x40)) continue;
            } else if (character == '\\') {
                (putchar)('\\');
                splice_input_line();
                continue;
            } else {
                process_special_character();
                continue;
            }
        }
        fputc(character, c1_bss.preprocessor_output);
    }
}
extern "C" void __cdecl skip_comment_continuation(void) { __asm { int 3 } }
extern "C" void __cdecl skip_line_comment(void)
{
    unsigned character;
    if (c1_bss.options.preserve_comments && !(lexer_flags & 0x8000)) {
        skip_preserved_line_comment();
        return;
    }
    for (;;) {
        do { character = *c1_bss.source_cursor++; }
        while (c1_data.scanner.flags[character] & 0x40);
        if (character == '\n') { --c1_bss.source_cursor; return; }
        unsigned category = c1_data.scanner.classes[character];
        if (!category) {
            if (process_special_character() != 1) continue;
            if (!*c1_bss.source_cursor) return;
            if (character == '\\') skip_comment_continuation();
        } else if (category == SC_LEAD) read_multibyte_character();
    }
}
extern "C" int __cdecl skip_block_comment(void) { __asm { int 3 } return 0; }
extern "C" int __cdecl skip_language_comment(void)
{
    BYTE character;
    int result;
    character = *c1_bss.source_cursor++;
    if (c1_data.scanner.classes[character] == SC_SPECIAL)
        character = (BYTE)resolve_special_character(character);
    if (character != '/') {
        --c1_bss.source_cursor;
        character = *c1_bss.source_cursor++;
        if (c1_data.scanner.classes[character] == SC_SPECIAL)
            character = (BYTE)resolve_special_character(character);
        if (character != '*') {
            --c1_bss.source_cursor;
            return 0;
        }
        result = 1;
    } else {
        character = *c1_bss.source_cursor++;
        if (c1_data.scanner.classes[character] == SC_SPECIAL)
            character = (BYTE)resolve_special_character(character);
        if (character != '*') {
            --c1_bss.source_cursor;
            report_language_diagnostic(4, 1);
            lexer_flags |= 0x800;
            skip_line_comment();
            lexer_flags &= ~0x800U;
            return 1;
        }
        report_language_diagnostic(4, 179);
        result = 0;
    }
    lexer_flags |= 0x800;
    if (c1_bss.options.preserve_comments && !(lexer_flags & 0x8000))
        skip_preserved_macro_comment();
    else skip_macro_block_comment();
    lexer_flags &= ~0x800U;
    return result;
}
extern "C" void __cdecl refill_lexer_source(void);
extern "C" unsigned __cdecl process_trigraph(void)
{
    BYTE character, replacement;
read_second:
    character = *c1_bss.source_cursor++;
    if (character != '?') {
        if (!character && !lexical_skip_mode) {
            refill_lexer_source();
            c1_bss.source_cursor[-1] = '?';
            goto read_second;
        }
        --c1_bss.source_cursor;
        return 0x40;
    }
read_third:
    character = *c1_bss.source_cursor++;
    switch (character) {
    case 0:
        if (lexical_skip_mode) goto ordinary_question;
        refill_lexer_source();
        goto read_third;
    case '!': replacement = '|'; break;
    case '\'': replacement = '^'; break;
    case '(': replacement = '['; break;
    case ')': replacement = ']'; break;
    case '-': replacement = '~'; break;
    case '/': replacement = '\\'; break;
    case '<': replacement = '{'; break;
    case '=': replacement = '#'; break;
    case '>': replacement = '}'; break;
    default:
ordinary_question:
        --c1_bss.source_cursor;
        c1_bss.source_cursor[-1] = '?';
        --c1_bss.source_cursor;
        c1_bss.source_cursor[-1] = '?';
        return 0x40;
    }
    c1_bss.source_cursor[-1] = replacement;
    --c1_bss.source_cursor;
    return 1;
}
extern "C" unsigned char __cdecl read_multibyte_character(void);
extern "C" void __cdecl discard_input_character(void)
{
    BYTE character;
    for (;;) {
        character = *c1_bss.source_cursor++;
        switch (c1_data.scanner.classes[character]) {
        case SC_NEWLINE:
            --c1_bss.source_cursor;
            return;
        case SC_SPECIAL:
            if (process_special_character() == 1 && !*c1_bss.source_cursor) return;
            break;
        case SC_LEAD:
            read_multibyte_character();
            break;
        case SC_SLASH:
            if (c1_data.options.language_extensions || c1_data.options.preprocess_only) {
                character = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[character] == SC_SPECIAL)
                    character = (BYTE)resolve_special_character(character);
                if (character == '/') goto line_comment;
                --c1_bss.source_cursor;
                character = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[character] == SC_SPECIAL)
                    character = (BYTE)resolve_special_character(character);
                if (character != '*') {
                    --c1_bss.source_cursor;
                    break;
                }
                lexer_flags |= 0x800;
                if (c1_bss.options.preserve_comments && !(lexer_flags & 0x8000)) {
                    skip_preserved_macro_comment();
                    goto block_done;
                }
                goto block_comment;
            }
            character = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[character] == SC_SPECIAL)
                character = (BYTE)resolve_special_character(character);
            if (character == '/') {
                character = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[character] == SC_SPECIAL)
                    character = (BYTE)resolve_special_character(character);
                if (character != '*') {
                    --c1_bss.source_cursor;
                    report_language_diagnostic(4, 1);
                    goto line_comment;
                }
                report_language_diagnostic(4, 179);
            } else {
                --c1_bss.source_cursor;
                character = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[character] == SC_SPECIAL)
                    character = (BYTE)resolve_special_character(character);
                if (character != '*') {
                    --c1_bss.source_cursor;
                    break;
                }
            }
            lexer_flags |= 0x800;
            if (c1_bss.options.preserve_comments && !(lexer_flags & 0x8000)) {
                skip_preserved_macro_comment();
                goto block_done;
            }
block_comment:
            skip_macro_block_comment();
block_done:
            lexer_flags &= ~0x800U;
            break;
line_comment:
            lexer_flags |= 0x800;
            skip_line_comment();
            lexer_flags &= ~0x800U;
            break;
        }
    }
}
extern "C" int __cdecl splice_input_line(void)
{
    unsigned character = *c1_bss.source_cursor++;
    for (;;) {
        if (!character) {
            process_special_character();
            c1_bss.source_cursor[-1] = '\\';
        } else if (character == '\n') {
            ++c1_bss.current_context.value;
            if (c1_bss.options.preprocess_stdout) {
                if (parser_flags & 0x4000)
                    fputc('\\', c1_bss.preprocessor_output);
                fputc('\n', c1_bss.preprocessor_output);
            }
            return 1;
        } else if (character != '\r') {
            --c1_bss.source_cursor;
            return 0;
        }
        character = *c1_bss.source_cursor++;
    }
}


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
extern "C" int __cdecl scan_identifier_continuation(const BYTE *start, unsigned hash,
    unsigned incoming)
{
    BYTE spelling[2048], marker = 0;
    BYTE character = (BYTE)incoming;
    unsigned length = (unsigned)(c1_bss.source_cursor - start - 1);
    unsigned folded;
    CompilerNameEntry **bucket, **link, *entry;
    CompilerPool *pool;
    CompilerArena *arena;
    if (length > sizeof(spelling)) {
#ifdef __linux__
        recon_unsupported("identifier prefix exceeds its original stack buffer");
#else
        __asm { int 3 }
#endif
    }
    memcpy(spelling, start, length);
    for (;;) {
        if (!character) {
            if ((int)lexical_skip_mode > 0) break;
            if (read_source_chunk()) compiler_fatal(4);
        } else if (c1_data.scanner.classes[character] == 41) {
            marker = *c1_bss.source_cursor++;
        } else if (character == '?') {
            if (!(process_trigraph() & ~0x40U)) break;
        } else if (character == '\\') {
            if (!splice_input_line()) break;
        } else break;
read_character:
        character = *c1_bss.source_cursor++;
        if (length >= sizeof(spelling)) compiler_fatal(64);
        if (c1_data.scanner.flags[character] & 3) {
            spelling[length++] = character;
            hash = (hash >> 4) + 4 * hash + character;
            goto read_character;
        }
    }
    --c1_bss.source_cursor;
    folded = ((hash >> 16) ^ hash) & 0xffff;
    bucket = &c1_data.name_buckets[hash & 0x7ff];
    link = bucket;
    for (entry = *link; entry; link = &entry->next, entry = *link) {
        if ((entry->flags & 0xffff) == folded && entry->name[length] == 0 &&
            memcmp(spelling, entry->name, length) == 0) {
            *link = entry->next;
            entry->next = *bucket;
            *bucket = entry;
            c1_data.current_identifier = entry;
            return marker == length;
        }
    }
    pool = &c1_bss.pools[4];
    if (pool->remaining < 16) pool->Refill(16);
    entry = (CompilerNameEntry *)pool->cursor;
    pool->cursor += 16;
    pool->remaining -= 16;
    entry->next = *bucket;
    *bucket = entry;
    arena = &c1_bss.arenas[0];
    entry->name = (char *)arena->cursor;
    arena->cursor += (length + 4) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    memcpy(entry->name, spelling, length);
    entry->name[length] = 0;
    c1_data.current_identifier = entry;
    /* Only the low hash WORD is written, retaining upper flags and payload. */
    entry->flags = (entry->flags & 0xffff0000U) | folded;
    return marker == length;
}

extern "C" CompilerParserToken *__cdecl scan_floating_token(char *cursor)
{
    BYTE character = read_numeric_character();
    CompilerType *type;
    CompilerParserToken *token;
    InitializerExtendedReal real;
    CompilerArena *arena;
    CompilerPool *pool;
    unsigned arena_kind;
    if (c1_bss.floating_mantissa_missing) {
        if (!c1_bss.macro_active_count) report_language_diagnostic(1, 12);
        c1_bss.floating_mantissa_missing = 0;
    }
    if (c1_data.scanner.classes[character] == SC_DIGIT || character == '.') {
        do {
            *cursor++ = character;
            if (cursor > floating_token_text + 128) compiler_fatal(64);
            character = read_numeric_character();
        } while (c1_data.scanner.classes[character] == SC_DIGIT);
    }
    if ((character & 0xdf) == 'E') {
        *cursor++ = character;
        character = read_numeric_character();
        if (character == '+' || character == '-') {
            *cursor++ = character;
            character = read_numeric_character();
        }
        if (c1_data.scanner.classes[character] != SC_DIGIT) {
            if (!c1_bss.macro_active_count) compiler_warning(21, character);
            *cursor++ = '0';
        } else {
            do {
                *cursor++ = character;
                character = read_numeric_character();
            } while (c1_data.scanner.classes[character] == SC_DIGIT);
        }
    }
    if ((character & 0xdf) == 'F') {
        type = c1_data.builtin_types.type_0106;
        *cursor++ = character;
    } else if ((character & 0xdf) == 'L') {
        type = c1_data.builtin_types.type_0108;
        *cursor++ = character;
    } else {
        --c1_bss.source_cursor;
        type = c1_data.builtin_types.type_0107;
    }
    if (c1_data.scanner.flags[*c1_bss.source_cursor] & 1)
        compiler_warning(59, 77, 306);
    *cursor++ = 0;
    arena_kind = c1_bss.symbol_arena_kind;
    if (arena_kind >= 6) {
#ifdef __linux__
        recon_unsupported("floating token arena exceeds the encountered owners");
#else
        __asm { int 3 }
#endif
    }
    if (c1_bss.macro_active_count) {
        c1_bss.macro_text_cursor = (BYTE *)cursor;
        if (!arena_kind) token = (CompilerParserToken *)c1_bss.pools[0].AllocateBytes(32);
        else {
            arena = &c1_bss.arenas[arena_kind];
            token = (CompilerParserToken *)arena->cursor;
            arena->cursor += 32;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        }
        if (token) {
            token->kind = 136;
#ifdef __linux__
            recon_unsupported("macro floating token requires its native payload view");
#else
            memcpy(&token->value, &c1_bss.literal, 24);
#endif
        }
        return token;
    }
    if ((c1_bss.options.preprocess_stdout || (lexer_flags & 0x800000)) &&
        !(lexer_flags & 0x4000)) {
        copy_numeric_spelling(&c1_bss.literal, floating_token_text,
            (WORD)(cursor - floating_token_text), arena_kind == 3 ? 0 : arena_kind);
        if (arena_kind) {
            arena = &c1_bss.arenas[arena_kind];
            token = (CompilerParserToken *)arena->cursor;
            arena->cursor += 32;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        } else {
            pool = &c1_bss.pools[0];
            if (pool->remaining < 32) pool->Refill(32);
            token = (CompilerParserToken *)pool->cursor;
            pool->cursor += 32;
            pool->remaining -= 32;
        }
        if (token) {
            token->kind = 136;
#ifdef __linux__
            token->value.string_attributes = c1_bss.literal.string_attributes;
#else
            memcpy(&token->value, &c1_bss.literal, 24);
#endif
        }
        return token;
    }
    if (parse_initializer_real(floating_token_text, &real)) {
        compiler_warning(177);
        /* The original parse-error fallback is the initialized extended zero. */
        memcpy(&real, &extended_zero, 12);
    }
    c1_bss.literal.real_constant.type = type;
    c1_bss.literal.real_constant.value = (InitializerExtendedReal *)copy_extended_constant((BYTE *)&real, 0);
    if (arena_kind) {
        arena = &c1_bss.arenas[arena_kind];
        token = (CompilerParserToken *)arena->cursor;
        arena->cursor += 32;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &c1_bss.pools[0];
        if (pool->remaining < 32) pool->Refill(32);
        token = (CompilerParserToken *)pool->cursor;
        pool->cursor += 32;
        pool->remaining -= 32;
    }
    if (token) {
        token->kind = 101;
#ifdef __linux__
        token->value.real_constant.type = c1_bss.literal.real_constant.type;
        token->value.real_constant.value = c1_bss.literal.real_constant.value;
#else
        memcpy(&token->value, &c1_bss.literal, 24);
#endif
    }
    return token;
}
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
extern "C" int __cdecl open_forced_include(const char *, int);
extern "C" CompilerParserToken *__cdecl make_source_state_token(void *);
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
        c1_bss.forced_include_name = (const char *)files->items[c1_bss.forced_include_index++];
        if (open_forced_include(c1_bss.forced_include_name, 0) <= -1)
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
            if (frame->second) {
#ifdef __linux__
                recon_unsupported("forced-include frame callback ABI not encountered");
#else
                __asm { int 3 }
#endif
                ((void (__cdecl *)(void))frame->second)();
            }
        } else recorded = NULL;
        c1_bss.current_context.Push((CompilerLexer *)recorded, 2, 1, 0,
                                    &c1_bss.current_context);
        saved = c1_bss.saved_symbol_arenas;
        c1_bss.symbol_arena_kind = saved->arena_kind;
        c1_bss.saved_symbol_arenas = saved->next;
        return 1;
    }
}
CompilerRecordedLexer *RECON_THISCALL CompilerRecordedLexer::Initialize(unsigned arena, int enabled)
{
    return new(this) CompilerRecordedLexer(c1_bss.current_context.object,
        c1_bss.current_context.value, arena, enabled);
}
/* A location record and end marker are recorded into a temporary token
 * stream; the returned token hands that stream back to the caller. */
extern "C" CompilerParserToken *__cdecl make_source_state_token(void *object)
{
    unsigned arena_kind = c1_bss.symbol_arena_kind;
    CompilerRecordedLexer *recorded = (CompilerRecordedLexer *)
        c1_bss.arenas[arena_kind].AllocateBytes(64);
    CompilerParserToken *token;
    CompilerContextFrame *frame;
    CompilerTokenStream *stream;
    CompilerPool *pool;
    if (recorded) recorded = recorded->Initialize(arena_kind, 0);
    c1_bss.context.Push((CompilerLexer *)recorded, arena_kind, 0, 0, &c1_bss.current_context);
    c1_bss.literal.context.object = object;
    if (c1_bss.symbol_arena_kind)
        token = (CompilerParserToken *)c1_bss.arenas[c1_bss.symbol_arena_kind].AllocateBytes(32);
    else token = (CompilerParserToken *)c1_bss.pools[6].AllocateBytes(32);
    if (token) {
        token->kind = 44;
        memcpy(&token->value, &c1_bss.literal, 24);
    }
    c1_bss.context.saved_frame->stream->Trace((unsigned)token);
    c1_bss.context.saved_frame->stream->Trace((unsigned)c1_data.predefined_tokens[45]);
    frame = c1_bss.context.saved_frame;
    if (frame) {
        stream = frame->stream;
        c1_bss.context.saved_frame = frame->next;
        c1_bss.context.object = frame->object;
        c1_bss.context.value = frame->value;
        if (frame->second) {
#ifdef __linux__
            recon_unsupported("source-state callback ABI has not been encountered");
#else
            __asm { int 3 }
#endif
            ((void (__cdecl *)(void))frame->second)();
        }
    } else stream = NULL;
    c1_bss.literal.recorded_stream = stream;
    if (c1_bss.symbol_arena_kind) {
        CompilerArena *arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
        token = (CompilerParserToken *)arena->cursor;
        arena->cursor += 32;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &c1_bss.pools[6];
        if (pool->remaining < 32) pool->Refill(32);
        token = (CompilerParserToken *)pool->cursor;
        pool->cursor += 32;
        pool->remaining -= 32;
    }
    if (token) {
        token->kind = 0xf2;
        /* The authentic24-byte transfer retains all caller-owned payload
         * bytes. Only the first pointer is interpreted by this owner. */
        memcpy(&token->value, &c1_bss.literal, 24);
    }
    return token;
}
extern "C" int __cdecl skip_newline_comment(void)
{
    BYTE character = *c1_bss.source_cursor++;
    if (!c1_data.scanner.classes[character]) resolve_special_character(character);
    if (character == '/') {
        lexer_flags |= 0x800;
        skip_line_comment();
    } else {
        --c1_bss.source_cursor;
        character = *c1_bss.source_cursor++;
        if (!c1_data.scanner.classes[character]) resolve_special_character(character);
        if (character != '*') { --c1_bss.source_cursor; return 0; }
        lexer_flags |= 0x800;
        if (c1_bss.options.preserve_comments && !(lexer_flags & 0x8000))
            skip_preserved_macro_comment();
        else skip_macro_block_comment();
    }
    lexer_flags &= ~0x800U;
    return 1;
}

extern "C" void __cdecl scan_class192_attributes(CompilerDeclarationAttributes *);

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
            attributes_seen = 1;
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
extern "C" CompilerParserToken *__cdecl read_tag_specifier(WORD, CompilerDeclarationAttributes *);
extern "C" CompilerType *__cdecl reference_tag_type(CompilerIrSymbol *);
int RECON_THISCALL CompilerDeclarationScanner::ScanClass27(void)
{
    WORD tag_flags;
    CompilerParserToken *declaration;
    /* WORD column at63e68+4*kind: enum, struct and union. The adjacent
     * token properties are independent and remain outside this relation. */
    switch (token->kind) {
    case 27: tag_flags = 0x0200; break;
    case 28: tag_flags = 0x0402; break;
    case 29: tag_flags = 0x0404; break;
    default:
#ifdef __linux__
        recon_unsupported("tag specifier outside the recovered relation");
#else
        __asm { int 3 }
#endif
        tag_flags = 0; /* Unreachable after the unsupported-domain trap. */
    }
    lexer_flags |= 0x80000;
    declaration = read_tag_specifier(tag_flags, &attributes);
    lexer_flags &= ~0x80000U;
    if (declaration->kind == 1) {
        token = c1_bss.current_context.ReadToken();
        token = recover_parser_token(token, 127, 1);
        return 2;
    }
    if (type || base_type_flags) {
        compiler_warning(236, 76, token->kind, declaration->value.tag_declaration.symbol);
        base_type_flags = 0;
    }
    declaration_context = declaration->value.tag_declaration.symbol;
    declaration_callback = declaration->value.tag_declaration.apply;
    type = reference_tag_type(declaration->value.tag_declaration.symbol);
    if (declaration->kind == 35) lexer_flags |= 1;
    else if (declaration->kind != 34) return 0;
    Normalize();
    if (attributes.flags & 0x400) compiler_warning(391, declaration_context);
    token = BuildToken(declaration->kind);
    return 2;
}
/* The original arena2 object is36 bytes. Every explicit constructor store
 * is retained; the three padding bytes after accept_name stay unwritten.
 * Native storage grows to40 only for its eight-byte member pointer. */
struct CompilerTagSpecifier {
    CompilerNameEntry *name;
    CompilerIrSymbol *symbol;
    CompilerParserToken *name_token;
    WORD type_flags, declaration_flags;
    CompilerDeclarationAttributes attributes;
    CompilerDeclarationAttributes *outer_attributes;
    CompilerDeclarationApply apply; /* Original+28/native+28, wider natively. */
    BYTE accept_name;
    BYTE padding_21[3];
    int RECON_THISCALL AcceptName(CompilerParserToken *);
    int RECON_THISCALL ReadAttributes(void);
    CompilerParserToken *RECON_THISCALL ReadDefinition(CompilerParserToken *);
    CompilerParserToken *RECON_THISCALL Finish(CompilerParserToken *);
};
#ifdef RECON_NATIVE_TEST
typedef char CompilerTagSpecifier_extent[sizeof(CompilerTagSpecifier) == 40 ? 1 : -1];
typedef char CompilerTagSpecifier_accept_offset[offsetof(CompilerTagSpecifier, accept_name) == 36 ? 1 : -1];
#else
typedef char CompilerTagSpecifier_extent[sizeof(CompilerTagSpecifier) == 36 ? 1 : -1];
typedef char CompilerTagSpecifier_accept_offset[offsetof(CompilerTagSpecifier, accept_name) == 32 ? 1 : -1];
#endif
typedef char CompilerTagSpecifier_attributes[offsetof(CompilerTagSpecifier, attributes) == 16 ? 1 : -1];
extern "C" CompilerParserToken *__cdecl read_tag_specifier(WORD type_flags,
                                                          CompilerDeclarationAttributes *attributes)
{
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerTagSpecifier *specifier = (CompilerTagSpecifier *)arena->cursor;
    CompilerParserToken *token;
    unsigned token_class;
    arena->cursor += sizeof(CompilerTagSpecifier);
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (specifier) {
        specifier->type_flags = type_flags;
        specifier->outer_attributes = attributes;
        specifier->name = NULL;
        specifier->symbol = NULL;
        specifier->name_token = NULL;
        specifier->declaration_flags = 0;
        specifier->attributes.flags = 0;
        specifier->attributes.extended_flags = 0;
        specifier->attributes.extra = 0;
        specifier->apply = 0;
        specifier->accept_name = 1;
    }
    token = c1_bss.current_context.ReadToken();
    for (;;) {
        if (token->kind >= PARSER_TOKEN_KIND_COUNT) {
#ifdef __linux__
            recon_unsupported("tag-reader token class is not yet recovered");
#else
            __asm { int 3 }
#endif
        }
        token_class = parser_token_classes[token->kind];
        switch (token_class) {
        case PARSER_CLASS_0:
            compiler_fatal(4);
            return specifier->Finish(token);
        case PARSER_CLASS_6:
            if (!specifier->AcceptName(token)) return specifier->Finish(token);
            break;
        case PARSER_CLASS_117:
            return specifier->ReadDefinition(token);
        case PARSER_CLASS_126:
            c1_bss.current_context.saved_frame->stream->Unread((unsigned)token);
            return specifier->Finish(token);
        case PARSER_CLASS_192:
            if (specifier->ReadAttributes()) break;
            /* A later attribute after an established tag is left unread. */
        default:
            c1_bss.current_context.saved_frame->stream->Unread((unsigned)token);
            return specifier->Finish(token);
        }
        token = c1_bss.current_context.ReadToken();
    }
}
int RECON_THISCALL CompilerTagSpecifier::AcceptName(CompilerParserToken *token)
{
    if (!name && !symbol) {
        name_token = token;
        name = token->value.name;
        return 1;
    }
    c1_bss.current_context.saved_frame->stream->Unread((unsigned)token);
    accept_name = 0;
    return 0;
}
int RECON_THISCALL CompilerTagSpecifier::ReadAttributes(void)
{ __asm { int 3 } return 0; }
extern "C" CompilerIrSymbol *__cdecl create_tag_definition(CompilerNameEntry *, WORD, CompilerDeclarationApply *);
extern "C" CompilerIrSymbol *__cdecl begin_existing_tag_definition(CompilerIrSymbol *, WORD);
extern "C" const char *__cdecl tag_type_name(WORD);
CompilerParserToken *RECON_THISCALL CompilerTagSpecifier::ReadDefinition(CompilerParserToken *token)
{
    const char *old_name;
    CompilerParserToken *result;
    CompilerArena *arena;
    CompilerPool *pool;
    c1_bss.current_context.saved_frame->stream->Unread((unsigned)token);
    if (symbol) {
        old_name = symbol->binding.name->name;
        symbol = begin_existing_tag_definition(symbol, type_flags);
    } else {
        old_name = name ? name->name : NULL;
        symbol = create_tag_definition(name, type_flags, &apply);
    }
    if (!symbol) {
        compiler_warning(649, old_name, tag_type_name(type_flags));
        return (CompilerParserToken *)c1_data.predefined_tokens[1];
    }
    if (symbol->scope_bits & 1)
        return (CompilerParserToken *)c1_data.predefined_tokens[1];
    c1_bss.literal.declaration.apply = apply;
    c1_bss.literal.declaration.unmodeled_0c = symbol;
    if (type_flags == 0x200) {
        lexer_flags |= 1;
        if (c1_bss.symbol_arena_kind) {
#ifdef RECON_NATIVE_TEST
            if (c1_bss.symbol_arena_kind >= 6)
                recon_unsupported("enum token arena is not modeled");
#endif
            result = (CompilerParserToken *)c1_bss.arenas[c1_bss.symbol_arena_kind].AllocateBytes(32);
        } else {
            pool = &c1_bss.pools[6];
            if (pool->remaining < 32) pool->Refill(32);
            result = (CompilerParserToken *)pool->cursor;
            pool->cursor += 32;
            pool->remaining -= 32;
        }
    } else {
        if (c1_bss.symbol_arena_kind) {
#ifdef RECON_NATIVE_TEST
            if (c1_bss.symbol_arena_kind >= 6)
                recon_unsupported("aggregate token arena is not modeled");
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
    }
    if (result) {
        result->kind = type_flags == 0x200 ? 35 : 34;
        /* Both value owners contain the authentic24-byte span. Opaque
         * retained bytes are copied without assigning new field meanings. */
        memcpy(&result->value, &c1_bss.literal, 24);
    }
    return result;
}
extern "C" CompilerIrSymbol *__cdecl find_or_create_tag(CompilerNameEntry *, WORD, CompilerDeclarationApply *);
extern "C" CompilerIrSymbol *__cdecl create_tag_definition(CompilerNameEntry *name, WORD flags,
                                                          CompilerDeclarationApply *apply)
{
    *apply = &CompilerTranslationUnit::InsertDeclaration;
    return find_or_create_tag(name, flags, apply);
}
void RECON_THISCALL CompilerTranslationUnit::InsertDeclaration(void *declaration)
{
    current->Insert((CompilerIrSymbol *)declaration);
}
extern "C" CompilerIrSymbol *__cdecl create_tag_symbol(CompilerNameEntry *, WORD, CompilerDeclarationApply *, int);
extern "C" CompilerIrSymbol *__cdecl find_or_create_tag(CompilerNameEntry *name, WORD flags,
                                                      CompilerDeclarationApply *apply)
{
    CompilerIrSymbol *symbol;
    CompilerIrSymbol **found = name ? c1_data.translation_unit->FindTag(name, 0) : NULL;
    if (found) {
        symbol = *found;
        *apply = 0;
        if (symbol->flags & 1) {
            compiler_warning(11, symbol, symbol->abstract_type);
            symbol->scope_bits |= 1;
            return symbol;
        }
    } else {
        symbol = create_tag_symbol(name, flags, apply, 1);
        if (!symbol) return NULL;
    }
    return begin_existing_tag_definition(symbol, flags);
}
CompilerIrSymbol **RECON_THISCALL CompilerTranslationUnit::FindTag(CompilerNameEntry *name, int root_only)
{
    CompilerScopeTable *scope = root_only ? root : current;
    return (CompilerIrSymbol **)scope->Lookup(name);
}
extern "C" CompilerIrSymbol *__cdecl allocate_tag_symbol(unsigned);
extern "C" void *__cdecl create_enum_context(unsigned, unsigned, unsigned, unsigned, int, int);
extern "C" CompilerIrSymbol *__cdecl create_tag_symbol(CompilerNameEntry *name, WORD flags,
                                                      CompilerDeclarationApply *apply, int define)
{
    CompilerIrSymbol *symbol = allocate_tag_symbol(0);
    CompilerArena *arena;
    CompilerType *description;
    if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
    symbol->record_id = (symbol->record_id & 0xff000000U) |
                       (c1_data.next_symbol_record_id++ & 0xffffff);
    symbol->scope_bits = (symbol->scope_bits & ~2) | (parameter_nesting ? 2 : 0);
    arena = &c1_bss.arenas[3];
    description = (CompilerType *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    /* This owner writes only flags and symbol_owner in the16-byte type.
     * Other bytes retain their allocator-provided storage, without invented
     * field values, zeroing or full-description equality. */
    description->flags = flags;
    if (flags & 0x200) {
        description->flags |= 3;
        symbol->kind4_context = create_enum_context(4, ((lexer_flags >> 17) & 1) * 2,
                                                   0, 0, 1, 1);
    }
    description->symbol_owner = symbol;
    symbol->value_type = intern_type(description);
    if (name) {
        symbol->binding.name = name;
        if (declaration_depth > 0) {
            report_language_diagnostic(c1_data.options.language_extensions ? 4 : 1, 115, symbol);
            if (!define && c1_data.options.language_extensions) {
                *apply = 0;
                c1_data.translation_unit->DeclarationScope()->Insert(symbol);
            }
        }
    } else {
        if (declaration_depth > 0) report_language_diagnostic(1, 116);
        symbol->binding.name = c1_data.builtin_names.unnamed;
    }
    if (c1_bss.browser_filename && (!parameter_nesting || browser_parameter_references))
        c1_data.backend->Slot20(symbol);
    c1_bss.tag_declaration_seen = 1;
    return symbol;
}
extern "C" CompilerIrSymbol *__cdecl allocate_tag_symbol(unsigned arena_kind)
{
    CompilerIrSymbol *symbol = NULL;
    CompilerSymbolBinding *binding;
    if (c1_data.record_sizes.known.tag_symbol_bytes != 52 ||
        (!arena_kind && c1_data.tag_symbol_cache_index != 10)) {
#ifdef __linux__
        recon_unsupported("tag symbol size/cache domain is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    if (!arena_kind) {
        binding = c1_data.symbol_cache.indexed[c1_data.tag_symbol_cache_index];
        if (binding) {
            symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
            c1_data.symbol_cache.indexed[c1_data.tag_symbol_cache_index] = binding->next;
            binding->name = NULL;
            binding->next = NULL;
            memset(symbol, 0, c1_data.record_sizes.known.tag_symbol_bytes);
            binding->name = NULL;
            symbol->kind = 4;
        }
    }
    if (!symbol) {
#ifdef RECON_NATIVE_TEST
        if (arena_kind >= 6) recon_unsupported("tag symbol arena is not modeled");
#endif
        BYTE *memory = c1_bss.arenas[arena_kind].AllocateBytes(
            (c1_data.record_sizes.known.tag_symbol_bytes + 3) & ~3U);
        symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(4, NULL, arena_kind) : NULL;
    }
    if ((lexer_flags & 0x20000) || c1_data.pch_separate_debug)
        symbol->flags &= ~2;
    else symbol->flags |= 2;
    return symbol;
}
extern "C" void *__cdecl create_enum_context(unsigned capacity, unsigned arena_kind,
                                            unsigned hash_kind, unsigned match_kind, int first_flag, int second_flag)
{
#ifdef RECON_NATIVE_TEST
    if (arena_kind >= 6) recon_unsupported("enum context arena is not modeled");
#endif
    CompilerArena *arena = &c1_bss.arenas[arena_kind];
    CompilerSymbolMap *context = (CompilerSymbolMap *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    context->match_kind = match_kind;
    context->hash_kind = hash_kind;
    context->capacity = (WORD)capacity;
    context->arena_kind = arena_kind;
    context->flags = (context->flags & ~3) | (first_flag & 1) | ((second_flag & 1) << 1);
    CompilerSymbolMapEntry **symbols = (CompilerSymbolMapEntry **)arena->cursor;
    arena->cursor += (capacity * 4 + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    context->symbols = symbols;
    context->count = 0;
    return context;
}
CompilerScopeTable *RECON_THISCALL CompilerTranslationUnit::DeclarationScope(void)
{ __asm { int 3 } return NULL; }
extern "C" CompilerIrSymbol *__cdecl match_tag_type(CompilerIrSymbol *, WORD);
extern "C" CompilerIrSymbol *__cdecl begin_existing_tag_definition(CompilerIrSymbol *symbol, WORD flags)
{
    symbol = match_tag_type(symbol, flags);
    if (!symbol) return NULL;
    if (symbol->scope_bits & 1) return symbol;
    symbol->tag_definition_flags = (symbol->tag_definition_flags & ~0x200) |
                                  (c1_constant_diagnostic_mode >= 0 ? 0x200 : 0);
    if (flags != 0x200) return symbol;
    enumerator_value = -1;
    symbol->enum_base_width = c1_primitive_widths[3];
    symbol->flags |= 1;
    enumerator_type = symbol->value_type;
    c1_bss.current_tag_symbol = symbol;
    if (c1_bss.browser_filename && (!parameter_nesting || browser_parameter_references) &&
        symbol->binding.name != c1_data.builtin_names.unnamed) {
        c1_data.backend->Slot0c();
        c1_data.backend->Slot20(symbol);
        c1_data.backend->Slot30(symbol);
    }
    return symbol;
}
extern "C" CompilerIrSymbol *__cdecl match_tag_type(CompilerIrSymbol *symbol, WORD flags)
{
    CompilerType *type = symbol->value_type;
    if (((flags ^ type->flags) & 0x2fcf) &&
        !(flags == 0x200 && (type->flags & 0x200))) {
        compiler_warning(11, symbol, type);
        symbol->scope_bits |= 1;
    }
    return symbol;
}
extern "C" const char *__cdecl tag_type_name(WORD flags)
{
    if (flags & 0x40) {
        unsigned code = (flags & 0x4f) - 0x41;
        switch (code) {
        case 0: return "char";
        case 1: return "short";
        case 2: return "int";
        case 3: return "long";
        }
        /* The original retains these overlapping cases in its second switch.
         * The first three return in the preceding switch at this entry. */
        switch (code) {
        case 0: return "__int8";
        case 1: return "__int16";
        case 2: return "__int32";
        case 11: return "__int64";
        case 12: return "__int128";
        default: return "";
        }
    }
    if (flags & 0x100) {
        switch (flags) {
        case 0x106: return "float";
        case 0x107: return "double";
        case 0x108: return "long double";
        default: return "";
        }
    }
    if (flags & 0x400) {
        if (!(c1_data.declaration_format_flags & 1)) {
            if ((flags & 0x408) == 0x408) return "class";
            if ((flags & 0x402) == 0x402) return "struct";
            if ((flags & 0x404) == 0x404) return "union";
        }
        return "";
    }
    if (flags & 0x200) return "enum";
    if (flags & 0x80) return "void";
    if (flags & 0x800) return "__segment";
    return "";
}
extern "C" CompilerIrSymbol *__cdecl lookup_tag_reference(CompilerNameEntry *, WORD,
    CompilerDeclarationApply *);
extern "C" CompilerIrSymbol *__cdecl validate_tag_reference(CompilerIrSymbol *, WORD);
extern "C" void __cdecl merge_tag_reference_attributes(CompilerIrSymbol *,
    CompilerDeclarationAttributes *);
CompilerParserToken *RECON_THISCALL CompilerTagSpecifier::Finish(CompilerParserToken *token)
{
    CompilerIrSymbol *previous = NULL;
    CompilerParserToken *result;
    CompilerArena *arena;
    CompilerPool *pool;
    if (symbol) {
        previous = symbol;
        symbol = validate_tag_reference(symbol, type_flags);
    } else {
        apply = &CompilerTranslationUnit::InsertDeclaration;
        if (c1_constant_diagnostic_mode < 0 || token->kind == 127)
            symbol = lookup_tag_reference(name, type_flags, &apply);
        else
            symbol = lookup_tag_reference(name, type_flags, &apply);
    }
    c1_bss.literal.declaration.unmodeled_0c = symbol;
    if (!symbol) {
        compiler_warning(649, previous->binding.name->name, tag_type_name(type_flags));
        return (CompilerParserToken *)c1_data.predefined_tokens[1];
    }
    merge_tag_reference_attributes(symbol, &attributes);
    c1_bss.literal.declaration.apply = apply;
    if (c1_bss.symbol_arena_kind) {
        if (c1_bss.symbol_arena_kind >= 6) {
#ifdef __linux__
            recon_unsupported("tag reference token arena is not yet modeled");
#else
            __asm { int 3 }
#endif
        }
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
        result->kind = 36;
        /* The existing literal and token owners retain the authentic24-byte
         * transfer, including opaque bytes, without assigning new defaults. */
        memcpy(&result->value, &c1_bss.literal, 24);
    }
    return result;
}
extern "C" CompilerIrSymbol *__cdecl lookup_tag_reference(CompilerNameEntry *name, WORD flags,
    CompilerDeclarationApply *apply)
{
    CompilerIrSymbol **found = c1_data.translation_unit->FindVisibleTag(name);
    CompilerIrSymbol *symbol;
    if (found) {
        symbol = *found;
        *apply = 0;
    } else symbol = create_tag_symbol(name, flags, apply, 0);
    return validate_tag_reference(symbol, flags);
}
CompilerIrSymbol **RECON_THISCALL CompilerTranslationUnit::FindVisibleTag(CompilerNameEntry *name)
{
    BYTE index;
    CompilerIrSymbol **result;
    if (!name || name == c1_data.builtin_names.unnamed) return NULL;
    index = depth;
    do {
        if (index >= 128) {
#ifdef __linux__
            recon_unsupported("tag lookup exceeds the actual scope stack");
#else
            __asm { int 3 }
#endif
        }
        result = (CompilerIrSymbol **)scopes[index]->Lookup(name);
        if (result) {
            c1_bss.tag_declaration_seen = index == depth;
            return result;
        }
    } while (index-- != 0);
    c1_bss.tag_declaration_seen = 0;
    return NULL;
}
extern "C" CompilerIrSymbol *__cdecl validate_tag_reference(CompilerIrSymbol *symbol, WORD flags)
{
    if (!symbol) return NULL;
    symbol = match_tag_type(symbol, flags);
    if (c1_bss.browser_filename) {
        if (!parameter_nesting || browser_parameter_references)
            c1_data.backend->Slot1c(symbol);
    }
    return symbol;
}
extern "C" void __cdecl merge_tag_reference_attributes(CompilerIrSymbol *symbol,
    CompilerDeclarationAttributes *attributes)
{
    if (attributes->extended_flags & 12) {
        symbol->linkage_flags = (BYTE)((symbol->linkage_flags & ~4U) |
                                      (attributes->extended_flags & 4));
        symbol->linkage_flags = (BYTE)((symbol->linkage_flags & ~8U) |
                                      (attributes->extended_flags & 8));
    }
}
extern "C" CompilerType *__cdecl tag_value_type(CompilerIrSymbol *);
extern "C" CompilerType *__cdecl reference_tag_type(CompilerIrSymbol *symbol)
{
    CompilerType *type = symbol->value_type;
    if (symbol->kind == 4) return tag_value_type(symbol);
    if ((symbol->reference_flags & 3) < 3)
        symbol->reference_flags = (symbol->reference_flags & ~3U) |
                                  ((symbol->reference_flags + 1) & 3);
    symbol->reference_flags = (symbol->reference_flags & ~4U) |
                              ((c1_data.ir_reference_epoch & 1) << 2);
    return type;
}
extern "C" CompilerType *__cdecl tag_value_type(CompilerIrSymbol *symbol)
{
    CompilerType *type = symbol->value_type;
    return (type->flags & 0x200) ? c1_data.builtin_types.type_0063 : type;
}
void RECON_THISCALL CompilerDeclarationScanner::ScanClass30(void)
{
    CompilerIrSymbol *symbol = token->value.symbol;
    declaration_context = symbol;
    type = reference_tag_type(symbol);
}
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
        compiler_warning(628, declaration_context, declaration_base_types[i].name);
    } else if (!base_type_flags) {
        base_type_flags = declaration_base_types[i].flags;
    } else {
        base_type_flags = combine_base_type_flags(base_type_flags,
                                                  declaration_base_types[i].flags);
    }
}
extern "C" WORD __cdecl combine_base_type_flags(WORD previous, WORD added)
{
    unsigned signedness = previous & 0xc000, added_signedness = added & 0xc000;
    unsigned result, first, second;
    int invalid = 0;
    if (signedness && added_signedness) {
        if (signedness & added_signedness) report_language_diagnostic(1, 114);
        else { compiler_warning(42); added &= 0x3fff; }
        return previous | added;
    }
    if ((!signedness && added_signedness) || (added & 0x4000) ||
        signedness == (previous & 0xefff)) return previous | added;
    first = previous & 0x2fef;
    second = added & 0x2fef;
    result = first;
    if (first == 0x62) {
        result = 0x2062;
        invalid = second != 0x63;
    } else if (first == 0x64) {
        if (second == 0x63) result = 0x2064;
        else if (second == 0x106) {
            report_language_diagnostic(1, 215);
            result = 0x107;
        } else if (second == 0x107) result = 0x108;
        else invalid = 1;
    } else if (first == 0x80) compiler_warning(120);
    else if (first == 0x106 && second == 0x64) {
        report_language_diagnostic(1, 216);
        result = 0x107;
    } else if (first == 0x107 && second == 0x64) result = 0x108;
    else if ((first >= 0x41 && first <= 0x43) || first == 0x4c || first == 0x4d ||
             first == 0x61 || first == 0x63 || first == 0x106 || first == 0x107 ||
             first == 0x108 || first == 0x402 || first == 0x404 ||
             first == 0x2062 || first == 0x2064 || (first & 0x200)) invalid = 1;
    if (invalid) {
        const char *second_name = tag_type_name((WORD)second);
        const char *result_name = tag_type_name((WORD)result);
        compiler_warning(632, result_name, second_name);
    }
    return (WORD)(result | signedness);
}
extern "C" void __cdecl set_declaration_storage(CompilerDeclarationAttributes *, unsigned, unsigned, ...);
void RECON_THISCALL CompilerDeclarationScanner::ScanClass32(void)
{
    unsigned storage;
    if (parser_flags & 8) {
        compiler_warning(721, token->kind);
        attributes.flags |= 8;
        return;
    }
    /* First-byte storage column at63e68+4*kind: static9/storage3,
     * extern10/storage1 and typedef12/storage7. Other properties stay unknown. */
    if (token->kind != 12 && token->kind != 9 && token->kind != 10) {
#ifdef __linux__
        recon_unsupported("storage keyword outside the encountered static/extern/typedef column");
#else
        __asm { int 3 }
#endif
    }
    storage = token->kind == 12 ? 7 : token->kind == 10 ? 1 : 3;
    set_declaration_storage(&attributes, storage, 1);
    attributes_seen = 1;
}
extern "C" void __cdecl set_declaration_storage(CompilerDeclarationAttributes *attributes,
                                                unsigned storage, unsigned attribute, ...)
{
    BYTE diagnostic = 1;
    if (!(attributes->flags & 7))
        attributes->flags = (attributes->flags & ~7) | (storage & 7);
    else if ((BYTE)storage)
        compiler_warning(159);
    switch ((BYTE)attribute) {
    case 3:
    case 14:
        if (attributes->flags & 0x100) diagnostic = 14;
        else attributes->flags |= 0x100;
        break;
    case 4:
        c1_bss.debug_module->Slotb4();
        if (attributes->extended_flags & 1) diagnostic = 6;
        else attributes->extended_flags |= 1;
        break;
    case 5:
        c1_bss.debug_module->Slotb4();
        if (attributes->extended_flags & 2) diagnostic = 6;
        else attributes->extended_flags |= 2;
        break;
    case 6:
        c1_bss.debug_module->Slotb4();
        if (attributes->extended_flags & 0x0c) diagnostic = 6;
        else attributes->extended_flags |= 8;
        break;
    case 7:
        c1_bss.debug_module->Slotb4();
        if (attributes->extended_flags & 0x0c) diagnostic = 6;
        else attributes->extended_flags |= 4;
        break;
    case 8: {
        /* Only this attribute has a fourth cdecl argument. Three-argument
         * storage-keyword callers never read beyond their actual formals. */
        va_list arguments;
        CompilerIrSymbol *segment;
        va_start(arguments, attribute);
        segment = va_arg(arguments, CompilerIrSymbol *);
        va_end(arguments);
        if (attributes->extended_flags & 0x100) diagnostic = 6;
        else {
            if (segment) attributes->extended_flags |= 0x100;
            else attributes->extended_flags &= ~0x100;
            attributes->allocated_segment = segment;
        }
        break;
    }
    case 10:
        c1_bss.debug_module->Slotb4();
        if (attributes->extended_flags & 0x40) diagnostic = 6;
        else {
            attributes->extended_flags |= 0x40;
            attributes->flags |= 0x40;
        }
        break;
    }
    if (diagnostic != 1) {
        if ((BYTE)attribute == 8)
            report_language_diagnostic(1, 141, "allocate");
        else if (diagnostic == 6)
            report_language_diagnostic(1, 141, c1_data.current_identifier->name);
        else {
#ifdef __linux__
            recon_unsupported("declaration diagnostic spelling is not yet modeled");
#else
            __asm { int 3 }
#endif
            report_language_diagnostic(1, 141, c1_token_spellings.indexed[diagnostic]);
        }
    }
}
void RECON_THISCALL CompilerDeclarationScanner::ScanClass33(void) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarationScanner::ScanClass34(void)
{
    named_type_seen = 1;
    type = token->value.declaration.type;
    declaration_context = token->value.declaration.context;
    declaration_callback = token->value.declaration.apply;
    type_attributes = type->attributes;
    declaration_flags = token->value.declaration.flags;
    declaration_value = token->value.declaration.value;
}
int RECON_THISCALL CompilerDeclarationScanner::ScanClass40(void)
{
    unsigned i;
    if (!(parser_flags & 8)) {
        if (declaration_depth < 0 || declaration_depth > 2) {
#ifdef __linux__
            recon_unsupported("type qualifier declaration control index is not yet recovered");
#else
            __asm { int 3 }
#endif
        }
        if (((declaration_control[declaration_depth].flags & 2) || (lexer_flags & 4)) &&
            !named_type_seen) return 2;
    }
    for (i = 0; i < sizeof(parser_token_qualifiers)/sizeof(parser_token_qualifiers[0]); ++i)
        if (parser_token_qualifiers[i].kind == token->kind) break;
    if (i == sizeof(parser_token_qualifiers)/sizeof(parser_token_qualifiers[0])) {
#ifdef __linux__
        recon_unsupported("type qualifier token relation is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    type_attributes = merge_declarator_qualifiers(type_attributes, parser_token_qualifiers[i].flags);
    return 0;
}
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
    if (!type_attributes) {
        resolved_type = type;
        return;
    }
    /* A named type specifier suppresses this redundant qualifier check. */
    if (!named_type_seen &&
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
CompilerType *RECON_THISCALL CompilerType::WithAttributes(unsigned added)
{
    CompilerTypeShape *selected = shape ? shape->FindMatching(&CompilerTypeShape::IsNotArray) : NULL;
    if (!selected) {
        CompilerType description = *this;
        description.attributes |= (WORD)added;
        return intern_type(&description);
    }
    if ((selected->flags & 7) == 4) return this;
    CompilerTypeShape replacement = *selected;
    replacement.attributes |= (WORD)added;
    return WithShape(shape->Replace(selected, &replacement));
}
CompilerType *RECON_THISCALL CompilerType::WithShape(CompilerTypeShape *replacement)
{
    CompilerType copy = *this;
    copy.shape = replacement;
    return intern_type(&copy);
}
CompilerTypeShape *RECON_THISCALL CompilerTypeShape::Replace(CompilerTypeShape *selected,
    const CompilerTypeShape *replacement)
{
    if (this == selected) return intern_type_shape((CompilerTypeShape *)replacement);
    CompilerTypeShape copy = *this;
    copy.base = base->Replace(selected, replacement);
    return intern_type_shape(&copy);
}
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
    c1_bss.literal.declaration.unmodeled_0c = declaration_context;
    c1_bss.literal.declaration.apply = declaration_callback;
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
        result->value.declaration.apply = c1_bss.literal.declaration.apply;
#else
        memcpy(&result->value, &c1_bss.literal, 24);
#endif
    }
    cached_token = result;
    return result;
}
/* Reserved gaps/other selectors carry no defaults or semantic support. The
 * keyword name column is separately consumed by initial keyword registration. */
static CompilerIrSymbol *unmodeled_data_segment; /* RVA5e5b8, source-only fallback. */
static CompilerIrSymbol *unmodeled_code_segment; /* RVA5e5bc, source-only fallback. */

extern "C" void __cdecl scan_class192_attributes(CompilerDeclarationAttributes *attributes)
{
    CompilerParserToken *token = c1_bss.current_context.ReadToken();
    CompilerKeywordEntry *name;
    const CompilerDeclspecKeyword *entry;
    CompilerIrSymbol *segment;
    const char *spelling;
    unsigned code;
    int parentheses;
    if (token->kind != 118) {
        compiler_warning(54, token->kind);
        c1_bss.current_context.saved_frame->stream->Unread((unsigned)token);
        attributes->flags |= 8;
        return;
    }
    token = c1_bss.current_context.ReadToken();
    while (token->kind != 126) {
        if (token->kind != 6) {
            compiler_warning(59, 76, token->kind);
            attributes->flags |= 8;
            parentheses = 1;
            for (;;) {
                switch (token->kind) {
                case 109: case 127:
                    c1_bss.current_context.saved_frame->stream->Unread((unsigned)token);
                    return;
                case 118:
                    ++parentheses;
                    break;
                case 126:
                    if (!--parentheses) return;
                    break;
                default:
                    break;
                }
                token = c1_bss.current_context.ReadToken();
            }
        }
        name = (CompilerKeywordEntry *)token->value.name;
        if (!(name->flags & 0x800000)) {
            compiler_warning(485, name);
        } else if (name->flags & 0x10000) {
            set_declaration_storage(attributes, 0, name->token);
        } else {
            code = name->token;
            /* The actual sixteen-byte row selects attribute7 for dllexport
             * and attribute5 for thread; both have no argument. Other rows
             * remain unread, including their middle value column. */
            if (code != 1 && code != 4) {
#ifdef __linux__
                recon_unsupported("declspec semantic row is not recovered");
#else
                __asm { int 3 }
#endif
            }
            entry = &c1_declspec_keywords[code];
            if (entry->selector == 2) {
                set_declaration_storage(attributes, *(const BYTE *)&entry->unmodeled_value, 1);
            } else if (!entry->has_argument) {
                set_declaration_storage(attributes, 0, (BYTE)entry->selector);
            } else if (entry->selector == 8) {
                token = c1_bss.current_context.ReadToken();
                if (token->kind != 118) {
                    if (!C1_HAS_TOKEN_SPELLING(token->kind)) {
#ifdef __linux__
                        recon_unsupported("declspec diagnostic spelling is not recovered");
#else
                        __asm { int 3 }
#endif
                    }
                    compiler_warning(59, 115, c1_token_spellings.indexed[token->kind]);
                    attributes->flags |= 8;
                    recover_parser_token(token, 126, 1);
                } else {
                    token = c1_bss.current_context.ReadToken();
                    if (token->kind != 7) {
                        compiler_warning(59, 76, token->kind);
                        attributes->flags |= 8;
                        recover_parser_token(token, 126, 1);
                    } else {
                        spelling = token->value.string_attributes.text;
                        if (!strcmp(spelling, "_DATA")) {
                            segment = c1_data.segment_defaults.definition;
                            if (!segment) segment = unmodeled_data_segment;
                        } else if (!strcmp(spelling, "_CODE")) {
                            segment = c1_bss.current_scope_symbol;
                            if (segment && (segment->function_flags & 1))
                                segment = lookup_ir_symbol(segment);
                            else {
                                segment = (CompilerIrSymbol *)function_context;
                                if (!segment) segment = unmodeled_code_segment;
                            }
                        } else {
                            segment = c1_data.translation_unit->FindAllocatedSegment(
                                find_or_create_identifier(spelling));
                            if (!segment) {
                                compiler_warning(341, spelling);
                                attributes->flags |= 8;
                            }
                        }
                        set_declaration_storage(attributes, 0, 8, segment);
                        token = c1_bss.current_context.ReadToken();
                        if (token->kind != 126) {
                            if (!C1_HAS_TOKEN_SPELLING(token->kind)) {
#ifdef __linux__
                                recon_unsupported("declspec diagnostic spelling is not recovered");
#else
                                __asm { int 3 }
#endif
                            }
                            compiler_warning(59, 115, c1_token_spellings.indexed[token->kind]);
                            attributes->flags |= 8;
                        }
                    }
                }
            }
        }
        token = c1_bss.current_context.ReadToken();
    }
}
extern "C" CompilerIrSymbol *__cdecl find_named_symbol(CompilerSymbolBinding *, CompilerNameEntry *);
typedef char TranslationUnitSegmentsOffset[offsetof(CompilerTranslationUnit, symbols) == 532 ? 1 : -1];
#pragma intrinsic(strlen)
CompilerIrSymbol *RECON_THISCALL CompilerTranslationUnit::FindAllocatedSegment(CompilerNameEntry *name)
{
    if (strlen(name->name) > 127) name->name[127] = 0;
    if (symbols && name && name != c1_data.builtin_names.unnamed)
        return find_named_symbol(symbols, name);
    return NULL;
}
