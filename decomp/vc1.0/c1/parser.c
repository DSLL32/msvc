/* Translation-unit LR driver and C grammar semantic actions.
   Original function 0x23192..0x25dd3; semantic callees remain named traps
   until reached independently by the differential reconstruction. */
#include "parser.h"
#include "source-input.h"
#include "global-ir.h"

ParserValue parser_value; /* original 0x5b1f0 */
int parser_lookahead = -1; /* original 0x6b550 */
char *parser_diagnostic_context; /* original 0x6d494 */
unsigned char expression_storage[2052]; /* original 0x5f508..0x5fd0c */
unsigned char *text_work_cursor = expression_storage; /* original 0x6bff8 */
unsigned constant_one; /* original 0x5ffc8 */
int aggregate_depth = -1; /* original 0x6bf94 */
int unevaluated_expression_depth; /* original 0x6c02c */
int parser_error_count; /* original 0x6b554 */
short parser_recovery_count; /* original 0x6b558 */
static short parser_state_stack[340]; /* original 0x650f0 */
typedef char ParserValue_must_be_eight_bytes[(sizeof(ParserValue) == 8) ? 1 : -1];
/* The original depth bound admits entry 340, aliasing two control words.
   Keep that observable overlap in one union rather than duplicate state. */
ParserValueStack parser_values; /* original 0x653a0..0x65e48 */
#define parser_value_stack parser_values.values

/* The declaration stack includes the entry immediately before depth zero;
   storage-class lookahead may update that preceding declaration. */
static DeclarationSpec declaration_spec_storage[61]; /* original 0x605a4 */
DeclarationSpec *declaration_spec_stack = declaration_spec_storage + 1;
int declaration_depth; /* original 0x60790 */
TypeShape *declaration_shapes[60]; /* original 0x604bc */
DeclarationControlStorage declaration_controls; /* original 0x60798..0x607d8 */
typedef char DeclarationControlStorage_must_be_64_bytes[
    sizeof(DeclarationControlStorage) == 64 ? 1 : -1];
unsigned char parameter_nesting; /* original 0x5b13c */
int parameter_depth; /* original 0x604a8 */
int unevaluated_type_depth; /* original 0x6539c */
unsigned enumerator_value = (unsigned)-1; /* original 0x6bfb8 */
CType *enumerator_context; /* original 0x6b648 */
CType *based_type; /* original 0x5ff10 */
unsigned array_bound; /* original 0x65398 */

/* A label stack cell is shared by loop, switch, and branch contexts. */
typedef struct LabelStack {
    struct LabelStack *next;
    IrToken *label;
} LabelStack;
LabelStack *continue_labels, *break_labels, *do_labels;
LabelStack *switch_labels, *default_labels;
int label_stack_count;
IrToken *default_label;

/* The original yacc bound permits one state beyond its 340-element array.
   That halfword aliases the following array-bound temporary at 0x65398. */
#define PARSER_STACK_STATE(index) ((index) == 340 ? (short)array_bound : parser_state_stack[index])
#define STORE_PARSER_STATE(index, value) do { \
    if ((index) == 340) array_bound = (array_bound & 0xffff0000UL) | (unsigned short)(value); \
    else parser_state_stack[index] = (value); \
} while (0)

#define POP_DECLARATION() do { if (declaration_depth > 0) --declaration_depth; } while (0)
#define CLEAR_DECLARATION_FLAGS() (declaration_spec_flags[declaration_depth] &= ~3)
#define RESET_EXPRESSION_ARENA() do { \
    if (!parser_recovery_count) release_arena(2); \
    text_work_cursor = expression_storage; \
} while (0)
#define COPY_STORAGE(destination, source) do { \
    (destination)[0] = (source)[0]; \
    (destination)[1] = (source)[1]; \
    (destination)[2] = (source)[2]; \
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

