#include <string.h>
#include <ctype.h>
#include "frontend.h"
#include "keywords.h"
#include "assembly-grammar.h"
#pragma intrinsic(memset, memcpy)
#include <recon.h>

extern "C" {
void __cdecl emit_control_record(int);
void __cdecl initialize_assembly_state(void);
int __cdecl parse_assembly_statements(void);
CompilerAssemblyOperand *__cdecl initialize_assembly_operand(unsigned);


/* The expression prefix is consumed only after its genuine builders return.
 * Kind is a WORD; payload+4 is an immediate or an outlined builder's operand. */

typedef char AssemblyExpression_payload[offsetof(CompilerAssemblyExpression, payload) == 4 ? 1 : -1];
#define ASM_EMPTY_TOKEN ((CompilerParserToken *)&c1_bss.assembly_empty_token)
#ifdef __linux__
#define ASM_UNSUPPORTED(reason) recon_unsupported(reason)
#else
#define ASM_UNSUPPORTED(reason) __asm { int 3 }
#endif
/* These contextual columns are not reconstructed data. Reserved storage is
 * representation only; every read is preceded by an unconditional semantic
 * guard in both builds. No zero/default or whole-column equality is claimed. */
static struct { WORD flags; BYTE unmodeled_02[2]; } unmodeled_assembly_type_properties[256]; /* RVA63e68 */
/* Encountered WORD54, DWORD55 and BYTE59 have width relations at67622. Prefix storage is
 * opaque representation; this does not establish the original table bound. */
static const union AssemblyRegisterWidths {
    struct { BYTE unmodeled_0_53[54], word_value, dword, unmodeled_56_58[3], byte_value; } known;
    BYTE indexed[60];
} assembly_register_widths = { { {0}, 1, 2, {0}, 0 } };
#define ASM_REGISTER_WIDTH(code, result) do { \
    int code_ = (short)(code); \
    if (code_ != 54 && code_ != 55 && code_ != 59) { ASM_UNSUPPORTED("assembler register width column is unmodeled"); } \
    (result) = assembly_register_widths.indexed[code_]; \
} while (0)
static const char *unmodeled_assembly_spellings[256]; /* RVA67780 */
extern const signed char c1_primitive_widths[16];
CompilerParserToken *__cdecl read_assembly_token(void);
void __cdecl report_assembly_expected(CompilerParserToken *, int);
CompilerParserValue *__cdecl make_assembly_recovery_value(CompilerParserValue *, int);
void __cdecl report_assembly_parse_error(const char *);
int __cdecl emit_assembly_statement(BYTE);
void __cdecl publish_assembly_label(CompilerNameEntry *);
CompilerParserNode *__cdecl make_symbol_node(BYTE, CompilerIrSymbol *, CompilerType *);
void __cdecl emit_ir_label(CompilerParserNode *);
void __cdecl emit_assembly_prefix(WORD, unsigned);
void __cdecl set_assembly_opcode(WORD);
void __cdecl finish_assembly_operands(void);
int __cdecl validate_assembly_operand_count(void);
int __cdecl validate_assembly_operand_forms(void);
void __cdecl select_assembly_encoding(void);
BYTE __cdecl select_assembly_operand_width(void);
BYTE __cdecl common_assembly_operand_width(void);
int __cdecl assembly_operand_accepts_width(CompilerAssemblyOperand *, BYTE);
int __cdecl assembly_operand_has_property(CompilerAssemblyOperand *, unsigned);
void __cdecl resolve_assembly_address_size(void);
void __cdecl report_assembly_operand_error(unsigned, unsigned);
int __cdecl assembly_should_swap_operands(void);
void __cdecl swap_assembly_pointer(void **, void **);
void __cdecl write_assembly_instruction(void);
unsigned __cdecl classify_assembly_operand(CompilerAssemblyValue *);
CompilerIrSymbol *__cdecl resolve_symbol_alias(CompilerIrSymbol *);
CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
unsigned __cdecl get_symbol_output_index(CompilerIrSymbol *);
int __cdecl assembly_immediate_fits(unsigned, unsigned);
void __cdecl emit_assembly_immediate(unsigned);
CompilerAssemblyExpression *__cdecl make_assembly_binary(BYTE, CompilerAssemblyExpression *, CompilerAssemblyExpression *);
CompilerAssemblyExpression *__cdecl combine_assembly_values(BYTE, CompilerAssemblyExpression *, CompilerAssemblyExpression *);
CompilerAssemblyExpression *__cdecl make_assembly_address(short, CompilerAssemblyExpression *, CompilerAssemblyExpression *);
CompilerAssemblyExpression *__cdecl make_assembly_selector(WORD, CompilerAssemblyExpression *);
CompilerAssemblyExpression *__cdecl make_assembly_integer(unsigned);
CompilerAssemblyExpression *__cdecl make_assembly_symbol(CompilerNameEntry *);
CompilerAssemblyExpression *__cdecl make_assembly_register(WORD);
CompilerAssemblyExpression *__cdecl make_assembly_register_expression(WORD);
void __cdecl select_assembly_width(BYTE);
void __cdecl set_assembly_address_mode(WORD);
unsigned __cdecl current_assembly_mnemonic(void);
CompilerIrSymbol *__cdecl resolve_assembly_symbol(CompilerNameEntry *, int);
unsigned __cdecl assembly_symbol_size(CompilerIrSymbol *);
unsigned __cdecl assembly_symbol_length(CompilerIrSymbol *);
unsigned __cdecl assembly_symbol_type(CompilerNameEntry *);
CompilerAssemblyExpression *__cdecl make_assembly_unary(BYTE, CompilerAssemblyExpression *);
CompilerAssemblyExpression *__cdecl dereference_assembly_value(CompilerAssemblyExpression *);
CompilerAssemblyExpression *__cdecl make_assembly_current_location(CompilerAssemblyExpression *);
CompilerAssemblyExpression *__cdecl qualify_assembly_register(WORD, CompilerType *);
BYTE __cdecl assembly_width_code(unsigned);
unsigned __cdecl global_type_size(CompilerType *);
CompilerType *__cdecl get_function_return_type(CompilerType *);
CompilerIrSymbol *__cdecl find_statement_label(CompilerNameEntry *);
CompilerIrSymbol *__cdecl make_statement_label(CompilerNameEntry *);
void __cdecl record_label_reference(CompilerIrSymbol *);
CompilerIrSymbol *__cdecl declare_implicit_function(CompilerNameEntry *, BYTE);
void __cdecl compiler_warning(int, ...);
void __cdecl compiler_fatal(int);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
short __cdecl lookup_assembly_opcode(CompilerNameEntry *);
short __cdecl lookup_assembly_register(CompilerNameEntry *);
int __cdecl compare_label_spelling(const char *, const char *);
/* Only the source-level indexed role is established. No instruction
 * encoding row/default is modeled until encountered; reads trap first. */
struct AssemblyInstructionEncoding { BYTE encoding, operand_encoding; };
/* Reached JE261/JLE265: canonical opcode15 with condition selectors4/14. The
 * preceding storage is opaque; no whole-column capacity/default is claimed. */
static const union AssemblyEncodingPrefix {
    struct { BYTE unmodeled_0_260[261 * 2]; AssemblyInstructionEncoding je;
             BYTE unmodeled_262_264[3 * 2]; AssemblyInstructionEncoding jle;
             BYTE unmodeled_266_296[31 * 2]; AssemblyInstructionEncoding movsd;
             BYTE unmodeled_298_388[91 * 2]; AssemblyInstructionEncoding setz; } known;
    AssemblyInstructionEncoding indexed[390];
} assembly_encoding_prefix = { { {0}, {15, 4}, {0}, {15, 14}, {0}, {56, 2}, {0}, {189, 4} } };
CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
void __cdecl handle_assembly_separator(void);
CompilerParserToken *__cdecl allocate_assembly_token(unsigned, unsigned);
CompilerParserToken *__cdecl scan_assembly_identifier(CompilerParserToken *);

void __cdecl parse_inline_assembly(void)
{
    c1_data.lexer_flags |= 0x200000;
    c1_bss.current_scope_symbol->qualifier_flags |= 0x10;
    emit_control_record(25);
    initialize_assembly_state();
    c1_bss.assembly_started = 1;
    parse_assembly_statements();
    c1_data.lexer_flags &= ~0x200000U;
    emit_control_record(26);
}

void __cdecl initialize_assembly_state(void)
{
    c1_bss.assembly.reset_54 = 0;
    c1_bss.assembly.reset_5c = 0;
    c1_bss.assembly.operand_index = 0;
    c1_bss.assembly.reset_08 = 0;
    c1_bss.assembly.reset_60 = 0;
    c1_bss.assembly.reset_9fc4 = 0;
    c1_bss.assembly.reset_9ffc = 0;
    c1_bss.assembly.current_operand = initialize_assembly_operand(0);
}
/* The assembler has its own grammar and 24-byte semantic stack. These are
 * logical shifts/reductions; yacc's packing indexes are not source data. */
