/* Separate yacc driver and semantic actions for inline assembly.
   Original 0x4ca52..0x4d58d; logical grammar is in asm-grammar.y. */
#include "asm.h"
#include "global-ir.h"
#include "diagnostics.h"
#include <recon.h>

/* The historical bound admits one final value over the beginning of the
   state stack. Its initial state pointer also aliases the last value halfword. */
typedef union AsmParserStackStorage {
    struct { ParserValue values[151]; short states[151]; } layout;
    ParserValue value_view[152];
    struct {
        ParserValue values[150];
        unsigned reserved_word;
        unsigned short reserved_halfword;
        short with_sentinel[152];
    } state_view;
} AsmParserStackStorage;
AsmParserStackStorage asm_parser_stack; /* original 0x68368..0x68950 */
typedef char AsmParserStack_size[(sizeof(AsmParserStackStorage) == 1512) ? 1 : -1];

/* The empty recovery list's third halfword is the low word of lookahead.
   Keep the original alias and its following parser counters in one owner. */
typedef union AsmParserControl {
    struct {
        short end_state, end_token;
        int lookahead;
        unsigned error_count;
        short recovering;
    } fields;
    ParserRecovery recoveries[2];
} AsmParserControl;
AsmParserControl asm_parser_control = { { -1000, 0, -1, 0, 0 } }; /* original 0x710f4 */
#define ASM_LOOKAHEAD asm_parser_control.fields.lookahead
#define ASM_RECOVERING asm_parser_control.fields.recovering
#define ASM_EXPRESSION(value_) ((AsmExpression *)(value_).words.low)
#define ASM_RESULT(expression_) (parser_value.words.low = (unsigned)(expression_))
/* The C vocabulary already owns these type codes; do not duplicate its table. */
#define ASM_DECLARATION_CODE(result_, token_) do { \
    result_ = 0; \
    for (declaration_index = 0; declaration_index < declaration_code_count; ++declaration_index) \
        if (declaration_codes[declaration_index].token == (token_)) { \
            result_ = declaration_codes[declaration_index].value; break; \
        } \
} while (0)

