/* C++ translation-unit yacc driver and its complete semantic switch.
   Original code 0x268f0..0x29fa1, switch data 0x29fa4..0x2a524.
   Logical tables are generated from the declarative grammar. */
#include "parser.h"
#include "parser-grammar.h"
#include "parser-state.h"
#include "source-input.h"
#include "parameters.h"
#include "operators.h"
#include <recon.h>

#define POP_DECLARATION() do { if (declaration_depth > 0) --declaration_depth; } while (0)
#define CLEAR_DECLARATION_FLAGS() (declaration_spec_flags[declaration_depth] &= ~3)
#define CURRENT_CLASS_SYMBOL (class_contexts[current_class_context].symbol)
#define READ_DECLARATION_CODE(destination_, token_) do { \
    for (i = 0; i < declaration_code_count; ++i) \
        if (declaration_codes[i].token == (token_)) break; \
    (destination_) = i < declaration_code_count ? declaration_codes[i].value : 0x9e; \
} while (0)
#define ASSIGN_TYPE_SYMBOL(name_, kind_) do { \
    if (next_symbol_index >= 0x7fffffffU) fatal_error(55); \
    if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover(); \
    parser_value.symbol->index = next_symbol_index++; \
    parser_value.symbol->kind = (kind_); \
    parser_value.symbol->name = (name_); \
    parser_value.symbol->references = 1; \
} while (0)
#define COPY_SYMBOL_STORAGE(value_) do { \
    (value_).bytes[0] &= 0x7f; \
    parser_value.symbol->storage[0] = (value_).bytes[0]; \
    parser_value.symbol->storage[1] = (value_).bytes[1]; \
    parser_value.symbol->storage[2] = (value_).bytes[2]; \
} while (0)
#define ALLOCATE_PARSER_NODE(destination, bytes, arena) do { \
    allocation_arena = (arena); \
    if (!allocation_arena) (destination) = allocate_permanent(bytes); \
    else { \
        if (allocation_arena == -1) allocation_arena = 0; \
        pool = &compiler_arena.pools[allocation_arena]; \
        if (pool->remaining < (int)(bytes)) refill_arena(&compiler_arena, pool); \
        (destination) = (void *)pool->next; \
        pool->remaining -= (bytes); \
        pool->next += (bytes); \
    } \
} while (0)
#define RESET_EXPRESSION_ARENA() do { \
    if (!parser_recovery_count) release_arena(2); \
    text_work_cursor = expression_storage; \
} while (0)

int parse_translation_unit(void)
{
    short state = 0, rule;
    ParserValue *string_destination;
    IrToken *switch_condition, *case_expression;
    int depth = -1, i, recovery, allocation_arena;
    ParserState *state_info;
    ParserGoto *goto_info;
    ParserProduction *production;
    ParserTransition *transition;
    ParserRecovery *repair;
    ParserValue *right;
    CppSymbol *symbol;
    CppSymbol *member_class;
    CType *type;
    TypeDescription *description;
    TypeShape *shape;
    IrToken *expression;
    QualifiedDeclarator *qualified;
    QualifierBinding *binding;
    ArenaPool *pool;
    unsigned flags, attributes;
    char *parser_name;
    for (;;) {
push_state:
        if (++depth > 220) {
            report_parser_error("yacc stack overflow");
            return 1;
        }
        parser_state_stack[depth] = state;
        parser_value_stack[depth] = parser_value;
read_action:
        state_info = &parser_states[state];
        if (state_info->needs_token && parser_lookahead < 0)
            parser_lookahead = read_parser_token();
        transition = parser_shifts + state_info->first_shift;
        for (i = 0; i < state_info->shift_count; ++i, ++transition) {
            if (transition->symbol == parser_lookahead) {
                state = transition->destination;
                parser_lookahead = -1;
                parser_value = current_token_value;
                if (parser_recovery_count > 0) --parser_recovery_count;
                goto push_state;
            }
        }
        rule = state_info->default_rule;
        transition = parser_exceptions + state_info->first_exception;
        for (i = 0; i < state_info->exception_count; ++i, ++transition) {
            if (transition->symbol == parser_lookahead) {
                rule = transition->destination;
                break;
            }
        }
        if (rule < 0) return 0;
        if (!rule) {
            recovery = parser_recovery_count;
            switch (recovery) {
            case 0:
                for (repair = parser_recoveries;
                     repair->state != -1000 && repair->state < state; ++repair)
                    ;
                if (repair->state == state) {
                    i = repair->token;
                    if (i < 0) {
                        parser_recovery_count = 1;
                        i = -i;
                    }
                    if (parser_recovery_count < 2)
                        report_inserted_token(parser_lookahead, i);
                    parser_value = recovery_token_value(i);
                    state = repair->next_state;
                    goto push_state;
                }
                report_parser_error("syntax error");
                ++parser_error_count;
            case 1:
            case 2:
                parser_recovery_count = 3;
                while (depth >= 0) {
                    state_info = &parser_states[parser_state_stack[depth]];
                    transition = parser_shifts + state_info->first_shift;
                    for (i = 0; i < state_info->shift_count; ++i, ++transition) {
                        if (transition->symbol == PARSER_ERROR_TOKEN) {
                            state = transition->destination;
                            goto push_state;
                        }
                    }
                    --depth;
                }
                return 1;
            case 3:
                if (!parser_lookahead) return 1;
                parser_lookahead = -1;
                goto read_action;
            }
        }
        right = &parser_value_stack[depth];
        production = &parser_productions[rule];
        depth -= production->length;
        parser_value = parser_value_stack[depth + 1];
        goto_info = &parser_gotos[production->nonterminal];
        state = goto_info->default_state;
        transition = parser_goto_transitions + goto_info->first_transition;
        for (i = 0; i < goto_info->transition_count; ++i, ++transition) {
            if (transition->symbol == parser_state_stack[depth]) {
                state = transition->destination;
                break;
            }
        }
        flags = cpp_parser_flags;
        switch (rule) {
#include "parser-declaration-actions.inc"
#include "parser-declarator-actions.inc"
#include "parser-statement-actions.inc"
#include "parser-expression-actions.inc"
#include "parser-allocation-actions.inc"
        }
    }
}