int __cdecl parse_assembly_statements(void)
{
    short state = 0;
    int depth = 0, rule, terminal, i, nonterminal, found;
    const AssemblyParserState *info;
    const AssemblyParserGoto *go;
    const AssemblyParserTransition *relation;
    const AssemblyParserProduction *production;
    const AssemblyParserRecovery *repair;
    CompilerParserValue *right;
    CompilerParserValue repair_value;
    CompilerAssemblyExpression *expression;
    CompilerIrSymbol *symbol;
    unsigned flags, width;
    for (;;) {
        /* The authentic comparison permits state index150, whose 24-byte
         * value would overlap the adjacent operand owner. Only the complete
         * 150-value span before that overlap is represented here. */
        if (depth > 150) {
            report_assembly_parse_error("yacc stack overflow");
            return 1;
        }
        if (depth >= 150) {
            ASM_UNSUPPORTED("assembler semantic stack overlap domain");
        }
        c1_bss.assembly_states[depth] = state;
        c1_bss.assembly_values[depth] = c1_bss.parser_value;
read_action:
        if ((unsigned short)state >= ASSEMBLY_STATE_COUNT) {
            ASM_UNSUPPORTED("assembler state outside recovered grammar");
        }
        info = &assembly_states[state];
        if (info->needs_token) {
            if (c1_data.assembly_lookahead == ASM_EMPTY_TOKEN)
                c1_data.assembly_lookahead = read_assembly_token();
            terminal = c1_data.assembly_lookahead->kind;
            relation = assembly_shifts + info->first_shift;
            for (i = 0; i < info->shift_count; ++i, ++relation) {
                if (relation->symbol == terminal) {
                    state = relation->destination;
                    c1_bss.parser_value = c1_data.assembly_lookahead->value;
                    c1_data.assembly_lookahead = ASM_EMPTY_TOKEN;
                    if (c1_data.assembly_recovery_shifts > 0)
                        --c1_data.assembly_recovery_shifts;
                    ++depth;
                    goto push_state;
                }
            }
        }
        rule = info->default_production;
        if (info->exception_count) {
            if (c1_data.assembly_lookahead == ASM_EMPTY_TOKEN)
                c1_data.assembly_lookahead = read_assembly_token();
            terminal = c1_data.assembly_lookahead->kind;
            relation = assembly_exceptions + info->first_exception;
            for (i = 0; i < info->exception_count; ++i, ++relation)
                if (relation->symbol == terminal) { rule = relation->destination; break; }
        }
        if (rule < 0) return 0;
        if (!rule) {
            switch (c1_data.assembly_recovery_shifts) {
            case 0:
                repair = assembly_recoveries;
                for (i = 0; i < 2; ++i, ++repair)
                    if (repair->state >= state) break;
                if (i < 2 && repair->state == state) {
                    terminal = repair->terminal;
                    if (terminal < 0) { ++c1_data.assembly_recovery_shifts; terminal = -terminal; }
                    if (c1_data.assembly_recovery_shifts <= 1)
                        report_assembly_expected(c1_data.assembly_lookahead, terminal);
                    c1_bss.parser_value = *make_assembly_recovery_value(&repair_value, terminal);
                    state = repair->destination;
                    ++depth;
                    goto push_state;
                }
                report_assembly_parse_error("syntax error");
                ++c1_data.assembly_syntax_errors;
                /* Fall through to stack recovery. */
            case 1: case 2:
                c1_data.assembly_recovery_shifts = 3;
                for (;;) {
                    if (depth < 0) return 1;
                    info = &assembly_states[c1_bss.assembly_states[depth]];
                    relation = assembly_shifts + info->first_shift;
                    found = 0;
                    for (i = 0; i < info->shift_count; ++i, ++relation) {
                        if (relation->symbol == ASSEMBLY_ERROR) {
                            state = relation->destination;
                            found = 1;
                            break;
                        }
                    }
                    if (found) break;
                    --depth;
                }
                ++depth;
                goto push_state;
            case 3:
                if (!c1_data.assembly_lookahead->kind) return 1;
                c1_data.assembly_lookahead = ASM_EMPTY_TOKEN;
                goto read_action;
            default:
                /* The original falls into reduction0 outside recovery0..3. */
                break;
            }
        }
        if ((unsigned)rule > ASSEMBLY_PRODUCTION_COUNT) {
            ASM_UNSUPPORTED("assembler production outside recovered grammar");
        }
        production = &assembly_productions[rule];
        right = &c1_bss.assembly_values[depth];
        depth -= production->length;
        c1_bss.parser_value = c1_bss.assembly_values[depth + 1];
        nonterminal = production->nonterminal;
        go = &assembly_gotos[nonterminal];
        state = go->default_state;
        relation = assembly_goto_transitions + go->first_transition;
        for (i = 0; i < go->transition_count; ++i, ++relation)
            if (relation->symbol == c1_bss.assembly_states[depth]) { state = relation->destination; break; }
        switch (rule) {
        case 3: case 4:
            if (rule == 4) c1_data.assembly_recovery_shifts = 0;
            if (right[0].operation == 240) emit_control_record(36);
            c1_bss.arenas[3].ClearUsed();
            if (emit_assembly_statement(right[0].operation)) return 1;
            initialize_assembly_state();
            break;
        case 7: ++c1_bss.assembly.brace_depth; break;
        case 9: publish_assembly_label(right[-1].name); break;
        case 11: emit_assembly_prefix((WORD)right[-1].words.low, right[0].assembly.value); break;
        case 12:
            c1_bss.assembly.reset_08 = 1;
            set_assembly_opcode((WORD)right[0].words.low);
            break;
        case 13: finish_assembly_operands(); break;
        case 14: c1_bss.assembly.reset_60 = 0; break;
        case 15: case 49: c1_bss.assembly.reset_60 = 1; break;
        case 16:
            c1_bss.assembly.operand_index = 0;
            c1_bss.parser_value.unmodeled_00[8] = 0;
            c1_bss.parser_value.unmodeled_00[9] = 0;
            c1_bss.parser_value.unmodeled_00[10] = 0;
            c1_bss.parser_value.unmodeled_00[11] = 0;
            c1_bss.parser_value.unmodeled_00[12] = 0;
            c1_bss.parser_value.unmodeled_00[13] = 0;
            c1_bss.parser_value.unmodeled_00[14] = 0;
            c1_bss.parser_value.unmodeled_00[15] = 0;
            break;
        case 17:
            c1_bss.assembly.operand_index = 1;
            memcpy(&c1_bss.parser_value, &right[0], 16);
            break;
        case 20: case 21:
            set_assembly_opcode((WORD)right[0].words.low);
            finish_assembly_operands();
            break;
        case 23: ++c1_bss.assembly.operand_index; break;
        case 25:
            if ((int)c1_bss.assembly.operand_index < 2)
                c1_bss.assembly.current_operand = initialize_assembly_operand(++c1_bss.assembly.operand_index);
            break;
        case 27:
            expression = (CompilerAssemblyExpression *)right[0].pointer;
            if (expression->kind != 3) emit_assembly_immediate(expression->payload);
            break;
        case 28:
            expression = (CompilerAssemblyExpression *)right[0].pointer;
            if (!expression->kind)
                c1_bss.parser_value.pointer = make_assembly_binary((BYTE)right[-1].operation,
                    (CompilerAssemblyExpression *)right[-2].pointer, expression);
            else
                c1_bss.parser_value.pointer = make_assembly_binary((BYTE)right[-1].operation,
                    expression, (CompilerAssemblyExpression *)right[-2].pointer);
            break;
        case 29: case 30: case 31: case 32: case 33: case 34: case 35:
        case 36: case 37: case 38: case 39: case 40: case 41:
            c1_bss.parser_value.pointer = combine_assembly_values((BYTE)right[-1].operation,
                (CompilerAssemblyExpression *)right[-2].pointer, (CompilerAssemblyExpression *)right[0].pointer);
            break;
        case 42: case 43:
            c1_bss.parser_value.pointer = make_assembly_address((BYTE)right[-1].operation,
                (CompilerAssemblyExpression *)right[-2].pointer, (CompilerAssemblyExpression *)right[0].pointer);
            break;
        case 44:
            c1_bss.assembly.reset_68 = c1_bss.assembly.reset_04 = 1;
            expression = (CompilerAssemblyExpression *)right[-2].pointer;
            if (!expression->kind && expression->payload == 0xffff)
                set_assembly_address_mode(61);
            else if (!expression->kind && expression->payload == 0xfffe)
                set_assembly_address_mode(62);
            else compiler_warning(408, current_assembly_mnemonic());
            c1_bss.parser_value.pointer = right[0].pointer;
            break;
        case 45:
            c1_bss.assembly.reset_60 = 1;
            symbol = resolve_assembly_symbol(right[-1].name, 0);
            c1_bss.assembly.reset_4c = symbol->value_type->auxiliary;
            break;
        case 46: case 48: case 50: case 82:
            if (rule != 82) c1_bss.assembly.reset_60 = 0;
            c1_bss.parser_value.pointer = make_assembly_address((BYTE)right[-2].operation,
                (CompilerAssemblyExpression *)right[(rule == 50 || rule == 82) ? -3 : -4].pointer,
                (CompilerAssemblyExpression *)right[0].pointer);
            break;
        case 47:
            c1_bss.assembly.reset_60 = 1;
            c1_bss.assembly.reset_4c = right[-1].assembly.type->auxiliary;
            break;
        case 52: c1_bss.parser_value.pointer = right[-1].pointer; break;
        case 53: case 54: case 55: case 56: case 57:
            c1_bss.parser_value.pointer = make_assembly_unary((BYTE)right[-1].operation,
                (CompilerAssemblyExpression *)right[0].pointer);
            break;
        case 58:
            c1_bss.parser_value.pointer = make_assembly_selector((WORD)right[-2].words.low,
                (CompilerAssemblyExpression *)right[0].pointer);
            break;
        case 59:
            c1_bss.parser_value.pointer = make_assembly_integer(global_type_size(right[0].assembly.type));
            break;
        case 60:
            ASM_UNSUPPORTED("assembler token type-property column is unmodeled");
            flags = unmodeled_assembly_type_properties[right[0].operation].flags;
            width = (!(flags & 0x340) && (flags & ~15U)) ? flags & 15 :
                (int)c1_primitive_widths[flags & 15];
            c1_bss.parser_value.pointer = make_assembly_integer(width);
            break;
        case 61:
            ASM_REGISTER_WIDTH(right[0].words.low, width);
            c1_bss.parser_value.pointer = make_assembly_integer(width);
            break;
        case 62:
            symbol = resolve_assembly_symbol(right[0].name, 1);
            c1_bss.parser_value.pointer = make_assembly_integer(assembly_symbol_size(symbol));
            break;
        case 63:
            symbol = resolve_assembly_symbol(right[0].name, 0);
            c1_bss.parser_value.pointer = make_assembly_integer(global_type_size(symbol->value_type));
            break;
        case 64:
            symbol = resolve_assembly_symbol(right[0].name, 0);
            c1_bss.parser_value.pointer = make_assembly_integer(assembly_symbol_length(symbol));
            break;
        case 65: case 66:
            c1_bss.parser_value.pointer = make_assembly_integer(right[0].assembly.value);
            break;
        case 67:
            if (c1_bss.assembly.reset_60)
                c1_bss.parser_value.pointer = make_assembly_integer(assembly_symbol_type(right[0].name));
            else c1_bss.parser_value.pointer = make_assembly_symbol(right[0].name);
            c1_bss.assembly.reset_60 = 0;
            break;
        case 68:
            c1_bss.parser_value.pointer = dereference_assembly_value(make_assembly_symbol(right[0].name));
            break;
        case 69:
            c1_bss.parser_value.pointer = make_assembly_current_location((CompilerAssemblyExpression *)right[0].pointer);
            break;
        case 70:
            c1_bss.parser_value.pointer = make_assembly_register((WORD)right[0].words.low);
            break;
        case 71:
            c1_bss.parser_value.pointer = qualify_assembly_register((WORD)right[-3].words.low, right[-1].assembly.type);
            break;
        case 72:
            ASM_REGISTER_WIDTH(right[-2].words.low, width);
            c1_bss.assembly.reset_68 = 1; c1_bss.assembly.reset_04 = 0;
            select_assembly_width(width);
            c1_bss.parser_value.pointer = right[0].pointer;
            break;
        case 73:
            c1_bss.assembly.reset_68 = 1; c1_bss.assembly.reset_04 = 0;
            select_assembly_width(assembly_width_code(global_type_size(right[-2].assembly.type)));
            c1_bss.parser_value.pointer = right[0].pointer;
            break;
        case 74:
            ASM_UNSUPPORTED("assembler token type-property column is unmodeled");
            c1_bss.assembly.reset_68 = 1; c1_bss.assembly.reset_04 = 0;
            flags = unmodeled_assembly_type_properties[right[-2].operation].flags;
            width = (!(flags & 0x340) && (flags & ~15U)) ? flags & 15 :
                (int)c1_primitive_widths[flags & 15];
            select_assembly_width(assembly_width_code(width));
            c1_bss.parser_value.pointer = right[0].pointer;
            break;
        case 75:
            c1_bss.assembly.reset_68 = c1_bss.assembly.reset_04 = 1;
            set_assembly_address_mode((WORD)right[-2].words.low);
            c1_bss.parser_value.pointer = right[0].pointer;
            break;
        case 76:
            ASM_UNSUPPORTED("assembler token type-property column is unmodeled");
            c1_bss.assembly.reset_68 = 0; c1_bss.assembly.reset_04 = 1;
            if ((unmodeled_assembly_type_properties[right[-1].operation].flags & 0x6f) == 0x62)
                set_assembly_address_mode(60);
            else compiler_warning(409, current_assembly_mnemonic());
            c1_bss.parser_value.pointer = make_assembly_symbol(right[0].name);
            break;
        case 77:
            c1_bss.assembly.reset_68 = 0; c1_bss.assembly.reset_04 = 1;
            set_assembly_address_mode((WORD)right[-1].words.low);
            c1_bss.parser_value.pointer = make_assembly_symbol(right[0].name);
            break;
        case 78: case 80:
            c1_bss.assembly.reset_9fc4 = 0;
            c1_bss.parser_value.pointer = right[-1].pointer;
            break;
        case 79: case 81:
            if (c1_bss.assembly.reset_9fc4) {
                ASM_UNSUPPORTED("assembler diagnostic token spelling is unmodeled");
                compiler_warning(426, unmodeled_assembly_spellings[right[0].operation], current_assembly_mnemonic());
            }
            c1_bss.assembly.reset_9fc4 = 1;
            break;
        default: break; /* Productions without semantic actions. */
        }
        ++depth;
push_state: ;
    }
}

CompilerAssemblyOperand *__cdecl initialize_assembly_operand(unsigned index)
{
    CompilerAssemblyOperand *operand;
    if (index > 2) {
#ifdef __linux__
        recon_unsupported("assembler operand index outside the encountered records0/1/2");
#else
        __asm { int 3 }
#endif
    }
    c1_bss.assembly.reset_4c = 0;
    operand = &c1_bss.assembly.operands[index];
    operand->form = 15;
    operand->modifiers = 0;
    operand->value = &c1_bss.assembly.values[index];
    /* The original's DWORD/BYTE stores clear all12 value bytes. Its two
     * complementary masks of value byte9 also leave that whole byte zero. */
    memset(operand->value, 0, sizeof(*operand->value));
    operand->reset_08 = 0;
    operand->reset_0c = 0;
    return operand;
}

/* Low-six-bit assembly classifications of the encountered C token kinds.
 * Unknown rows are unsupported before lookup in both builds. */
static const struct AssemblyLexicalClass { BYTE token, classification; } assembly_lexical_classes[] = {
    { 0, ASSEMBLY_CLASS_37 }, { 6, ASSEMBLY_CLASS_44 },
    { 17, ASSEMBLY_CLASS_42 }, { 54, ASSEMBLY_CLASS_26 }, { 55, ASSEMBLY_CLASS_27 },
    { 65, ASSEMBLY_CLASS_28 },
    { 74, ASSEMBLY_CLASS_24 },
    { 80, ASSEMBLY_CLASS_35 }, /* Member-selection dot: low six bits of RVA64290. */
    { 81, ASSEMBLY_CLASS_22 },
    { 101, ASSEMBLY_CLASS_45 },
    { 117, ASSEMBLY_CLASS_32 }, { 118, ASSEMBLY_CLASS_30 },
    { 122, ASSEMBLY_CLASS_29 },
    { 123, ASSEMBLY_CLASS_34 }, { 126, ASSEMBLY_CLASS_31 },
    { 127, ASSEMBLY_CLASS_33 }, { 240, ASSEMBLY_CLASS_34 }
};
CompilerParserToken *__cdecl read_assembly_token(void)
{
    CompilerParserToken *token, *copy, *next;
    unsigned kind, classification, i;
read_again:
    token = c1_bss.current_context.ReadToken();
    if (!token) compiler_fatal(4);
    if (token->kind == 127) {
        handle_assembly_separator();
        goto read_again;
    }
    if (c1_bss.assembly_started) {
        if (token->kind == 240) goto read_again;
        c1_bss.assembly_started = 0;
    }
    copy = allocate_assembly_token(32, c1_bss.symbol_arena_kind);
    if (copy) memcpy(copy, token, 32);
    token = copy;
    kind = token->kind;
    for (i = 0; i < sizeof(assembly_lexical_classes) / sizeof(assembly_lexical_classes[0]); ++i)
        if (assembly_lexical_classes[i].token == kind) break;
    if (i == sizeof(assembly_lexical_classes) / sizeof(assembly_lexical_classes[0])) {
        ASM_UNSUPPORTED("assembler lexical classification row is unmodeled");
    }
    classification = assembly_lexical_classes[i].classification;
    if (classification == ASSEMBLY_CLASS_44) {
        if (kind != 6) {
            if (!C1_HAS_TOKEN_SPELLING(kind)) {
                ASM_UNSUPPORTED("assembler keyword spelling is unmodeled");
            }
            token->kind = 44;
            token->value.name = find_or_create_identifier(c1_token_spellings.indexed[kind]);
        } else token->kind = 44;
        return scan_assembly_identifier(token);
    }
    if (kind == 17 && !c1_bss.assembly.reset_08) {
        token->kind = 48;
        token->value.words.low = (token->value.words.low & ~0xffffU) | 7;
        return token;
    }
    if (classification == ASSEMBLY_CLASS_34) {
        token->kind = (BYTE)classification;
        token->value.operation = (BYTE)kind;
        return token;
    }
    if (classification == ASSEMBLY_CLASS_33) {
        do {
            next = c1_bss.current_context.ReadToken();
            if (next->kind == 127) handle_assembly_separator();
        } while (next->kind != 240);
        token->kind = ASSEMBLY_CLASS_34;
        token->value.words.low = (token->value.words.low & ~0xffffU) | 240;
        return token;
    }
    token->kind = (BYTE)classification;
    if (classification < ASSEMBLY_CLASS_38)
        token->value.words.low = (token->value.words.low & ~0xffffU) | classification;
    return token;
}