int parse_inline_assembler(void)
{
    short state = 0;
    short *state_top = asm_parser_stack.state_view.with_sentinel;
    short *state_base = state_top + 1;
    ParserValue *value_top = asm_parser_stack.value_view;
    ParserValue value = parser_value, *right;
    ParserState *row;
    ParserTransition *transition;
    ParserGoto *go;
    ParserRecovery *recovery;
    AsmExpression *expression;
    FrontendSymbol *symbol;
    unsigned i, rule, length, nonterminal, declaration_code, declaration_index;
    int reduction, expected;

    for (;;) {
    push_state:
        parser_value = value;
        if (++state_top > asm_parser_stack.state_view.with_sentinel + 151) {
            report_asm_parser_error("yacc stack overflow");
            return 1;
        }
        *state_top = state;
        *++value_top = parser_value;
        for (;;) {
            row = &asm_states[state];
            if (row->needs_token && ASM_LOOKAHEAD < 0) ASM_LOOKAHEAD = read_asm_token();
            transition = asm_shifts + row->first_shift;
            for (i = 0; i < row->shift_count; ++i, ++transition) {
                if (transition->symbol == ASM_LOOKAHEAD) {
                    ASM_LOOKAHEAD = -1;
                    state = transition->destination;
                    value.token = current_token_value;
                    if (ASM_RECOVERING > 0) --ASM_RECOVERING;
                    goto push_state;
                }
            }
            reduction = row->default_rule;
            transition = asm_reductions + row->first_exception;
            for (i = 0; i < row->exception_count; ++i, ++transition)
                if (transition->symbol == ASM_LOOKAHEAD) {
                    reduction = transition->destination;
                    break;
                }
            if (reduction < 0) return 0;
            if (reduction) break;
            if (!ASM_RECOVERING) {
                recovery = asm_parser_control.recoveries;
                while (recovery->state != -1000 && state > recovery->state) ++recovery;
                if (recovery->state == state) {
                    expected = recovery->token;
                    if (expected < 0) {
                        ++ASM_RECOVERING;
                        expected = -expected;
                    }
                    if (ASM_RECOVERING < 2)
                        report_asm_expected_token(ASM_LOOKAHEAD, expected);
                    value = asm_recovery_value(expected);
                    state = recovery->next_state;
                    goto push_state;
                }
                report_asm_parser_error("syntax error");
                ++asm_parser_control.fields.error_count;
                goto recover_error;
            }
            if (ASM_RECOVERING < 1) break;
            if (ASM_RECOVERING < 3) goto recover_error;
            if (ASM_RECOVERING != 3) break;
            if (!ASM_LOOKAHEAD) return 1;
            ASM_LOOKAHEAD = -1;
        }
        rule = reduction;
        length = asm_productions[rule].length;
        right = value_top;
        value_top -= length;
        state_top -= length;
        parser_value = value_top[1];
        nonterminal = asm_productions[rule].nonterminal;
        go = &asm_gotos[nonterminal];
        state = go->default_state;
        transition = asm_goto_rows + go->first_transition;
        for (i = 0; i < go->transition_count; ++i, ++transition)
            if (transition->symbol == *state_top) {
                state = transition->destination;
                break;
            }
        switch (rule) {
        case 3:
            release_arena(2);
            if (finish_asm_line()) return 1;
            initialize_inline_assembler();
            break;
        case 4:
            ASM_RECOVERING = 0;
            release_arena(2);
            if (finish_asm_line()) return 1;
            initialize_inline_assembler();
            break;
        case 7:
            ++asm_open_braces;
            break;
        case 9:
            emit_asm_label(right[-1].name);
            break;
        case 11:
            set_asm_directive((short)right[-1].words.low, right[0].words.low);
            break;
        case 12:
            asm_operand_mode = 1;
            select_asm_opcode((short)right[0].words.low);
            break;
        case 13:
            emit_asm_instruction();
            break;
        case 14:
            asm_member_mode = 0;
            break;
        case 15:
            asm_member_mode = 1;
            break;
        case 16:
            asm_operand_count = 0;
            parser_value.words.low = 0;
            break;
        case 17:
            asm_operand_count = 1;
            parser_value.words.low = right[0].words.low;
            break;
        case 20:
        case 21:
            select_asm_opcode((short)right[0].words.low);
            emit_asm_instruction();
            break;
        case 23:
            ++asm_operand_count;
            break;
        case 25:
            if (asm_operand_count < 2) asm_operand = reset_asm_operand(++asm_operand_count);
            break;
        case 27:
            expression = ASM_EXPRESSION(right[0]);
            if (expression->kind != 3) add_asm_displacement(expression->value.integer);
            break;
        case 28:
            expression = ASM_EXPRESSION(right[0]);
            if (!expression->kind)
                expression = multiply_asm_expression(right[-1].bytes[0], ASM_EXPRESSION(right[-2]), expression);
            else
                expression = multiply_asm_expression(right[-1].bytes[0], expression, ASM_EXPRESSION(right[-2]));
            ASM_RESULT(expression);
            break;
        case 29: case 30: case 31: case 32: case 33: case 34: case 35:
        case 36: case 37: case 38: case 39: case 40: case 41:
            ASM_RESULT(evaluate_asm_binary(right[-1].bytes[0], ASM_EXPRESSION(right[-2]), ASM_EXPRESSION(right[0])));
            break;
        case 42: case 43:
            ASM_RESULT(combine_asm_addresses(right[-1].bytes[0], ASM_EXPRESSION(right[-2]), ASM_EXPRESSION(right[0])));
            break;
        case 44:
            asm_explicit_size = 1;
            asm_address_mode = 1;
            expression = ASM_EXPRESSION(right[-2]);
            if (!expression->kind && expression->value.integer == 65535)
                set_asm_address_operator(0x35);
            else if (!expression->kind && expression->value.integer == 65534)
                set_asm_address_operator(0x36);
            else {
                asm_error_context();
                report_syntax_error(408);
            }
            parser_value.words.low = right[0].words.low;
            break;
        case 45:
            asm_member_mode = 1;
            symbol = lookup_asm_symbol(right[-1].name, 0);
            asm_member_context = (FrontendSymbol *)symbol->type->field08;
            break;
        case 46:
            asm_member_mode = 0;
            ASM_RESULT(combine_asm_addresses(right[-2].bytes[0], ASM_EXPRESSION(right[-4]), ASM_EXPRESSION(right[0])));
            break;
        case 47:
            asm_member_mode = 1;
            asm_member_context = (FrontendSymbol *)((CType *)right[-1].words.high)->field08;
            break;
        case 48:
            asm_member_mode = 0;
            ASM_RESULT(combine_asm_addresses(right[-2].bytes[0], ASM_EXPRESSION(right[-4]), ASM_EXPRESSION(right[0])));
            break;
        case 49:
            asm_member_mode = 1;
            break;
        case 50:
            asm_member_mode = 0;
            ASM_RESULT(combine_asm_addresses(right[-2].bytes[0], ASM_EXPRESSION(right[-3]), ASM_EXPRESSION(right[0])));
            break;
        case 52:
            parser_value.words.low = right[-1].words.low;
            break;
        case 53: case 54: case 55: case 56: case 57:
            ASM_RESULT(evaluate_asm_unary(right[-1].bytes[0], ASM_EXPRESSION(right[0])));
            break;
        case 58:
            ASM_RESULT(apply_asm_segment((short)right[-2].words.low, ASM_EXPRESSION(right[0])));
            break;
        case 59:
            ASM_RESULT(make_asm_constant(type_size((CType *)right[0].words.high)));
            break;
        case 60:
            ASM_DECLARATION_CODE(declaration_code, right[0].bytes[0]);
            ASM_RESULT(make_asm_constant(declaration_code & 15));
            break;
        case 61:
            ASM_RESULT(make_asm_constant(asm_operand_size_codes[(short)right[0].words.low - 46]));
            break;
        case 62:
            symbol = lookup_asm_symbol(right[0].name, 1);
            ASM_RESULT(make_asm_constant(asm_symbol_type_size(symbol)));
            break;
        case 63:
            symbol = lookup_asm_symbol(right[0].name, 0);
            ASM_RESULT(make_asm_constant(type_size(symbol->type)));
            break;
        case 64:
            symbol = lookup_asm_symbol(right[0].name, 0);
            ASM_RESULT(make_asm_constant(asm_symbol_length(symbol)));
            break;
        case 65: case 66:
            ASM_RESULT(make_asm_constant(right[0].words.low));
            break;
        case 67:
            if (!asm_member_mode) ASM_RESULT(make_asm_symbol(right[0].name));
            else {
                ASM_RESULT(make_asm_constant(asm_member_offset(right[0].name)));
                asm_member_mode = 0;
            }
            break;
        case 68:
            ASM_RESULT(make_asm_segment_value(make_asm_symbol(right[0].name)));
            break;
        case 69:
            ASM_RESULT(make_asm_offset(ASM_EXPRESSION(right[0])));
            break;
        case 70:
            ASM_RESULT(make_asm_register((short)right[0].words.low));
            break;
        case 71:
            ASM_RESULT(make_asm_stack_register((short)right[-3].words.low, right[-1].words.low));
            break;
        case 72:
            asm_explicit_size = 1;
            asm_address_mode = 0;
            set_asm_operand_size(asm_operand_size_codes[(short)right[-2].words.low - 46]);
            parser_value.words.low = right[0].words.low;
            break;
        case 73:
            asm_explicit_size = 1;
            asm_address_mode = 0;
            set_asm_operand_size(asm_size_encoding(type_size((CType *)right[-2].words.high)));
            parser_value.words.low = right[0].words.low;
            break;
        case 74:
            asm_explicit_size = 1;
            asm_address_mode = 0;
            ASM_DECLARATION_CODE(declaration_code, right[-2].bytes[0]);
            set_asm_operand_size(asm_size_encoding(declaration_code & 15));
            parser_value.words.low = right[0].words.low;
            break;
        case 75:
            asm_explicit_size = 1;
            asm_address_mode = 1;
            set_asm_address_operator((short)right[-2].words.low);
            parser_value.words.low = right[0].words.low;
            break;
        case 76:
            asm_explicit_size = 0;
            asm_address_mode = 1;
            ASM_DECLARATION_CODE(declaration_code, right[-1].bytes[0]);
            if (declaration_code == 0x42) set_asm_address_operator(0x34);
            else {
                asm_error_context();
                report_syntax_error(409);
            }
            ASM_RESULT(make_asm_symbol(right[0].name));
            break;
        case 77:
            asm_explicit_size = 0;
            asm_address_mode = 1;
            set_asm_address_operator((short)right[-1].words.low);
            ASM_RESULT(make_asm_symbol(right[0].name));
            break;
        case 78:
            asm_brackets_used = 0;
            parser_value.words.low = right[-1].words.low;
            break;
        case 79:
            if (asm_brackets_used) {
                asm_error_context();
                report_syntax_error(426);
            }
            asm_brackets_used = 1;
            break;
        case 80:
            asm_brackets_used = 0;
            parser_value.words.low = right[-1].words.low;
            break;
        case 81:
            if (asm_brackets_used) {
                asm_error_context();
                report_syntax_error(426);
            }
            asm_brackets_used = 1;
            break;
        case 82:
            ASM_RESULT(combine_asm_addresses(right[-2].bytes[0], ASM_EXPRESSION(right[-3]), ASM_EXPRESSION(right[0])));
            break;
        }
        value = parser_value;
        continue;

    recover_error:
        ASM_RECOVERING = 3;
        while (state_top >= state_base) {
            row = &asm_states[*state_top];
            transition = asm_shifts + row->first_shift;
            for (i = 0; i < row->shift_count; ++i, ++transition)
                if (transition->symbol == AS_ERROR) {
                    state = transition->destination;
                    value = parser_value;
                    goto push_state;
                }
            --state_top;
            --value_top;
        }
        return 1;
    }
}