/* Original 0x23192..0x25dd3. */
int parse_translation_unit(void)
{
    short state = 0, rule, next_state;
    int depth = -1, i, recovery, allocation_arena;
    unsigned char storage_kind;
    ParserState *state_info;
    ParserGoto *goto_info;
    ParserProduction *production;
    ParserTransition *transition;
    ParserRecovery *repair;
    ParserValue *right;
    FrontendSymbol *symbol, *label;
    CType *type;
    TypeShape *shape;
    ArenaPool *pool;
    ExpressionNode *node;
    unsigned has_function;
    for (;;) {
        if (++depth > 340) {
            report_parser_error("yacc stack overflow");
            return 1;
        }
        STORE_PARSER_STATE(depth, state);
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
                parser_value.token = current_token_value;
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
                        ++parser_recovery_count;
                        i = -i;
                    }
                    if (parser_recovery_count <= 1)
                        report_expected_token((unsigned char)parser_lookahead, i);
                    parser_value = recovery_token_value(i);
                    state = repair->next_state;
                    goto push_state;
                }
                report_parser_error("syntax error");
                ++parser_error_count;
                /* A reported error and a second failure both pop to an
                   error-token transition before retrying the input token. */
            case 1:
            case 2:
                parser_recovery_count = 3;
                while (depth >= 0) {
                    state_info = &parser_states[PARSER_STACK_STATE(depth)];
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
            if (transition->symbol == PARSER_STACK_STATE(depth)) {
                state = transition->destination;
                break;
            }
        }
        switch (rule) {
        case 3:
            finish_external_declaration(parser_recovery_count);
            break;
        case 4:
            parser_flags &= ~0x100;
            if (right[-1].symbol) begin_function_definition();
            break;
        case 5:
            if (right[-3].symbol) finish_function_definition();
            parser_flags &= ~0x2000;
            break;
        case 6:
            if (scanner_flags & 0x2000000) compiler_warning(4, 218);
            break;
        case 7:
            if (!options.z_a) compiler_warning(4, 19);
            break;
        case 8:
            if (!(scanner_flags & 0x2000000))
                process_declaration(&right[-1].declaration);
            break;
        case 9:
            parser_recovery_count = 0;
            scanner_flags &= ~0x1000000;
            declaration_depth = 0;
            break;
        case 10:
            skip_declaration_tokens();
            finish_compound_statement();
            finish_function_definition();
            parser_recovery_count = 0;
            parser_lookahead = -1;
            scanner_flags &= ~0x1000000;
            declaration_depth = 0;
            break;
        case 12:
            report_syntax_error(449);
            skip_declaration_tokens();
            if (source_text_cursor) ++source_text_cursor;
            parser_recovery_count = 0;
            parser_lookahead = -1;
            break;
        case 13:
            scanner_flags &= ~0x2000000;
            CLEAR_DECLARATION_FLAGS();
            if (parser_lookahead == -1) parser_lookahead = read_parser_token();
            else if (parser_lookahead == TOK_IDENTIFIER)
                parser_lookahead = resolve_identifier_token((unsigned char)parser_lookahead);
            switch (parser_lookahead) {
            case TOK_CLASS_HEAD:
            case TOK_TYPE_NAME:
                if (right[0].symbol && !(right[0].symbol->attributes & 0x10)) {
                    report_syntax_error(444, right[0].symbol->name);
                    unget_token((unsigned char)parser_lookahead);
                    current_token_code = TOK_SEMICOLON;
                    parser_lookahead = current_token_code;
                    parser_value.symbol = 0;
                    parser_flags |= 0x2000;
                    break;
                }
                /* A type name immediately following a declarator also
                   serves as the first token of an old-style declaration. */
            case TOK_LEFT_BRACE:
                prepare_function_definition(&right[-1].declaration, right[0].symbol);
                parser_value.symbol = right[0].symbol;
                break;
            case TOK_IDENTIFIER:
                symbol = right[0].symbol;
                if (!symbol) {
                    report_syntax_error(61, current_token_value.identifier);
                    current_token_code = TOK_SEMICOLON;
                    parser_lookahead = current_token_code;
                    break;
                }
                shape = symbol->type->related;
                if (shape) {
                    has_function = (shape->flags & 7) == 4;
                    while (shape->related) {
                        shape = shape->related;
                        if ((shape->flags & 7) == 4) has_function = 1;
                    }
                    if ((shape->flags & 7) == 4) {
                        if (symbol->attributes & 0x10)
                            report_syntax_error(501, current_token_value.identifier);
                        else report_syntax_error(280, current_token_value.identifier);
                    } else if (has_function)
                        report_syntax_error(281, symbol->name, current_token_value.identifier);
                    else report_syntax_error(282, current_token_value.identifier);
                } else report_syntax_error(282, current_token_value.identifier);
                parser_lookahead = recover_parser_token((unsigned char)parser_lookahead,
                    TOK_SEMICOLON, TOK_LEFT_BRACE, 1);
                if (parser_lookahead == TOK_LEFT_BRACE) {
                    skip_declaration_tokens();
                    ++source_text_cursor;
                    current_token_code = TOK_SEMICOLON;
                    parser_lookahead = current_token_code;
                }
                parser_value.symbol = 0;
                parser_flags |= 0x2000;
                break;
            default:
                report_expected_token((unsigned char)parser_lookahead, TOK_LEFT_BRACE);
                unget_token((unsigned char)parser_lookahead);
                current_token_code = TOK_SEMICOLON;
                parser_lookahead = current_token_code;
                parser_value.symbol = 0;
                parser_flags |= 0x2000;
                break;
            }
            break;
        case 14:
            parser_flags |= 0x100;
            break;
        case 15:
            CLEAR_DECLARATION_FLAGS();
            break;
        case 21:
            report_syntax_error(146, parser_diagnostic_context, right[0].name);
            parser_recovery_count = 0;
            parser_lookahead = -1;
            break;
        case 22:
            set_declaration_name(&declaration_spec_stack[declaration_depth], right[0].name);
            break;
        case 23:
            parser_value.declaration.type = type_0064;
            parser_value.bytes[0] = (parser_value.bytes[0] & ~5) | 2;
            storage_kind = parser_value.bytes[0] & 7;
            COPY_STORAGE(parser_value.declaration.storage, current_storage);
            parser_value.bytes[0] = (parser_value.bytes[0] & ~7) | storage_kind;
            scanner_flags |= 0x2000000;
            declaration_spec_stack[declaration_depth] = parser_value.declaration;
            CLEAR_DECLARATION_FLAGS();
            break;
        case 25:
            if (parser_lookahead == TOK_LEFT_PAREN) {
                declaration_spec_stack[declaration_depth - 1] = right[0].declaration;
                declaration_spec_flags[declaration_depth - 1] |= 1;
            } else {
                declaration_spec_stack[declaration_depth] = right[0].declaration;
                declaration_spec_flags[declaration_depth] |= 1;
            }
            break;
        case 27:
            current_token_value = parser_value.token;
            parser_flags |= 1;
            resolve_identifier_token(TOK_CLASS_HEAD);
            parser_flags &= ~1;
            parser_value.token = current_token_value;
            break;
        case 28:
            parser_value.declaration.type = parse_struct_specifier();
            break;
        case 29:
            parser_value.declaration.type = parse_enum_specifier();
            break;
        case 35:
            enumerator_value = new_enumerator(enumerator_value, right[0].name);
            add_enumerator(right[0].name, enumerator_context, enumerator_value);
            break;
        case 36:
            add_enumerator(right[-2].name, enumerator_context, right[0].words.low);
            if (!unevaluated_type_depth && !type_description_depth) release_arena(2);
            break;
        case 39:
            if (!unevaluated_type_depth && !type_description_depth) release_arena(2);
            CLEAR_DECLARATION_FLAGS();
            break;
        case 40:
            add_struct_member(&declaration_spec_stack[declaration_depth], 0);
            break;
        case 42:
        case 43:
            add_struct_member(&declaration_spec_stack[declaration_depth], right[0].symbol);
            break;
        case 44:
        case 45:
            parser_lookahead = recover_parser_token((unsigned char)parser_lookahead,
                TOK_SEMICOLON, TOK_RIGHT_BRACE, 1);
            if (parser_lookahead != TOK_SEMICOLON) {
                unget_token((unsigned char)parser_lookahead);
                parser_lookahead = TOK_SEMICOLON;
            }
            parser_recovery_count = 0;
            break;
        case 46:
            parser_value.symbol->kind = 3;
            break;
        case 47:
            parser_value.symbol->kind = 2;
            ((unsigned char *)parser_value.symbol)[0x25] = right[0].bytes[0];
            break;
        case 48:
            parser_value.symbol = make_abstract_declarator(0);
            parser_value.symbol->name = name_alignment;
            break;
        case 52:
            apply_declarator_type(&declaration_spec_stack[declaration_depth], right[0].symbol);
            break;
        case 53:
            if (!options.prototypes_only) finish_initializer(right[-1].symbol, right[0].node);
            if (!unevaluated_type_depth && !type_description_depth) release_arena(2);
            text_work_cursor = expression_storage;
            break;
        case 54:
            if (!options.prototypes_only) begin_aggregate_initializer(right[0].node);
            break;
        case 55:
            finish_aggregate_initializer(right[-5].symbol);
            break;
        case 56:
            right[-1].symbol->attributes |= 4;
            parser_value.symbol = declare_initialized_symbol(
                apply_declarator_type(&declaration_spec_stack[declaration_depth], right[-1].symbol));
            break;
        case 57:
            parser_value.symbol = 0;
            report_declaration_error(513, &declaration_spec_stack[declaration_depth]);
            break;
        case 60:
            if (!options.prototypes_only) emit_initializer(right[0].node);
            if (!unevaluated_type_depth && !type_description_depth) release_arena(2);
            text_work_cursor = expression_storage;
            break;
        case 61:
            if (!options.prototypes_only) begin_nested_initializer();
            break;
        case 62:
            if (!options.prototypes_only) finish_nested_initializer();
            break;
        case 63:
            parser_value.symbol = make_named_declarator(right[0].name);
            break;
        case 64:
            parser_value.symbol = apply_declarator_prefix(make_named_declarator(right[0].name), right[-1].shape);
            break;
        case 65:
            parser_value.symbol = right[0].symbol;
            attach_type_shape(parser_value.symbol, new_type_shape(1, 0, 0));
            break;
        case 66:
            parser_value.symbol = right[0].symbol;
            right[-2].shape->flags = 1;
            attach_type_shape(parser_value.symbol, right[-2].shape);
            break;
        case 67:
            array_bound = right[-1].words.low;
            attach_type_shape(parser_value.symbol, new_type_shape(2, 0, &array_bound));
            break;
        case 68:
            parser_value.symbol = finish_function_declarator(right[-1].symbol, right[0].symbol);
            break;
        case 70:
            parser_value.symbol = right[-1].symbol;
            POP_DECLARATION();
            break;
        case 71:
            ++parameter_nesting;
            ++parameter_depth;
            ++unevaluated_type_depth;
            begin_parameter_list(0);
            break;
        case 72:
            if (right[-1].symbol) {
                insert_current_scope_symbol(right[-1].symbol);
                parser_value.words.low = (unsigned)append_parameter((FrontendSymbol *)right[-2].words.low,
                    right[-1].symbol);
            } else parser_value.words.low = right[-2].words.low;
            --parameter_nesting;
            --parameter_depth;
            --unevaluated_type_depth;
            POP_DECLARATION();
            CLEAR_DECLARATION_FLAGS();
            break;
        case 73:
            parser_value.words.low = 0;
            break;
        case 74:
            parser_value.words.low = (unsigned)append_parameter(0, right[0].symbol);
            break;
        case 75:
            parser_value.words.low = (unsigned)append_parameter((FrontendSymbol *)right[-2].words.low,
                right[0].symbol);
            break;
        case 76:
            parser_value.symbol = make_parameter(right[0].name);
            break;
        case 78:
            insert_current_scope_symbol(parser_value.symbol);
            break;
        case 79:
            parser_value.words.low = 0;
            parser_recovery_count = 0;
            parser_lookahead = -1;
            break;
        case 80:
            ++parameter_nesting;
            ++parameter_depth;
            ++unevaluated_type_depth;
            begin_parameter_list(0);
            break;
        case 81:
            if (right[-1].symbol) {
                insert_current_scope_symbol(right[-1].symbol);
                parser_value.words.low = (unsigned)append_parameter((FrontendSymbol *)right[-2].words.low,
                    right[-1].symbol);
            } else parser_value.words.low = right[-2].words.low;
            --parameter_nesting;
            --parameter_depth;
            --unevaluated_type_depth;
            POP_DECLARATION();
            parser_value.words.low = right[-2].words.low;
            break;
        case 82:
            parser_value.words.low = 0;
            break;
        case 83:
            parser_value.words.low = (unsigned)append_parameter(0, right[0].symbol);
            break;
        case 84:
            parser_value.words.low = (unsigned)append_parameter((FrontendSymbol *)right[-2].words.low,
                right[0].symbol);
            break;
        case 86:
            insert_current_scope_symbol(parser_value.symbol);
            break;
        case 87:
            parser_value.words.low = 0;
            parser_recovery_count = 0;
            parser_lookahead = -1;
            break;
        case 88:
            right[0].symbol->kind = 10;
            COPY_STORAGE(right[0].symbol->storage, right[-1].declaration.storage);
            parser_value.symbol = declare_symbol(&right[-1].declaration, right[0].symbol);
            CLEAR_DECLARATION_FLAGS();
            break;
        case 89:
            parser_value.symbol = right[0].symbol;
            parser_value.symbol->kind = 11;
            COPY_STORAGE(parser_value.symbol->storage, right[-1].declaration.storage);
            parser_value.symbol->name = name_formal;
            finalize_symbol_type(right[-1].declaration.type, parser_value.symbol);
            CLEAR_DECLARATION_FLAGS();
            break;
        case 90:
            parser_value.shape = new_type_shape(0, right[0].modifier, &based_type);
            break;
        case 91:
            parser_value.shape = append_type_shape(right[-1].shape,
                new_type_shape(0, right[0].modifier, &based_type));
            break;
        case 92:
        case 93:
        case 94:
            for (i = 0; declaration_codes[i].token != right[0].bytes[0]; ++i)
                ;
            parser_value.modifier = declaration_codes[i].value;
            break;
        case 96:
            scanner_flags |= 0x800;
            POP_DECLARATION();
            parser_value.modifier = right[-1].modifier;
            break;
        case 97:
            parser_value.modifier = 5;
            based_type = (CType *)right[0].symbol;
            break;
        case 99:
            parser_value.modifier = 5;
            based_type = (CType *)resolve_type_symbol(right[0].symbol);
            if (!based_type) parser_value.modifier = 2;
            break;
        case 100:
            parser_value.modifier = 4;
            if (!(right[0].type_name.type->flags & 0x80)) report_syntax_error(218);
            break;
        case 101:
            parser_value.modifier = based_string_modifier(right[-2].type_name.type->flags, right[0].symbol);
            POP_DECLARATION();
            break;
        case 102:
            parser_value.modifier = based_symbol_modifier(
                right[-3].type_name.type->flags, right[0].symbol);
            POP_DECLARATION();
            break;
        case 103:
            parser_value.modifier = based_type_modifier(right[-2].type_name.type->flags);
            POP_DECLARATION();
            break;
        case 106:
            parser_value.symbol = make_abstract_declarator(0);
            break;
        case 107:
            parser_value.symbol = make_abstract_declarator(right[0].shape);
            break;
        case 108:
            finish_parameter_scope();
            parser_value.symbol = attach_function_parameters(make_abstract_declarator(0), right[0].symbol);
            break;
        case 109:
            finish_parameter_scope();
            parser_value.symbol = attach_function_parameters(make_abstract_declarator(right[-1].shape), right[0].symbol);
            break;
        case 110:
            parser_value.symbol = right[0].symbol;
            attach_type_shape(parser_value.symbol, new_type_shape(1, 0, 0));
            break;
        case 111:
            parser_value.symbol = right[0].symbol;
            right[-2].shape->flags = 1;
            attach_type_shape(parser_value.symbol, right[-2].shape);
            break;
        case 112:
            array_bound = right[-1].words.low;
            attach_type_shape(parser_value.symbol, new_type_shape(2, 0, &array_bound));
            break;
        case 113:
            finish_parameter_scope();
            parser_value.symbol = attach_function_parameters(right[-1].symbol, right[0].symbol);
            break;
        case 115:
            parser_value.symbol = right[-1].symbol;
            POP_DECLARATION();
            break;
        case 116:
            parser_value.words.low = 0;
            break;
        case 117:
            report_syntax_error(59, ")");
            ALLOCATE_PARSER_NODE(parser_value.symbol, sizeof(FrontendSymbol), parameter_nesting != 0);
            parser_value.symbol->kind = 10;
            parser_value.symbol->name = name_ellipsis;
            parser_value.symbol->type = 0;
            parser_value.symbol->flags |= 2;
            break;
        case 118:
            ALLOCATE_PARSER_NODE(parser_value.symbol, sizeof(FrontendSymbol), parameter_nesting != 0);
            parser_value.symbol->kind = 10;
            parser_value.symbol->name = name_ellipsis;
            parser_value.symbol->type = 0;
            parser_value.symbol->flags |= 2;
            break;
        case 119:
            if (options.prototypes_only || (scanner_flags & 0x1000000)) {
                scanner_flags |= 0x1000000;
                skip_declaration_tokens();
            } else {
                begin_compound_statement();
                CLEAR_DECLARATION_FLAGS();
            }
            break;
        case 120:
            scope_serial = 0;
            break;
        case 121:
            if (options.prototypes_only || (scanner_flags & 0x1000000))
                scanner_flags &= ~0x1000000;
            else finish_compound_statement();
            break;
        case 124:
            CLEAR_DECLARATION_FLAGS();
            break;
        case 126:
            process_declaration(&right[-1].declaration);
            break;
        case 127:
            parser_recovery_count = 0;
            break;
        case 131:
            emit_expression_statement(right[-1].node);
            RESET_EXPRESSION_ARENA();
            break;
        case 133:
        case 134:
            emit_code_label(right[-1].node);
            break;
        case 135:
        case 138:
            if (normalized_Ol && options.option_qc) begin_inline_region(0);
            break;
        case 136:
            begin_for_statement(0, right[-1].node, 0);
            POP_DECLARATION();
            break;
        case 137:
        case 141:
            finish_for_statement();
            if (normalized_Ol && options.option_qc) finish_inline_region();
            break;
        case 139:
            begin_for_statement(right[-4].node, right[-2].node, right[0].node);
            break;
        case 140:
            POP_DECLARATION();
            CLEAR_DECLARATION_FLAGS();
            break;
        case 142:
            if (normalized_Ol && options.option_qc) begin_inline_region(0);
            begin_do_statement();
            break;
        case 143:
            emit_code_label(do_labels->label);
            if (right[-2].node)
                emit_conditional_branch(right[-2].node, continue_labels->label, 1);
            record_line_information(1);
            emit_code_label(break_labels->label);
            continue_labels = continue_labels->next;
            do_labels = do_labels->next;
            break_labels = break_labels->next;
            if (!(label_stack_count -= 3)) release_arena(3);
            RESET_EXPRESSION_ARENA();
            if (normalized_Ol && options.option_qc) finish_inline_region();
            POP_DECLARATION();
            CLEAR_DECLARATION_FLAGS();
            break;
        case 144:
            if (normalized_Ol && options.option_qc) begin_inline_region(16);
            begin_switch_statement(right[-1].node);
            scope_serial = 1;
            POP_DECLARATION();
            CLEAR_DECLARATION_FLAGS();
            break;
        case 145:
            record_line_information(1);
            write_ir_record(0x3a, break_labels->label);
            finish_switch_statement();
            continue_labels = continue_labels->next;
            break_labels = break_labels->next;
            switch_labels = switch_labels->next;
            default_labels = default_labels->next;
            if (!(label_stack_count -= 4)) release_arena(3);
            scope_serial = 0;
            if (normalized_Ol && options.option_qc) finish_inline_region();
            break;
        case 146:
            emit_default_statement();
            break;
        case 147:
            if (break_labels) {
                if (normalized_Ol && options.option_qc) emit_inline_jump(1);
                write_ir_record(0x3a, break_labels->label);
            } else report_syntax_error(43);
            break;
        case 148:
            if (do_labels) {
                if (normalized_Ol && options.option_qc) emit_inline_jump(2);
                write_ir_record(0x3a, do_labels->label);
            } else report_syntax_error(44);
            break;
        case 149:
            emit_return_statement(0, 1);
            break;
        case 150:
            emit_return_statement(right[-1].node, 1);
            RESET_EXPRESSION_ARENA();
            break;
        case 151:
            if (normalized_Ol && options.option_qc) emit_inline_jump(0);
            label = find_label(right[-1].name);
            if (!label) {
                label = declare_label(right[-1].name);
                reference_label(label);
            }
            write_ir_record(0x3a, make_symbol_reference(label));
            RESET_EXPRESSION_ARENA();
            break;
        case 152:
            parse_asm_statement();
            break;
        case 153:
            emit_leave_statement();
            break;
        case 154:
            begin_try_statement();
            break;
        case 155:
            finish_try_statement();
            break;
        case 157:
            report_syntax_error(181);
            break;
        case 158:
            parser_recovery_count = 0;
            break;
        case 159:
            begin_except_statement();
            break;
        case 160:
            finish_except_statement();
            break;
        case 161:
            begin_finally_statement();
            break;
        case 162:
            set_except_filter(right[-1].node);
            finish_finally_statement();
            POP_DECLARATION();
            CLEAR_DECLARATION_FLAGS();
            break;
        case 163:
            default_label = 0;
            break;
        case 165:
            if (normalized_Ol && options.option_qc) emit_inline_jump(0);
            emit_named_label(right[-1].name);
            break;
        case 166:
            if (right[-1].node && switch_labels) {
                scope_serial = 0;
                emit_case_statement(right[-1].node);
            } else report_syntax_error(46);
            break;
        case 167:
            scope_serial = 0;
            if (!default_labels) report_syntax_error(47);
            else if (default_labels->label) report_syntax_error(48);
            else {
                if (default_label) default_labels->label = default_label;
                else {
                    default_labels->label = make_internal_label(20);
                    default_label = default_labels->label;
                }
                emit_code_label(default_labels->label);
            }
            break;
        case 168:
            parser_value.node = make_internal_label(30);
            if (right[-1].node) emit_conditional_branch(right[-1].node, parser_value.node, 0);
            POP_DECLARATION();
            CLEAR_DECLARATION_FLAGS();
            break;
        case 169:
            parser_value.node = make_internal_label(30);
            write_ir_record(0x3a, parser_value.node);
            emit_code_label(right[-2].node);
            record_line_information(1);
            CLEAR_DECLARATION_FLAGS();
            break;
        case 170:
            parser_value.words.low = 0;
            break;
        case 171:
            if ((aggregate_depth < 0 || !options.z_a) && !right[0].node)
                report_syntax_error(466);
            break;
        case 172:
            parser_value.words.low = integer_constant_value(right[0].node);
            break;
        case 173:
            parser_value.words.low = 0;
            break;
        case 175: case 176: case 177: case 178: case 179:
        case 180: case 181: case 182: case 183: case 184:
        case 185: case 186: case 187: case 188: case 189:
            parser_value.node = make_binary_expression(right[-1].bytes[0], right[-2].node, right[0].node);
            break;
        case 190:
            parser_value.node = make_binary_expression(right[-3].bytes[0], right[-4].node,
                make_binary_expression(right[-1].bytes[0], right[-2].node, right[0].node));
            break;
        case 192:
            constant_one = 1;
            parser_value.node = make_binary_expression(right[0].bytes[0], right[-1].node,
                make_constant_token(TOK_INT_CONSTANT, &constant_one));
            break;
        case 193:
            parser_value.node = make_unary_expression(TOK_UNARY_PLUS, right[0].node);
            break;
        case 194:
            parser_value.node = make_unary_expression(TOK_UNARY_MINUS, right[0].node);
            break;
        case 195:
            parser_value.node = make_unary_expression(TOK_BIT_NOT, right[0].node);
            break;
        case 196:
            parser_value.node = make_unary_expression(TOK_LOGICAL_NOT, right[0].node);
            break;
        case 197:
            constant_one = 1;
            right[-1].bytes[0] = right[-1].bytes[0] == TOK_PREINCREMENT ? TOK_POSTINCREMENT : TOK_POSTDECREMENT;
            parser_value.node = make_binary_expression(right[-1].bytes[0], right[0].node,
                make_constant_token(TOK_INT_CONSTANT, &constant_one));
            break;
        case 198:
            parser_value.node = make_unary_expression(TOK_LEFT_PAREN, right[-1].node);
            POP_DECLARATION();
            break;
        case 199:
            parser_value.node = make_unary_expression(TOK_DEREFERENCE, right[0].node);
            break;
        case 200:
            parser_value.node = make_unary_expression(TOK_BIT_AND, right[0].node);
            break;
        case 201:
            if (right[0].node) parser_value.node = make_sizeof_expression(right[0].node);
            else parser_value.node = 0;
            --unevaluated_expression_depth;
            break;
        case 202:
            parser_value.node = make_sizeof_type(right[0].symbol->type);
            --unevaluated_expression_depth;
            break;
        case 204:
            parser_value.node = make_cast_expression((CType *)right[-1].words.low, right[0].node);
            break;
        case 205:
            parser_value.node = make_binary_expression(right[-2].bytes[0], right[-3].node, right[-1].node);
            break;
        case 206:
            if ((scanner_flags & 0x800) && parser_value.symbol)
                parser_value.symbol = finish_declarator(parser_value.symbol);
            finish_type_description((CType *)parser_value.symbol);
            POP_DECLARATION();
            break;
        case 207:
            if (parser_value.symbol) {
                parser_value.symbol->index = right[-1].words.low;
                if (scanner_flags & 0x800) parser_value.symbol = finish_declarator(parser_value.symbol);
            }
            finish_type_description((CType *)parser_value.symbol);
            POP_DECLARATION();
            break;
        case 208:
            scanner_flags |= 0x80;
            break;
        case 209:
            parser_value.node = make_function_call(right[-2].bytes[0], right[-3].node,
                (void *)right[0].words.low);
            scanner_flags &= ~0x80;
            break;
        case 210:
            parser_value.node = make_member_expression(right[-2].node, right[0].name);
            break;
        case 211:
            parser_value.node = make_identifier_expression(right[0].name);
            break;
        case 213:
            record_version_token(right[0].node);
            break;
        case 214:
            ALLOCATE_PARSER_NODE(node, sizeof(ExpressionNode), 2);
            parser_value.node = (IrToken *)node;
            node->flags = 4;
            node->kind = 42;
            node->value.symbol = right[0].symbol;
            node->type = right[0].symbol->type;
            break;
        case 215:
            parser_value.symbol = declare_named_entity(intern_identifier(right[-1].node->value.text),
                name_unnamed, 8, 2);
            POP_DECLARATION();
            break;
        case 216:
            ALLOCATE_PARSER_NODE(node, sizeof(ExpressionNode), 2);
            parser_value.node = (IrToken *)node;
            node->flags = 5;
            node->kind = 3;
            node->value.words.low = right[0].words.low;
            node->value.words.high = right[0].words.high;
            break;
        case 217:
            concatenate_string_literals((char *)right[-1].node + 12, right);
            break;
        case 218:
            ++unevaluated_type_depth;
            break;
        case 219:
            parser_value.symbol = right[-1].symbol;
            POP_DECLARATION();
            break;
        case 220:
            parser_value.symbol = right[0].symbol;
            finalize_symbol_type(right[-1].declaration.type, parser_value.symbol);
            --unevaluated_type_depth;
            CLEAR_DECLARATION_FLAGS();
            break;
        case 221:
            parser_value.node = begin_sizeof_expression(right[-1].node);
            break;
        case 222:
            parser_value.node = begin_alignof_expression(right[-1].node);
            break;
        case 223:
            parser_value.words.low = (unsigned)append_argument(0, right[0].node, 0);
            break;
        case 224:
            parser_value.words.low = (unsigned)append_argument((void *)right[-2].words.low, right[0].node, 0);
            break;
        }
        goto push_state;