void __cdecl report_assembly_expected(CompilerParserToken *, int) { __asm { int 3 } }
CompilerParserValue * __cdecl make_assembly_recovery_value(CompilerParserValue *, int) { __asm { int 3 } }
void __cdecl report_assembly_parse_error(const char *) { __asm { int 3 } }
int __cdecl emit_assembly_statement(BYTE separator)
{
    for (;;) {
        switch (separator) {
        case 0:
            compiler_fatal(4);
            /* The canonical fallback proceeds to read the next token. */
            break;
        case 42:
            return 0;
        case 123:
            --c1_bss.assembly.brace_depth;
            if (!c1_bss.assembly.brace_depth) return 1;
            if ((int)c1_bss.assembly.brace_depth >= 0) return 0;
            c1_bss.current_context.saved_frame->stream->Unread((unsigned)c1_data.predefined_tokens[123]);
            c1_bss.assembly.brace_depth = 0;
            return 1;
        case 240:
            return c1_bss.assembly.brace_depth == 0;
        default:
            break;
        }
        separator = c1_bss.current_context.ReadToken()->kind;
    }
}
void __cdecl publish_assembly_label(CompilerNameEntry *name)
{
    CompilerIrSymbol *symbol;
    int found = 0;
    for (symbol = c1_data.label_symbols; symbol; symbol = symbol->binding.next ?
        (CompilerIrSymbol *)((BYTE *)symbol->binding.next - 8) : NULL) {
        if (symbol->symbol_value || compare_label_spelling(name->name, symbol->binding.name->name)) continue;
        symbol->label_flags |= 0x10;
        found = 1;
        if (symbol->flags & 1) compiler_warning(412, symbol);
        else emit_ir_label(make_symbol_node(6, symbol, NULL));
    }
    if (!found) {
        symbol = make_statement_label(name);
        symbol->label_flags |= 0x10;
        record_label_reference(symbol);
        emit_ir_label(make_symbol_node(6, symbol, NULL));
    }
}
void __cdecl emit_assembly_prefix(WORD, unsigned) { __asm { int 3 } }
struct AssemblyOpcodeDescription {
    unsigned first_form;
    BYTE processor_mask, unmodeled_05[3];
};
typedef char AssemblyOpcodeDescription_size[sizeof(AssemblyOpcodeDescription) == 8 ? 1 : -1];
/* Encountered8-byte opcode relations at66e68. Named rows expose first-form
 * and processor-mask fields only. Gaps and the remaining bytes are unmodeled;
 * every indexed read is guarded, without an original capacity/default claim. */
static const union AssemblyOpcodePrefix {
    struct { BYTE unmodeled_00[8]; AssemblyOpcodeDescription mov;
             BYTE unmodeled_2_5[4 * 8]; AssemblyOpcodeDescription imul;
             BYTE unmodeled_7_8[2 * 8]; AssemblyOpcodeDescription exchange;
             BYTE unmodeled_10_11[2 * 8]; AssemblyOpcodeDescription pop, push;
             BYTE unmodeled_14[8]; AssemblyOpcodeDescription conditional_jump;
             BYTE unmodeled_16_17[2 * 8]; AssemblyOpcodeDescription lea;
             BYTE unmodeled_19_32[14 * 8]; AssemblyOpcodeDescription inc;
             BYTE unmodeled_34_41[8 * 8]; AssemblyOpcodeDescription shift_left;
             BYTE unmodeled_43_44[2 * 8]; AssemblyOpcodeDescription add_instruction, and_instruction;
             AssemblyOpcodeDescription compare;
             BYTE unmodeled_48_49[2 * 8]; AssemblyOpcodeDescription subtract;
             AssemblyOpcodeDescription test, exclusive_or;
             BYTE unmodeled_53_55[3 * 8]; AssemblyOpcodeDescription move_string;
             BYTE unmodeled_57_154[98 * 8]; AssemblyOpcodeDescription clear_direction;
             BYTE unmodeled_156_162[7 * 8]; AssemblyOpcodeDescription imul_three;
             BYTE unmodeled_164_180[17 * 8]; AssemblyOpcodeDescription repeat;
             BYTE unmodeled_182_188[7 * 8]; AssemblyOpcodeDescription set_condition;
             BYTE unmodeled_190_193[4 * 8]; AssemblyOpcodeDescription zero_extend;
             BYTE unmodeled_195[8]; AssemblyOpcodeDescription sign_extend; } known;
    AssemblyOpcodeDescription indexed[197];
} assembly_opcode_prefix = { { {0}, {13, 0x1f, {0}}, {0}, {9, 0x1f, {0}}, {0}, {4, 0x1f, {0}}, {0}, {12, 0x1f, {0}}, {11, 0x1f, {0}}, {0}, {16, 0x1f, {0}}, {0}, {7, 0x1f, {0}}, {0}, {18, 0x1f, {0}}, {0}, {8, 0x1f, {0}}, {0}, {6, 0x1f, {0}}, {6, 0x1f, {0}}, {6, 0x1f, {0}}, {0}, {6, 0x1f, {0}}, {5, 0x1f, {0}}, {6, 0x1f, {0}}, {0}, {10, 0x1f, {0}}, {0}, {1, 0x1f, {0}}, {0}, {20, 0x1e, {0}}, {0}, {0, 0x1f, {0}}, {0}, {30, 0x18, {0}}, {0}, {51, 0x18, {0}}, {0}, {51, 0x18, {0}} } };
typedef char AssemblyOpcode_known_offsets[
    offsetof(AssemblyOpcodePrefix, known.conditional_jump) == 15 * 8 &&
    offsetof(AssemblyOpcodePrefix, known.compare) == 47 * 8 &&
    offsetof(AssemblyOpcodePrefix, known.move_string) == 56 * 8 &&
    offsetof(AssemblyOpcodePrefix, known.clear_direction) == 155 * 8 &&
    offsetof(AssemblyOpcodePrefix, known.imul_three) == 163 * 8 &&
    offsetof(AssemblyOpcodePrefix, known.repeat) == 181 * 8 &&
    offsetof(AssemblyOpcodePrefix, known.set_condition) == 189 * 8 &&
    offsetof(AssemblyOpcodePrefix, known.zero_extend) == 194 * 8 &&
    offsetof(AssemblyOpcodePrefix, known.sign_extend) == 196 * 8 ? 1 : -1];
void __cdecl set_assembly_opcode(WORD opcode)
{
    const AssemblyOpcodeDescription *description;
    if (opcode != 1 && opcode != 6 && opcode != 9 && opcode != 12 && opcode != 13 && opcode != 15 && opcode != 18 && opcode != 33 && opcode != 42 && opcode != 45 && opcode != 46 && opcode != 47 && opcode != 50 && opcode != 51 && opcode != 52 && opcode != 56 && opcode != 155 && opcode != 181 && opcode != 189 && opcode != 194 && opcode != 196) {
        ASM_UNSUPPORTED("assembler opcode description row is unmodeled");
    }
    description = &assembly_opcode_prefix.indexed[opcode];
    if (!(description->processor_mask & (BYTE)c1_bss.processor_instruction_mask))
        compiler_warning(416, c1_data.current_identifier);
    c1_bss.assembly.reset_54 = opcode;
    c1_bss.assembly.first_form = description->first_form;
    c1_bss.assembly.current_operand = &c1_bss.assembly.operands[0];
    c1_bss.assembly.operands[0].value = &c1_bss.assembly.values[0];
}
void __cdecl finish_assembly_operands(void)
{
    if (!validate_assembly_operand_count()) {
        compiler_warning(414);
        return;
    }
    if ((int)c1_bss.assembly.operand_index > 0 && !validate_assembly_operand_forms()) {
        compiler_warning(415);
        return;
    }
    select_assembly_encoding();
    write_assembly_instruction();
    emit_control_record(22);
}
/* Encountered general registers and their machine register encodings.
 * Row0 is the authentic no-register WORD003f, including its property bits.
 * Gaps reserve actual indexed positions without assigning defaults/capacity. */
static const union AssemblyRegisterPropertyPrefix {
    struct { WORD no_register, eax, ecx, edx, ebx, unmodeled_5, ebp, esi, edi, unmodeled_9_18[10], dl; } known;
    WORD indexed[20];
} assembly_register_properties = { {0x003f, 0x7200, 0x6201, 0x6202, 0x6203, 0, 0x6205, 0x6206, 0x6207, {0}, 0x0002} };
/* Encountered form rows expose operand count, allowed class combinations,
 * segment constraints and width bits. Unnamed rows have no semantic default. */
static const union AssemblyFormPrefix {
    struct { WORD prefix_no_operands, no_operands, unmodeled_2_3[2], exchange, test, arithmetic, lea, shift, imul, string_operation, push, pop, mov, unmodeled_14_15[2], conditional_jump, unmodeled_17, inc, unmodeled_19, imul_three, unmodeled_21_29[9], set_condition, unmodeled_31_50[20], extend_move; } known;
    WORD indexed[52];
} assembly_form_prefix = { { 0x3f00, 0x3f00, {0}, 0x871a, (2U << 14) | 0x3e | (7U << 8), (2U << 14) | 0x3e | (7U << 8), 0x8608, (2U << 14) | 0x36 | (7U << 8), (1U << 14) | 3 | (7U << 8), 0x070b, (1U << 14) | 7 | 0x40 | (6U << 8), (1U << 14) | 3 | 0x40 | (6U << 8), (2U << 14) | 0x3e | 0x40 | (7U << 8),
    {0}, (1U << 14) | 4 | (7U << 8), 0, (1U << 14) | 3 | (7U << 8), 0, (3U << 14) | 0x38 | (6U << 8), {0}, (1U << 14) | 3 | (1U << 8), {0}, 0x8618 } };
typedef char AssemblyForm_known_offsets[
    offsetof(AssemblyFormPrefix, known.test) == 5 * 2 &&
    offsetof(AssemblyFormPrefix, known.conditional_jump) == 16 * 2 &&
    offsetof(AssemblyFormPrefix, known.set_condition) == 30 * 2 &&
    offsetof(AssemblyFormPrefix, known.extend_move) == 51 * 2 ? 1 : -1];
/* Exact encountered relations: register flags4 -> class1 -> group1;
 * direct-address flags19 -> class3 -> group0;
 * address flags22 -> class4 -> group0, immediate flags2 -> class2 -> group2.
 * The two tables keep separate owners
 * and guards. Reserved positions are not initialized semantic defaults. */
