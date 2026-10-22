/* Declaration-lookahead yacc driver. Original code 0x75230..0x75a52;
   its 165-entry semantic switch occupies 0x75a54..0x75ce8. */
#include "lookahead.h"
#include "lookahead-grammar.h"
#include <recon.h>

#define POP_LOOKAHEAD_DECLARATION() do { \
    if (declaration_depth > 0) --declaration_depth; \
} while (0)

int lookahead_parse(void)
{
    ParserState *state_info;
    ParserTransition *transition;
    ParserGoto *goto_info;
    ParserProduction *production;
    ParserValue *right;
    short rule;
    unsigned i;
    for (;;) {
push_state:
        if (++lookahead_state_top > lookahead_state_stack + 150) {
            report_lookahead_error("yacc stack overflow");
            return 1;
        }
        *lookahead_state_top = lookahead_state;
        *++lookahead_value_top = parser_value;
read_action:
        state_info = &lookahead_states[lookahead_state];
        if (state_info->needs_token && lookahead_token < 0)
            lookahead_token = read_parser_token();
        transition = lookahead_shifts + state_info->first_shift;
        for (i = 0; i < state_info->shift_count; ++i, ++transition) {
            if (transition->symbol == lookahead_token) {
                lookahead_token = -1;
                parser_value = current_token_value;
                lookahead_state = transition->destination;
                if (lookahead_recovery_count > 0) --lookahead_recovery_count;
                goto push_state;
            }
        }
        rule = state_info->default_rule;
        transition = lookahead_exceptions + state_info->first_exception;
        for (i = 0; i < state_info->exception_count; ++i, ++transition) {
            if (transition->symbol == lookahead_token) {
                rule = transition->destination;
                break;
            }
        }
        if (rule < 0) return 0;
        if (!rule) {
            switch (lookahead_recovery_count) {
            case 0:
                report_lookahead_error("syntax error");
                ++lookahead_error_count;
                /* Continue into the ordinary yacc error-token search. */
            case 1:
            case 2:
                lookahead_recovery_count = 3;
                while (lookahead_state_top >= lookahead_state_stack) {
                    state_info = &lookahead_states[*lookahead_state_top];
                    transition = lookahead_shifts + state_info->first_shift;
                    for (i = 0; i < state_info->shift_count; ++i, ++transition) {
                        if (transition->symbol == PARSER_ERROR_TOKEN) {
                            lookahead_state = transition->destination;
                            goto push_state;
                        }
                    }
                    --lookahead_state_top;
                    --lookahead_value_top;
                }
                return 1;
            case 3:
                if (!lookahead_token) return 1;
                lookahead_token = -1;
                goto read_action;
            }
        }
        right = lookahead_value_top;
        production = &lookahead_productions[rule];
        lookahead_state_top -= production->length;
        lookahead_value_top -= production->length;
        parser_value = lookahead_value_top[1];
        goto_info = &lookahead_gotos[production->nonterminal];
        lookahead_state = goto_info->default_state;
        transition = lookahead_goto_transitions + goto_info->first_transition;
        for (i = 0; i < goto_info->transition_count; ++i, ++transition) {
            if (transition->symbol == *lookahead_state_top) {
                lookahead_state = transition->destination;
                break;
            }
        }
        switch (rule) {
        case 1: /* completed declaration: classify its enclosing delimiter */
            if (!declaration_depth) {
                if (right[0].words.low & 2) return 0;
                if (lookahead_token == -1) lookahead_token = read_parser_token();
                return lookahead_token == 127 || lookahead_token == 133 ||
                    lookahead_token == 138;
            }
            if (right[0].words.low & 1) return 0;
            if (lookahead_token == -1) lookahead_token = read_parser_token();
            return lookahead_token == 136;
        case 2:
            return right[0].words.low;
        case 3:
            return 0;
        case 4:
            parser_value.words.low = 0;
            declaration_spec_flags[declaration_depth] &= ~2;
            break;
        case 5:
            parser_value.words.low |= right[0].words.low;
            declaration_spec_flags[declaration_depth] &= ~2;
            break;
        case 6:
        case 7:
        case 8:
            parser_value.words.low = 1;
            break;
        case 9:
            return 1;
        case 10:
            parser_value.words.low = 2;
            break;
        case 11:
        case 50:
        case 53:
        case 56:
        case 57:
        case 58:
        case 83:
        case 133:
        case 141:
        case 142:
        case 143:
        case 146:
        case 157:
        case 165:
            POP_LOOKAHEAD_DECLARATION();
            break;
        case 21:
        case 71:
            POP_LOOKAHEAD_DECLARATION();
            declaration_spec_flags[declaration_depth] |= 4;
            break;
        case 25:
        case 27:
        case 138:
        case 140:
            token_allocation_flags &= ~4UL;
            break;
        case 32:
            POP_LOOKAHEAD_DECLARATION();
            parser_value.words.low = 5;
            break;
        case 33:
        case 34:
            POP_LOOKAHEAD_DECLARATION();
            parser_value.words.low = 3;
            break;
        case 37:
        case 38:
            declaration_spec_flags[declaration_depth] &= ~3;
            break;
        case 60:
        case 61:
        case 62:
            parser_value.words.low = 2;
            break;
        case 64:
            parser_value.words.low = right[0].words.low;
            break;
        case 117:
            token_allocation_flags &= ~1UL;
            break;
        case 119:
        case 120:
            --allow_incomplete_class;
            break;
        case 121:
            cpp_parser_flags &= ~1UL;
            break;
        case 149:
            cpp_parser_flags &= ~0x40UL;
            declaration_spec_flags[declaration_depth] |= 3;
            cpp_parser_flags &= ~1UL;
            break;
        case 154:
            cpp_parser_flags |= 1;
            break;
        case 155:
            cpp_parser_flags |= 0x41;
            declaration_spec_flags[declaration_depth] &= ~3;
            break;
        }
    }
}