push_state:
        ;
    }
}

/* Original 0x2cf00..0x2cf85. Declarators reserve a mutable type descriptor
   in the expression arena, even when the symbol itself is permanent. */
FrontendSymbol *__stdcall make_named_declarator(char *name)
{
    FrontendSymbol *symbol;
    ArenaPool *pool;
    if (!parameter_nesting) symbol = allocate_permanent(sizeof(FrontendSymbol));
    else {
        pool = &compiler_arena.pools[1];
        if (pool->remaining < (int)sizeof(FrontendSymbol)) refill_arena(&compiler_arena, pool);
        symbol = (FrontendSymbol *)pool->next;
        pool->remaining -= sizeof(FrontendSymbol);
        pool->next += sizeof(FrontendSymbol);
    }
    symbol->kind = 1;
    symbol->name = name;
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(TypeDescription)) refill_arena(&compiler_arena, pool);
    symbol->type = (TypeDescription *)pool->next;
    pool->remaining -= sizeof(TypeDescription);
    pool->next += sizeof(TypeDescription);
    return symbol;
}

FrontendSymbol *current_parameter; /* original 0x60064 */

/* Original 0x2b6a0..0x2b72a. */
void __stdcall begin_parameter_list(FrontendSymbol *parameter)
{
    SymbolScope *context;
    ArenaPool *pool = &compiler_arena.pools[1];
    if (pool->remaining < (int)sizeof(SymbolScope)) refill_arena(&compiler_arena, pool);
    context = (SymbolScope *)pool->next;
    pool->remaining -= sizeof(SymbolScope);
    pool->next += sizeof(SymbolScope);
    context->bucket_mask = 0;
    context->flags = (context->flags & 0xff00) | 1;
    if (parameter) {
        if (pool->remaining < (int)sizeof(ParameterList)) refill_arena(&compiler_arena, pool);
        context->buckets = (FrontendSymbol **)pool->next;
        pool->remaining -= sizeof(ParameterList);
        pool->next += sizeof(ParameterList);
        current_parameter = parameter;
        context->buckets[0] = parameter;
    }
    push_symbol_scope(context);
    activate_symbol_scope(context);
}