static const union AssemblyOperandClasses {
    struct { BYTE unmodeled_0_1[2], immediate_class, symbolic_immediate_class, register_class, unmodeled_5_18[14], direct_address_class, unmodeled_20_21[2], address_class; } known;
    BYTE indexed[23];
} assembly_operand_classes = { { {0}, 2, 8, 1, {0}, 3, {0}, 4 } };
static const union AssemblyClassGroups {
    struct { BYTE unmodeled_0, register_group, immediate_group, direct_address_group, address_group, unmodeled_5_7[3], symbolic_immediate_group; } known;
    BYTE indexed[9];
} assembly_class_groups = { { 0, 1, 2, 0, 0, {0}, 2 } };
#define ASM_OPERAND_CLASS(value, result) do { \
    unsigned flags_ = (value)->kind_flags & 31; unsigned class_; \
    if (flags_ != 2 && flags_ != 3 && flags_ != 4 && flags_ != 19 && flags_ != 22) { ASM_UNSUPPORTED("assembler operand class relation is unmodeled"); } \
    class_ = assembly_operand_classes.indexed[flags_]; \
    if (class_ != 1 && class_ != 2 && class_ != 3 && class_ != 4 && class_ != 8) { ASM_UNSUPPORTED("assembler class group relation is unmodeled"); } \
    (result) = assembly_class_groups.indexed[class_]; \
} while (0)
#define ASM_FORM_FLAGS(result) do { \
    if (c1_bss.assembly.first_form != 0 && c1_bss.assembly.first_form != 1 && c1_bss.assembly.first_form != 4 && c1_bss.assembly.first_form != 10 && c1_bss.assembly.first_form != 5 && c1_bss.assembly.first_form != 6 && c1_bss.assembly.first_form != 7 && c1_bss.assembly.first_form != 8 && c1_bss.assembly.first_form != 9 && c1_bss.assembly.first_form != 11 && c1_bss.assembly.first_form != 12 && c1_bss.assembly.first_form != 13 && c1_bss.assembly.first_form != 16 && c1_bss.assembly.first_form != 18 && c1_bss.assembly.first_form != 20 && c1_bss.assembly.first_form != 30 && c1_bss.assembly.first_form != 51) { ASM_UNSUPPORTED("assembler form property relation is unmodeled"); } \
    (result) = assembly_form_prefix.indexed[c1_bss.assembly.first_form]; \
} while (0)
#define ASM_REGISTER_PROPERTIES(code, result) do { \
    unsigned code_ = (code); \
    if (code_ != 0 && code_ != 1 && code_ != 2 && code_ != 3 && code_ != 4 && code_ != 6 && code_ != 7 && code_ != 8 && code_ != 19) { ASM_UNSUPPORTED("assembler register property relation is unmodeled"); } \
    (result) = assembly_register_properties.indexed[code_]; \
} while (0)
#define ASM_REGISTER_ENCODING(code, result) do { \
    unsigned code_ = (code); \
    if (code_ != 0 && code_ != 1 && code_ != 2 && code_ != 3 && code_ != 4 && code_ != 6 && code_ != 7 && code_ != 8 && code_ != 19) { ASM_UNSUPPORTED("assembler register encoding relation is unmodeled"); } \
    (result) = (BYTE)assembly_register_properties.indexed[code_]; \
} while (0)
int __cdecl validate_assembly_operand_count(void)
{
    unsigned form = c1_bss.assembly.first_form;
    int count = c1_bss.assembly.operand_index;
    WORD opcode = c1_bss.assembly.reset_54;
    unsigned classification;
    if (form != 0 && form != 1 && form != 4 && form != 10 && form != 5 && form != 6 && form != 7 && form != 8 && form != 9 && form != 11 && form != 12 && form != 13 && form != 16 && form != 18 && form != 20 && form != 30 && form != 51) {
        ASM_UNSUPPORTED("assembler form operand-count relation is unmodeled");
    }
    if ((short)count == (assembly_form_prefix.indexed[form] >> 14)) return 1;
    if (form == 9) {
        if (opcode != 6 || count == 0) return 0;
        if (count == 3) opcode = 163;
        else if (count == 2) {
            ASM_OPERAND_CLASS(&c1_bss.assembly.operands[1].value->operand, classification);
            opcode = classification == 2 ? 163 : 193;
        }
        c1_bss.assembly.reset_54 = opcode;
        if (opcode != 163) ASM_UNSUPPORTED("assembler alternate opcode form relation is unmodeled");
        c1_bss.assembly.first_form = assembly_opcode_prefix.indexed[(short)opcode].first_form;
        return 1;
    }
    if (form == 2) return opcode == 60 && count == 1;
    if (form == 40) return count == 0 && opcode == 95;
    if (form == 17) return count == 1;
    if (form == 10 && c1_bss.assembly.reset_5c != opcode) return 0;
    if (form == 10 && (opcode == 57 || opcode == 53)) return count == 2;
    if (form == 10 || form == 43) {
        if (opcode == 55 || opcode == 58 || opcode == 59 || opcode == 92 ||
            opcode == 82 || opcode == 93) return count == 1;
        return count <= 2;
    }
    return 0;
}
int __cdecl validate_assembly_operand_forms(void)
{
    CompilerAssemblyOperandValue *first = &c1_bss.assembly.operands[0].value->operand;
    CompilerAssemblyOperandValue *second = &c1_bss.assembly.operands[1].value->operand;
    CompilerAssemblyOperandValue *left, *right, *third;
    unsigned first_class, second_class, third_class, combination, class_mask, check_mask;
    WORD form_flags, first_properties, second_properties, opcode;
    BYTE selector;
    int count = c1_bss.assembly.operand_index, segment_count;
    ASM_OPERAND_CLASS(first, first_class);
    combination = first_class;
    if (count > 1) {
        ASM_OPERAND_CLASS(second, second_class);
        combination = first_class * 3 + second_class;
    }
    class_mask = 1U << (combination & 31);
    opcode = c1_bss.assembly.reset_54;
    if (combination < 6) {
        ASM_FORM_FLAGS(form_flags);
        if (!(class_mask & form_flags)) return 0;
    } else if (opcode != 11 && opcode != 5 && opcode != 4) return 0;
    check_mask = class_mask & 0x3a;
    if (check_mask) {
        segment_count = 0;
        if (count > 1) {
            ASM_REGISTER_PROPERTIES(second->register_code, second_properties);
            segment_count = (second_properties & 0xc0) == 0x40;
        }
        ASM_REGISTER_PROPERTIES(first->register_code, first_properties);
        segment_count += (first_properties & 0xc0) == 0x40;
        if (segment_count > 0) {
            if (segment_count > 1) return 0;
            ASM_FORM_FLAGS(form_flags);
            if ((form_flags & 0xc0) != 0x40) return 0;
        }
    }
    ASM_FORM_FLAGS(form_flags);
    form_flags &= 0xc0;
    if (form_flags != 0xc0) {
        if (check_mask) {
            ASM_REGISTER_PROPERTIES(first->register_code, first_properties);
            if ((first_properties & 0x80c0) == 0x8080) return 0;
        }
        if (class_mask & 0x10) {
            ASM_REGISTER_PROPERTIES(second->register_code, second_properties);
            if ((second_properties & 0x80c0) == 0x8080) return 0;
        }
    }
    if (form_flags == 0x80) {
        if (check_mask && !(first->kind_flags & 0x34)) return 0;
        if ((class_mask & 0x10) && !(second->kind_flags & 0x34)) return 0;
    }
    switch (c1_bss.assembly.first_form) {
    case 8:
        if (class_mask & 0x24) {
            if (c1_bss.processor_instruction_mask == 1 && second->displacement != 1) return 0;
            if (second->kind_flags & 1) return 0;
            return assembly_immediate_fits(8, second->displacement);
        }
        return second->register_code == 18;
    case 10:
        left = first; right = second;
        if (opcode == 57 || opcode == 53) { left = second; right = first; }
        if (opcode != 57 && opcode != 55) {
            selector = left->segment_and_scale & 63;
            if (selector) {
                if (selector != 25) return 0;
                left->segment_and_scale &= 0xc0;
            }
        }
        if ((left->segment_and_scale & 63) == 28) left->segment_and_scale &= 0xc0;
        if (count != 2) return 1;
        if ((right->segment_and_scale & 63) == 28) right->segment_and_scale &= 0xc0;
        if (opcode != 57 && opcode != 54) return 1;
        return right->register_code == 11;
    case 11:
        return c1_bss.processor_instruction_mask != 1 || combination != 2;
    case 12:
        if (!check_mask) return 1;
        return first->register_code != 26;
    case 13:
        ASM_REGISTER_PROPERTIES(first->register_code, first_properties);
        ASM_REGISTER_PROPERTIES(second->register_code, second_properties);
        if ((first_properties & 0xc0) == 0xc0 || (second_properties & 0xc0) == 0xc0) {
            if (combination != 4) return 0;
            if ((first_properties & 0xc0) == 0xc0 && (second_properties & 0xc0) == 0xc0) return 0;
            ASM_UNSUPPORTED("assembler alternate opcode208 form is unmodeled");
            c1_bss.assembly.reset_54 = 208;
            c1_bss.assembly.first_form = assembly_opcode_prefix.indexed[208].first_form;
            return 1;
        }
        if (combination == 5 && opcode == 64) return 0;
        if (check_mask && first->register_code == 26) return 0;
        return 1;
    case 14: case 15:
        return combination != 2 || (first->kind_flags & 1) != 0;
    case 19:
        if (combination != 8) return 0;
        return assembly_immediate_fits(8, second->displacement);
    case 20:
        if (c1_bss.processor_instruction_mask == 1 || !check_mask) return 0;
        if (c1_bss.processor_instruction_mask == 8) return 1;
        return (combination == 5) == (count == 2);
    case 25:
        if (first_class != 2 || !assembly_immediate_fits(6, first->displacement) || combination == 8) return 0;
        if (combination != 7) return 1;
        ASM_REGISTER_PROPERTIES(second->register_code, second_properties);
        return (second_properties & 0xc0) != 0x40;
    case 26:
        return assembly_immediate_fits(8, first->displacement);
    case 27:
        return first->displacement == 0 && (first->kind_flags & 1);
    case 28:
        left = second; right = first;
        if (opcode != 11) { left = first; right = second; }
        ASM_OPERAND_CLASS(left, second_class);
        if (second_class != 1) return 0;
        ASM_REGISTER_PROPERTIES(left->register_code, second_properties);
        if (!(second_properties & 0x1000)) return 0;
        ASM_OPERAND_CLASS(right, first_class);
        if (first_class == 2 && assembly_immediate_fits(8, right->displacement)) return 1;
        return first_class == 1 && right->register_code == 11;
    case 38: case 39:
        if ((opcode == 110 || opcode == 108) && first->register_code == 9) return 1;
        return combination == 0;
    case 43: case 44:
        if (opcode == 92 || opcode == 93 || opcode == 209 || opcode == 210) return combination != 4;
        if (count != 2) return combination == 0;
        if (combination != 4) return 0;
        if (!second->register_code) return 1;
        if (c1_bss.assembly.first_form == 44) return 0;
        return first->register_code == 0;
    case 50:
        third = &c1_bss.assembly.operands[2].value->operand;
        ASM_UNSUPPORTED("assembler third operand semantics are unmodeled");
        ASM_OPERAND_CLASS(third, third_class);
        if (third_class == 2 && assembly_immediate_fits(8, third->displacement)) return 1;
        return third_class == 1 && third->register_code == 18;
    case 52:
        ASM_OPERAND_CLASS(second, second_class);
        if (second_class != 2) return 1;
        if (second->kind_flags & 1) return 0;
        return assembly_immediate_fits(8, second->displacement);
    }
    return 1;
}