/* Original0x730a0: display vocabulary in AsmToken order. This table
   names logical tokens used by diagnostics rather than packed parser data. */
char *asm_token_names[50] = {
    "no token",
    "EQ", "GE", "GT", "LE", "LT", "NE",
    "AND", "MOD", "NOT", "OR", "SHL", "SHR", "XOR",
    "HIGH", "LENGTH", "LOW", "OFFSET", "PTR", "SEG", "SIZE", "TYPE",
    "+", "-", "*", "/", ":", ",", "[", "]", "(", ")", "{", ";",
    "newline", ".", "end of file", "bad token", "mapped", "register",
    "asm data type", "asm code type", "data type", "typedef name",
    "identifier", "constant", "enum member", "prefix", "opcode", "directive"
};

/* Original0x4c9f5..0x4ca52, RET4. The identifier special case tests
   the parser message's first byte, not the identifier's first byte; retain
   that distinct original predicate. Only yacc stack overflow is fatal. */
void __stdcall report_asm_parser_error(char *text)
{
    char *found, *context;
    if (*text == 'y') fatal_error(26);
    if (asm_current_token == AS_IDENTIFIER && current_token_value.identifier && *text)
        found = current_token_value.identifier;
    else found = asm_token_names[asm_current_token];
    context = asm_error_context();
    report_syntax_error(400,context,found);
    asm_operand_mode = 1;
}