/* Original 0x2b1e0..0x2b227. A plain function declarator can introduce
   a definition; a function nested in another type only supplies a shape. */
FrontendSymbol *__stdcall finish_function_declarator(FrontendSymbol *symbol, FrontendSymbol *parameters)
{
    TypeShape *shape = symbol->type->related;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    if (!shape && !(parser_flags & 0x100))
        return make_function_declaration(symbol, parameters);
    finish_parameter_scope();
    return attach_function_parameters(symbol, parameters);
}

FrontendSymbol *function_parameter_declarations; /* original 0x5ff14 */
int function_parameter_context; /* original 0x60228 */

/* Original 0x2b160..0x2b1e0. */
FrontendSymbol *__stdcall make_function_declaration(FrontendSymbol *symbol, FrontendSymbol *parameters)
{
    symbol = attach_function_parameters(symbol, parameters);
    if (declaration_context == 2) {
        compiler_warning(4, 131, symbol->name);
        symbol->attributes |= 0x10;
    } else symbol->attributes &= ~0x10;
    symbol->type->related->attributes |= symbol->type->attributes;
    symbol->attributes |= 0x80;
    function_parameter_declarations = finish_parameter_scope();
    if (symbol->type->related->flags & 0x4000) function_parameter_context = 5;
    else function_parameter_context = declaration_context;
    return symbol;
}