void __cdecl select_assembly_encoding(void)
{
    BYTE initial_flags = select_assembly_operand_width();
    CompilerAssemblyOperandValue *value;
    CompilerIrSymbol *symbol;
    WORD opcode, replacement;
    unsigned classification;
    int swap = 0, disable_relocation;
    c1_bss.assembly.encoding_flags = initial_flags;
    switch (c1_bss.assembly.first_form) {
    case 0:
        if (c1_bss.assembly.reset_54 == 184) c1_bss.assembly.reset_54 = 181;
        break;
    case 2:
        value = &c1_bss.assembly.operands[0].value->operand;
        if ((value->segment_and_scale & 63) == 28) value->segment_and_scale &= 0xc0;
        /* Falls through to the optional operand/opcode normalization. */
    case 10:
        if ((int)c1_bss.assembly.operand_index <= 0) break;
        opcode = c1_bss.assembly.reset_54;
        replacement = opcode;
        if (opcode == 53) replacement = 224;
        else if (opcode == 55) replacement = 225;
        else if (opcode == 56) replacement = 226;
        else if (opcode == 57) replacement = 227;
        else if (opcode == 60) replacement = 228;
        value = &c1_bss.assembly.operands[0].value->operand;
        if (!(value->segment_and_scale & 63) && (int)c1_bss.assembly.operand_index > 1)
            value = &c1_bss.assembly.operands[1].value->operand;
        if ((value->segment_and_scale & 63) && opcode != replacement)
            c1_bss.assembly.reset_54 = replacement;
        else c1_bss.assembly.operand_index = 0;
        break;
    case 14: case 15:
        disable_relocation = initial_flags;
        goto check_relocation;
    case 16:
        if (c1_bss.assembly.reset_5c != 261 && c1_bss.assembly.reset_5c != 265) ASM_UNSUPPORTED("assembler implicit encoding column is unmodeled");
        c1_bss.assembly.implicit_encoding = assembly_encoding_prefix.indexed[
            (short)c1_bss.assembly.reset_5c].operand_encoding;
        disable_relocation = 0;
        goto check_relocation;
    case 27:
        disable_relocation = 0;
check_relocation:
        value = &c1_bss.assembly.operands[0].value->operand;
        symbol = value->symbol;
        if (!symbol || (value->kind_flags & 0x10) || disable_relocation) break;
        if (value->displacement && !(symbol->flags & 1)) {
            compiler_warning(405);
            break;
        }
        if (symbol->kind == 7 || symbol->kind == 8 || symbol->kind == 6) {
            report_language_diagnostic(3, 414, symbol);
            c1_bss.assembly.encoding_flags = 2;
        }
        break;
    case 17:
        if (!c1_bss.assembly.operand_index) {
            c1_bss.assembly.operand_index = 1;
            value = &c1_bss.assembly.operands[0].value->operand;
            value->kind_flags = 2;
            value->displacement = 0;
        }
        break;
    case 18:
        c1_bss.assembly.operand_index = 2;
        c1_bss.assembly.current_operand = initialize_assembly_operand(1);
        value = &c1_bss.assembly.operands[1].value->operand;
        value->kind_flags = 2;
        value->displacement = 1;
        break;
    case 20:
        if (c1_bss.assembly.operand_index != 2) break;
        /* The actual two-operand immediate form duplicates the destination
         * and moves the immediate into operand2, retaining all16/12 copied
         * bytes rather than inventing defaults for their padding. */
        c1_bss.assembly.operands[2] = c1_bss.assembly.operands[1];
        c1_bss.assembly.operand_index = 3;
        c1_bss.assembly.operands[2].value = &c1_bss.assembly.values[2];
        memcpy(&c1_bss.assembly.values[2], c1_bss.assembly.operands[1].value, 12);
        c1_bss.assembly.operands[1] = c1_bss.assembly.operands[0];
        c1_bss.assembly.operands[1].value = &c1_bss.assembly.values[1];
        memcpy(&c1_bss.assembly.values[1], c1_bss.assembly.operands[0].value, 12);
        break;
    case 28:
        swap = c1_bss.assembly.reset_54 == 10;
        break;
    case 30:
        if (c1_bss.assembly.reset_5c != 389) ASM_UNSUPPORTED("assembler implicit encoding column is unmodeled");
        c1_bss.assembly.implicit_encoding = assembly_encoding_prefix.indexed[
            (short)c1_bss.assembly.reset_5c].operand_encoding;
        break;
    case 31: case 33: case 34: case 35: case 36: case 37: case 43: case 44:
        c1_bss.assembly.encoding_flags = initial_flags | 0x20;
        /* Falls through to the real outlined swapping predicate. */
    case 32: case 38: case 39: case 40: case 41: case 62: case 63: case 64:
        swap = assembly_should_swap_operands();
        break;
    case 50:
        ASM_UNSUPPORTED("assembler third operand classification is unmodeled");
        value = &c1_bss.assembly.operands[2].value->operand;
        c1_bss.assembly.operand_index = 2;
        ASM_OPERAND_CLASS(value, classification);
        if (classification == 2) {
            c1_bss.assembly.operands[1].value->operand.displacement = value->displacement;
            c1_bss.assembly.operands[1].value->operand.kind_flags |= 2;
        }
        break;
    case 51:
        if (c1_bss.assembly.operands[1].form == 1)
            c1_bss.assembly.reset_54 = c1_bss.assembly.reset_54 == 196 ? 197 : 195;
        break;
    }
    resolve_assembly_address_size();
    c1_bss.assembly.output_values[0] = &c1_bss.assembly.values[0];
    c1_bss.assembly.output_values[1] = &c1_bss.assembly.values[1];
    c1_bss.assembly.output_values[2] = &c1_bss.assembly.values[2];
    c1_bss.assembly.output_operands[0] = &c1_bss.assembly.operands[0];
    c1_bss.assembly.output_operands[1] = &c1_bss.assembly.operands[1];
    c1_bss.assembly.output_operands[2] = &c1_bss.assembly.operands[2];
    if (swap) {
        swap_assembly_pointer((void **)&c1_bss.assembly.output_values[1],
            (void **)&c1_bss.assembly.output_values[0]);
        swap_assembly_pointer((void **)&c1_bss.assembly.operands[0].value,
            (void **)&c1_bss.assembly.operands[1].value);
        swap_assembly_pointer((void **)&c1_bss.assembly.output_operands[0],
            (void **)&c1_bss.assembly.output_operands[1]);
    }
    c1_bss.assembly.encoded_opcode = (BYTE)c1_bss.assembly.reset_54;
}
BYTE __cdecl select_assembly_operand_width(void)
{
    BYTE width = common_assembly_operand_width(), encoded;
    unsigned classification, allowed, form = c1_bss.assembly.first_form;
    WORD form_flags;
    short opcode = (short)c1_bss.assembly.reset_54;
    short original_opcode = (short)c1_bss.assembly.reset_5c;
    int incompatible = 0;
    switch (form) {
    case 7:
        if (opcode != 18) {
            if (width == 1)
                incompatible = !assembly_operand_accepts_width(&c1_bss.assembly.operands[1], 2);
            else incompatible = !assembly_operand_accepts_width(&c1_bss.assembly.operands[1],
                (BYTE)(opcode == 3 ? 3 : 5));
        }
        break;
    case 15:
        ASM_OPERAND_CLASS(&c1_bss.assembly.operands[0].value->operand, classification);
        if (!classification && (!width || width == 15)) { incompatible = 1; width = 1; }
        break;
    case 21:
        width = (opcode == 62 || opcode == 65) ? 1 : 5;
        incompatible = !assembly_operand_accepts_width(&c1_bss.assembly.operands[0], width);
        break;
    case 23:
        incompatible = !assembly_operand_accepts_width(&c1_bss.assembly.operands[1], width);
        break;
    case 27:
        width = opcode == 22 ? (original_opcode == 22 ? 1 : 2) : 0;
        break;
    case 41:
        if (opcode != 98) break;
        ASM_OPERAND_CLASS(&c1_bss.assembly.operands[0].value->operand, classification);
        if (classification) break;
        width = c1_bss.assembly.operands[0].form;
        if (width == 15) width = 3;
        if (width == 4) { width = 3; incompatible = 1; }
        break;
    case 43:
        ASM_OPERAND_CLASS(&c1_bss.assembly.operands[0].value->operand, classification);
        if (classification) break;
        width = c1_bss.assembly.operands[0].form;
        if (width == 15) width = 3;
        incompatible = !assembly_operand_has_property(&c1_bss.assembly.operands[0], 0x100) ||
            (width != 2 && width != 3 && width != 4);
        break;
    case 62: case 63:
        width = c1_bss.assembly.operands[0].form;
        if (width == 15) width = 1;
        incompatible = !assembly_operand_has_property(&c1_bss.assembly.operands[0], 0x40) ||
            (width != 1 && width != 2 && !(width == 3 && form != 63));
        break;
    }
    if (incompatible) report_language_diagnostic(1, 410);
    if (width != 15) {
        ASM_FORM_FLAGS(form_flags);
        if (form_flags & (1U << ((width + 8) & 31))) {
            if (opcode != original_opcode && original_opcode >= 286 && original_opcode <= 359) {
                if (original_opcode != 297) ASM_UNSUPPORTED("assembler width override column is unmodeled");
                encoded = assembly_encoding_prefix.indexed[original_opcode].operand_encoding;
                if (encoded & 0x30) width = encoded & 15;
            }
            return width;
        }
        report_language_diagnostic(1, 409);
    }
    ASM_FORM_FLAGS(form_flags);
    allowed = form_flags & 0x3f00;
    width = 0;
    while (!(allowed & (1U << ((width + 8) & 31)))) ++width;
    if (opcode != original_opcode && original_opcode >= 286 && original_opcode <= 359) {
        if (original_opcode != 297) ASM_UNSUPPORTED("assembler fallback width override column is unmodeled");
        encoded = assembly_encoding_prefix.indexed[original_opcode].operand_encoding;
        if (encoded & 0x30) width = encoded & 15;
    }
    if (form == 0) {
        if (original_opcode == 153) return 0;
        if (original_opcode == 159 || original_opcode == 308) return 1;
        if (original_opcode == 307) return 2;
    } else if (form == 1) {
        if (original_opcode == 165 || (original_opcode > 169 && original_opcode <= 173)) return 1;
    } else if (form == 11) return 2;
    else if (form == 38 || form == 39) return 4;
    return width;
}
BYTE __cdecl common_assembly_operand_width(void)
{
    short opcode = (short)c1_bss.assembly.reset_54;
    short original_opcode = (short)c1_bss.assembly.reset_5c;
    int count;
    unsigned classification, form;
    BYTE first, second, encoding;
    if (opcode != original_opcode && original_opcode >= 286 && original_opcode <= 359) {
        if (original_opcode != 297) ASM_UNSUPPORTED("assembler explicit width column is unmodeled");
        encoding = assembly_encoding_prefix.indexed[original_opcode].operand_encoding;
        if (!(encoding & 0x30)) return encoding & 15;
    }
    count = c1_bss.assembly.operand_index;
    if (count <= 0) return 15;
    first = c1_bss.assembly.operands[0].form;
    second = c1_bss.assembly.operands[1].form;
    form = c1_bss.assembly.first_form;
    if (count == 2 && first != second && (BYTE)(first | second) != 15 &&
        (form == 13 || form == 6 || form == 4 || form == 5 || form == 55) &&
        c1_bss.assembly.operands[1].value->operand.symbol != c1_data.local_size_symbol) {
        compiler_warning(443);
        opcode = (short)c1_bss.assembly.reset_54;
        count = c1_bss.assembly.operand_index;
        first = c1_bss.assembly.operands[0].form;
        second = c1_bss.assembly.operands[1].form;
    }
    if (opcode == 11 || opcode == 57) return second;
    if (opcode == 53 || opcode == 56) return first == 15 ? second : first;
    if (count == 1) return first;
    ASM_OPERAND_CLASS(&c1_bss.assembly.operands[0].value->operand, classification);
    if (classification == 1) return first;
    ASM_OPERAND_CLASS(&c1_bss.assembly.operands[1].value->operand, classification);
    if (classification == 2 || c1_bss.assembly.first_form == 8 || opcode == 54) return first;
    return second;
}
int __cdecl assembly_operand_accepts_width(CompilerAssemblyOperand *, BYTE) { __asm { int 3 } }
int __cdecl assembly_operand_has_property(CompilerAssemblyOperand *, unsigned) { __asm { int 3 } }

void __cdecl resolve_assembly_address_size(void)
{
    unsigned index, classification;
    WORD properties;
    BYTE encoded;
    CompilerAssemblyOperand *operand;
    CompilerAssemblyOperandValue *value;
    for (index = 0; (int)index < (int)c1_bss.assembly.operand_index; ++index) {
        if (index >= 3) { ASM_UNSUPPORTED("assembler address operand index is unmodeled"); }
        operand = &c1_bss.assembly.operands[index];
        value = &operand->value->operand;
        if (operand->modifiers & 1) {
            ASM_OPERAND_CLASS(value, classification);
            if (classification == 1) report_assembly_operand_error(421, index);
        }
        if (value->index_register == 5) {
            if ((value->segment_and_scale & 0xc0) || value->register_code == 5)
                report_assembly_operand_error(431, index);
            else {
                value->index_register = value->register_code;
                value->register_code = 5;
            }
        }
        ASM_REGISTER_ENCODING(value->segment_and_scale & 63, encoded);
        value->segment_and_scale = (value->segment_and_scale & 0xc0) | (encoded & 63);
        ASM_REGISTER_ENCODING(value->index_register, encoded);
        value->index_register = encoded;
        if (value->kind_flags & 0x20) value->register_code += 40;
        else if (value->register_code) {
            ASM_REGISTER_PROPERTIES(value->register_code, properties);
            ASM_REGISTER_ENCODING(value->register_code, encoded);
            value->register_code = (properties & 0x80c0) == 0x8080 ? encoded + 40 : encoded;
        } else if (value->segment_and_scale & 0xc0) {
            ASM_REGISTER_ENCODING(value->register_code, encoded);
            value->register_code = encoded;
        } else {
            value->register_code = value->index_register;
            ASM_REGISTER_ENCODING(0, encoded);
            value->index_register = encoded;
        }
    }
}
void __cdecl report_assembly_operand_error(unsigned, unsigned) { __asm { int 3 } }
int __cdecl assembly_should_swap_operands(void) { __asm { int 3 } }
void __cdecl swap_assembly_pointer(void **, void **) { __asm { int 3 } }

/* Encountered operand formats and symbol relocations fit this16-byte scratch
 * prefix. This is a supported extent, not the original buffer's full capacity. */