/* Original 0x2b230..0x2b276. */
FrontendSymbol *__stdcall attach_function_parameters(FrontendSymbol *symbol, FrontendSymbol *parameters)
{
    FrontendSymbol *typed_parameters = 0;
    declaration_context = classify_parameter_declarations(parameters);
    if (declaration_context != 2) typed_parameters = parameters;
    attach_type_shape(symbol, new_type_shape(4, 0, &typed_parameters));
    return symbol;
}

/* Original 0x2b280..0x2b413. */
int __stdcall classify_parameter_declarations(FrontendSymbol *parameters)
{
    FrontendSymbol *first = 0, *parameter;
    int kind = 1;
    for (parameter = parameters; parameter; parameter = parameter->next) {
        if ((parameter->kind & 0x1f) == 10 || (parameter->kind & 0x1f) == 11) {
            first = parameter->detail.parameter_next;
            break;
        }
    }
    parameter = first;
    if (!parameter) return kind;
    do {
        switch (kind) {
        case 1:
            if ((parameter->kind & 0x1f) == 11) kind = 4;
            else if ((parameter->kind & 0x1f) == 10)
                kind = (parameter->attributes & 0x10) ? 2 : 3;
            break;
        case 2:
            if ((parameter->kind & 0x1f) == 11)
                report_type_syntax_error(122, parameter->type);
            else if ((parameter->kind & 0x1f) == 10 && !(parameter->attributes & 0x10)) {
                if (parameter->name == name_ellipsis && options.z_a)
                    compiler_warning(4, 202);
                else report_syntax_error(122, parameter->name);
            }
            break;
        case 3:
        case 4:
            if ((parameter->kind & 0x1f) == 11) kind = 4;
            else if ((parameter->kind & 0x1f) == 10 && (parameter->attributes & 0x10))
                report_syntax_error(81, parameter->name);
            break;
        }
        if (!parameter) break;
        parameter = parameter->detail.parameter_next;
    } while (parameter != first);
    return kind;
}

/* Original 0x2b730..0x2b771. Return the circular parameter chain while
   restoring the scope that preceded its temporary declaration context. */
FrontendSymbol *finish_parameter_scope(void)
{
    FrontendSymbol *parameters = 0;
    if ((current_scope->flags & 0xff) != 1 || current_scope->bucket_mask) return 0;
    if (current_scope->buckets) parameters = current_scope->buckets[0];
    deactivate_symbol_scope();
    pop_symbol_scope();
    return parameters;
}

FrontendSymbol *function_symbol; /* original 0x5b140 */
CType *function_return_type; /* original 0x6b6c0 */
FrontendSymbol *default_code_segment; /* original 0x6b7b0 */
unsigned browser_macro_context; /* original 0x63868 */

/* Original 0x2a530..0x2a82b. Bind a function declarator to the global
   declaration, check its definition attributes, and enter function state. */
void __stdcall prepare_function_definition(DeclarationSpec *declaration, FrontendSymbol *symbol)
{
    unsigned short attributes;
    unsigned saved_browser_context;
    FrontendSymbol *parameter, *first;
    symbol->flags |= 1;
    symbol = declare_symbol(declaration, symbol);
    if ((symbol->kind & 0x1f) != 7) {
        report_syntax_error((symbol->kind & 0x1f) == 6 ? 169 : 54, symbol->name);
        symbol->flags |= 0x80;
        abort_function_definition();
        return;
    }
    if (!(symbol->attributes & 0x80)) {
        report_syntax_error(206, symbol->name);
        function_parameter_declarations = 0;
    } else if (function_parameter_context == 4)
        report_syntax_error(55, symbol->name);
    attributes = symbol->type->related->attributes;
    if ((((attributes & 0x70) == 0x40) && ((attributes & 7) == 2)) || (attributes & 0x200)) {
        if (options.option_Gw || ((options.option_GA || options.option_GD) &&
            ((attributes & 0x400) || options.option_GEf))) report_syntax_error(80);
    }
    if ((symbol->storage[2] & 4) && !(symbol->storage[2] & 8) &&
        !(symbol->storage[1] & 1) && !(scanner_flags & 0x400))
        report_syntax_error(491, symbol->name, "function");
    if ((symbol->type->attributes & 7) == 3)
        report_syntax_error(703, symbol->name, "_far16");
    if (default_code_segment) {
        if (symbol->attributes & 2) {
            if ((FrontendSymbol *)symbol->type->related->field08 != default_code_segment)
                report_syntax_error(193, symbol->name);
        } else assign_symbol_segment(symbol, default_code_segment);
    }
    function_symbol = initialize_function_definition(symbol);
    current_parameter = function_parameter_declarations;
    active_parameter_context = function_parameter_context;
    if (function_return_type->flags != 0x80 && !type_size(function_return_type))
        report_syntax_error(79, function_symbol->name,
            ((FrontendSymbol *)function_return_type->field08)->name);
    if (browser_filename) {
        saved_browser_context = browser_macro_context;
        browser_macro_context = 0;
        browser_begin_function_scope();
        if (!function_symbol->index) {
            if (next_symbol_index >= 0x7fffffff) fatal_error(55);
            if (next_symbol_index >= browser_symbol_limit && browser_filename)
                browser_index_overflow();
            function_symbol->index = next_symbol_index++;
        }
        browser_symbol_definition(function_symbol);
        browser_function_parameters(function_symbol);
        browser_macro_context = saved_browser_context;
        first = 0;
        for (parameter = current_parameter; parameter; parameter = parameter->next) {
            if ((parameter->kind & 0x1f) == 10 || (parameter->kind & 0x1f) == 11) {
                first = parameter->detail.parameter_next;
                break;
            }
        }
        parameter = first;
        if (parameter) {
            do {
                if (parameter->type) browser_symbol_definition(parameter);
                if (!parameter) break;
                parameter = parameter->detail.parameter_next;
            } while (parameter != first);
        }
    }
}