#define ASM_RECORD_RESERVE(n) do { \
    if ((unsigned)(cursor - begin) > sizeof(c1_bss.assembly_record) - (n)) { \
        ASM_UNSUPPORTED("assembler record exceeds its established prefix"); \
    } \
} while (0)
#define ASM_MARK_SYMBOL(s) do { \
    unsigned refs_ = (s)->reference_flags & 3U; \
    if (refs_ < 3) (s)->reference_flags = ((s)->reference_flags & ~3U) | (refs_ + 1); \
    (s)->reference_flags = ((s)->reference_flags & ~4U) | ((c1_data.ir_reference_epoch & 1U) << 2); \
    if ((s)->declaration_flags & 0x40) { \
        (s) = resolve_symbol_alias(s); (s)->MarkAssemblyReference(); \
    } \
    if (((s)->function_flags & 1) && (lookup_ir_symbol(s)->record_id & 0xffffff) == 5 && \
        c1_bss.current_scope_symbol && !(c1_bss.current_scope_symbol->scope_flags & 1)) { \
        c1_data.ir_symbol_state_changed = 1; c1_data.ir_symbol_state_ready = 0; \
    } \
} while (0)
#define ASM_RECORD_SYMBOL(s) do { \
    unsigned output_index_ = get_symbol_output_index(s); \
    ASM_RECORD_RESERVE(output_index_ > 0x7fff ? 4 : 2); \
    if (output_index_ > 0x7fff) { \
        *(WORD *)cursor = (WORD)(output_index_ | 0x8000); \
        *(WORD *)(cursor + 2) = (WORD)(output_index_ >> 15); cursor += 4; \
    } else { *(WORD *)cursor = (WORD)output_index_; cursor += 2; } \
} while (0)
void __cdecl write_assembly_instruction(void)
{
    BYTE *begin = (BYTE *)&c1_bss.assembly_record, *cursor = begin + 3;
    const char *format;
    CompilerAssemblyOperand *operand;
    CompilerAssemblyOperandValue *value;
    CompilerIrSymbol *symbol;
    unsigned index, mode;
    BYTE encoding;
    c1_bss.assembly_record.opcode = c1_bss.assembly.encoded_opcode;
    if (c1_bss.assembly.reset_54 == 15 || c1_bss.assembly.reset_54 == 189)
        c1_bss.assembly_record.width = c1_bss.assembly.implicit_encoding;
    else {
        switch (c1_bss.assembly.encoding_flags & 15) {
        case 0: c1_bss.assembly_record.width = 1; break;
        case 1: c1_bss.assembly_record.width = 2; break;
        case 2: c1_bss.assembly_record.width = 4; break;
        case 3: c1_bss.assembly_record.width = 8; break;
        case 4: c1_bss.assembly_record.width = 10; break;
        case 5: c1_bss.assembly_record.width = 6; break;
        default: c1_bss.assembly_record.width = 0; break;
        }
    }
    for (index = 0; (int)index < (int)c1_bss.assembly.operand_index; ++index) {
        if (index >= 3) { ASM_UNSUPPORTED("assembler output operand index is unmodeled"); }
        operand = c1_bss.assembly.output_operands[index];
        value = &c1_bss.assembly.output_values[index]->operand;
        if (operand->source_symbol) {
            ASM_RECORD_RESERVE(1); *cursor++ = 18;
            symbol = operand->source_symbol;
            ASM_MARK_SYMBOL(symbol);
            ASM_RECORD_SYMBOL(symbol);
            ASM_REGISTER_ENCODING(8, encoding);
            ASM_RECORD_RESERVE(1); *cursor++ = encoding & 63;
        }
        if (operand->symbol) {
            ASM_RECORD_RESERVE(1); *cursor++ = 18;
            symbol = operand->symbol;
            ASM_MARK_SYMBOL(symbol);
            ASM_RECORD_SYMBOL(symbol);
            ASM_REGISTER_ENCODING(6, encoding);
            ASM_RECORD_RESERVE(1); *cursor++ = encoding & 63;
        }
        mode = classify_assembly_operand((CompilerAssemblyValue *)value);
        if (mode == 1) format = "mr";
        else if (mode == 2) format = "mv";
        else if (mode == 3) format = "msog";
        else if (mode == 4) format = "mdbg";
        else if (mode == 6) format = "mdibxg";
        else if (mode == 8) format = "mso";
        else { ASM_UNSUPPORTED("assembler output operand format is unmodeled"); format = ""; }
        for (; *format; ++format) {
            switch ((signed char)*format) {
            case 'b': case 'r':
                ASM_RECORD_RESERVE(1);
                *cursor++ = value->register_code == 63 ? 255 : value->register_code & 63;
                break;
            case 'd': case 'o': case 'v':
                ASM_RECORD_RESERVE(4); *(unsigned *)cursor = value->displacement; cursor += 4;
                break;
            case 'g':
                ASM_RECORD_RESERVE(1); encoding = value->segment_and_scale & 63;
                *cursor++ = encoding == 63 ? 0 : encoding;
                break;
            case 'i':
                ASM_RECORD_RESERVE(1);
                *cursor++ = value->index_register == 63 ? 255 : value->index_register & 63;
                break;
            case 'm':
                ASM_RECORD_RESERVE(1); *cursor++ = (BYTE)mode;
                break;
            case 's':
                symbol = value->symbol;
                if (symbol) {
                    ASM_MARK_SYMBOL(symbol);
                    ASM_RECORD_SYMBOL(symbol);
                } else {
                    ASM_RECORD_RESERVE(2); *(WORD *)cursor = 0; cursor += 2;
                }
                break;
            case 'x':
                ASM_RECORD_RESERVE(1); *cursor++ = (BYTE)(1U << (value->segment_and_scale >> 6));
                break;
            }
        }
    }
    c1_bss.assembly_record.length = (BYTE)(cursor - begin);
}
unsigned __cdecl classify_assembly_operand(CompilerAssemblyValue *operand)
{
    CompilerAssemblyOperandValue *value = &operand->operand;
    unsigned flags = value->kind_flags & 31;
    BYTE classification;
    if (flags != 2 && flags != 3 && flags != 4 && flags != 19 && flags != 22) {
        ASM_UNSUPPORTED("assembler output classification row is unmodeled");
    }
    classification = assembly_operand_classes.indexed[flags];
    switch (classification) {
    case 1:
        if ((value->register_code & 0xc0) == 0xc0) classification = 20;
        break;
    case 3:
        if (!value->symbol) classification = 16;
        break;
    case 4:
        if (value->index_register != 63) classification = 6;
        break;
    case 5:
        if (value->index_register != 63) classification = 7;
        break;
    case 8:
        if (value->symbol == c1_data.anonymous_global_symbol) {
            if (c1_bss.assembly.first_form == 16 || c1_bss.assembly.first_form == 15 ||
                c1_bss.assembly.first_form == 27 || c1_bss.assembly.first_form == 14)
                classification = 12;
        }
        if (value->symbol == c1_data.local_size_symbol &&
            (c1_bss.assembly.first_form == 6 || c1_bss.assembly.first_form == 13))
            classification = 11;
        break;
    }
    return classification;
}
int __cdecl assembly_immediate_fits(unsigned bits, unsigned value)
{
    unsigned magnitude = value;
    if ((int)magnitude < 0) magnitude = 0U - magnitude;
    /* Original NEG preserves the INT_MIN bit pattern, and SETL compares
     * signed DWORDs. CL supplies the hardware's low-five-bit shift count. */
    return (int)magnitude < (int)(1U << (bits & 31));
}
void __cdecl emit_assembly_immediate(unsigned displacement)
{
    CompilerAssemblyOperandValue *value = &c1_bss.assembly.current_operand->value->operand;
    value->displacement += displacement;
    value->kind_flags |= 2;
}
unsigned __cdecl assembly_scale_code(unsigned scale)
{
    unsigned code = 0;
    if (scale > 8 || (scale & (scale - 1))) compiler_warning(423, scale);
    while ((scale >>= 1) != 0) ++code;
    return code;
}
CompilerAssemblyExpression *__cdecl make_assembly_binary(BYTE operation,
    CompilerAssemblyExpression *left, CompilerAssemblyExpression *right)
{
    CompilerAssemblyOperandValue *value;
    BYTE scale;
    if (!c1_bss.assembly.reset_9fc4 || left->kind != 1 || right->kind != 0)
        return combine_assembly_values(operation, left, right);
    value = &c1_bss.assembly.current_operand->value->operand;
    if (value->segment_and_scale & 0xc0)
        compiler_warning(430, current_assembly_mnemonic());
    scale = (BYTE)assembly_scale_code(right->payload);
    value = &c1_bss.assembly.current_operand->value->operand;
    value->segment_and_scale = (value->segment_and_scale & 63) | (BYTE)(scale << 6);
    if (value->segment_and_scale & 0xc0) {
        if (!value->register_code) compiler_warning(431, current_assembly_mnemonic());
        else if (!value->index_register) {
            value->index_register = value->register_code;
            value->register_code = 0;
        }
    }
    return left;
}
CompilerAssemblyExpression * __cdecl combine_assembly_values(BYTE, CompilerAssemblyExpression *, CompilerAssemblyExpression *) { __asm { int 3 } }
CompilerAssemblyExpression *__cdecl make_assembly_address(short operation,
    CompilerAssemblyExpression *left, CompilerAssemblyExpression *right)
{
    CompilerArena *arena;
    CompilerAssemblyExpression *result;
    const char *spelling;
    if (operation != 22 && operation != 23 && operation != 28 && operation != 35)
        ASM_UNSUPPORTED("assembler address operator return domain is unmodeled");
    if (left->kind > 3 || right->kind > 3)
        ASM_UNSUPPORTED("assembler address node kind is unmodeled");
    if (operation == 23) {
        if (right->kind) {
            compiler_warning(425, "-", current_assembly_mnemonic());
            return left;
        }
        if (left->kind == 3)
            ASM_UNSUPPORTED("assembler subtraction register payload has no established writer");
        arena = &c1_bss.arenas[3];
        result = (CompilerAssemblyExpression *)arena->cursor;
        arena->cursor += 12;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        result->payload = left->payload - right->payload;
        result->kind = left->kind;
        return result;
    }
    if (left->kind == 3 || right->kind == 3 || (left->kind == 2 && right->kind == 2)) {
        switch (operation) {
        case 22: spelling = "+"; break;
        case 28: spelling = "["; break;
        default: spelling = "."; break;
        }
        compiler_warning(424, spelling, current_assembly_mnemonic());
        return left;
    }
    arena = &c1_bss.arenas[3];
    result = (CompilerAssemblyExpression *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    result->payload = left->payload + right->payload;
    result->kind = (short)left->kind > (short)right->kind ? left->kind : right->kind;
    return result;
}
CompilerAssemblyExpression * __cdecl make_assembly_selector(WORD, CompilerAssemblyExpression *) { __asm { int 3 } }
CompilerAssemblyExpression *__cdecl make_assembly_integer(unsigned value)
{
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerAssemblyExpression *expression = (CompilerAssemblyExpression *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    expression->kind = 0;
    expression->payload = value;
    return expression;
}
CompilerAssemblyExpression *__cdecl make_assembly_symbol(CompilerNameEntry *name)
{
    CompilerIrSymbol *symbol = resolve_assembly_symbol(name, 1);
    CompilerAssemblyExpression *expression;
    CompilerArena *arena;
    CompilerAssemblyOperand *operand;
    unsigned storage;
    if (symbol->kind < 7 || symbol->kind > 11) {
        compiler_warning(420, name, current_assembly_mnemonic());
        arena = &c1_bss.arenas[3];
        expression = (CompilerAssemblyExpression *)arena->cursor;
        arena->cursor += 12;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        expression->kind = 0;
        expression->payload = 0;
        return expression;
    }
    operand = c1_bss.assembly.current_operand;
    operand->value->operand.kind_flags = (operand->value->operand.kind_flags & 0xec) | 3;
    if (!symbol->value_type->shape && (symbol->value_type->flags & 0x100))
        operand->value->operand.kind_flags |= 0x20;
    symbol->MarkAssemblyReference();
    if (operand->modifiers & 1) {
        if (!c1_bss.assembly.reset_04)
            operand->value->operand.kind_flags = operand->value->operand.kind_flags | 0x10;
    } else {
        if (symbol->kind != 9 && symbol->kind != 7 && symbol->kind != 8 &&
            c1_bss.assembly.first_form != 16 && c1_bss.assembly.first_form != 15 &&
            c1_bss.assembly.first_form != 27 && c1_bss.assembly.first_form != 14 &&
            symbol != c1_data.anonymous_global_symbol && symbol != c1_data.local_size_symbol)
            operand->value->operand.kind_flags = operand->value->operand.kind_flags | 0x10;
        operand->form = assembly_width_code(assembly_symbol_size(symbol));
    }
    storage = symbol->storage_class & 7;
    if (storage >= 4 && storage <= 6) {
        if (operand->value->operand.symbol)
            report_language_diagnostic(1, 411, c1_data.current_identifier);
        operand->value->operand.kind_flags &= 0xde;
        expression = make_assembly_register_expression(6);
        expression->payload = 0;
        operand->symbol = symbol;
    } else {
        if (!operand->value->operand.symbol) operand->value->operand.symbol = symbol;
        arena = &c1_bss.arenas[3];
        expression = (CompilerAssemblyExpression *)arena->cursor;
        arena->cursor += 12;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        expression->kind = 2;
        expression->payload = 0;
    }
    return expression;
}
CompilerAssemblyExpression *__cdecl make_assembly_register(WORD register_code)
{
    WORD properties;
    CompilerAssemblyExpression *expression;
    CompilerArena *arena;
    if (c1_bss.assembly.reset_9fc4)
        return make_assembly_register_expression(register_code);
    if (register_code != 1 && register_code != 2 && register_code != 3 && register_code != 4 && register_code != 7 && register_code != 8 && register_code != 19) {
        ASM_UNSUPPORTED("assembler register property row is unmodeled");
    }
    properties = assembly_register_properties.indexed[(short)register_code];
    if ((properties & 0x80c0) == 0x0080)
        return qualify_assembly_register(register_code, NULL);
    if ((properties & 0xc0) == 0xc0 &&
        (c1_bss.assembly.reset_54 != 1 ||
         (c1_data.options.processor != 4 && register_code >= 49 && register_code <= 51)))
        compiler_warning(404, c1_data.current_identifier, current_assembly_mnemonic());
    c1_bss.assembly.current_operand->value->operand.kind_flags = 4;
    c1_bss.assembly.current_operand->value->operand.register_code = (BYTE)register_code;
    c1_bss.assembly.current_operand->form = (BYTE)((properties >> 8) & 15);
    arena = &c1_bss.arenas[3];
    expression = (CompilerAssemblyExpression *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    expression->kind = 3;
    expression->register_code = (int)(short)register_code;
    return expression;
}
void __cdecl select_assembly_width(BYTE width)
{
    CompilerAssemblyOperand *operand = c1_bss.assembly.current_operand;
    CompilerAssemblyOperandValue *value;
    if (operand->modifiers & 1) compiler_warning(408, current_assembly_mnemonic());
    else {
        operand->form = width;
        if (width == 15) compiler_warning(408, current_assembly_mnemonic());
    }
    operand = c1_bss.assembly.current_operand;
    if (!c1_bss.assembly.reset_04) {
        value = &operand->value->operand;
        if (value->symbol) {
            if (value->symbol == c1_data.anonymous_global_symbol && !(value->segment_and_scale & 63))
                value->segment_and_scale = (value->segment_and_scale & 0xda) | 0x1a;
            c1_bss.assembly.current_operand->value->operand.kind_flags |= 0x10;
        }
    }
    c1_bss.assembly.current_operand->modifiers |= 1;
}
void __cdecl set_assembly_address_mode(WORD) { __asm { int 3 } }
/* This diagnostic subject is a compiler-message ID, not a text pointer. */
unsigned __cdecl current_assembly_mnemonic(void)
{
    return c1_bss.assembly.reset_08 ?
        C1_TEXT_FIRST_OPERAND + c1_bss.assembly.operand_index : C1_TEXT_OPCODE;
}
CompilerIrSymbol *__cdecl resolve_assembly_symbol(CompilerNameEntry *name, int allow_labels)
{
    CompilerIrSymbol **found, *symbol;
    if (name == c1_data.builtin_names.dollar) return c1_data.anonymous_global_symbol;
    if (name == c1_data.builtin_names.local_size) {
        if (!(c1_bss.current_scope_symbol->linkage_flags & 1)) compiler_warning(486);
        return c1_data.local_size_symbol;
    }
    found = c1_data.translation_unit->FindName(name);
    symbol = found ? *found : NULL;
    if (symbol) return symbol;
    if (allow_labels) {
        symbol = find_statement_label(name);
        if (!symbol) {
            symbol = make_statement_label(name);
            record_label_reference(symbol);
        }
        return symbol;
    }
    compiler_warning(406, name, current_assembly_mnemonic());
    return declare_implicit_function(name, c1_bss.parameter_nesting);
}
unsigned __cdecl assembly_symbol_size(CompilerIrSymbol *symbol)
{
    if ((symbol->kind == 7 || symbol->kind == 8) &&
        global_type_size(symbol->value_type) == 4) return 0xffff;
    if (symbol->kind == 9) return 0xffff;
    if (symbol->qualifier_flags & 4)
        return global_type_size(get_function_return_type(symbol->value_type));
    return global_type_size(symbol->value_type);
}
unsigned __cdecl assembly_symbol_length(CompilerIrSymbol *symbol)
{
    unsigned count;
    if (symbol->kind == 9 || !(symbol->qualifier_flags & 4)) return 1;
    count = symbol->value_type->shape->value;
    return count ? count : 1;
}
CompilerIrSymbol *__cdecl find_assembly_member(CompilerIrSymbol *symbol, CompilerNameEntry *name)
{
    while (symbol) {
        if (symbol->binding.name != c1_data.builtin_names.alignment_member &&
            symbol->binding.name == name) return symbol;
        symbol = symbol->binding.next ?
            (CompilerIrSymbol *)((BYTE *)symbol->binding.next - offsetof(CompilerIrSymbol, binding)) : NULL;
    }
    return NULL;
}
unsigned __cdecl assembly_symbol_type(CompilerNameEntry *name)
{
    CompilerIrSymbol *owner = c1_bss.assembly.member_owner, *symbol;
    CompilerAssemblyOperand *operand;
    CompilerRecordMembers *members;
    if (!owner || !owner->kind4_context) {
        operand = c1_bss.assembly.current_operand;
        if (operand->value->operand.symbol && (operand->value->operand.kind_flags & 1))
            owner = operand->value->operand.symbol->value_type->symbol_owner;
        else if (operand->symbol)
            owner = operand->symbol->value_type->symbol_owner;
        else if (operand->source_symbol)
            owner = operand->source_symbol->value_type->symbol_owner;
        else owner = NULL;
        c1_bss.assembly.member_owner = owner;
    }
    if (owner && owner->kind4_context) {
        members = (CompilerRecordMembers *)owner->kind4_context;
        symbol = find_assembly_member(members->first ?
            (CompilerIrSymbol *)((BYTE *)members->first - offsetof(CompilerIrSymbol, binding)) : NULL, name);
    } else symbol = c1_data.translation_unit->FindAssemblySymbol(name);
    if (symbol && symbol->kind >= 1 && symbol->kind <= 14 && symbol->kind != 5 &&
        (symbol->kind == 2 || symbol->kind == 3) && (symbol->storage_class & 7) != 3) {
        c1_bss.assembly.member_owner = symbol->value_type->symbol_owner;
        if (!c1_bss.assembly.member_owner) {
            if (symbol->kind == 2) report_language_diagnostic(1, 401, symbol);
            c1_bss.assembly.current_operand->form = (BYTE)assembly_width_code(assembly_symbol_size(symbol));
        }
        return symbol->symbol_value;
    }
    compiler_warning(411, name, current_assembly_mnemonic());
    return 0;
}
CompilerAssemblyExpression * __cdecl make_assembly_unary(BYTE, CompilerAssemblyExpression *) { __asm { int 3 } }
CompilerAssemblyExpression * __cdecl dereference_assembly_value(CompilerAssemblyExpression *) { __asm { int 3 } }
/* OFFSET normalizes the existing expression and operand, despite this
 * historical frontier name. The final BYTE+4 store is on the operand
 * itself (its form), rather than on the separately allocated value. */
CompilerAssemblyExpression *__cdecl make_assembly_current_location(CompilerAssemblyExpression *expression)
{
    CompilerAssemblyOperandValue *value = &c1_bss.assembly.current_operand->value->operand;
    if (!(value->kind_flags & 1)) {
        if (expression->kind == 3)
            compiler_warning(404, c1_data.current_identifier, current_assembly_mnemonic());
        else compiler_warning(415);
        c1_bss.assembly.current_operand->value->operand.symbol = c1_data.anonymous_global_symbol;
        expression->kind = 2;
    }
    value = &c1_bss.assembly.current_operand->value->operand;
    value->kind_flags = (value->kind_flags & 0xe8) | 3;
    c1_bss.assembly.current_operand->form = 2;
    return expression;
}
CompilerAssemblyExpression * __cdecl qualify_assembly_register(WORD, CompilerType *) { __asm { int 3 } }
BYTE __cdecl assembly_width_code(unsigned size)
{
    if (size == 1) return 0;
    if (size == 2) return 1;
    if (size == 4 || size == 0xffff) return 2;
    if (size == 8) return 3;
    if (size == 10) return 4;
    if (size == 6 || size == 0xfffe) return 5;
    return 15;
}
void __cdecl handle_assembly_separator(void) { __asm { int 3 } }
CompilerParserToken *__cdecl allocate_assembly_token(unsigned bytes, unsigned arena_kind)
{
    BYTE *memory;
    CompilerArena *arena;
    CompilerPool *pool;
    if (!arena_kind) {
        pool = &c1_bss.pools[6]; /* Actual token-pool owner54000. */
        if (bytes > pool->remaining) pool->Refill(bytes);
        memory = pool->cursor;
        pool->cursor += bytes;
        pool->remaining -= bytes;
    } else {
        if (arena_kind >= 6) {
            ASM_UNSUPPORTED("assembler allocation arena outside known owners");
        }
        arena = &c1_bss.arenas[arena_kind];
        memory = arena->cursor;
        arena->cursor += bytes;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    }
    return (CompilerParserToken *)memory;
}
CompilerParserToken *__cdecl scan_assembly_identifier(CompilerParserToken *token)
{
    CompilerNameEntry *name = token->value.name;
    CompilerIrSymbol **found, *symbol;
    short code;
    BYTE encoding;
    if (c1_bss.assembly.reset_08) {
        if (c1_bss.assembly.reset_60) return token;
        code = lookup_assembly_register(name);
        if (code != -1) {
            if (code >= 0 && code <= 53) token->kind = 39;
            else if (code >= 54 && code <= 59) token->kind = 40;
            else if (code >= 60 && code <= 62) token->kind = 41;
            else {
                token->kind = (BYTE)((BYTE)code - 62);
                code = token->kind;
            }
            token->value.words.low = (token->value.words.low & ~0xffffU) | (unsigned short)code;
            return token;
        }
        if (lookup_assembly_opcode(name) != -1)
            report_language_diagnostic(1, 405, name);
        found = c1_data.translation_unit->FindName(name);
        symbol = found ? *found : NULL;
        if (symbol && symbol->kind == 5) {
            c1_bss.literal.typed_constant.value = (__int64)symbol->integer_value;
            token->kind = 46;
            memcpy(&token->value, &c1_bss.literal, 24);
        }
        return token;
    }
    code = lookup_assembly_opcode(name);
    if (code != -1) {
        if (code >= 0 && code <= 253) {
            c1_bss.assembly.reset_5c = (WORD)code;
            token->kind = (code >= 181 && code <= 184) ? 47 : 48;
            token->value.words.low = (token->value.words.low & ~0xffffU) | (unsigned short)code;
        } else if (code >= 254 && code <= 393) {
            if (code != 261 && code != 265 && code != 297 && code != 389) ASM_UNSUPPORTED("assembler instruction encoding row is unmodeled");
            c1_bss.assembly.reset_5c = (WORD)code;
            encoding = assembly_encoding_prefix.indexed[code].encoding;
            token->kind = (code <= 255) ? 47 : 48;
            token->value.words.low = (token->value.words.low & ~0xffffU) | encoding;
        } else {
            token->kind = 49;
            token->value.words.low = (token->value.words.low & ~0xffffU) | (unsigned short)code;
        }
        return token;
    }
    if (lookup_assembly_register(name) != -1)
        report_language_diagnostic(1, 405, name);
    return token;
}
/* The first encountered opcode bucket is m. These are the authentic
 * suffix/selector relations in search order, not a snapshot of the table.
 * Other initial letters remain unsupported before any bucket access. */
static const struct AssemblyOpcodeSpec { const char *suffix; short code; } assembly_m_opcodes[] = {
    { "ul", 34 }, { "ovzx", 194 }, { "ovsx", 196 },
    { "ovsw", 296 }, { "ovsd", 297 }, { "ovsb", 295 },
    { "ovs ", 226 }, { "ovs", 56 }, { "ovq", 319 },
    { "ovmm ", 230 }, { "ovd", 318 }, { "ov", 1 }
};
/* The encountered identifier value also searches the v bucket before C
 * lookup. Keep both authentic suffixes/order, even though value matches none. */
/* Encountered i bucket, in authentic search order. */
static const AssemblyOpcodeSpec assembly_i_opcodes[] = {
    { "retd", 315 }, { "ret", 165 }, { "nvlpg", 221 }, { "nvd", 219 },
    { "nto", 164 }, { "nt", 7 }, { "nsw", 290 }, { "nsd", 291 },
    { "nsb", 289 }, { "ns", 54 }, { "nc", 33 }, { "n", 10 },
    { "mul", 6 }, { "div", 32 }
};
/* All nine a suffix/selector relations, in the original search order. */
static const AssemblyOpcodeSpec assembly_a_opcodes[] = {
    { "rpl", 2 }, { "nd", 46 }, { "lign", 394 }, { "dd", 45 },
    { "dc", 44 }, { "as", 152 }, { "am", 151 }, { "ad", 150 }, { "aa", 149 }
};
static const AssemblyOpcodeSpec assembly_v_opcodes[] = { { "erw", 68 }, { "err", 67 } };
/* The packet identifier traverses all72 P suffix/selector relations,
 * including the packed-integer families. Lookup support gives no instruction
 * description/encoding support; those separate indexed reads stay guarded. */
static const AssemblyOpcodeSpec assembly_p_opcodes[] = {
    { "xor", 252 },
    { "ushfd", 309 },
    { "ushf", 173 },
    { "ushad", 311 },
    { "usha", 172 },
    { "ush", 13 },
    { "unpcklwd", 358 },
    { "unpckldq", 359 },
    { "unpcklbw", 357 },
    { "unpckl ", 251 },
    { "unpckhwd", 355 },
    { "unpckhdq", 356 },
    { "unpckhbw", 354 },
    { "unpckh ", 250 },
    { "subw", 348 },
    { "subusw", 353 },
    { "subusb", 352 },
    { "subus ", 249 },
    { "subsw", 351 },
    { "subsb", 350 },
    { "subs ", 248 },
    { "subd", 349 },
    { "subb", 347 },
    { "sub ", 247 },
    { "srlw", 344 },
    { "srlq", 346 },
    { "srld", 345 },
    { "srl", 246 },
    { "sraw", 342 },
    { "srad", 343 },
    { "sra", 245 },
    { "sllw", 339 },
    { "sllq", 341 },
    { "slld", 340 },
    { "sll", 244 },
    { "or", 243 },
    { "opfd", 310 },
    { "opf", 171 },
    { "opad", 312 },
    { "opa", 170 },
    { "op", 12 },
    { "mullw", 338 },
    { "mull ", 242 },
    { "mulhw", 337 },
    { "mulh ", 241 },
    { "maddwd", 336 },
    { "madd ", 240 },
    { "cmpgtw", 334 },
    { "cmpgtd", 335 },
    { "cmpgtb", 333 },
    { "cmpgt ", 239 },
    { "cmpeqw", 331 },
    { "cmpeqd", 332 },
    { "cmpeqb", 330 },
    { "cmpeq ", 238 },
    { "andn", 237 },
    { "and", 236 },
    { "addw", 324 },
    { "addusw", 329 },
    { "addusb", 328 },
    { "addus ", 235 },
    { "addsw", 327 },
    { "addsb", 326 },
    { "adds ", 234 },
    { "addd", 325 },
    { "addb", 323 },
    { "add ", 233 },
    { "ackuswb", 322 },
    { "ackus ", 232 },
    { "acksswb", 320 },
    { "ackssdw", 321 },
    { "ackss ", 231 },
};
static const AssemblyOpcodeSpec assembly_l_opcodes[] = { { "ea", 18 } };
/* All55 S relations are traversed for the reached non-opcode label. */
static const AssemblyOpcodeSpec assembly_s_opcodes[] = {
    { "ub", 50 },
    { "tr", 66 },
    { "tosw", 305 },
    { "tosd", 306 },
    { "tosb", 304 },
    { "tos", 59 },
    { "ti", 178 },
    { "td", 177 },
    { "tc", 176 },
    { "msw", 65 },
    { "ldt", 64 },
    { "idt", 29 },
    { "hrd", 203 },
    { "hr", 43 },
    { "hld", 202 },
    { "hl", 42 },
    { "gdt", 28 },
    { "etz", 389 },
    { "ets", 388 },
    { "etpo", 386 },
    { "etpe", 385 },
    { "etp", 387 },
    { "eto", 384 },
    { "etnz", 383 },
    { "etns", 382 },
    { "etnp", 381 },
    { "etno", 380 },
    { "etnle", 379 },
    { "etnl", 378 },
    { "etnge", 377 },
    { "etng", 376 },
    { "etne", 375 },
    { "etnc", 374 },
    { "etnbe", 373 },
    { "etnb", 372 },
    { "etnae", 371 },
    { "etna", 370 },
    { "etle", 369 },
    { "etl", 368 },
    { "etge", 367 },
    { "etg", 366 },
    { "ete", 365 },
    { "etc", 364 },
    { "etbe", 363 },
    { "etb", 362 },
    { "etae", 361 },
    { "eta", 360 },
    { "casw", 302 },
    { "casd", 303 },
    { "casb", 301 },
    { "cas", 58 },
    { "bb", 49 },
    { "ar", 41 },
    { "al", 391 },
    { "ahf", 174 },
};
/* The destination identifier traverses the complete D bucket. */
static const AssemblyOpcodeSpec assembly_d_opcodes[] = {
    { "iv", 31 },
    { "ec", 30 },
    { "as", 161 },
    { "aa", 160 },
};
/* Complete W search relations encountered by the words identifier. */
static const AssemblyOpcodeSpec assembly_w_opcodes[] = {
    { "binvd", 220 },
    { "ait", 179 },
};
static const AssemblyOpcodeSpec assembly_c_opcodes[] = { { "mp", 47 }, { "ld", 155 } };
static const AssemblyOpcodeSpec assembly_j_opcodes[] = { { "le", 265 }, { "e", 261 } };
static const AssemblyOpcodeSpec assembly_x_opcodes[] = { { "or", 52 }, { "chg", 9 } };
static const AssemblyOpcodeSpec assembly_h_opcodes[] = { { "lt", 162 } };
static const AssemblyOpcodeSpec assembly_t_opcodes[] = { { "est", 51 } };
/* The reached C variable result traverses every R relation before no match. */
static const AssemblyOpcodeSpec assembly_r_opcodes[] = {
    { "or", 40 }, { "ol", 39 }, { "etn", 316 }, { "etf", 317 }, { "et", 8 },
    { "epz", 184 }, { "epnz", 183 }, { "epne", 255 }, { "epe", 254 }, { "ep", 181 },
    { "cr", 38 }, { "cl", 37 }
};
short __cdecl lookup_assembly_opcode(CompilerNameEntry *name)
{
    const char *text = name->name;
    const AssemblyOpcodeSpec *first, *last;
    int initial;
    if (*text == '$') return -1;
    initial = (tolower)((signed char)*text++);
    if (initial == 'x') {
        first = assembly_x_opcodes;
        last = first + sizeof(assembly_x_opcodes) / sizeof(assembly_x_opcodes[0]);
    } else if (initial == 'h') {
        first = assembly_h_opcodes;
        last = first + sizeof(assembly_h_opcodes) / sizeof(assembly_h_opcodes[0]);
    } else if (initial == 't') {
        first = assembly_t_opcodes;
        last = first + sizeof(assembly_t_opcodes) / sizeof(assembly_t_opcodes[0]);
    } else if (initial == 'r') {
        first = assembly_r_opcodes;
        last = first + sizeof(assembly_r_opcodes) / sizeof(assembly_r_opcodes[0]);
    } else if (initial == 'w') {
        first = assembly_w_opcodes;
        last = first + sizeof(assembly_w_opcodes) / sizeof(assembly_w_opcodes[0]);
    } else if (initial == 'd') {
        first = assembly_d_opcodes;
        last = first + sizeof(assembly_d_opcodes) / sizeof(assembly_d_opcodes[0]);
    } else if (initial == 'c') {
        first = assembly_c_opcodes;
        last = first + sizeof(assembly_c_opcodes) / sizeof(assembly_c_opcodes[0]);
    } else if (initial == 'j') {
        first = assembly_j_opcodes;
        last = first + sizeof(assembly_j_opcodes) / sizeof(assembly_j_opcodes[0]);
    } else if (initial == 'a') {
        first = assembly_a_opcodes;
        last = first + sizeof(assembly_a_opcodes) / sizeof(assembly_a_opcodes[0]);
    } else if (initial == 'p') {
        first = assembly_p_opcodes;
        last = first + sizeof(assembly_p_opcodes) / sizeof(assembly_p_opcodes[0]);
    } else if (initial == 's') {
        first = assembly_s_opcodes;
        last = first + sizeof(assembly_s_opcodes) / sizeof(assembly_s_opcodes[0]);
    } else if (initial == 'l') {
        first = assembly_l_opcodes;
        last = first + sizeof(assembly_l_opcodes) / sizeof(assembly_l_opcodes[0]);
    } else if (initial == 'm') {
        first = assembly_m_opcodes;
        last = first + sizeof(assembly_m_opcodes) / sizeof(assembly_m_opcodes[0]);
    } else if (initial == 'i') {
        first = assembly_i_opcodes;
        last = first + sizeof(assembly_i_opcodes) / sizeof(assembly_i_opcodes[0]);
    } else if (initial == 'v') {
        first = assembly_v_opcodes;
        last = first + sizeof(assembly_v_opcodes) / sizeof(assembly_v_opcodes[0]);
    } else {
        ASM_UNSUPPORTED("assembler opcode initial-letter bucket is unmodeled");
    }
    for (; first != last; ++first)
        if (!compare_label_spelling(first->suffix, text)) return first->code;
    if (initial == 'l' || initial == 'c' || initial == 'j' || initial == 'x') ASM_UNSUPPORTED("assembler suffix relation is unmodeled");
    return -1;
}
/* Encountered e bucket in original search order. The relation also includes
 * assembler word operators (eq), so codes above62 keep their token role. */
static const struct AssemblyRegisterSpec { const char *suffix; short code; } assembly_e_registers[] = {
    { "sp", 5 }, { "si", 7 }, { "s", 25 }, { "q", 63 },
    { "dx", 3 }, { "di", 8 }, { "cx", 2 }, { "bx", 4 },
    { "bp", 6 }, { "ax", 1 }
};
static const AssemblyRegisterSpec assembly_d_registers[] = {
    { "x", 11 }, { "word", 55 }, { "s", 28 }, { "r7", 48 }, { "r6", 47 },
    { "r3", 46 }, { "r2", 45 }, { "r1", 44 }, { "r0", 43 },
    { "l", 19 }, { "i", 16 }, { "h", 23 }
};
static const AssemblyRegisterSpec assembly_b_registers[] = {
    { "yte", 59 }, { "x", 12 }, { "p", 14 }, { "l", 20 }, { "h", 24 }
};
static const AssemblyRegisterSpec assembly_h_registers[] = { { "igh", 76 } };
static const AssemblyRegisterSpec assembly_l_registers[] = {
    { "t", 67 }, { "ow", 78 }, { "ength", 77 }, { "e", 66 }
};
static const AssemblyRegisterSpec assembly_o_registers[] = {
    { "r", 72 }, { "ffset", 79 }
};
static const AssemblyRegisterSpec assembly_p_registers[] = { { "tr", 80 } };
static const AssemblyRegisterSpec assembly_s_registers[] = {
    { "t", 31 }, { "s", 27 }, { "p", 13 }, { "ize", 82 }, { "i", 15 },
    { "hr", 74 }, { "hort", 60 }, { "hl", 73 }, { "eg", 81 }
};
/* Complete W search relations encountered by the words identifier. */
static const AssemblyRegisterSpec assembly_t_registers[] = {
    { "ype", 83 }, { "r7", 53 }, { "r6", 52 }, { "r5", 51 },
    { "r4", 50 }, { "r3", 49 }, { "byte", 58 }
};
static const AssemblyRegisterSpec assembly_w_registers[] = {
    { "ord", 54 },
};
short __cdecl lookup_assembly_register(CompilerNameEntry *name)
{
    const char *text = name->name;
    const AssemblyRegisterSpec *first, *last;
    int initial;
    if (*text == '$') return -1;
    initial = (tolower)((signed char)*text++);
    /* The original i and v boundary pairs are genuinely empty buckets. */
    if (initial == 'i' || initial == 'r' || initial == 'v') return -1;
    if (initial == 'b') {
        first = assembly_b_registers;
        last = first + sizeof(assembly_b_registers) / sizeof(assembly_b_registers[0]);
    } else if (initial == 'e') {
        first = assembly_e_registers;
        last = first + sizeof(assembly_e_registers) / sizeof(assembly_e_registers[0]);
    } else if (initial == 's') {
        first = assembly_s_registers;
        last = first + sizeof(assembly_s_registers) / sizeof(assembly_s_registers[0]);
    } else if (initial == 't') {
        first = assembly_t_registers;
        last = first + sizeof(assembly_t_registers) / sizeof(assembly_t_registers[0]);
    } else if (initial == 'w') {
        first = assembly_w_registers;
        last = first + sizeof(assembly_w_registers) / sizeof(assembly_w_registers[0]);
    } else if (initial == 'd') {
        first = assembly_d_registers;
        last = first + sizeof(assembly_d_registers) / sizeof(assembly_d_registers[0]);
    } else if (initial == 'h') {
        first = assembly_h_registers;
        last = first + sizeof(assembly_h_registers) / sizeof(assembly_h_registers[0]);
    } else if (initial == 'l') {
        first = assembly_l_registers;
        last = first + sizeof(assembly_l_registers) / sizeof(assembly_l_registers[0]);
    } else if (initial == 'o') {
        first = assembly_o_registers;
        last = first + sizeof(assembly_o_registers) / sizeof(assembly_o_registers[0]);
    } else if (initial == 'p') {
        first = assembly_p_registers;
        last = first + sizeof(assembly_p_registers) / sizeof(assembly_p_registers[0]);
    } else {
        ASM_UNSUPPORTED("assembler register initial-letter bucket is unmodeled");
    }
    for (; first != last; ++first)
        if (!compare_label_spelling(first->suffix, text)) return first->code;
    return -1;
}
/* Shared by label and assembler spelling lookup. On an unequal pair whose
 * uppercase forms differ, the original subtracts the signed raw bytes rather
 * than the folded values. Pointer identity and NULL shortcuts precede reads. */
int __cdecl compare_label_spelling(const char *left, const char *right)
{
    signed char left_character, right_character;
    if (left == right) return 0;
    if (!left || !right) return -1;
    for (;;) {
        left_character = (signed char)*left++;
        right_character = (signed char)*right++;
        if (left_character != right_character) {
            if ((toupper)(left_character) != (toupper)(right_character))
                return (int)left_character - (int)right_character;
        } else if (!left_character) return 0;
    }
}
CompilerAssemblyExpression *__cdecl make_assembly_register_expression(WORD register_code)
{
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerAssemblyExpression *expression = (CompilerAssemblyExpression *)arena->cursor;
    CompilerAssemblyOperandValue *value;
    WORD properties;
    BYTE base_register;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    expression->kind = 0;
    expression->payload = 0;
    value = &c1_bss.assembly.current_operand->value->operand;
    base_register = value->register_code;
    if (base_register && value->index_register) {
        compiler_warning(404, c1_data.current_identifier, current_assembly_mnemonic());
        return expression;
    }
    if (register_code != 1 && register_code != 2 && register_code != 3 && register_code != 4 && register_code != 6) {
        ASM_UNSUPPORTED("assembler base/index register property row is unmodeled");
    }
    properties = assembly_register_properties.indexed[(short)register_code];
    if (!(properties & 0xc0) && (properties & 0xf00) == 0x200) {
        if (base_register) value->index_register = (BYTE)register_code;
        else value->register_code = (BYTE)register_code;
        value->kind_flags = (value->kind_flags & 0xef) | 0x16;
        expression->kind = 1;
    } else if (!(properties & 0x6000)) {
        compiler_warning(403, c1_data.current_identifier, current_assembly_mnemonic());
    } else if ((properties & 0x2000) && base_register) {
        compiler_warning(402, c1_data.current_identifier, current_assembly_mnemonic());
    } else if ((properties & 0x4000) && value->index_register) {
        compiler_warning(401, c1_data.current_identifier, current_assembly_mnemonic());
    } else {
        compiler_warning(432, current_assembly_mnemonic());
    }
    return expression;
}
}
void RECON_THISCALL CompilerIrSymbol::MarkAssemblyReference(void)
{
    unsigned count = reference_flags & 3U;
    if (count < 3) reference_flags = (reference_flags & ~3U) | (count + 1);
    reference_flags = (reference_flags & ~4U) | ((c1_data.ir_reference_epoch & 1U) << 2);
}

CompilerIrSymbol *RECON_THISCALL CompilerTranslationUnit::FindAssemblySymbol(CompilerNameEntry *) { __asm { int 3 } }