IrToken *function_exit_label; /* original 0x60084 */
unsigned short function_scope_level; /* original 0x6bfec */

/* Original 0x2a880..0x2a915. */
FrontendSymbol *__stdcall initialize_function_definition(FrontendSymbol *symbol)
{
    inline_candidate = default_inline_candidate;
    function_call_seen = 0;
    function_not_inlinable = 0;
    if (normalized_Ol && options.option_qc) reset_inline_function_state();
    function_exit_label = make_internal_label(31);
    function_return_type = function_result_type(symbol);
    parser_flags &= ~0x8000;
    if (!(scanner_flags & 0x40)) begin_parameter_list(function_parameter_declarations);
    global_record_kind = 1;
    parameter_nesting = 1;
    function_scope_level = 1;
    string_record_count = 0;
    return symbol;
}

/* Original 0x2a9e0..0x2ad36. Validate the definition's parameter list,
   replace its prototype shape, and enter the statement-scanning context. */
void begin_function_definition(void)
{
    TypeShape *shape;
    FrontendSymbol *chain, *first, *parameter, *parameters;
    ParameterInfo *information;
    CType *replacement, *saved_type;
    unsigned convention, bytes;
    int index;
    if (!function_symbol) return;
    start_function_ir();
    shape = function_symbol->type->related;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    convention = function_symbol->type->attributes & 0x70;
    if (!parser_values.control.fastcall_stack_warning && convention == 0x40 && normalized_Gs) {
        compiler_warning(1, 124);
        parser_values.control.fastcall_stack_warning = 1;
    }
    chain = current_parameter;
    if (!options.prototypes_only && (convention == 0x60 || convention == 0x40)) {
        bytes = parameter_list_bytes(current_parameter);
        if (function_symbol->attributes & 0x4000) {
            if (function_symbol->detail.value != bytes)
                report_syntax_error(708, function_symbol->name);
        } else {
            function_symbol->detail.value = bytes;
            function_symbol->attributes |= 0x4000;
        }
    }
    for (parameter = chain; parameter; parameter = parameter->next)
        if ((parameter->kind & 0x1f) == 10 || (parameter->kind & 0x1f) == 11) break;
    first = parameter ? parameter->detail.parameter_next : 0;
    parameter = first;
    information = (ParameterInfo *)shape->field08;
    if (parameter) {
        if ((int)information->count > 0) {
            index = 1;
            if (shape->flags & 0x4000) {
                if (parameter->type != type_0080) {
                    compiler_warning(1, 27, index);
                    goto parameters_checked;
                }
            } else if (parameter->type == type_0080) {
                compiler_warning(1, 26);
                goto parameters_checked;
            }
            while (parameter && index <= (int)information->count) {
                if (information->types[index - 1] != parameter->type &&
                    !compatible_parameter_types(information->types[index - 1], parameter->type))
                    compiler_warning(1, 28, index);
                ++index;
                parameter = parameter->detail.parameter_next;
                if (parameter == first) parameter = 0;
            }
            if ((int)information->count - index != -1 || (parameter && !(shape->flags & 0x8000)))
                compiler_warning(1, 29);
        } else if (function_parameter_context != 2) {
            if (shape->flags & 0x4000) compiler_warning(1, 27);
            else if ((shape->flags & 0x800) && !chain->type) {
                if (options.z_a) compiler_warning(1, 52);
                else report_syntax_error(142);
            }
        }
    } else if (!(shape->flags & 0x4000) && information->count && function_parameter_context == 1)
        compiler_warning(1, 26);
parameters_checked:
    if (active_parameter_context == 1) active_parameter_context = 5;
    parameters = current_parameter;
    replacement = replace_outer_type_shape(function_symbol->type,
        (unsigned short)(active_parameter_context == 5 ? 0x4004 : 4), shape->attributes, &parameters);
    if (active_parameter_context == 3 || active_parameter_context == 5) {
        function_symbol->type = replacement;
        if (options.prototypes_only) {
            begin_syntax_trace();
            print_function_declaration(function_symbol, preprocessor_stream);
            end_syntax_trace();
        }
    } else if (options.prototypes_only) {
        saved_type = function_symbol->type;
        function_symbol->type = replacement;
        begin_syntax_trace();
        print_function_declaration(function_symbol, preprocessor_stream);
        end_syntax_trace();
        function_symbol->type = saved_type;
    }
    if (options.debug_info && !options.prototypes_only) record_line_information(0x11);
    scanner_flags |= 0x80000;
}

/* Original 0x2ad90..0x2afaa. Begin the expression stream and account for
   parameters, retaining the separate inline-parameter pass. */
void start_function_ir(void)
{
    FrontendSymbol *parameter, *first;
    unsigned storage;
    function_symbol->flags |= 1;
    function_ir.parameter_count = 0;
    function_symbol->attributes &= ~0x10;
    if (normalized_Ol && options.option_qc) reset_inline_function_state();
    if (!options.prototypes_only) {
        initialize_expression_stream(0);
        write_ir_record(0x53, 0);
        write_ir_record(0x53, 0);
        emit_ir_expression(make_ir_expression(0x6c, make_symbol_reference(function_symbol), 0));
        for (parameter = current_parameter; parameter; parameter = parameter->next)
            if ((parameter->kind & 0x1f) == 10 || (parameter->kind & 0x1f) == 11) break;
        if (parameter) {
            first = parameter->detail.parameter_next;
            parameter = first;
            do {
                storage = parameter->storage[0] & 7;
                if (storage >= 5 && storage <= 6) define_register_parameter(parameter);
                else if (!storage) parameter->storage[0] = (parameter->storage[0] & ~3) | 4;
                if (parameter->type && parameter->type != type_0080) ++function_ir.parameter_count;
                parameter = parameter->detail.parameter_next;
            } while (parameter && parameter != first);
        }
    }
    if (normalized_Ol && options.option_qc) {
        for (parameter = current_parameter; parameter; parameter = parameter->next)
            if ((parameter->kind & 0x1f) == 10 || (parameter->kind & 0x1f) == 11) break;
        if (parameter) {
            first = parameter->detail.parameter_next;
            parameter = first;
            do {
                if (parameter->type) record_inline_parameter(parameter);
                parameter = parameter->detail.parameter_next;
            } while (parameter && parameter != first);
        }
    }
}

/* Original 0x39f70..0x3a032. Save stream positions before the function's
   expression and symbol records, then emit its source/option boundaries. */
void __stdcall initialize_expression_stream(int mode)
{
    int saved_stack_check;
    function_ir.expression_offset = ftell(expression_file);
    function_ir.symbol_offset = ftell(symbol_file);
    function_ir.complexity = 0;
    if (scanner_flags & 0x40) {
        saved_stack_check = normalized_Gs;
        normalized_Gs = 0;
    }
    record_line_information(0x1f);
    if (scanner_flags & 0x40) normalized_Gs = saved_stack_check;
    if (options.option_Ob > 0) record_line_information(0x20);
    if (!mode) {
        record_line_information(2);
        record_line_information(1);
        record_line_information(0x23);
    } else {
        ++global_record_kind;
        write_global_record(6, 0);
        --global_record_kind;
    }
}

/* Original 0x3f420..0x3f4ea. References retain the symbol's type, removing
   its declaration prefix or supplying the base of a based pointer. */
IrToken *__stdcall make_symbol_reference(FrontendSymbol *symbol)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    IrToken *node;
    CType *type, *inner_type;
    TypeShape *shape;
    if (!symbol) return 0;
    if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
    node = (IrToken *)pool->next;
    pool->remaining -= sizeof(IrToken);
    pool->next += sizeof(IrToken);
    node->flags = 4;
    node->kind = 2;
    node->value.pointer = symbol;
    if (symbol->flags & 4) {
        report_syntax_error(275, symbol->name);
        return 0;
    }
    node->type = symbol->type;
    if (symbol->attributes & 2) {
        shape = symbol->type->related;
        if ((symbol->kind & 0x1f) != 7 && (shape->attributes & 7) >= 5 &&
            (((FrontendSymbol *)shape->field08)->kind & 0x1f) != 13) {
            scanner_flags |= 0x800;
            type = node->type;
            inner_type = drop_first_type_shape(type);
            return make_typed_ir_expression(0x5c, node, make_based_pointer_base(type), inner_type);
        }
        node->type = drop_first_type_shape(symbol->type);
    }
    return node;
}

/* Original 0x3f320..0x3f33b. Infer the result type from the left child. */
IrToken *__stdcall make_ir_expression(unsigned char operation, IrToken *left, IrToken *right)
{
    return make_typed_ir_expression(operation, left, right, left->type);
}

/* Original 0x3f360..0x3f3ad. The general two-child IR node constructor. */
IrToken *__stdcall make_typed_ir_expression(unsigned char operation, IrToken *left, IrToken *right, CType *type)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    ExpressionNode *node;
    if (pool->remaining < (int)sizeof(ExpressionNode)) refill_arena(&compiler_arena, pool);
    node = (ExpressionNode *)pool->next;
    pool->remaining -= sizeof(ExpressionNode);
    pool->next += sizeof(ExpressionNode);
    node->flags = 2;
    node->kind = operation;
    node->value.children.left = left;
    node->value.children.right = right;
    node->type = type;
    return (IrToken *)node;
}

/* Original 0x2b110..0x2b15e. End the function's statement context before
   releasing parameter symbols and the per-function alias table. */
void finish_function_definition(void)
{
    end_function_context(0);
    function_parameter_declarations = 0;
    current_parameter = 0;
    function_parameter_context = 0;
    active_parameter_context = 0;
    scanner_flags &= ~0x80000;
    function_symbol = 0;
    release_arena(1);
    clear_symbol_aliases();
}

FrontendSymbol *initializer_temporary; /* original 0x60460 */
/* This image only clears the reserved per-function word at 0x6bf70. */
static unsigned function_reserved_state;

/* Original 0x2afb0..0x2b109. Finish the common exit, serialize completed
   scopes and function state, then reset per-function parser workspaces. */
void __stdcall end_function_context(int direct_scope_exit)
{
    CType *result;
    if (function_symbol) {
        if (!options.prototypes_only) {
            if (!(parser_flags & 0x8000)) {
                result = function_return_type;
                if (result->flags != 0x80 || result->related) {
                    if (!result->related && (result->flags == 0x128 ||
                        ((function_symbol->type->attributes & 0x70) == 0x40 && (result->flags & 0x100))))
                        compiler_warning(1, 137, function_symbol->name);
                    else compiler_warning(3, 35, function_symbol->name);
                }
            }
            if (!direct_scope_exit) record_line_information(1);
            emit_code_label(function_exit_label);
            if (options.debug_info) record_line_information(0x12);
            write_ir_record(0x47, 0);
        }
        if (direct_scope_exit) leave_compound_scope();
        else finish_compound_statement();
        finish_function_records();
        if (!options.prototypes_only) write_ir_record(0x54, make_integer_constant(0));
    }
    completed_scopes = 0;
    function_return_type = 0;
    scope_cleanup_stack = 0;
    initializer_temporary = 0;
    global_expression_scope_active = 0;
    text_work_cursor = expression_storage;
    global_record_kind = 0;
    parameter_nesting = 0;
    function_reserved_state = 0;
    function_scope_level = 0;
}

/* Original 0x230c0..0x23192. Return the parser to declaration context,
   retaining arenas when this entry is used during error recovery. */
void __stdcall finish_external_declaration(int recovering)
{
    if (!recovering) release_arena(1);
    if (!recovering) release_arena(2);
    text_work_cursor = expression_storage;
    function_symbol = 0;
    function_parameter_declarations = 0;
    current_parameter = function_parameter_declarations;
    function_parameter_context = 0;
    active_parameter_context = function_parameter_context;
    scanner_flags &= ~0x02000000;
    parser_flags &= ~0x2000;
    declaration_spec_flags[declaration_depth] &= ~3;
    if (error_count) {
        scope_depth = 0;
        current_scope = global_scope;
        declaration_depth = 0;
        global_record_kind = 0;
        function_scope_level = 0;
    }
}

/* Original 0x2b780..0x2b7b6. The list value is the last element of a
   circular parameter chain, whose link points to the first parameter. */
FrontendSymbol *__stdcall append_parameter(FrontendSymbol *list, FrontendSymbol *parameter)
{
    if (!parameter) return list;
    if (!list) parameter->detail.parameter_next = parameter;
    else link_parameter_after(list, parameter);
    return parameter;
}

/* Original 0x2c950..0x2c975. Retain both the circular order and the
   backwards declaration link used while constructing a parameter list. */
void __stdcall link_parameter_after(FrontendSymbol *list, FrontendSymbol *parameter)
{
    if (!parameter) return;
    if (!list) parameter->detail.parameter_next = parameter;
    else {
        parameter->detail.parameter_next = list->detail.parameter_next;
        list->detail.parameter_next = parameter;
        parameter->next = list;
    }
}

/* Original 0x2d010..0x2d0d2. Abstract declarators use the cast sentinel
   and allocate both symbol and mutable type in the parameter-selected arena. */
FrontendSymbol *__stdcall make_abstract_declarator(TypeShape *prefix)
{
    FrontendSymbol *symbol;
    ArenaPool *pool;
    int allocation_arena;
    ALLOCATE_PARSER_NODE(symbol, sizeof(FrontendSymbol), parameter_depth <= 0 ? 2 : 1);
    symbol->kind = 12;
    symbol->name = name_cast;
    ALLOCATE_PARSER_NODE(symbol->type, sizeof(TypeDescription), parameter_depth <= 0 ? 2 : 1);
    if (!prefix) prefix = new_type_shape(0, 0, 0);
    return apply_declarator_prefix(symbol, prefix);
}
