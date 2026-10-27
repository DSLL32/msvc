#include "code-input.h"
#include <recon.h>

extern int c2_basic_mode, c2_codeview_types, c2_global_optimize;
extern int retain_function_input_pool, input_error_seen, c2_il_extended_header;
extern C2InputSymbol *pending_optimized_input;
extern C2Function *input_function;
extern C2SymbolListScope *current_symbol_list_scope;

int input_preserve_operand, input_address_mode;
int input_end_of_module;
int input_saved_global_optimize = 1;
unsigned input_aggregate_bytes, input_last_source_index;
C2InputClause *input_clauses;

/* Only the two high-byte properties read by this dispatcher are represented.
 * Encountered function-entry70, parameter-end76, pragma79, scope-begin83 and filename86
 * are boundaries; variable38 and member-address39 have neither consumed property.
 * Reserved bytes are NOT reconstructed defaults; guards precede unknown reads.
 * The original WORDs, including unmodeled low bits, stay in analysis evidence. */
static struct C2InputOpcodeProperties {
    unsigned char pending00[2], add, subtract, multiply, divide, remainder, pending07, negate8, shift_left, shift_right, bit_and, bit_or, bit_xor, pending14, assignment_operator, assignment_subtract, pending17[7], assignment_xor24, pending25[9], relational34, pending35, relational36, pending37, variable, member_address39, operand_type40, symbol_operand, pending42[2], operand_type44, pending45[3], conversion, pending49, typed_operator;
    unsigned char number, pending52[4], branch56, pending57, call_symbol, pending59[3], indirect_call62, pending63[2], return_value, argument_list66, ignored67, pending68[2];
    unsigned char function_entry, function_exit, pending72[3], statement_end;
    unsigned char parameter_end, pending77[2];
    unsigned char pragma, pending80[3], scope_begin, scope_end, call_argument85, filename, pending87[2], discarded_value89, pending90[68];
} input_opcode_properties = {{0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}, 0, {0}, 0, 0, 0, 0, 0, 0, 0, 0x10, {0}, 0, {0}, 0, 0, 0, 0, {0}, 0x90, 0, 0x10, {0}, 0x80, {0}, 0x90, 0, 0, {0}, 0x10, 0x10, {0}, 0x10, 0x10, {0}, 0x10, {0}, 0x10, 0x10, 0, 0x10, {0}, 0, {0}};
typedef char InputOpcodePropertiesLayout[
    offsetof(struct C2InputOpcodeProperties, discarded_value89) == 89 &&
    offsetof(struct C2InputOpcodeProperties, negate8) == 8 &&
    offsetof(struct C2InputOpcodeProperties, assignment_operator) == 15 &&
    offsetof(struct C2InputOpcodeProperties, assignment_subtract) == 16 &&
    offsetof(struct C2InputOpcodeProperties, assignment_xor24) == 24 &&
    offsetof(struct C2InputOpcodeProperties, relational34) == 34 &&
    offsetof(struct C2InputOpcodeProperties, variable) == 38 &&
    offsetof(struct C2InputOpcodeProperties, member_address39) == 39 &&
    offsetof(struct C2InputOpcodeProperties, relational36) == 36 &&
    offsetof(struct C2InputOpcodeProperties, operand_type40) == 40 &&
    offsetof(struct C2InputOpcodeProperties, operand_type44) == 44 &&
    offsetof(struct C2InputOpcodeProperties, branch56) == 56 &&
    offsetof(struct C2InputOpcodeProperties, indirect_call62) == 62 &&
    offsetof(struct C2InputOpcodeProperties, argument_list66) == 66 &&
    offsetof(struct C2InputOpcodeProperties, ignored67) == 67 &&
    offsetof(struct C2InputOpcodeProperties, call_argument85) == 85 &&
    offsetof(struct C2InputOpcodeProperties, filename) == 86 ? 1 : -1];
/* Original48d1a8: the seventeen positively recovered comparison-negation
 * relations. Numbered codes remain until their producers establish names. */
const unsigned char input_inverse_condition[17] = {
    0,             /* 0 is unchanged */
    2, 1,          /* 1 <-> 2 */
    6, 5, 4, 3,    /* 3 <-> 6, 4 <-> 5 */
    10, 9, 8, 7,   /* 7 <-> 10, 8 <-> 9 */
    12, 11,        /* 11 <-> 12 */
    14, 13,        /* 13 <-> 14 */
    16, 15         /* 15 <-> 16 */
};
/* The third row is also consumed by global operand normalization. Unknown
 * rows remain storage representation only, with no default/equality claim. */
C2InputRegisterOpcodes input_register_opcodes = {{{0}, 0x120, 0x120, 0x120, {0}, 0x121, {0}}};
/* Typed operand kind5 is the encountered local-register opcode. All other
 * backing rows remain UNMODELED, and native guards precede their reads. */
static union {
    struct { unsigned pending00[5], local, pending06[11]; } row;
    unsigned values[17];
} input_operand_opcodes = {{{0}, 0x120, {0}}};
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define INPUT_PROPERTIES(op) \
    ((unsigned)(op) == 2 ? input_opcode_properties.add : \
     (unsigned)(op) == 3 ? input_opcode_properties.subtract : \
     (unsigned)(op) == 4 ? input_opcode_properties.multiply : \
     (unsigned)(op) == 5 ? input_opcode_properties.divide : \
     (unsigned)(op) == 6 ? input_opcode_properties.remainder : \
     (unsigned)(op) == 8 ? input_opcode_properties.negate8 : \
     (unsigned)(op) == 9 ? input_opcode_properties.shift_left : \
     (unsigned)(op) == 10 ? input_opcode_properties.shift_right : \
     (unsigned)(op) == 11 ? input_opcode_properties.bit_and : \
     (unsigned)(op) == 12 ? input_opcode_properties.bit_or : \
     (unsigned)(op) == 13 ? input_opcode_properties.bit_xor : \
     (unsigned)(op) == 15 ? input_opcode_properties.assignment_operator : \
     (unsigned)(op) == 16 ? input_opcode_properties.assignment_subtract : \
     (unsigned)(op) == 24 ? input_opcode_properties.assignment_xor24 : \
     (unsigned)(op) == 34 ? input_opcode_properties.relational34 : \
     (unsigned)(op) == 36 ? input_opcode_properties.relational36 : \
     (unsigned)(op) == 38 ? input_opcode_properties.variable : \
     (unsigned)(op) == 39 ? input_opcode_properties.member_address39 : \
     (unsigned)(op) == 40 ? input_opcode_properties.operand_type40 : \
     (unsigned)(op) == 41 ? input_opcode_properties.symbol_operand : \
     (unsigned)(op) == 44 ? input_opcode_properties.operand_type44 : \
     (unsigned)(op) == 48 ? input_opcode_properties.conversion : \
     (unsigned)(op) == 50 ? input_opcode_properties.typed_operator : \
     (unsigned)(op) == 51 ? input_opcode_properties.number : \
     (unsigned)(op) == 56 ? input_opcode_properties.branch56 : \
     (unsigned)(op) == 58 ? input_opcode_properties.call_symbol : \
     (unsigned)(op) == 62 ? input_opcode_properties.indirect_call62 : \
     (unsigned)(op) == 65 ? input_opcode_properties.return_value : \
     (unsigned)(op) == 66 ? input_opcode_properties.argument_list66 : \
     (unsigned)(op) == 67 ? input_opcode_properties.ignored67 : \
     (unsigned)(op) == 70 ? input_opcode_properties.function_entry : \
     (unsigned)(op) == 71 ? input_opcode_properties.function_exit : \
     (unsigned)(op) == 75 ? input_opcode_properties.statement_end : \
     (unsigned)(op) == 76 ? input_opcode_properties.parameter_end : \
     (unsigned)(op) == 79 ? input_opcode_properties.pragma : \
     (unsigned)(op) == 83 ? input_opcode_properties.scope_begin : \
     (unsigned)(op) == 84 ? input_opcode_properties.scope_end : \
     (unsigned)(op) == 85 ? input_opcode_properties.call_argument85 : \
     (unsigned)(op) == 86 ? input_opcode_properties.filename : \
     (unsigned)(op) == 89 ? input_opcode_properties.discarded_value89 : \
     (recon_unsupported("unmodeled input opcode properties"), 0))
#define INPUT_INVERSE(condition) \
    ((unsigned)(condition) < 16 ? \
     input_inverse_condition[(condition)] : \
     (recon_unsupported("unmodeled input comparison inverse"), 0))
#define INPUT_REGISTER_OPCODE(kind) \
    ((unsigned)(kind) == 4 ? input_register_opcodes.row.tracked : \
     (unsigned)(kind) == 5 ? input_register_opcodes.row.parameter : \
     (recon_unsupported("unmodeled input register opcode"), 0))
#define INPUT_OPERAND_OPCODE(kind) \
    ((unsigned)(kind) == 5 ? \
     input_operand_opcodes.row.local : \
     (recon_unsupported("unmodeled input operand opcode"), 0))
#else
#define INPUT_PROPERTIES(op) (((unsigned char *)&input_opcode_properties)[(op)])
#define INPUT_INVERSE(condition) input_inverse_condition[(condition)]
#define INPUT_REGISTER_OPCODE(kind) input_register_opcodes.values[(kind)]
#define INPUT_OPERAND_OPCODE(kind) input_operand_opcodes.values[(kind)]
#endif
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
/* No opcode has yet consumed this distinct high-byte qualifier property. */
#define INPUT_TYPE_PROPERTY(op) \
    (recon_unsupported("unmodeled input type property"), 0)
#else
#define INPUT_TYPE_PROPERTY(op) (((unsigned char *)&input_opcode_properties)[(op)] & 4)
#endif
#define INPUT_IS_REGISTER(node) ((1U << ((node)->kind & 31)) & 6U)
#define INPUT_IS_OPERAND(node) ((1U << ((node)->kind & 31)) & 0x7feU)
#define INPUT_LINK(prior, node) \
    link_code_nodes((C2Record *)(prior), (C2Record *)(node))

C2Record *__fastcall read_function_code(C2Function *function, C2InputStack **pending,
                                   C2Record *first, int limit, int outermost)
{
    C2InputStack *stack = *pending, *scratch;
    C2Expression *value, *left, *right, *record, *saved, *argument;
    C2Expression *directives = NULL, *last_directive = NULL;
    C2InputClause *clause;
    C2FunctionRegister *reg;
    C2InputSymbol *symbol;
    C2ILToken token, *operation;
    C2Record *cursor = first, *boundary, *nested_last, *handler_last;
    C2Expression **location;
    unsigned bytes, type_bytes;
    unsigned short type, saved_line, nested_line;
    unsigned char condition;
    int done = 0, arguments = 0;

    for (;;) {
        operation = &token;
        read_input_token(function, operation);
        if (!operation)
            c2_report_internal("E:\\utc\\src\\\\P2\\reader.c", 490);
        if (token.opcode != 2 && token.opcode != 3 && token.opcode != 4 &&
        token.opcode != 5 &&
        token.opcode != 6 &&
        token.opcode != 8 &&
        token.opcode != 9 &&
        token.opcode != 10 &&
        token.opcode != 11 &&
        token.opcode != 12 &&
        token.opcode != 13 &&
        token.opcode != 15 &&
        token.opcode != 16 &&
        token.opcode != 24 &&
        token.opcode != 34 &&
        token.opcode != 36 &&
        token.opcode != 38 &&
        token.opcode != 39 &&
        token.opcode != 40 &&
        token.opcode != 41 &&
        token.opcode != 44 &&
        token.opcode != 48 &&
        token.opcode != 50 &&
        token.opcode != 51 &&
        token.opcode != 56 &&
        token.opcode != 58 &&
        token.opcode != 62 &&
        token.opcode != 65 &&
        token.opcode != 66 &&
        token.opcode != 67 &&
        token.opcode != 70 &&
        token.opcode != 71 &&
        token.opcode != 75 &&
        token.opcode != 76 &&
        token.opcode != 79 &&
        token.opcode != 83 &&
        token.opcode != 84 &&
        token.opcode != 85 &&
        token.opcode != 86 &&
        token.opcode != 89) { __asm { int 3 } }
        if ((INPUT_PROPERTIES(token.opcode) & 0x80) && stack && stack->after) {
            if (token.opcode == 56 || token.opcode == 57) {
                input_comparison(&stack);
                input_preserve_operand = 1;
            } else if (token.opcode == 59) {
                input_preserve_operand = 1;
            }
            left = pop_input_expression(stack);
            input_preserve_operand = 0;
            if ((token.opcode == 59 || token.opcode == 62 || token.opcode == 64) &&
                (!INPUT_IS_REGISTER(left) || left->value.reg->kind != 3 ||
                 left->value.reg->value)) {
                right = new_input_temporary(left->type);
                right->value.reg->flags |= 0x40;
                stack->last = new_binary_expression(0x131, left->type,
                    right, left, link_code_nodes, (C2Record *)stack->last);
                left = right;
            }
            if (!INPUT_IS_REGISTER(left) || left->value.reg->kind != 3) {
                stack->last = new_assignment_expression(0x131, left->type,
                    left, link_code_nodes, (C2Record *)stack->last);
                left = stack->last->tail.right;
            }
            append_deferred_input(stack);
            INPUT_LINK(stack->last, left);
            stack->last = left;
        }
        switch (token.opcode) {
        case 1: case 42: case 43: case 67:
            break;
        case 2: input_binary_operator(&stack, 0x141); break;
        case 3: input_binary_operator(&stack, 0x142); break;
        case 4: input_binary_operator(&stack, 0x143); break;
        case 5: input_binary_operator(&stack, 0x149); break;
        case 6: input_binary_operator(&stack, 0x14a); break;
        case 8: input_unary_operator(&stack, 0x137); break;
        case 9: input_binary_operator(&stack, 0x145); break;
        case 10: input_binary_operator(&stack, 0x14b); break;
        case 11: input_binary_operator(&stack, 0x146); break;
        case 12: input_binary_operator(&stack, 0x147); break;
        case 13: input_binary_operator(&stack, 0x148); break;
        case 14: input_unary_operator(&stack, 0x136); break;
        case 15: input_assignment_operator(&stack, 0x141, &token); break;
        case 16: input_assignment_operator(&stack, 0x142, &token); break;
        case 17: input_assignment_operator(&stack, 0x143, &token); break;
        case 18: input_assignment_operator(&stack, 0x149, &token); break;
        case 19: input_assignment_operator(&stack, 0x14a, &token); break;
        case 21: input_assignment_operator(&stack, 0x145, &token); break;
        case 22: input_assignment_operator(&stack, 0x14b, &token); break;
        case 23: input_assignment_operator(&stack, 0x146, &token); break;
        case 24: input_assignment_operator(&stack, 0x148, &token); break;
        case 25: input_assignment_operator(&stack, 0x147, &token); break;
        case 26:
            if (!(stack->flags & 1)) {
                condition = input_comparison(&stack);
                record = stack->last->tail.right;
                record->location.expression.comparison =
                    (record->location.expression.comparison & 15) |
                    (INPUT_INVERSE(condition) << 4);
            } else
                stack->flags ^= 2;
            break;
        case 27: case 28:
            value = NULL;
            if ((stack->flags & 1) &&
                (((stack->flags & 2) && token.opcode == 27) ||
                 (!(stack->flags & 2) && token.opcode == 28)))
                value = stack->alternatives;
            swap_input_stacks(&stack);
            input_condition(&stack, value, &token);
            value = stack->alternatives;
            swap_input_stacks(&stack);
            input_condition(&stack, value, &token);
            if (value != stack->alternatives)
                INPUT_LINK(value, stack->alternatives);
            stack->alternatives = NULL;
            input_operand_pair(&stack, &left, &right, 1);
            stack->alternatives = value;
            break;
        case 31: case 32: case 33: case 34: case 35: case 36:
            /* High-nibble conditions are the six corresponding relational ops. */
            input_binary_operator(&stack, 0x151);
            record = stack->last->tail.right;
            switch (token.opcode) {
            case 31: condition = 0x20; break;
            case 32: condition = 0x10; break;
            case 33: condition = 0x40; break;
            case 34: condition = 0x60; break;
            case 35: condition = 0x30; break;
            default: condition = 0x50; break;
            }
            record->location.expression.comparison =
                (record->location.expression.comparison & 15) | condition;
            break;
        case 38:
            value = variable_input_expression(token.value.scalar.argument.symbol);
            push_input_trees(&stack, value);
            symbol = token.value.scalar.argument.symbol;
            if (symbol->attributes & 0x1000)
                convert_input_operand(stack, 0x3004, 0, 0);
            if (symbol->symbol.kind == 1 && (symbol->symbol.flags & 0x1000))
                finish_input_declaration(stack);
            break;
        case 39:
            swap_input_stacks(&stack);
            value = stack->last;
            if (!INPUT_IS_OPERAND(value)) value = value->tail.right;
            if ((value->type & 0xf000) == 0x5000)
                value = convert_input_pointer((C2InputStack *)stack, 0x3004);
            swap_input_stacks(&stack);
            if (!((1U << (value->kind & 31)) & 0x18)) {
                input_binary_operator(&stack, 0x141);
            } else {
                input_operand_pair(&stack, &left, &right, 0);
                reg = left->value.reg;
                release_small_node(left);
                saved = right->left;
                release_small_node(right);
                reg = new_address_register(reg, 0x5000, 0, (long)saved);
                value = new_register_expression(INPUT_REGISTER_OPCODE(reg->kind), reg);
                value->type = 0x3004;
                INPUT_LINK(stack->last, value);
                stack->last = value;
            }
            break;
        case 40:
            set_input_operand_type(stack, 0x1004);
            input_binary_operator(&stack, 0x141);
            break;
        case 41:
            value = token.value.scalar.argument.symbol->input_operand;
            if (!value) value = symbol_input_expression(token.value.scalar.argument.symbol);
            if (directives) {
                value->flags |= 2;
                if (!last_directive) directives->detail.data = value;
                else {
                    last_directive->next = value;
                    value->previous = last_directive;
                }
                last_directive = value;
            } else
                push_input_trees(&stack, value);
            break;
        case 44: case 52:
            value = stack->last;
            if (value->kind != 16 && ((1U << ((token.type >> 12) & 31)) & 14) &&
                (token.type & 0xf000) != 0x3000)
                adjust_input_operand(value);
            set_input_operand_type(stack, token.type);
            break;
        case 48: case 90:
            convert_input_operand(stack, token.type, token.bytes, token.flags & 0x800);
            if (token.opcode == 90) {
                value = stack->last;
                if (!INPUT_IS_OPERAND(value)) value = value->tail.right;
                value->location.expression.properties |= 0x80;
            }
            if ((token.type & 0xf000) == 0x5000) input_aggregate_bytes = token.bytes;
            break;
        case 50:
            input_store_operator(&stack, &token);
            break;
        case 51:
            if ((token.type & 0xf000) == 0x4000)
                value = new_real_expression(token.type, token.value.real_pointer);
            else
                value = new_integer_expression(token.type,
                    token.value.integer);
            push_input_trees(&stack, value);
            break;
        case 53: case 54:
            value = pop_input_trees(&stack);
            record = save_input_expression(stack);
            push_input_trees(&stack->after, record);
            if (stack->flags & 4) {
                stack->after->flags = stack->flags;
                if (stack->alternatives)
                    stack->after->alternatives = copy_input_expression_list(stack->alternatives);
            }
            push_input_trees(&stack->after, value);
            input_assignment_operator(&stack->after, token.opcode != 53 ? 0x142 : 0x141, &token);
            convert_input_operand(stack, token.type, token.bytes, token.flags & 0x800);
            stack->flags |= 0x20;
            break;
        case 55:
            left = pop_input_trees(&stack);
            left->type = 0x1001;
            right = pop_input_trees(&stack);
            right->type = 0x1001;
            stack->alternatives = left;
            INPUT_LINK(left, right);
            stack->flags |= 4;
            break;
        case 56: case 57:
            left = token.value.scalar.argument.symbol->input_operand;
            if (!left) left = symbol_input_expression(token.value.scalar.argument.symbol);
            condition = input_comparison(&stack);
            if (token.opcode == 57) condition = INPUT_INVERSE(condition);
            input_preserve_operand = 1;
            right = pop_input_expression(stack);
            input_preserve_operand = 0;
            right->location.expression.comparison &= 15;
            value = new_call_expression(0x159, condition, left, right,
                right->value.reg->value, link_code_nodes,
                (C2Record *)stack->last);
            stack->last = value;
            break;
        case 58:
            left = token.value.scalar.argument.symbol->input_operand;
            if (!left) left = symbol_input_expression(token.value.scalar.argument.symbol);
            value = new_call_expression(0x15a, 0, left, NULL, 0, NULL, NULL);
            push_input_trees(&stack, value);
            break;
        case 59:
            left = pop_input_expression(stack);
            right = new_input_temporary(left->type);
            stack->last = new_binary_expression(0x131, left->type,
                right, left, link_code_nodes, (C2Record *)stack->last);
            saved_line = record_line_delta;
            boundary = new_statement_boundary();
            scratch = NULL;
            nested_last = read_function_code(function, &scratch, boundary, limit, 0);
            saved = (C2Expression *)boundary->next;
            release_small_node(boundary);
            saved->previous = NULL;
            nested_line = record_line_delta;
            record_line_delta = saved_line;
            stack->last = finish_input_clauses(input_clauses, stack->last);
            INPUT_LINK(stack->last, saved);
            clause = NULL;
            record_line_delta = nested_line;
            stack->last = (C2Expression *)nested_last;
            input_clauses = NULL;
            break;
        case 60:
            value = pop_input_trees(&stack);
            release_small_node(value);
            clause = c2_pool_allocate(4, sizeof(C2InputClause));
            input_clauses = clause;
            clause->label = token.value.scalar.argument.pointer;
            break;
        case 61:
            value = pop_input_trees(&stack);
            clause->next = c2_pool_allocate(4, sizeof(C2InputClause));
            clause = clause->next;
            clause->start = value->left;
            clause->end = value->tail.right;
            clause->first = clause->start;
            clause->last = clause->end;
            clause->label = token.value.scalar.argument.pointer;
            release_small_node(value);
            break;
        case 62:
            input_call(&stack, &token, limit, function, arguments);
            arguments = 0;
            break;
        case 64:
            record = stack->last;
            if (record->immediate == 0xdb) function->flags |= 0x40;
            if (record->immediate == 0xd8) function->flags |= 0x80;
            input_call(&stack, &token, limit, function, arguments);
            arguments = 0;
            break;
        case 65:
            left = pop_input_expression(stack);
            type = token.type;
            if (left->kind == 1 && left->value.reg->kind == 3 &&
                left->value.reg->value && left->value.reg->value != stack->last) {
                value = left->value.reg->value;
                location = &value;
            } else
                location = &stack->last;
            convert_input_expression(&left, token.type, location);
            bytes = function->symbol->return_bytes;
            type_bytes = adjust_input_return_size((unsigned short)(function->symbol->attributes & 15), bytes);
            if (needs_input_return_storage(function->symbol->symbol.flags & 0x1c,
                                           (unsigned short)type_bytes, bytes)) {
                symbol = function->return_storage;
                type = 0x3004;
                if (!symbol)
                    c2_report_internal("E:\\utc\\src\\\\P2\\reader.c", 1065);
                symbol->attributes = (symbol->attributes & ~12U) | 3;
                value = input_symbol_operand(symbol);
                if (token.bytes < 4096) {
                    unsigned short aggregate_type = (unsigned short)token.bytes | 0x5000;
                    right = new_aggregate_expression(0x122, aggregate_type,
                        NULL, 0, value, NULL, 0);
                    saved = new_integer_expression(0x1004, token.bytes);
                    stack->last = new_block_copy_expression(0x13f, aggregate_type,
                        right, left, saved, link_code_nodes, (C2Record *)stack->last);
                } else {
                    INPUT_LINK(stack->last, left);
                    stack->last = left;
                    push_input_trees(&stack, value);
                    swap_input_stacks(&stack);
                    input_aggregate_return(&stack, &token);
                    pop_input_expression(stack);
                }
                left = copy_input_expression(value);
                input_aggregate_bytes = token.bytes;
            }
            stack->last = new_transfer_expression(0x140, type, NULL, left,
                link_code_nodes, (C2Record *)stack->last);
            break;
        case 66:
            input_argument_list(&stack);
            break;
        case 68:
            if (arguments < 1)
                swap_input_stacks(&stack);
            else {
                scratch = stack->next;
                stack->next = scratch->next;
                scratch->next = stack;
                stack = scratch;
            }
            finish_input_expression(stack);
            if (stack->before) finish_input_temporaries(stack);
            boundary = new_statement_boundary();
            left = (C2Expression *)append_input_trees(stack, boundary);
            value = (C2Expression *)boundary->next;
            if (!value)
                stack->last = stack->first;
            else {
                INPUT_LINK(stack->first, value);
                stack->last = left;
            }
            release_small_node(boundary);
            if (stack->after) append_deferred_input(stack);
            value = pop_input_trees(&stack);
            if (value) push_input_trees(&stack->before, value);
            break;
        case 70:
            if (c2_il_extended_header && input_error_seen && function->symbol->input_order) {
                input_last_source_index = function->symbol->input_order;
                record_source_line = resolve_input_source_line(input_last_source_index);
                if (record_source_line < function->source_line_base + 0x10000U)
                    record_line_delta = (unsigned short)(record_source_line - function->source_line_base);
                else
                    record_line_delta = 0xffff;
            }
            argument = NULL;
            if (!arguments)
                left = pop_input_trees(&stack);
            else {
                argument = pop_input_trees(&stack);
                mark_first_input_expression(argument);
                left = pop_input_trees(&stack);
            }
            value = new_function_entry(function);
            release_small_node(left);
            push_input_trees(&stack, value);
            operation = read_input_token(function, &token);
            while (operation && token.opcode != 76) {
                record = stack->last;
                value = input_symbol_operand(token.value.scalar.argument.symbol);
                if (!record->left)
                    record->left = value;
                else {
                    INPUT_LINK(value, record->left);
                    record->left = value;
                }
                record->left->location.expression.properties |= 0x10;
                operation = read_input_token(function, &token);
            }
            bytes = function->symbol->return_bytes;
            type_bytes = adjust_input_return_size((unsigned short)(function->symbol->attributes & 15), bytes);
            if (needs_input_return_storage(function->symbol->symbol.flags & 0x1c,
                                           (unsigned short)type_bytes, bytes)) {
                symbol = new_input_local(function->symbols, 3, 4, 3, 1, 4, 0x100);
                symbol->storage.temporary.anonymous = 1;
                symbol->symbol.name = NULL;
                function->return_storage = symbol;
            }
            if (argument) {
                record = stack->last;
                if (!record->left)
                    record->left = argument;
                else {
                    INPUT_LINK(argument, record->left);
                    record->left = argument;
                }
                argument->location.expression.properties |= 0x10;
            }
            arguments = 0;
            break;
        case 71:
            value = new_function_exit(function);
            push_input_trees(&stack, value);
            break;
        case 75:
            finish_input_expression(stack);
            break;
        case 76:
            done = 1;
            break;
        case 79:
            switch ((unsigned short)token.value.scalar.control) {
            case 1: case 36:
                if (token.value.scalar.low) {
                    record_source_line = resolve_input_source_line(token.value.scalar.low);
                    if (record_source_line < function->source_line_base + 0x10000U)
                        record_line_delta = (unsigned short)(record_source_line - function->source_line_base);
                    else
                        record_line_delta = 0xffff;
                }
                break;
            case 15:
                value = new_control_expression(0x17e);
                push_input_trees(&stack, value);
                break;
            case 16:
                value = new_control_expression(0x17f);
                push_input_trees(&stack, value);
                break;
            case 17: case 18:
                if (c2_codeview_types) {
                    value = new_control_expression((short)token.value.scalar.control == 17 ? 0x180 : 0x181);
                    push_input_trees(&stack, value);
                }
                break;
            case 22:
                value = parse_input_assembly((unsigned char *)token.value.scalar.low, function->symbols);
                goto append_directive;
            case 24:
                value = new_control_expression(0x185);
                value->value.pointer = (void *)(token.value.scalar.low & 0xffff);
            append_directive:
                value->flags |= 2;
                if (!last_directive)
                    directives->detail.data = value;
                else {
                    last_directive->next = value;
                    value->previous = last_directive;
                }
                last_directive = value;
                break;
            case 25:
                if (!cursor || cursor->opcode != 0x160) {
                    directives = new_input_assembly_record(0x160, 0x1004);
                    function->flags |= 1;
                } else {
                    directives = (C2Expression *)cursor;
                    cursor = cursor->previous;
                    last_directive = (C2Expression *)directives->detail.data;
                    unlink_input_record((C2Record *)directives);
                    while (last_directive->next)
                        last_directive = last_directive->next;
                    function->flags |= 1;
                }
                break;
            case 26:
                if (!last_directive)
                    release_small_node(directives);
                else
                    push_input_trees(&stack, directives);
                directives = NULL;
                last_directive = NULL;
                break;
            case 31:
                function->options = token.value.scalar.low;
                if (!retain_function_input_pool) {
                    set_function_input_options(token.value.scalar.low);
                    input_saved_global_optimize = c2_global_optimize;
                }
                break;
            case 32:
                value = new_control_expression(0x182);
                value->value.pointer = (void *)token.value.scalar.low;
                push_input_trees(&stack, value);
                break;
            case 35:
                function->warning_state = token.value.scalar.argument.pointer;
                if (!retain_function_input_pool)
                    set_input_warning_state(token.value.scalar.argument.pointer);
                break;
            case 37:
                if (!token.value.scalar.low) {
                    if (function->symbol->symbol.flags & 0x1000) {
                        value = new_input_expression(0x16a, 0x1004);
                        push_input_trees(&stack, value);
                    }
                } else {
                    symbol = (C2InputSymbol *)lookup_symbol(function->symbols, token.value.scalar.low);
                    if (symbol->detail.variable.flags) {
                        left = symbol->input_operand;
                        symbol->symbol.source_kind = 0x58;
                        if (!left) left = symbol_input_expression(symbol);
                        value = new_call_expression(0x15c, 0, left, NULL, 0, NULL, NULL);
                        push_input_trees(&stack, value);
                    } else
                        pending_optimized_input = symbol;
                }
                break;
            default:
                c2_report_internal("E:\\utc\\src\\\\P2\\reader.c", 1416);
                break;
            }
            break;
        case 83:
            if (++limit > 1) {
                current_symbol_list_scope = current_symbol_list_scope->child;
                value = new_control_expression(0x17e);
                value->value.pointer = current_symbol_list_scope->records;
                push_input_trees(&stack, value);
            }
            break;
        case 84:
            if (limit > 1) {
                C2SymbolListScope *scope;
                value = new_control_expression(0x17f);
                value->value.pointer = current_symbol_list_scope->records;
                push_input_trees(&stack, value);
                scope = current_symbol_list_scope->parent;
                scope->child = scope->child->child;
                current_symbol_list_scope = scope;
                if (scope->child) scope->child->parent = scope;
            }
            if (!(token.value.scalar.low | token.value.scalar.high)) done = 1;
            --limit;
            break;
        case 85:
            input_call_argument(stack, &token);
            break;
        case 86:
            set_input_filename(token.value.scalar.argument.text);
            record_source_line = 0;
            record_line_delta = 0;
            input_error_seen = 1;
            break;
        case 89:
            value = pop_input_expression(stack);
            if (!stack->first->next) {
                INPUT_LINK(stack->last, value);
                stack->last = value;
            } else {
                value = new_assignment_expression(0x138, value->type, value,
                    link_code_nodes, (C2Record *)stack->last);
                if (c2_basic_mode && (value->type & 0xf000) != 0x4000) {
                    left = new_scalar_expression(0x12a, (unsigned)pending_optimized_input);
                    copy_input_operand(value, left);
                    value->opcode = 0x14c;
                }
                stack->last = value;
            }
            break;
        case 92:
            value = new_input_expression(0x16d, 0x1004);
            value->detail.data = c2_pool_allocate(1, 16);
            ((C2InputHandler *)value->detail.data)->kind = token.value.scalar.control;
            boundary = new_statement_boundary();
            handler_last = boundary;
            scratch = stack->before;
            while (scratch) {
                handler_last = append_input_trees(scratch, handler_last);
                pop_input_trees(&scratch);
            }
            stack->before = NULL;
            handler_last = append_input_trees(stack, handler_last);
            scratch = stack->after;
            while (scratch) {
                handler_last = append_input_trees(scratch, handler_last);
                pop_input_trees(&scratch);
            }
            stack->after = NULL;
            pop_input_trees(&stack);
            ((C2InputHandler *)value->detail.data)->body = (C2Expression *)boundary->next;
            release_small_node(boundary);
            if (((C2InputHandler *)value->detail.data)->body)
                ((C2InputHandler *)value->detail.data)->body->previous = NULL;
            left = pop_input_expression(stack);
            if (left->location.expression.properties & 0x10)
                left = copy_input_expression(left);
            value->left = copy_input_operand(value,
                left->kind == 6 ? left->aggregate_value : left);
            value->type = value->left->type;
            value->tail.right = new_temporary_operand(value->left->type);
            value->tail.right->location.expression.properties |= 0x10;
            bind_input_temporary(value->tail.right->value.reg, value);
            INPUT_LINK(stack->last, value);
            stack->last = value;
            if (left->kind == 6) {
                convert_input_operand(stack, left->type, input_aggregate_bytes, 0);
                left->aggregate_value = NULL;
                release_small_node(left);
            }
            break;
        case 93: case 94:
            value = new_input_expression(token.opcode == 93 ? 0x16e : 0x16f, 0x1004);
            value->detail.data = c2_pool_allocate(1, 16);
            ((C2InputHandler *)value->detail.data)->body = (C2Expression *)token.value.scalar.low;
            ((C2InputHandler *)value->detail.data)->kind = token.value.scalar.control;
            push_input_trees(&stack, value);
            break;
        case 96: case 98:
            value = new_input_expression(token.opcode == 96 ? 0x171 : 0x173, 0x1004);
            value->detail.data = c2_pool_allocate(1, 16);
            push_input_trees(&stack, value);
            break;
        case 97:
            value = new_input_expression(0x172, 0x1004);
            value->detail.data = c2_pool_allocate(1, 16);
            ((C2InputHandler *)value->detail.data)->body = (C2Expression *)token.value.scalar.low;
            ((C2InputHandler *)value->detail.data)->filter = token.value.scalar.argument.pointer;
            ((C2InputHandler *)value->detail.data)->kind = token.value.scalar.control;
            push_input_trees(&stack, value);
            break;
        case 104:
            input_address_mode = 1;
            input_binary_operator(&stack, 0x141);
            input_address_mode = 0;
            left = stack->last->tail.right;
            if (INPUT_IS_REGISTER(left) && left->value.reg->value) {
                left->value.reg->properties |= 1;
                left->value.reg->value->type = 0x3004;
                set_input_expression_type(left, 0x3004);
            }
            break;
        case 119:
            left = pop_input_trees(&stack);
            if (!left->left)
                release_small_node(left);
            else {
                value = save_input_expression(stack);
                push_input_trees(&stack, value);
                swap_input_stacks(&stack);
                value = new_integer_expression(0x3004, 0);
                push_input_trees(&stack, value);
                input_binary_operator(&stack, 0x151);
                stack->last->tail.right->location.expression.comparison =
                    (stack->last->tail.right->location.expression.comparison & 15) | 0x10;
                swap_input_stacks(&stack);
                push_input_trees(&stack, left);
                input_binary_operator(&stack, 0x141);
                value = new_integer_expression(0x1004, 0);
                push_input_trees(&stack, value);
                input_argument_list(&stack);
            }
            break;
        case 139:
            input_address_mode = 1;
            input_binary_operator(&stack, 0x142);
            input_address_mode = 0;
            left = stack->last;
            if (left->opcode == 0x142 && left->left->kind == 7) {
                value = new_integer_expression(0x1004, 0);
                attach_input_expression(left, left->left, value);
            }
            break;
        case 141:
            value = new_input_expression(0x167, 0x1004);
            input_sequence_point(stack, value);
            value = new_input_expression(0x168, 0x1004);
            left = pop_input_expression(stack);
            copy_input_operand(value, left);
            INPUT_LINK(stack->last, value);
            stack->last = value;
            break;
        case 142:
            value = new_input_expression(0x163, 0x1004);
            push_input_trees(&stack, value);
            break;
        case 143:
            value = new_input_expression(0x161, 0x1004);
            push_input_trees(&stack, value);
            break;
        case 144:
            value = new_input_expression(0x162, 0x1004);
            push_input_trees(&stack, value);
            break;
        case 153:
            convert_input_pointer(stack, 0x3004);
            arguments = 1;
            break;
        case 154:
            convert_input_pointer(stack, 0x3004);
            convert_input_pointer(stack, 0x3004);
            arguments = 2;
            break;
        case 155:
            bytes = (token.type & 0xf000) == 0x5000 ? token.bytes : token.type & 0xfff;
            type_bytes = input_type_alignment(token.type);
            symbol = new_input_storage(function, token.value.scalar.control, type_bytes, bytes);
            value = variable_input_expression(symbol);
            push_input_trees(&stack, value);
            break;
        case 157:
            input_exception(&stack, limit, function);
            break;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\reader.c", 1651);
            break;
        }
        if (token.opcode == 75 || (INPUT_PROPERTIES(token.opcode) & 0x10)) {
            if (done) {
                *pending = stack;
                return cursor;
            }
            if (stack) {
                if (pending_input_records && outermost) {
                    value = new_control_expression(0x17e);
                    value->value.pointer = pending_input_records;
                    value->control_flags = 1;
                    INPUT_LINK(cursor, value);
                    cursor = (C2Record *)value;
                }
                scratch = stack->before;
                while (scratch) {
                    cursor = append_input_trees(scratch, cursor);
                    pop_input_trees(&scratch);
                }
                stack->before = NULL;
                cursor = append_input_trees(stack, cursor);
                scratch = stack->after;
                while (scratch) {
                    cursor = append_input_trees(scratch, cursor);
                    pop_input_trees(&scratch);
                }
                stack->after = NULL;
                pop_input_trees(&stack);
                if (pending_input_records && outermost) {
                    value = new_control_expression(0x17f);
                    value->value.pointer = pending_input_records;
                    value->control_flags = 1;
                    INPUT_LINK(cursor, value);
                    pending_input_records = NULL;
                    cursor = (C2Record *)value;
                }
            }
        }
        if (done) {
            *pending = stack;
            return cursor;
        }
    }
}

C2ILToken *__fastcall read_input_token(C2Function *function, C2ILToken *token)
{
    unsigned char opcode;
    C2ILToken *result;
    if (input_end_of_module)
        return NULL;
    opcode = (unsigned char)c2_read_il_byte();
    if (opcode == 0xff)
        c2_fatal(1, "E:\\utc\\src\\\\P2\\reader.c", 1709);
    if (opcode == 77) {
        input_end_of_module = 1;
        return NULL;
    }
    token->flags = 0;
    token->type = 0;
    result = decode_input_token(opcode, function, token);
    if ((result->type & 0xf000) == 0x4000)
        function->flags |= 8;
    return result;
}

/* Only opcode38/45/70/79/83 classifier bytes are modeled. Padding is unavailable
 * backing, not a reconstruction of any other opcode's form/default. */
enum InputTokenForm {
    INPUT_FORM_NONE, INPUT_FORM_TYPE, INPUT_FORM_SYMBOL,
    INPUT_FORM_TYPED_SYMBOL_TRAILER, INPUT_FORM_SYMBOL_TRAILER,
    INPUT_FORM_TYPE_TRAILER, INPUT_FORM_NUMBER, INPUT_FORM_TYPED_SYMBOL,
    INPUT_FORM_SYMBOL_ONLY, INPUT_FORM_TYPED_CODE, INPUT_FORM_CODE,
    INPUT_FORM_INVALID, INPUT_FORM_PRAGMA, INPUT_FORM_SIGNED,
    INPUT_FORM_SYMBOL_PAIR, INPUT_FORM_SHORT_TRAILER, INPUT_FORM_EMPTY,
    INPUT_FORM_SIGNED_ALT, INPUT_FORM_UNSIGNED_CONTROL, INPUT_FORM_SIGNED_CONTROL,
    INPUT_FORM_SIGNED_PAIR, INPUT_FORM_SYMBOLS_SHORT
};
static struct C2InputTokenForms {
    unsigned char pending00[2], add, subtract, multiply, divide, remainder, pending07, negate8, shift_left, shift_right, bit_and, bit_or, bit_xor, pending14, assignment_operator, assignment_subtract, pending17[7], assignment_xor24, pending25[9], relational34, pending35, relational36, pending37, variable, member_address39, operand_type40, symbol_operand, pending42[2], operand_type44, parameter, pending46[2];
    unsigned char conversion, pending49, typed_operator, number, pending52[4], branch56, pending57, call_symbol, pending59[3], indirect_call62, pending63[2], return_value, argument_list66, ignored67, pending68[2];
    unsigned char function_entry, function_exit, pending72[3], statement_end, parameter_end, pending77[2], pragma, pending80[3], scope_begin, scope_end, call_argument85, pending86[3], discarded_value89;
} input_token_forms = {{0}, INPUT_FORM_NONE, 0, INPUT_FORM_NONE, INPUT_FORM_NONE, INPUT_FORM_NONE, 0, INPUT_FORM_NONE, INPUT_FORM_NONE, INPUT_FORM_NONE, INPUT_FORM_NONE, INPUT_FORM_NONE, INPUT_FORM_NONE, 0, INPUT_FORM_TYPE, INPUT_FORM_TYPE, {0}, INPUT_FORM_TYPE, {0}, 0, 0, 0, 0, INPUT_FORM_SYMBOL, INPUT_FORM_TYPE, INPUT_FORM_SYMBOL, INPUT_FORM_SYMBOL, {0}, INPUT_FORM_TYPE_TRAILER, INPUT_FORM_SYMBOL, {0}, INPUT_FORM_TYPE, 0, INPUT_FORM_TYPE, INPUT_FORM_NUMBER, {0}, INPUT_FORM_SYMBOL, 0, INPUT_FORM_SYMBOL, {0}, INPUT_FORM_TYPED_CODE, {0}, INPUT_FORM_TYPE, 0, INPUT_FORM_NONE, {0}, INPUT_FORM_NONE, INPUT_FORM_NONE, {0}, INPUT_FORM_NONE, INPUT_FORM_NONE, {0}, INPUT_FORM_PRAGMA, {0}, INPUT_FORM_NONE, INPUT_FORM_SIGNED, INPUT_FORM_TYPE, {0}, INPUT_FORM_NONE};
typedef char InputTokenFormsLayout[
    offsetof(struct C2InputTokenForms, discarded_value89) == 89 &&
    offsetof(struct C2InputTokenForms, negate8) == 8 &&
    offsetof(struct C2InputTokenForms, assignment_operator) == 15 &&
    offsetof(struct C2InputTokenForms, assignment_subtract) == 16 &&
    offsetof(struct C2InputTokenForms, assignment_xor24) == 24 &&
    offsetof(struct C2InputTokenForms, relational34) == 34 &&
    offsetof(struct C2InputTokenForms, variable) == 38 &&
    offsetof(struct C2InputTokenForms, member_address39) == 39 &&
    offsetof(struct C2InputTokenForms, relational36) == 36 &&
    offsetof(struct C2InputTokenForms, operand_type40) == 40 &&
    offsetof(struct C2InputTokenForms, operand_type44) == 44 &&
    offsetof(struct C2InputTokenForms, branch56) == 56 &&
    offsetof(struct C2InputTokenForms, indirect_call62) == 62 &&
    offsetof(struct C2InputTokenForms, argument_list66) == 66 &&
    offsetof(struct C2InputTokenForms, ignored67) == 67 &&
    offsetof(struct C2InputTokenForms, scope_end) == 84 &&
    offsetof(struct C2InputTokenForms, call_argument85) == 85 ? 1 : -1];
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define INPUT_TOKEN_FORM(op) ((unsigned)(op) == 2 ? input_token_forms.add : \
    (unsigned)(op) == 3 ? input_token_forms.subtract : \
    (unsigned)(op) == 4 ? input_token_forms.multiply : \
    (unsigned)(op) == 5 ? input_token_forms.divide : \
    (unsigned)(op) == 6 ? input_token_forms.remainder : \
    (unsigned)(op) == 8 ? input_token_forms.negate8 : \
    (unsigned)(op) == 9 ? input_token_forms.shift_left : \
    (unsigned)(op) == 10 ? input_token_forms.shift_right : \
    (unsigned)(op) == 11 ? input_token_forms.bit_and : \
    (unsigned)(op) == 12 ? input_token_forms.bit_or : \
    (unsigned)(op) == 13 ? input_token_forms.bit_xor : \
    (unsigned)(op) == 15 ? input_token_forms.assignment_operator : \
    (unsigned)(op) == 16 ? input_token_forms.assignment_subtract : \
    (unsigned)(op) == 24 ? input_token_forms.assignment_xor24 : \
    (unsigned)(op) == 34 ? input_token_forms.relational34 : \
    (unsigned)(op) == 36 ? input_token_forms.relational36 : \
    (unsigned)(op) == 38 ? input_token_forms.variable : \
    (unsigned)(op) == 39 ? input_token_forms.member_address39 : \
    (unsigned)(op) == 40 ? input_token_forms.operand_type40 : \
    (unsigned)(op) == 41 ? input_token_forms.symbol_operand : \
    (unsigned)(op) == 44 ? input_token_forms.operand_type44 : \
    (unsigned)(op) == 45 ? input_token_forms.parameter : \
    (unsigned)(op) == 48 ? input_token_forms.conversion : \
    (unsigned)(op) == 50 ? input_token_forms.typed_operator : \
    (unsigned)(op) == 51 ? input_token_forms.number : \
    (unsigned)(op) == 56 ? input_token_forms.branch56 : \
    (unsigned)(op) == 58 ? input_token_forms.call_symbol : \
    (unsigned)(op) == 62 ? input_token_forms.indirect_call62 : \
    (unsigned)(op) == 65 ? input_token_forms.return_value : \
    (unsigned)(op) == 66 ? input_token_forms.argument_list66 : \
    (unsigned)(op) == 67 ? input_token_forms.ignored67 : \
    (unsigned)(op) == 70 ? input_token_forms.function_entry : \
    (unsigned)(op) == 71 ? input_token_forms.function_exit : \
    (unsigned)(op) == 75 ? input_token_forms.statement_end : \
    (unsigned)(op) == 76 ? input_token_forms.parameter_end : \
    (unsigned)(op) == 79 ? input_token_forms.pragma : \
    (unsigned)(op) == 83 ? input_token_forms.scope_begin : \
    (unsigned)(op) == 84 ? input_token_forms.scope_end : \
    (unsigned)(op) == 85 ? input_token_forms.call_argument85 : \
    (unsigned)(op) == 89 ? input_token_forms.discarded_value89 : \
    (recon_unsupported("unmodeled input token form"), 0))
#else
#define INPUT_TOKEN_FORM(op) (((unsigned char *)&input_token_forms)[(op)])
#endif

C2ILToken *__fastcall decode_input_token(unsigned opcode, C2Function *function,
                                        C2ILToken *token)
{
    unsigned form;
    unsigned serial;
    int kind;
    C2ScopeHash *scope;
    C2Real real, temporary, *storage;
    union { unsigned __int64 bits; double real; } number;
    float single;
    if (opcode != 2 && opcode != 3 && opcode != 4 &&
        opcode != 5 &&
        opcode != 6 &&
        opcode != 8 &&
        opcode != 9 &&
        opcode != 10 &&
        opcode != 11 &&
        opcode != 12 &&
        opcode != 13 &&
        opcode != 15 &&
        opcode != 16 &&
        opcode != 24 &&
        opcode != 34 &&
        opcode != 36 &&
        opcode != 38 &&
        opcode != 39 &&
        opcode != 40 &&
        opcode != 41 &&
        opcode != 44 &&
        opcode != 45 &&
        opcode != 48 &&
        opcode != 50 &&
        opcode != 51 &&
        opcode != 56 &&
        opcode != 58 &&
        opcode != 62 &&
        opcode != 65 &&
        opcode != 66 &&
        opcode != 67 &&
        opcode != 70 &&
        opcode != 71 &&
        opcode != 75 &&
        opcode != 76 &&
        opcode != 79 &&
        opcode != 83 &&
        opcode != 84 &&
        opcode != 85 &&
        opcode != 89) { __asm { int 3 } }
    form = INPUT_TOKEN_FORM(opcode);
    token->opcode = opcode;
    switch (form) {
    case INPUT_FORM_NONE: case INPUT_FORM_EMPTY:
        break;
    case INPUT_FORM_TYPE:
        read_input_type(token);
        break;
    case INPUT_FORM_TYPED_SYMBOL_TRAILER:
        serial = input_varuint();
        token->value.scalar.argument.symbol =
            (C2InputSymbol *)lookup_symbol(function->symbols, serial);
        read_input_type(token);
        c2_read_il_byte();
        c2_read_il_byte();
        break;
    case INPUT_FORM_SYMBOL_TRAILER:
        serial = input_varuint();
        token->value.scalar.argument.symbol =
            (C2InputSymbol *)lookup_symbol(function->symbols, serial);
        c2_read_il_byte();
        break;
    case INPUT_FORM_TYPE_TRAILER:
        read_input_type(token);
        c2_read_il_byte();
        break;
    case INPUT_FORM_TYPED_SYMBOL:
        read_input_type(token);
        /* Fall through to the common serial lookup. */
    case INPUT_FORM_SYMBOL: case INPUT_FORM_SYMBOL_ONLY:
        serial = input_varuint();
        token->value.scalar.argument.symbol =
            (C2InputSymbol *)lookup_symbol(function->symbols, serial);
        break;
    case INPUT_FORM_TYPED_CODE:
        read_input_type(token);
        token->value.scalar.control_byte = (unsigned char)c2_read_il_byte();
        break;
    case INPUT_FORM_CODE:
        token->value.scalar.control_byte = (unsigned char)c2_read_il_byte();
        break;
    case INPUT_FORM_INVALID:
        c2_report_internal("E:\\utc\\src\\\\P2\\getattr.c", 311);
        break;
    case INPUT_FORM_PRAGMA:
        token->value.scalar.control_short = input_compact_short();
        read_control_fields(token);
        break;
    case INPUT_FORM_SIGNED: case INPUT_FORM_SIGNED_ALT:
        token->value.integer = input_compact_signed();
        break;
    case INPUT_FORM_SYMBOL_PAIR:
        read_input_type(token);
        serial = input_varuint();
        scope = function->symbols;
        token->value.scalar.argument.symbol = (C2InputSymbol *)lookup_symbol(scope, serial);
        serial = input_varuint();
        token->value.scalar.second_symbol = lookup_symbol(scope, serial);
        break;
    case INPUT_FORM_SHORT_TRAILER:
        input_compact_short();
        break;
    case INPUT_FORM_UNSIGNED_CONTROL:
        read_input_type(token);
        token->value.scalar.control = input_varuint();
        break;
    case INPUT_FORM_SIGNED_CONTROL:
        read_input_type(token);
        token->value.scalar.control = input_compact_signed();
        break;
    case INPUT_FORM_SIGNED_PAIR:
        token->value.scalar.low = input_compact_signed();
        token->value.scalar.control = input_compact_signed();
        break;
    case INPUT_FORM_SYMBOLS_SHORT:
        serial = input_varuint();
        scope = function->symbols;
        token->value.scalar.low = (unsigned)lookup_symbol(scope, serial);
        serial = input_varuint();
        token->value.scalar.argument.symbol = (C2InputSymbol *)lookup_symbol(scope, serial);
        token->value.scalar.control = (long)input_compact_short();
        break;
    case INPUT_FORM_NUMBER:
        read_input_type(token);
        if ((token->type & 0xf000) != 0x4000 && token->type != 0x400a) {
            if ((token->type & 0xfff) == 8)
                token->value.integer = read_input_longlong();
            else
                token->value.integer = input_compact_signed();
            break;
        }
        if (token->type == 0x400a) {
            temporary = read_input_real();
            real = temporary;
            kind = input_real_kind(&real);
            token->type = kind == 10 ? 0x400a : (unsigned short)kind | 0x4000;
            if (token->type == 0x4004) {
                if (input_real_to_float(&real, &single)) compiler_warning(2, 0x2f4);
                input_real_from_float(&single, &real);
            } else {
                if (input_real_to_double(&real, &number.real)) compiler_warning(2, 0x2f4);
                input_real_from_double(&number.real, &real);
            }
        } else {
            c2_report_internal("E:\\utc\\src\\\\P2\\getattr.c", 209);
            number.bits = read_input_qword();
            input_real_from_double(&number.real, &real);
        }
        storage = c2_pool_allocate(0, sizeof(C2Real));
        token->value.real_pointer = storage;
        *storage = real;
        break;
    default:
        c2_fatal(1, "E:\\utc\\src\\\\P2\\getattr.c", 416);
        break;
    }
    return token;
}

/* Encountered pragmas consume a source line (1), filename (2), signed long
 * (31), or short (32).
 * Other rows and the
 * unused second DWORD of each row are unmodeled, not zero-valued defaults. */
typedef struct C2ControlFields {
    signed char *fields;
    unsigned pending04;
} C2ControlFields;
static struct {
    C2ControlFields pending00, source_line, filename, pending03[28], signed_value, short_value;
} input_control_fields = {{0}, {"l", 0}, {"s", 0}, {{0}}, {"\024", 0}, {"\025", 0}};
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define INPUT_CONTROL_FIELDS(control) \
    ((control) == 1 ? input_control_fields.source_line.fields : \
     (control) == 2 ? input_control_fields.filename.fields : \
     (control) == 31 ? input_control_fields.signed_value.fields : \
     (control) == 32 ? input_control_fields.short_value.fields : \
     (recon_unsupported("unmodeled input control fields"), (signed char *)0))
#else
#define INPUT_CONTROL_FIELDS(control) \
    (((C2ControlFields *)&input_control_fields)[(control)].fields)
#endif
char *input_current_filename;
unsigned input_pragma_value, input_saved_pragma_value;

void __fastcall read_control_fields(C2ILToken *token)
{
    signed char *fields = INPUT_CONTROL_FIELDS((signed char)token->value.scalar.control);
    union {
        char text[0x80c];
        struct { short code, state; } warnings[0x203];
    } buffer;
    unsigned bytes, index;
    short length;
    void *destination;
    if (fields == NULL) return;
    for (; *fields != 0; ++fields) {
        switch (*fields) {
        case 2:
            token->value.integer = (long)(input_last_source_index +
                                         (unsigned short)input_compact_short());
            break;
        case 11:
            read_module_directive((signed char)c2_read_il_byte());
            break;
        case 12:
            length = read_counted_input_text(buffer.text, 0x80a);
            bytes = (int)length + 1;
            destination = c2_pool_allocate(2, bytes);
            token->value.scalar.low = (unsigned)destination;
            buffer.text[length] = 0;
            memcpy(destination, buffer.text, bytes);
            break;
        case 13:
            c2_report_internal("E:\\utc\\src\\\\P2\\p2pragma.c", 142);
            break;
        case 14:
            *(short *)&token->value.scalar.low = input_compact_short();
            break;
        case 20:
            token->value.integer = input_compact_signed();
            break;
        case 22:
            input_pragma_value = input_saved_pragma_value = input_varuint();
            break;
        case 23:
            index = 0;
            bytes = 4;
            buffer.warnings[0].code = input_compact_short();
            while (buffer.warnings[index].code != -1) {
                buffer.warnings[index].state = (signed char)c2_read_il_byte();
                ++index;
                bytes += 4;
                buffer.warnings[index].code = input_compact_short();
            }
            /* Original defect: the terminal pair's state is uninitialized,
             * although all four bytes are copied. Keep that padding defined. */
            buffer.warnings[index].state = 0;
            destination = c2_pool_allocate(1, bytes);
            token->value.scalar.argument.pointer = destination;
            memcpy(destination, buffer.text, bytes);
            break;
        case 24:
            token->value.scalar.low = input_varuint();
            break;
        case 108:
            if (c2_il_extended_header) {
                input_last_source_index = input_compact_signed();
                token->value.integer = (long)input_last_source_index;
                break;
            }
            /* Fall through to the old-format short source line. */
        case 21:
            token->value.integer = (long)input_compact_short();
            break;
        case 115:
            input_current_filename = lookup_input_filename(input_varuint());
            token->value.scalar.argument.text = input_current_filename;
            token->opcode = 86;
            break;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\p2pragma.c", 219);
            break;
        }
    }
}

extern int c2_favor_speed, c2_alias_optimization, c2_float_precision;
extern int c2_frame_optimization, c2_global_optimization, c2_exception_optimization;
extern int extra_function_passes, special_function_passes;
/* These independent DWORDs are written by the per-function option decoder.
 * Active global optimization is retained when an input disables bit21. */
unsigned input_option_high_bit;
int input_no_global_optimization, input_global_optimization_active;

void __fastcall set_function_input_options(unsigned options)
{
    input_option_high_bit = options >> 31;
    c2_favor_speed = (options >> 23) & 1;
    c2_alias_optimization = (options >> 25) & 1;
    c2_float_precision = (options >> 18) & 1;
    c2_frame_optimization = (options >> 11) & 1;
    special_function_passes = (options >> 9) & 1;
    c2_global_optimize = (options >> 21) & 1;
    c2_global_optimization = (options >> 24) & 1;
    extra_function_passes = (options >> 10) & 1;
    if (c2_global_optimize)
        input_global_optimization_active = 1;
    else
        c2_alias_optimization = 0;
    input_no_global_optimization = c2_global_optimization == 0;
    if (c2_global_optimization)
        c2_alias_optimization = 1;
    if (c2_exception_optimization &&
        (((C2InputSymbol *)current_symbol)->additional_flags & 0x100)) {
        c2_global_optimize = 0;
        input_global_optimization_active = 0;
    }
}

void __fastcall push_input_trees(C2InputStack **stack, C2Expression *trees)
{
    C2InputStack *entry = new_input_stack();
    link_code_nodes((C2Record *)entry->last, (C2Record *)trees);
    while (trees->next)
        trees = trees->next;
    entry->last = trees;
    entry->next = *stack;
    *stack = entry;
}

C2InputStack *free_input_stacks;
C2InputStack *new_input_stack(void)
{
    C2InputStack *entry = free_input_stacks;
    C2Expression *sentinel;
    if (entry) {
        free_input_stacks = entry->next;
        sentinel = entry->first;
        memset(entry, 0, sizeof(C2InputStack));
        entry->first = entry->last = sentinel;
    } else {
        entry = c2_pool_allocate(4, sizeof(C2InputStack));
        entry->first = entry->last = (C2Expression *)new_statement_boundary();
    }
    return entry;
}

C2Record *__fastcall append_input_trees(C2InputStack *stack, C2Record *prior)
{
    C2Expression *sentinel;
    if (stack->flags & 0x20) {
        discard_input_tree(stack->last);
        stack->last = last_input_tree(stack->first);
    } else
        stack->last = prune_input_tree(stack->last, 0);
    sentinel = stack->first;
    if (sentinel->next) {
        link_code_nodes(prior, (C2Record *)sentinel->next);
        prior = (C2Record *)stack->last;
        sentinel->next = NULL;
        stack->last = sentinel;
    }
    return prior;
}

C2Expression *__fastcall prune_input_tree(C2Expression *tree, int retain_value)
{
    C2Expression *prior, *operand, *result, *next, *temporary;
    unsigned short type, bytes;
    int effects;
    if (tree == NULL) return NULL;
    if (INPUT_IS_OPERAND(tree)) {
        effects = input_has_side_effects(tree);
        prior = tree->previous;
        if (!effects) {
            unlink_input_record((C2Record *)tree);
            operand = tree;
            if (tree->kind == 6 &&
                (!tree->aggregate_value ||
                 (INPUT_IS_REGISTER(tree->aggregate_value) &&
                  tree->aggregate_value->value.reg->value)) &&
                (!tree->aggregate_second ||
                 (INPUT_IS_REGISTER(tree->aggregate_second) &&
                  tree->aggregate_second->value.reg->value)) && tree->aggregate_value)
                operand = tree->aggregate_value;
            if (INPUT_IS_REGISTER(operand) && operand->value.reg->value) {
                if (operand->value.reg->value == prior)
                    prior = prune_cached_input(operand);
                else
                    prune_cached_input(operand);
            }
            if (tree->kind == 30 || (tree->location.expression.properties & 0x10))
                c2_report_internal("E:\\utc\\src\\\\P2\\sdsu.c", 328);
            release_small_node(tree);
            return prior;
        }
        unlink_input_record((C2Record *)tree);
        type = tree->type;
        if ((type & 0xf000) == 0x5000) {
            bytes = type & 0xfff;
            if (bytes == 0 || bytes >= 3) bytes = 4;
            type = bytes | 0x1000;
            tree->type = type;
            if (tree->kind == 2)
                tree->value.reg = new_address_register(tree->value.reg, type,
                                                       bytes, tree->value.reg->displacement);
        }
        temporary = new_input_temporary(tree->type);
        return new_binary_expression(0x131, tree->type, temporary, tree,
                                     link_code_nodes, (C2Record *)prior);
    }
    if (!(tree->flags & 1)) return tree;
    for (result = tree->tail.right; result; result = next) {
        next = result->next;
        if (INPUT_IS_REGISTER(result) && result->value.reg->value) {
            if (result->kind == 1)
                rewrite_input_reference(tree, result);
            else if (tree->opcode == 0x13e || tree->opcode == 0x13f)
                remove_input_result(tree, result);
        }
    }
    for (result = tree->tail.right; result; result = result->next) {
        if (result->kind != 1 || ((C2Symbol *)result->left)->kind != 1)
            return tree;
    }
    if (!retain_value && input_has_side_effects(tree)) {
        switch (tree->kind) {
        case 11:
            if (!tree->tail.right) {
                result = new_input_temporary(tree->type);
                tree->tail.right = result;
                result->location.expression.properties |= 0x10;
            }
            break;
        case 13: case 14: case 16: case 17: case 19: case 20:
            break;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\sdsu.c", 419);
            break;
        }
        return tree;
    }
    for (operand = tree->left; operand; operand = operand->next) {
        if (INPUT_IS_REGISTER(operand) && operand->value.reg->value)
            prune_cached_input(operand);
    }
    prior = tree->previous;
    unlink_input_record((C2Record *)tree);
    release_small_node(tree);
    return prior;
}

C2Expression *__fastcall pop_input_trees(C2InputStack **stack)
{
    C2InputStack *entry = *stack;
    C2Expression *trees = entry->first->next;
    if (trees) trees->previous = NULL;
    *stack = entry->next;
    if (entry->alternatives)
        release_small_node_list((C2SmallNode *)entry->alternatives);
    release_input_stack(entry);
    return trees;
}

void __fastcall release_input_stack(C2InputStack *stack)
{
    stack->last = stack->first;
    stack->first->next = NULL;
    stack->next = free_input_stacks;
    free_input_stacks = stack;
}

C2Expression *__fastcall variable_input_expression(C2InputSymbol *symbol)
{
    C2FunctionRegister *reg;
    C2Expression *tree;
    switch (symbol->symbol.kind) {
    case 1:
        reg = input_symbol_register(symbol);
        return new_register_expression(INPUT_REGISTER_OPCODE(reg->kind), reg);
    case 3:
        tree = (C2Expression *)allocate_small_node(4);
        tree->opcode = 0x12a;
        tree->type = input_value_type;
        tree->value.symbol = &symbol->symbol;
        return tree;
    case 4:
        if (symbol->attributes & 0x1000) {
            reg = input_symbol_register(symbol);
            return new_register_expression(0x121, reg);
        }
        tree = (C2Expression *)allocate_small_node(4);
        tree->opcode = 0x129;
        tree->type = input_value_type;
        tree->value.symbol = &symbol->symbol;
        return tree;
    default:
        return (C2Expression *)symbol;
    }
}

C2Expression *__fastcall input_symbol_operand(C2InputSymbol *symbol)
{
    C2FunctionRegister *reg = input_symbol_register(symbol);
    return new_typed_register_expression(INPUT_OPERAND_OPCODE(reg->kind), reg->type, reg);
}

C2FunctionRegister *__fastcall input_symbol_register(C2InputSymbol *symbol)
{
    C2FunctionRegister *reg;
    unsigned char kind;
    unsigned selector, bytes, type;

    if (symbol->symbol.input_register) return symbol->symbol.input_register;
    kind = 0;
    switch (symbol->symbol.kind) {
    case 1:
        switch ((symbol->attributes >> 4) & 7) {
        case 0: case 1:
            kind = 4;
            break;
        case 2: case 6:
            kind = (symbol->attributes & 0x780) == 0x200 ? 8 : 7;
            break;
        case 3:
            kind = 5;
            break;
        case 4: case 7:
            selector = (symbol->attributes >> 7) & 15;
            kind = selector == 1 || selector == 2 ? 7 : selector == 4 ? 8 : 9;
            break;
        case 5:
            kind = ((symbol->symbol.flags >> 4) & 2) | 5;
            break;
        default:
            kind = 0;
            break;
        }
        break;
    case 2: kind = 4; break;
    case 3: kind = 11; break;
    case 4: kind = 10; break;
    default: kind = 0; break;
    }
    reg = new_function_register(kind);
    symbol->symbol.input_register = reg;
    reg->symbol = &symbol->symbol;
    if (symbol->symbol.kind == 3) {
        bytes = 4;
        type = 0x1004;
    } else {
        bytes = symbol->symbol.kind == 4 ? symbol->storage.value_bytes :
            (unsigned)symbol->symbol.value;
        type = adjust_input_return_size((unsigned short)(symbol->attributes & 15), bytes);
    }
    reg->kind = kind;
    reg->type = (unsigned short)type;
    reg->value_bytes = bytes;
    reg->canonical = reg;
    return reg;
}

unsigned __fastcall adjust_input_return_size(unsigned short type, unsigned bytes)
{
    switch (type) {
    case 0:
        return bytes & 0xffff0000U;
    case 1:
        return bytes | 0x1000;
    case 2:
        return bytes | 0x2000;
    case 3: case 4: case 11:
        return (bytes & 0xffff0000U) | input_value_type;
    case 5: case 10:
        return bytes == 10 ? (bytes & 0xffff0000U) | 0x400a : bytes | 0x4000;
    case 6:
        return ((int)bytes >= 0x1000 ? 0 : bytes) | 0x5000;
    case 7:
        return 0x1004;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\getattr.c", 533);
        /* The original clears AX after this diagnostic. Its upper return
         * bits depend on the diagnostic's unspecified EAX; callers use AX. */
        return 0;
    }
}

int __fastcall needs_input_return_storage(unsigned convention, unsigned short type, unsigned bytes)
{
#if !defined(__linux__)
    unsigned saved_eax;
    /* The original's convention>24 path returns entry EAX unchanged. Ordinary
     * fastcall C cannot express that incoming register; capture only this ABI
     * value before the compiler uses EAX. No extra argument or helper exists. */
    __asm { mov saved_eax, eax }
#endif
    switch (convention) {
    case 0: case 4: case 8: case 12: case 16: case 20: case 24:
        if ((type >> 12) == 4) return convention == 8;
        if ((type >> 12) != 5) return 0;
        return !bytes || bytes > 8 || (bytes & (bytes - 1)) != 0;
    default:
        if (convention <= 24) return 7;
#if defined(__linux__)
        /* This unvisited domain needs the original incoming-EAX ABI, which
         * the instrumented native C entry does not supply. */
        recon_unsupported("unmodeled return-convention incoming EAX");
#else
        return saved_eax;
#endif
    }
}

void __fastcall read_input_type(C2ILToken *token)
{
    unsigned encoded = input_type();
    unsigned bytes = (encoded >> 4) & 31;
    unsigned short kind = (unsigned short)(encoded & 15);
    if ((encoded & 0x1000) && INPUT_TYPE_PROPERTY(token->opcode))
        token->flags |= 0x800;
    if (kind == 6 && !bytes) bytes = input_compact_signed();
    if (token->opcode == 39) return;
    token->type = (unsigned short)adjust_input_return_size(kind, bytes);
    if ((token->type & 0xf000) == 0x5000) token->bytes = bytes;
}

C2Expression *__fastcall convert_input_operand(C2InputStack *stack, unsigned short type,
                                               unsigned bytes, int flags)
{
    C2Expression *tree, *assignment, *operand, *constant;
    C2FunctionRegister *reg, *canonical;
    if (stack->flags & 4) return convert_deferred_input_operand(stack, type, flags);
    tree = pop_input_expression(stack);
    if (tree->type != 0x3004)
        convert_input_expression(&tree, 0x3004, &stack->last);
    if (tree->location.expression.properties & 0x10)
        tree = copy_input_expression(tree);
    switch (tree->kind) {
    case 3:
        mark_first_input_expression(tree);
        tree->type = type;
        if (tree->kind == 2) {
            reg = tree->value.reg;
            canonical = reg->canonical;
            tree->value.reg = new_address_register(canonical, type, bytes,
                reg->displacement - canonical->displacement);
        }
        break;
    case 4: case 7:
        assignment = new_assignment_expression(0x131, 0x3004, tree,
                                                link_code_nodes, (C2Record *)stack->last);
        tree = new_aggregate_expression(0x122, type, NULL, 0,
                                        assignment->tail.right, NULL, 0);
        stack->last = assignment;
        break;
    case 6:
        assignment = new_assignment_expression(0x131, 0x3004, tree,
                                                link_code_nodes, (C2Record *)stack->last);
        stack->last = assignment;
        tree = assignment->tail.right;
        /* Fall through to the same register-result folding. */
    case 1:
        if (INPUT_IS_REGISTER(tree) && (assignment = tree->value.reg->value) != NULL &&
            assignment->opcode == 0x141 && assignment->left->kind == 3 &&
            assignment->left->next->kind == 7 &&
            (long)assignment->left->next->immediate >= 0) {
            release_small_node(tree);
            operand = assignment->left;
            constant = operand->next;
            reg = new_address_register(operand->value.reg, type, bytes,
                                       (long)constant->immediate);
            tree = new_typed_register_expression(operand->opcode, type, reg);
            if (stack->last == assignment) stack->last = assignment->previous;
            unlink_input_record((C2Record *)assignment);
            release_small_node(assignment);
            break;
        }
        /* Fall through to the ordinary conversion. */
    case 2:
        tree = new_aggregate_expression(0x122, type, NULL, 0, tree, NULL, 0);
        break;
    default:
        break;
    }
    link_code_nodes((C2Record *)stack->last, (C2Record *)tree);
    stack->last = tree;
    if (flags) tree->location.expression.properties |= 0x40;
    return tree;
}

/* Rebuild a value's defining list when it can be reused without effects;
 * otherwise split its current result into a new temporary. */
extern C2Expression *__fastcall collect_region_value_records(C2FunctionRegister *reg,
    C2Expression **first, C2Expression **last);

extern int __fastcall register_views_differ(C2Expression *operand);

int __fastcall aggregate_result_needs_temporary(C2Expression *value)
{
    C2Expression *record, *operand;
    if (value->kind == 6) value = value->aggregate_value;
    record = value->value.reg->value;
    if (input_has_side_effects(record)) return 1;
    for (operand = record->tail.right; operand; operand = operand->next) {
        if (INPUT_IS_REGISTER(operand) && operand->value.reg->value &&
            register_views_differ(operand)) return 1;
    }
    for (operand = record->left; operand; operand = operand->next) {
        if (INPUT_IS_REGISTER(operand) && operand->value.reg->value) {
            if (register_views_differ(operand) ||
                aggregate_result_needs_temporary(operand)) return 1;
        } else if (operand->kind == 2 && (operand->value.reg->flags & 0x20))
            return 1;
    }
    return 0;
}

C2Expression *__fastcall extract_aggregate_result(C2Expression *value, C2Expression *owner,
    C2Expression **first, C2Expression **last)
{
    C2Expression *component = value->kind == 6 ? value->aggregate_value : value;
    C2Expression *result, *output, *temporary;
    unsigned short type;

    if (!input_has_side_effects(value) && !aggregate_result_needs_temporary(value)) {
        result = collect_region_value_records(component->value.reg, first, last);
        if (value->kind == 6)
            result = new_aggregate_expression(value->opcode, value->type,
                NULL, 0, result, NULL, value->immediate);
        return result;
    }
    output = component->value.reg->value->tail.right;
    type = output->type;
    temporary = new_input_temporary(type);
    *first = *last = new_binary_expression(0x131, type, temporary, output, NULL, NULL);
    if (value->kind == 6) {
        result = new_aggregate_expression(value->opcode, value->type,
            NULL, 0, temporary, NULL, value->immediate);
        if (owner) {
            if (is_input_result(owner, value))
                replace_input_result(owner, value, result);
            else
                attach_input_expression(owner, value, result);
            result = copy_input_expression(result);
        }
        return result;
    }
    result = copy_input_expression(temporary);
    result->type = value->type;
    return result;
}

C2Expression *__fastcall pop_input_expression(C2InputStack *stack)
{
    C2InputStack *local = stack;
    C2Expression *tree, *value, *component, *first, *last;
    local->flags &= ~0x20;
    if (local->flags & 4) {
        materialize_input_operand(local, local->last->type);
        return pop_input_expression(local);
    }
    tree = local->last;
    if (INPUT_IS_OPERAND(tree)) {
        value = tree->previous;
        tree->previous = NULL;
        local->last = value;
        local->last->next = NULL;
        return tree;
    }
    if (!(local->flags & 1) && (input_preserve_operand || tree->opcode != 0x151)) {
        value = tree->tail.right;
        if (value->kind == 6 &&
            (!(component = value->aggregate_value) ||
             (INPUT_IS_REGISTER(component) && component->value.reg->value)) &&
            (!(component = value->aggregate_second) ||
             (INPUT_IS_REGISTER(component) && component->value.reg->value))) {
            value = extract_aggregate_result(value, tree, &first, &last);
            if ((value->kind == 2 && value->value.reg->kind == 3 && !value->value.reg->value) ||
                (value->kind == 6 && value->aggregate_value->kind == 2 &&
                 value->aggregate_value->value.reg->kind == 3 &&
                 !value->aggregate_value->value.reg->value)) {
                link_code_nodes((C2Record *)local->last->previous, (C2Record *)first);
                return value;
            }
            link_code_nodes((C2Record *)local->last, (C2Record *)first);
            local->last = last;
            return value;
        }
        if (value->next) {
            value = copy_input_expression(value);
            value->next = NULL;
        }
        return value;
    }
    tree = new_integer_expression(0x1004, 1);
    push_input_trees(&local, tree);
    tree = new_integer_expression(0x1004, 0);
    push_input_trees(&local, tree);
    input_argument_list(&local);
    if (local != stack)
        c2_report_internal("E:\\utc\\src\\\\P2\\reader.c", 4385);
    return pop_input_expression(local);
}

/* Change implicit register kind3 to explicit kind2; all other kinds become
 * aggregate kind6. The provisional historical symbol is kept for trace identity. */
void __fastcall mark_first_input_expression(C2Expression *expression)
{
    expression->kind = expression->kind == 3 ? 2 : 6;
}

C2Expression *__fastcall new_runtime_call(unsigned opcode, unsigned short type,
    C2Expression *result, C2Expression *target, unsigned char convention,
    C2InputLink link, C2Record *prior);
C2Expression *__fastcall new_input_call_expression(unsigned opcode, unsigned short type,
    C2Expression *target, unsigned char convention, C2InputLink link, C2Record *prior);
C2Expression *__fastcall new_input_intrinsic_expression(unsigned opcode, unsigned short type,
    C2Expression *first, C2Expression *second, C2Expression *third,
    C2Expression *fourth, unsigned short intrinsic, C2InputLink link, C2Record *prior);
int __fastcall input_intrinsic_needs_result(unsigned short intrinsic)
{
    return intrinsic == 0xa5 || intrinsic == 0xaa || intrinsic == 0xd8;
}
C2InputStack *__fastcall prepare_intrinsic_argument_stacks(C2InputStack *arguments);
C2Expression *__fastcall lower_input_intrinsic_call(C2Function *function, C2Expression *call);
C2Expression *__fastcall materialize_input_call_result(C2Function *function,
    C2Expression *call, unsigned bytes);
C2Expression *__fastcall finish_input_call_prefix(C2Expression *prefix, C2Expression *call);
/* Move the current trees into the final deferred-before stack. */
void __fastcall finish_input_call_stack(C2InputStack *stack)
{
    C2InputStack **tail = &stack->before, *deferred;
    C2Expression *first = stack->first;
    while (*tail) tail = &(*tail)->next;
    deferred = new_input_stack();
    *tail = deferred;
    link_code_nodes((C2Record *)deferred->first, (C2Record *)first->next);
    first->next = NULL;
    deferred->last = stack->last;
    stack->last = first;
}

/* Promote scalar arguments and materialize aggregate argument values. */
void __fastcall input_call_argument(C2InputStack *stack, C2ILToken *token)
{
    C2Expression *value = pop_input_expression(stack), *temporary, *size;
    C2Expression *first, *second;
    unsigned short source = value->type, type = token->type;
    unsigned width;

    if (type) {
        if (((1U << (type >> 12)) & 14) &&
            (type & 0xfff) < (source & 0xfff) &&
            !((1U << (value->kind & 31)) & 0x180))
            compiler_warning(1, 0x2f9);
    } else {
        type = source;
        width = source & 0xfff;
        if ((1U << (source >> 12)) & 14)
            type = (source & 0xf000) | (width > 4 ? width : 4);
        else if ((source & 0xf000) == 0x4000)
            type = 0x4000 | (width > 8 ? width : 8);
        else if ((source & 0xf000) == 0x3000)
            type = 0x3000 | (width > 4 ? width : 4);
        else if ((source & 0xf000) == 0x5000)
            token->bytes = input_aggregate_bytes;
    }
    if ((type & 0xf000) != 0x5000) {
        convert_input_expression(&value, type, &stack->last);
        if (stack->after) {
            temporary = new_input_temporary(type);
            temporary->value.reg->flags |= 0x40;
            stack->last = new_binary_expression(0x131, type, temporary, value,
                link_code_nodes, (C2Record *)stack->last);
            value = temporary;
        }
        stack->last = new_assignment_expression(0x130, type, value,
            link_code_nodes, (C2Record *)stack->last);
        return;
    }
    input_aggregate_bytes = token->bytes;
    if (stack->after) {
        link_code_nodes((C2Record *)stack->last, (C2Record *)value);
        stack->last = value;
        convert_input_pointer(stack, 0x3004);
        value = pop_input_expression(stack);
        temporary = new_input_temporary(0x3004);
        temporary->value.reg->flags |= 0x40;
        stack->last = new_binary_expression(0x131, 0x3004, temporary, value,
            link_code_nodes, (C2Record *)stack->last);
        convert_input_operand(stack, type, input_aggregate_bytes, 0);
        value = pop_input_expression(stack);
    }
    if ((long)input_aggregate_bytes > 0xfff) {
        link_code_nodes((C2Record *)stack->last, (C2Record *)value);
        stack->last = value;
        convert_input_pointer(stack, 0x3004);
        value = pop_input_expression(stack);
        first = new_assignment_expression(0x130, 0x3004, value,
            link_code_nodes, (C2Record *)stack->last);
        stack->last = first;
        size = new_integer_expression(0x1004, (__int64)(long)input_aggregate_bytes);
        second = new_assignment_expression(0x130, 0x3004, size,
            link_code_nodes, (C2Record *)stack->last);
        stack->last = second;
        stack->last = new_input_intrinsic_expression(0x15f, 0x3004,
            first->tail.right, second->tail.right, NULL, NULL, 0x157,
            link_code_nodes, (C2Record *)stack->last);
        stack->last = new_assignment_expression(0x130, 0x3004,
            stack->last->tail.right, link_code_nodes, (C2Record *)stack->last);
        return;
    }
    temporary = new_input_temporary(type);
    size = new_integer_expression(0x1004, (__int64)(long)input_aggregate_bytes);
    stack->last = new_block_copy_expression(0x13e, type, temporary, value,
        size, link_code_nodes, (C2Record *)stack->last);
    temporary->value.reg->value = stack->last;
}

void __fastcall input_call(C2InputStack **stack, C2ILToken *token, int depth,
    C2Function *function, int arguments)
{
    C2InputStack *local = *stack, *argument_stack;
    C2Expression *prefix = NULL, *target, *first, *second, *hidden = NULL;
    C2Expression *record, *result = NULL, *replacement, *boundary;
    C2InputSymbol *symbol;
    C2FunctionRegister *reg;
    unsigned short requested_type = token->type, type = requested_type, intrinsic = 0;
    unsigned bytes, number;

    if (local->before) {
        record = local->before->last;
        if ((record->flags & 1) && record->opcode == 0x16e &&
            local->before->first->next == record)
            prefix = pop_input_trees(&local->before);
    }
    if (!arguments) target = pop_input_expression(local);
    else if (arguments == 1) {
        input_operand_pair(&local, &target, &second, 0);
        local->last = new_assignment_expression(0x130, 0x3004, second,
            link_code_nodes, (C2Record *)local->last);
        hidden = local->last->tail.right;
    } else {
        first = save_input_expression(local);
        push_input_trees(&local, first);
        convert_input_operand(local, 0x3004, 0, 0);
        input_operand_pair(&local, &second, &target, 0);
        link_code_nodes((C2Record *)local->last, (C2Record *)target);
        local->last = target;
        input_binary_operator(&local, 0x141);
        convert_input_operand(local, 0x3004, 0, 0);
        target = pop_input_expression(local);
        target->location.expression.properties |= 0x80;
        local->last = new_assignment_expression(0x130, 0x3004, second,
            link_code_nodes, (C2Record *)local->last);
        hidden = local->last->tail.right;
    }
    if (token->opcode == 64) {
        intrinsic = (unsigned short)target->immediate;
        release_small_node(target);
        local->last = new_input_intrinsic_expression(0x15f, type,
            NULL, NULL, NULL, NULL, intrinsic, link_code_nodes,
            (C2Record *)local->last);
        if (input_intrinsic_needs_result(intrinsic)) {
            if (intrinsic == 0xde || intrinsic == 0xdf) {
                symbol = new_input_local(function->symbols, 6, type & 0xfff, 1, 1, 4, 0);
                result = input_symbol_operand(symbol);
            } else result = new_input_temporary(type);
            replace_input_result(local->last, local->last->tail.right, result);
        }
        bytes = intrinsic;
    } else {
        if ((type & 0xf000) == 0x5000) bytes = input_aggregate_bytes = token->bytes;
        else bytes = type & 0xfff;
        if (type == 0x6000) {
            local->last = new_runtime_call(0x158, 0x6000, NULL, target,
                token->value.scalar.control_byte, link_code_nodes, (C2Record *)local->last);
        } else if ((type & 0xf000) == 0x5000 &&
            (!bytes || (long)bytes > 8 || (bytes & (bytes - 1)))) type = 0x3004;
        /* The original void-return path also emits this ordinary call after
         * the runtime-call constructor above; preserve both real calls. */
        local->last = new_input_call_expression(0x158, type, target,
            token->value.scalar.control_byte, link_code_nodes, (C2Record *)local->last);
    }
    if (hidden) copy_input_operand(local->last, hidden);
    boundary = (C2Expression *)new_statement_boundary();
    argument_stack = NULL;
    read_function_code(function, &argument_stack, (C2Record *)boundary, depth, 0);
    record = local->last;
    if (record->opcode == 0x15f) {
        number = record->detail.intrinsic;
        if (number >= C2_INTRINSIC_COUNT) { __asm { int 3 } }
        if (intrinsics[number].argument_signature & 0x20)
            argument_stack = prepare_intrinsic_argument_stacks(argument_stack);
    }
    release_small_node(boundary);
    if (argument_stack) {
        copy_input_operand(local->last, argument_stack->last->tail.right);
        while (argument_stack->next) {
            input_operand_pair(&argument_stack, &second, &target, 0);
            copy_input_operand(local->last, second);
        }
        if (argument_stack->after) append_deferred_input(argument_stack);
        argument_stack->next = local->next;
        local->next = argument_stack;
        input_operand_pair(&local, &second, &target, 1);
    }
    if ((requested_type & 0xf000) == 0x5000) input_aggregate_bytes = bytes;
    record = local->last;
    if (record->opcode == 0x15f)
        local->last = lower_input_intrinsic_call(function, record);
    else if (needs_input_return_storage(token->value.scalar.control_byte, requested_type, bytes))
        local->last = result = materialize_input_call_result(function, record, bytes);
    if (prefix) {
        replacement = finish_input_call_prefix(prefix, local->last);
        if (replacement) link_code_nodes((C2Record *)local->last->previous, (C2Record *)replacement);
    }
    if (input_intrinsic_needs_result(intrinsic)) {
        finish_input_call_stack(local);
        if (!result) result = new_input_temporary(0x1004);
        reg = result->kind == 6 ? result->aggregate_value->value.reg : result->value.reg;
        reg->flags |= 0x60;
        replacement = copy_input_expression(result);
        link_code_nodes((C2Record *)local->first, (C2Record *)replacement);
        local->last = local->first->next;
    }
    *stack = local;
}

C2Expression *__fastcall save_input_expression(C2InputStack *stack)
{
    C2Expression *tree = stack->last, *value = tree, *owner = NULL;
    C2Expression *component, *first, *last, *temporary;
    int materialize;

    if (!INPUT_IS_OPERAND(tree)) {
        owner = tree;
        value = tree->tail.right;
    }
    component = value->kind == 6 ? value->aggregate_value : value;
    if (INPUT_IS_REGISTER(component) && component->value.reg->value) {
        value = extract_aggregate_result(value, owner, &first, &last);
        link_code_nodes((C2Record *)last, (C2Record *)value);
        last = value;
        component = value->kind == 6 ? value->aggregate_value : value;
        if (component->value.reg->kind != 3 || component->value.reg->value)
            return first;
        if (INPUT_IS_OPERAND(tree)) {
            stack->last = tree->previous;
            unlink_input_record((C2Record *)tree);
            if (!(tree->location.expression.properties & 0x10)) release_small_node(tree);
        }
        link_code_nodes((C2Record *)stack->last, (C2Record *)first);
        stack->last = last;
        return copy_input_expression(value);
    }
    materialize = component->kind == 2 && (component->value.reg->view_flags & 0x20);
    if (!materialize) materialize = input_has_side_effects(value);
    if (materialize) {
        if (INPUT_IS_OPERAND(tree)) {
            stack->last = tree->previous;
            unlink_input_record((C2Record *)tree);
        } else tree = NULL;
        temporary = new_input_temporary(value->type);
        stack->last = new_binary_expression(0x131, value->type, temporary, value,
            link_code_nodes, (C2Record *)stack->last);
        if (tree && !(tree->location.expression.properties & 0x10)) release_small_node(tree);
        value = temporary;
    }
    return copy_input_expression(value);
}

void __fastcall input_assignment_operator(C2InputStack **stack, unsigned opcode, C2ILToken *token)
{
    C2InputStack *saved = *stack, *local;
    C2Expression *left, *right, *copy;
    unsigned short type = token->type;

    *stack = saved->next;
    copy = save_input_expression(*stack);
    push_input_trees(stack, copy);
    local = *stack;
    if (local->next->flags & 4) {
        local->flags = local->next->flags;
        if (local->next->alternatives)
            local->alternatives = copy_input_expression_list(local->next->alternatives->next);
    }
    swap_input_stacks(stack);
    saved->next = *stack;
    *stack = saved;
    swap_input_stacks(stack);
    convert_input_operand(*stack, type, token->bytes, token->flags & 0x800);
    if ((type & 0xff00) == 0x8000) input_aggregate_bytes = token->bytes;
    left = pop_input_expression(*stack);
    swap_input_stacks(stack);
    right = pop_input_expression(*stack);
    if (opcode == 0x149 || opcode == 0x14a || (right->type & 0xf000) == 0x4000) {
        type = common_input_value_type(left->type, right->type);
        convert_input_expression(&left, type, &(*stack)->next->last);
    } else type = left->type;
    convert_input_expression(&right, type, &(*stack)->last);
    local = (*stack)->next;
    link_code_nodes((C2Record *)local->last, (C2Record *)left);
    local->last = left;
    link_code_nodes((C2Record *)(*stack)->last, (C2Record *)right);
    (*stack)->last = right;
    input_binary_operator(stack, opcode);
    input_store_operator(stack, token);
}

void __fastcall input_binary_operator(C2InputStack **stack, unsigned opcode)
{
    C2Expression *left, *right, *record, *constant, *result;
    C2Expression **prior;
    unsigned short type;
    int pointer_pair = 0;
    unsigned char shift;
    unsigned __int64 mask;

    input_operand_pair(stack, &left, &right, 0);
    if (opcode == 0x14b || opcode == 0x145) {
        type = left->type;
        prior = &(*stack)->last;
        if ((right->type & 0xfff) < 5)
            convert_input_expression(&right, type, prior);
        else
            convert_input_expression(&right, 0x1004, prior);
        convert_input_expression(&left, type, prior);
    } else {
        type = common_input_value_type(left->type, right->type);
        pointer_pair = (left->type & 0xf000) == 0x3000 &&
                       (right->type & 0xf000) == 0x3000;
        if (opcode == 0x151 && ((type & 0xf000) == 0x3000 || pointer_pair))
            type = (type & 0xfff) | 0x2000;
        prior = &(*stack)->last;
        convert_input_expression(&left, type, prior);
        convert_input_expression(&right, type, prior);
        if ((right->type ^ left->type) & 0xfff)
            c2_report_internal("E:\\utc\\src\\\\P2\\reader.c", 1787);
    }
    if (left->kind == 7 && !input_address_mode &&
        (opcode == 0x141 || opcode == 0x143 || opcode == 0x146 ||
         opcode == 0x147 || opcode == 0x148)) {
        result = right;
        right = left;
        left = result;
    }
    if (right->kind != 7 || input_address_mode) goto build;
    switch (opcode) {
    case 0x141:
        while (INPUT_IS_REGISTER(left) && (record = left->value.reg->value) != NULL &&
               record->opcode == 0x141 && record->left->next->kind == 7) {
            if (!(left->location.expression.properties & 0x10)) release_small_node(left);
            left = copy_input_expression(record->left);
            constant = record->left->next;
            right->integer.value += constant->integer.value;
            if (left->type != type) {
                left->type = type;
                right->integer.value = normalize_integer_value(type, right->integer.value);
            }
            if (*prior == record) *prior = record->previous;
            unlink_input_record((C2Record *)record);
            release_small_node(record);
        }
        /* Fall through to the zero identity shared with subtraction/shifts/xor. */
    case 0x142: case 0x145: case 0x148: case 0x14b:
        if (!right->integer.value) goto keep_left;
        break;
    case 0x143:
        if (!right->integer.value) goto keep_right;
        /* Fall through to the one identity. */
    case 0x149:
        if (right->integer.value == 1) goto keep_left;
        break;
    case 0x146:
        if (!right->integer.value) goto keep_right;
        shift = (unsigned char)((8 - (right->type & 0xfff)) * 8);
        mask = shift >= 64 ? 0 : ~(unsigned __int64)0 >> shift;
        if ((right->integer.value & mask) == mask) goto keep_left;
        break;
    case 0x147:
        if (!right->integer.value) goto keep_left;
        shift = (unsigned char)((8 - (right->type & 0xfff)) * 8);
        mask = shift >= 64 ? 0 : ~(unsigned __int64)0 >> shift;
        if ((right->integer.value & mask) == mask) goto keep_right;
        break;
    default:
        break;
    }
    goto build;
keep_left:
    if (!(right->location.expression.properties & 0x10)) release_small_node(right);
    if (left->location.expression.properties & 0x10) left = copy_input_expression(left);
    link_code_nodes((C2Record *)*prior, (C2Record *)left);
    *prior = left;
    return;
keep_right:
    if (!(left->location.expression.properties & 0x10)) release_small_node(left);
    if (right->location.expression.properties & 0x10) right = copy_input_expression(right);
    *prior = (C2Expression *)prune_input_tree((C2Record *)*prior, 0);
    link_code_nodes((C2Record *)*prior, (C2Record *)right);
    *prior = right;
    return;
build:
    if (opcode != 0x151) {
        result = new_folded_binary_expression(opcode, type, left, right,
                                               link_code_nodes, (C2Record *)*prior);
        *prior = result;
        return;
    }
    result = new_comparison_expression(0x151, type, left, right,
                                       link_code_nodes, (C2Record *)*prior);
    prepare_input_comparison(result->left, result->left->next, pointer_pair);
    *prior = result;
}

void __fastcall set_input_operand_type(C2InputStack *stack, unsigned short type)
{
    C2Expression *value = pop_input_expression(stack);
    if (!convert_input_expression(&value, type, &stack->last) &&
        !(value->location.expression.properties & 0x10)) {
        link_code_nodes((C2Record *)stack->last, (C2Record *)value);
        stack->last = value;
    }
}

unsigned char __fastcall input_comparison(C2InputStack **stack)
{
    C2InputStack *local = *stack;
    C2Expression *value = local->last, *zero, *output;
    if (value->flags & 1) {
        if (value->opcode == 0x131) {
            output = value->tail.right;
            if (output->kind != 1 || output->value.reg->kind != 3 ||
                !output->value.reg->value)
                compiler_warning(4, 0x2c2);
        }
        value = value->tail.right;
    }
    if (value && INPUT_IS_OPERAND(value) && value->type == 0x8000)
        return value->location.expression.comparison >> 4;
    value = pop_input_expression(local);
    if ((value->type & 0xf000) == 0x4000)
        zero = new_real_expression(value->type, &c2_real_zero);
    else zero = new_integer_expression(0x1004, 0);
    link_code_nodes((C2Record *)local->last, (C2Record *)value);
    local->last = value;
    push_input_trees(stack, zero);
    input_binary_operator(stack, 0x151);
    output = (*stack)->last->tail.right;
    output->location.expression.comparison =
        (output->location.expression.comparison & 15) | 0x10;
    return 1;
}

void __fastcall adjust_input_operand(C2Expression *operand)
{
    C2Expression *record, *first, *constant;
    C2FunctionRegister *reg;
    unsigned properties;
    for (;;) {
        if (operand->flags & 1) operand = operand->tail.right;
        if (!INPUT_IS_OPERAND(operand) || !INPUT_IS_REGISTER(operand)) return;
        reg = operand->value.reg;
        record = reg->value;
        if (!record) return;
        if (record->opcode == 0x141 && ((properties = reg->properties) & 1)) {
            constant = new_integer_expression(0x1004, 0);
            attach_input_expression(record, record->left->next, constant);
            reg->properties = properties & ~1U;
        }
        if (record->opcode < 0x141 || record->opcode > 0x142) return;
        first = record->left;
        adjust_input_operand(first);
        operand = first->next;
    }
}

void __fastcall prepare_input_comparison(C2Expression *left, C2Expression *right, int pointer_pair)
{
    unsigned category;
    if (right->kind == 7) adjust_input_operand(left);
    if (left->kind == 7) adjust_input_operand(right);
    if (pointer_pair) return;
    category = right->type & 0xf000;
    if (category == 0x1000 || category == 0x2000) adjust_input_operand(left);
    category = left->type & 0xf000;
    if (category == 0x1000 || category == 0x2000) adjust_input_operand(right);
}

/* Preserve the two operands' deferred stacks while removing their tree lists. */
void __fastcall input_operand_pair(C2InputStack **stack, C2Expression **left,
                                  C2Expression **right, int condition)
{
    C2InputStack *local, *before, *after, *tail, *saved_after;
    C2Expression *last, *trees;

    if (!condition) *right = pop_input_expression(*stack);
    local = *stack;
    last = local->last;
    before = local->before;
    local->before = NULL;
    after = local->after;
    local->after = NULL;
    trees = pop_input_trees(stack);
    if (!condition) *left = pop_input_expression(*stack);
    local = *stack;
    if (!local->before) local->before = before;
    else if (before) {
        tail = local->before;
        while (tail->next) tail = tail->next;
        tail->next = before;
    }
    if (after) {
        saved_after = local->after;
        local->after = after;
        if (saved_after) {
            while (after->next) after = after->next;
            after->next = saved_after;
        }
    }
    if (trees) {
        link_code_nodes((C2Record *)local->last, (C2Record *)trees);
        local->last = last;
    }
}

unsigned short __fastcall common_input_value_type(unsigned short first, unsigned short second)
{
    unsigned first_class = first & 0xf000, second_class = second & 0xf000;
    if (first_class == 0x3000)
        return second_class == 0x3000 ? 0x1004 : first;
    if (second_class == 0x3000) return second;
    if (first == second) return first;
    if (first_class == 0x4000) {
        if (second_class != 0x4000) return first;
        return (first & 0xfff) >= (second & 0xfff) ? first : second;
    }
    if (second_class == 0x4000) return second;
    if ((first & 0xfff) > (second & 0xfff)) return first;
    if ((first & 0xfff) < (second & 0xfff)) return second;
    return first_class == 0x2000 ? first : second;
}

/* Convert an operand, retaining the first emitted conversion as the result. */
C2Expression *__fastcall convert_input_expression(C2Expression **expression,
    unsigned short type, C2Expression **prior)
{
    C2Expression *tree = *expression, *first = NULL, *record, *emitted;
    C2Expression *old_prior, *previous, *copy;
    C2FunctionRegister *reg;
    unsigned source_bytes, bytes;
    unsigned short source_type;

    if (((1U << (tree->kind & 31)) & 0x180) &&
        (first = fold_input_constant(tree, type, NULL))) {
        *expression = first;
        return NULL;
    }
    source_type = tree->type;
    if (type == source_type) return NULL;
    if ((type & 0xf000) == 0x4000) {
        if ((source_type & 0xf000) == 0x4000) {
            if (!((type ^ source_type) & 0xfff)) return first;
            record = new_assignment_expression(0x135, type, tree, link_code_nodes,
                                               (C2Record *)*prior);
            *prior = record;
            *expression = record->tail.right;
            return record;
        }
        if ((source_type & 0xfff) < 4) {
            first = new_assignment_expression(0x135, 0x1004, tree, link_code_nodes,
                                              (C2Record *)*prior);
            *prior = first;
            *expression = first->tail.right;
        }
        record = new_assignment_expression(0x135, type, *expression, link_code_nodes,
                                           (C2Record *)*prior);
        *prior = record;
        *expression = record->tail.right;
        return first ? first : record;
    }
    if ((source_type & 0xf000) == 0x4000) {
        first = new_assignment_expression(0x135, type, tree, link_code_nodes,
                                          (C2Record *)*prior);
        *prior = first;
        if (tree->kind == 1 && tree->value.reg->kind == 3 &&
            (record = tree->value.reg->value) && record->opcode == 0x138) {
            attach_input_expression(first, first->left, record->left);
            unlink_input_record((C2Record *)record);
            release_small_node(record);
        }
        *expression = first->tail.right;
        return first;
    }
    if (tree->kind == 1 && tree->value.reg->kind == 3 &&
        (record = tree->value.reg->value) && record->opcode == 0x135) {
        bytes = type & 0xfff;
        source_bytes = source_type & 0xfff;
        if (bytes <= source_bytes && bytes <= (record->left->type & 0xfff) &&
            ((1U << (record->left->type >> 12)) & 0xe) &&
            (record->left->type & 0xfff) <= 4) {
            old_prior = *prior;
            previous = old_prior->previous;
            if (!(tree->location.expression.properties & 0x10))
                release_small_node(tree);
            copy = copy_input_expression(record->left);
            *expression = copy;
            copy->location.expression.properties &= ~0x10;
            unlink_input_record((C2Record *)record);
            if (old_prior == record) *prior = previous;
            release_function_register(record->tail.right->value.reg->canonical);
            release_small_node(record);
            goto convert_width;
        }
        if ((record->left->type & 0xfff) <= source_bytes &&
            ((source_type & 0xf000) != 0x2000 ||
             (record->left->type & 0xf000) != 0x1000)) {
            if (!(tree->location.expression.properties & 0x10)) {
                release_small_node(tree);
                *expression = record->tail.right;
            }
            tree = *expression;
            release_function_register(tree->value.reg);
            reg = new_temporary_register(type);
            tree->value.reg = reg;
            bind_input_temporary(reg, record);
            tree->type = type;
            record->type = type;
            record->tail.right->type = type;
            return first;
        }
    }
convert_width:
    tree = *expression;
    bytes = type & 0xfff;
    source_bytes = tree->type & 0xfff;
    if (source_bytes < bytes || (bytes < source_bytes && source_bytes > 4)) {
        if (bytes > 4 && source_bytes < 4)
            first = convert_input_expression(expression, 0x1004, prior);
        emitted = new_assignment_expression(0x135, type, *expression, link_code_nodes,
                                            (C2Record *)*prior);
        *prior = emitted;
        *expression = emitted->tail.right;
        if (!first) first = emitted;
    } else {
        if (tree->location.expression.properties & 0x10)
            *expression = copy_input_expression(tree);
        tree = *expression;
        tree->type = type;
        if ((1U << (tree->kind & 31)) & 6)
            tree->value.reg = new_address_register(tree->value.reg, type, bytes, 0);
    }
    return first;
}

/* Fold conversions of literal integer and extended-real input records. */
C2Expression *__fastcall fold_input_constant(C2Expression *expression,
    unsigned short type, C2Expression *owner)
{
    unsigned short source_type = expression->type;
    unsigned target_class = type >> 12, source_class = source_type >> 12;
    C2Expression *replacement;
    C2Real *real;
    union { int integer; unsigned unsigned_integer; float single; } value;
    unsigned __int64 old_value;
    int status;

    if (((1U << target_class) & 0xe) || target_class == 3) {
        if (((1U << source_class) & 0xe) || source_class == 3) {
            old_value = expression->integer.value;
            expression->integer.value = normalize_integer_value(type, old_value);
            if (old_value != expression->integer.value)
                expression->location.expression.comparison |= 0x40;
        } else {
            if ((type & 0xfff) == 8) return NULL;
            if (target_class == 1)
                status = c2_real_to_int(expression->value.real, &value.integer);
            else
                status = c2_real_to_unsigned(expression->value.real, &value.unsigned_integer);
            if (status) compiler_warning(2, 0x2f4);
            /* The original sign-extends this DWORD even for unsigned targets. */
            replacement = new_integer_expression(type, (__int64)value.integer);
            if (owner) attach_input_expression(owner, expression, replacement);
            else release_small_node(expression);
            expression = replacement;
        }
    } else if ((1U << source_class) & 0xe) {
        if ((source_type & 0xfff) == 8) return NULL;
        real = (C2Real *)c2_pool_allocate(0, 12);
        value.unsigned_integer = expression->integer.low;
        if (source_class == 1) c2_real_from_int(&value.integer, real);
        else c2_real_from_unsigned(&value.unsigned_integer, real);
        replacement = new_real_expression(type, real);
        if (owner) attach_input_expression(owner, expression, replacement);
        else release_small_node(expression);
        expression = replacement;
    } else if (c2_float_precision) {
        if (type == source_type) return expression;
        if (type == 0x4004) {
            real = expression->value.real;
            if (input_real_to_float(real, &value.single)) compiler_warning(2, 0x2f4);
            input_real_from_float(&value.single, real);
        }
    }
    expression->type = type;
    return expression;
}

/* Typed store: normalize the two stacks, then emit scalar or block assignment. */
void __fastcall input_store_operator(C2InputStack **stack, C2ILToken *token)
{
    unsigned short type = token->type;
    unsigned category = type & 0xf000;
    C2InputStack *local;
    C2Expression *value, *temporary, *size, *left, *right;
    C2Expression **prior;
    if (category == 0x5000) {
        input_aggregate_bytes = token->bytes;
        if ((long)input_aggregate_bytes > 0xfff) {
            input_aggregate_return(stack, token);
            return;
        }
    }
    local = *stack;
    value = pop_input_expression(local);
    if (value->kind == 6 && category == 0x5000) {
        if (!input_aggregate_bytes || (long)input_aggregate_bytes > 8 ||
            (input_aggregate_bytes & (input_aggregate_bytes - 1)))
            temporary = new_input_temporary(value->type);
        else
            temporary = new_temporary_operand(value->type);
        size = new_integer_expression(0x1004, (__int64)(long)input_aggregate_bytes);
        value = new_block_copy_expression(0x13f, value->type, temporary, value,
                                          size, link_code_nodes, (C2Record *)local->last);
        value->tail.right->value.reg->value = value;
    } else {
        if (value->location.expression.properties & 0x10)
            value = copy_input_expression(value);
        link_code_nodes((C2Record *)local->last, (C2Record *)value);
    }
    local->last = value;
    swap_input_stacks(stack);
    if ((*stack)->flags & 4) {
        input_deferred_store(stack, token);
        return;
    }
    convert_input_operand(*stack, type, token->bytes, token->flags & 0x800);
    input_operand_pair(stack, &left, &right, 0);
    if (category == 0x5000) {
        size = new_integer_expression(0x1004, (__int64)(long)input_aggregate_bytes);
        prior = &(*stack)->last;
        value = new_block_copy_expression(0x13f, right->type, right, left,
                                          size, link_code_nodes, (C2Record *)*prior);
    } else {
        prior = &(*stack)->last;
        convert_input_expression(&left, type, prior);
        value = new_binary_expression(0x131, right->type, right, left,
                                      link_code_nodes, (C2Record *)*prior);
    }
    *prior = value;
    if (right->kind != 2 && !((1U << (right->kind & 31)) & 0x180) &&
        ((left->kind == 2 && !left->value.reg->value &&
          !(left->value.reg->flags & 0x20) &&
          !(left->location.expression.properties & 0x40)) ||
         ((1U << (left->kind & 31)) & 0x180))) {
        left = copy_input_expression(left);
        link_code_nodes((C2Record *)*prior, (C2Record *)left);
        *prior = left;
    }
}

void __fastcall swap_input_stacks(C2InputStack **stack)
{
    C2InputStack *first = *stack;
    *stack = first->next;
    first->next = (*stack)->next;
    (*stack)->next = first;
}

void __fastcall finish_input_expression(C2InputStack *stack)
{
    unsigned char flags = stack->flags;
    C2Expression *tail, *block, *assignment, *replacement, *operand, *source, *record;
    C2FunctionRegister *reg;
    C2InputStack *deferred;
    unsigned short type;
    if (flags & 1) {
        link_code_nodes((C2Record *)stack->last, (C2Record *)stack->alternatives);
        stack->last = last_input_tree(stack->alternatives);
        stack->alternatives = NULL;
        stack->flags = flags & ~1;
    }
    tail = stack->last;
    if (INPUT_IS_REGISTER(tail) && tail->value.reg->kind == 3 &&
        (block = tail->previous) && block->opcode == 0x13f &&
        block->tail.right->kind == 6 &&
        block->tail.right->aggregate_value->value.reg == tail->value.reg &&
        (assignment = block->previous)->opcode == 0x131 &&
        ((1U << (assignment->tail.right->kind & 31)) & 0x1e) &&
        assignment->tail.right->value.reg == tail->value.reg) {
        replacement = copy_input_expression(assignment->left);
        assignment = assignment->previous;
        assignment->next = NULL;
        link_code_nodes((C2Record *)assignment, (C2Record *)replacement);
        type = block->type;
        stack->last = replacement;
        operand = convert_input_operand(stack, type, type & 0xfff, 0);
        replacement = copy_input_expression(operand);
        replace_input_result(block, block->tail.right, replacement);
        release_small_node(block->previous);
        release_small_node(block->next);
        replacement = operand->previous;
        block->next = NULL;
        link_code_nodes((C2Record *)replacement, (C2Record *)block);
        block->next = NULL;
        stack->last = block;
        release_small_node(operand);
    }
    deferred = stack->before;
    if (!deferred) return;
    while (deferred->next) deferred = deferred->next;
    tail = stack->last;
    if (stack->first->next != tail || !INPUT_IS_OPERAND(tail)) return;
    if (tail->kind == 2) reg = tail->value.reg;
    else if (tail->kind == 6 && tail->aggregate_value->kind == 2)
        reg = tail->aggregate_value->value.reg;
    else reg = NULL;
    if (!reg || reg->kind != 3 || reg->value) return;
    source = deferred->last;
    record = INPUT_IS_OPERAND(source) ? source->previous : source;
    if ((record->kind == 13 || record->kind == 17) &&
        record->tail.right && record->tail.right->value.reg == reg) {
        remove_input_result(record, record->tail.right);
        if (record != source) {
            unlink_input_record((C2Record *)source);
            release_small_node(source);
            deferred->last = record;
        }
        tail = stack->last;
        unlink_input_record((C2Record *)tail);
        release_small_node(tail);
        stack->last = stack->first;
        finish_input_temporaries(stack);
    }
}

C2Expression *__fastcall find_aggregate_operand_owner(C2Expression *owner,
    C2Expression *operand, C2Expression **first);

C2Expression *__fastcall find_aggregate_operand_owner(C2Expression *owner,
    C2Expression *operand, C2Expression **first)
{
    C2Expression *head, *scan;
    head = owner->left;
    for (scan = head; scan; scan = scan->next)
        if (((1U << (scan->kind & 31)) & 0x60) &&
            (scan->aggregate_value == operand || scan->aggregate_second == operand)) {
            *first = head;
            return scan;
        }
    head = owner->tail.right;
    for (scan = head; scan; scan = scan->next)
        if (((1U << (scan->kind & 31)) & 0x60) &&
            (scan->aggregate_value == operand || scan->aggregate_second == operand)) {
            *first = head;
            return scan;
        }
    return NULL;
}

void __fastcall replace_aggregate_operand(C2Expression *owner,
    C2Expression *old, C2Expression *replacement)
{
    C2Expression *aggregate, *first;
    aggregate = find_aggregate_operand_owner(owner, old, &first);
    if (aggregate->aggregate_value == old)
        aggregate->aggregate_value = replacement;
    else
        aggregate->aggregate_second = replacement;
    old->location.expression.properties &= ~0x20;
    replacement->location.expression.properties |= 0x20;
}

void __fastcall release_result_aggregate_operands(C2Expression *owner,
    C2Expression *result)
{
    C2Expression *operand, *scan, *previous;
    unsigned component;
    for (component = 0; component != 2; ++component) {
        operand = component ? result->aggregate_second : result->aggregate_value;
        if (!operand) continue;
        operand->location.expression.properties &= ~0x20;
        if (!(result->location.expression.comparison & 0x20)) {
            previous = NULL;
            for (scan = owner->left; scan && scan != operand; scan = scan->next)
                previous = scan;
            if (previous) previous->next = operand->next;
            else owner->left = operand->next;
            if ((1U << (operand->kind & 31)) & 0x60)
                release_result_aggregate_operands(owner, operand);
        }
        release_small_node(operand);
        if (component) result->aggregate_second = NULL;
        else result->aggregate_value = NULL;
    }
}

C2Expression *__fastcall attach_input_expression(C2Expression *owner,
    C2Expression *prior, C2Expression *expression)
{
    C2Expression *scan, *previous;

    if ((1U << (expression->kind & 31)) & 0x7fe) {
        if (expression->location.expression.properties & 0x10)
            expression = copy_input_expression(expression);
        expression->location.expression.properties |= 0x10;
    }
    expression->next = prior->next;
    prior->next = expression;
    if (((1U << (prior->kind & 31)) & 0x7fe) &&
        (prior->location.expression.properties & 0x20))
        replace_aggregate_operand(owner, prior, expression);
    previous = NULL;
    for (scan = owner->left; scan && scan != prior; scan = scan->next)
        previous = scan;
    if (previous) previous->next = prior->next;
    else owner->left = prior->next;
    if ((1U << (prior->kind & 31)) & 0x60)
        release_result_aggregate_operands(owner, prior);
    release_small_node(prior);
    if (((1U << (expression->kind & 31)) & 0x7fe) &&
        ((1U << (expression->kind & 31)) & 0x60) &&
        !(expression->location.expression.comparison & 0x20)) {
        if (expression->aggregate_value) {
            expression->aggregate_value->location.expression.properties &= ~0x10;
            copy_input_operand(owner, expression->aggregate_value);
        }
        if (expression->aggregate_second) {
            expression->aggregate_second->location.expression.properties &= ~0x10;
            copy_input_operand(owner, expression->aggregate_second);
        }
    }
    return expression;
}

C2Expression *__fastcall set_input_expression_type(C2Expression *expression,
    unsigned short type)
{
    if (expression->kind > 0 && expression->kind <= 2)
        expression->value.reg = new_address_register(expression->value.reg, type, 0, 0);
    else if (expression->kind == 7)
        expression->integer.value = normalize_integer_value(type, expression->integer.value);
    expression->type = type;
    return expression;
}

int __fastcall input_operand_has_effects(C2Expression *operand)
{
    switch (operand->kind) {
    case 1: case 2: case 3: case 4:
        return (operand->location.expression.properties >> 6) & 1;
    case 5: case 6:
        return 1;
    case 7: case 8: case 9: case 10:
        return 0;
    default:
        /* The original default carries its normalized kind value in EAX. */
        return (unsigned)operand->kind - 1;
    }
}
int __fastcall input_tree_has_effects(C2Expression *tree);
int __fastcall input_record_has_effects(C2Expression *record)
{
    C2Expression *value;
    switch (record->kind) {
    case 11: case 15:
        if (record->opcode == 0x131 || record->opcode == 0x149) return 1;
        for (value = record->tail.right; value; value = value->next)
            if (input_operand_has_effects(value)) return 1;
        for (value = record->left; value; value = value->next)
            if (input_tree_has_effects(value)) return 1;
        return 0;
    case 13: case 14: case 16: case 17: case 18: case 19: case 20:
        return 1;
    default:
        return 0;
    }
}
int __fastcall input_tree_has_effects(C2Expression *tree)
{
    if (!tree) return 0;
    if (INPUT_IS_OPERAND(tree)) return input_operand_has_effects(tree) != 0;
    do {
        if (input_record_has_effects(tree)) return 1;
        tree = tree->previous;
    } while (tree);
    return 0;
}
C2InputSymbol *new_flow_label(void);
C2InputSymbol *__fastcall new_input_flow_label(C2Function *function)
{
    C2ScopeHash *saved;
    C2InputSymbol *label;
    if (!retain_function_input_pool) {
        if (local_symbol_table)
            c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 0x692);
        local_symbol_table = function->symbols;
        label = new_flow_label();
        local_symbol_table = NULL;
    } else {
        saved = local_symbol_table;
        local_symbol_table = function->symbols;
        label = new_flow_label();
        local_symbol_table = saved;
    }
    return label;
}
int __fastcall evaluate_comparison_rewrite(unsigned short type, C2Expression *result,
    C2Expression *left, C2Expression *right, unsigned condition, int mode);
C2Expression *__fastcall new_select_expression(unsigned opcode, unsigned short type,
    C2Expression *condition, C2Expression *left, C2Expression *right,
    unsigned char comparison, C2InputLink link, C2Record *prior);

/* Join the true/false value stacks, using a select when both arms are pure,
 * otherwise retaining the original conditional control-flow lists. */
void __fastcall input_argument_list(C2InputStack **stack)
{
    C2InputStack *local, *second_stack;
    C2Expression *first, *second, *condition_value, *labels = NULL;
    C2Expression *temporary, *size, *tree, *list, *end_label, *arm;
    unsigned short type;
    unsigned char condition;
    int effects = 0, indirect;

    if (((*stack)->next->next->flags & 1) &&
        ((*stack)->next->next->flags & 2))
        swap_input_stacks(stack);
    local = *stack;
    if ((local->last->flags & 1) && local->last->type == 0x6000)
        first = NULL;
    else first = pop_input_expression(local);
    if (local->before || local->after || input_tree_has_effects(first) ||
        input_tree_has_effects(local->last)) effects = 1;
    second_stack = local->next;
    if ((second_stack->last->flags & 1) && second_stack->last->type == 0x6000)
        second = NULL;
    else second = pop_input_expression(second_stack);
    if (second_stack->before || second_stack->after || input_tree_has_effects(second) ||
        input_tree_has_effects(second_stack->last)) effects = 1;
    local = second_stack->next;
    if (!(local->flags & 1)) {
        condition = input_comparison(&local);
        second_stack->next = local;
        input_preserve_operand = 1;
        condition_value = pop_input_expression(local);
        input_preserve_operand = 0;
        if (local->after) {
            effects = 1;
            append_deferred_input(local);
        }
    } else {
        labels = local->alternatives;
        local->alternatives = NULL;
        condition_value = NULL;
        if (local->after)
            c2_report_internal("E:\\utc\\src\\\\P2\\reader.c", 0xb3e);
    }
    /* The original expects at least one value arm here. */
    if (!second) type = first->type;
    else if (!first) type = second->type;
    else {
        type = common_input_value_type(second->type, first->type);
        convert_input_expression(&first, type, &(*stack)->last);
        swap_input_stacks(stack);
        convert_input_expression(&second, type, &(*stack)->last);
        swap_input_stacks(stack);
    }
    if (!effects && condition_value &&
        evaluate_comparison_rewrite(type, condition_value, second, first, condition, 0)) {
        tree = pop_input_trees(stack);
        if (!tree) tree = pop_input_trees(stack);
        else if ((*stack)->first == (*stack)->last)
            pop_input_trees(stack);
        else {
            link_code_nodes((C2Record *)(*stack)->last, (C2Record *)tree);
            tree = pop_input_trees(stack);
        }
        local = *stack;
        if (tree) link_code_nodes((C2Record *)local->last, (C2Record *)tree);
        if (condition > 16) { __asm { int 3 } }
        local->last = new_select_expression(0x15e, type, condition_value, second, first,
            input_inverse_condition[condition], link_code_nodes, (C2Record *)local->last);
        return;
    }
    indirect = (type & 0xf000) == 0x5000 && type != 0x5001 && type != 0x5002 &&
        type != 0x5004 && type != 0x5008 &&
        (!second || !INPUT_IS_REGISTER(second) || second->value.reg->kind != 3) &&
        (!first || !INPUT_IS_REGISTER(first) || first->value.reg->kind != 3);
    if (indirect) {
        temporary = new_input_temporary(0x3004);
        if (first) {
            if (INPUT_IS_REGISTER(first) && first->value.reg->kind == 3)
                c2_report_internal("E:\\utc\\src\\\\P2\\reader.c", 0xb84);
            push_input_trees(stack, first);
            convert_input_pointer(*stack, 0x3004);
            first = pop_input_trees(stack);
        }
    } else if (type == 0x5001 || type == 0x5002 || type == 0x5004 || type == 0x5008)
        temporary = input_symbol_operand(new_input_local(input_function->symbols,
            6, type & 0xfff, 1, 1, 4, 0));
    else temporary = new_input_temporary(type);
    if (first) {
        local = *stack;
        if ((temporary->type & 0xf000) == 0x5000) {
            size = new_integer_expression(0x1004, (__int64)(long)input_aggregate_bytes);
            local->last = new_block_copy_expression(0x13f, temporary->type,
                temporary, first, size, link_code_nodes, (C2Record *)local->last);
        } else local->last = new_binary_expression(0x131, temporary->type, temporary,
            first, link_code_nodes, (C2Record *)local->last);
    }
    local = *stack;
    if (local->before) finish_input_temporaries(local);
    if (local->after) append_deferred_input(local);
    if (!labels) labels = symbol_input_expression(new_input_flow_label(input_function));
    list = local->first->next;
    tree = last_input_tree(labels);
    link_code_nodes((C2Record *)tree, (C2Record *)list);
    local->first->next = labels;
    end_label = symbol_input_expression(new_input_flow_label(input_function));
    link_code_nodes((C2Record *)local->last, (C2Record *)end_label);
    local->last = end_label;
    arm = pop_input_trees(stack);
    if (indirect && second) {
        if (INPUT_IS_REGISTER(second) && second->value.reg->kind == 3)
            c2_report_internal("E:\\utc\\src\\\\P2\\reader.c", 0xbb0);
        push_input_trees(stack, second);
        convert_input_pointer(*stack, 0x3004);
        second = pop_input_trees(stack);
    }
    if (second) {
        local = *stack;
        if ((temporary->type & 0xf000) == 0x5000) {
            size = new_integer_expression(0x1004, (__int64)(long)input_aggregate_bytes);
            local->last = new_block_copy_expression(0x13f, temporary->type,
                temporary, second, size, link_code_nodes, (C2Record *)local->last);
        } else local->last = new_binary_expression(0x131, temporary->type, temporary,
            second, link_code_nodes, (C2Record *)local->last);
    }
    local = *stack;
    if (local->before) finish_input_temporaries(local);
    if (local->after) append_deferred_input(local);
    tree = new_call_expression(0x15a, 0, end_label, NULL, NULL,
        link_code_nodes, (C2Record *)local->last);
    local->last = tree;
    link_code_nodes((C2Record *)tree, (C2Record *)arm);
    local->last = end_label;
    list = pop_input_trees(stack);
    local = *stack;
    if (!(local->flags & 1)) {
        condition_value->location.expression.comparison &= 15;
        local->last = new_call_expression(0x159, condition, labels, condition_value,
            condition_value->value.reg->value, link_code_nodes, (C2Record *)local->last);
    } else local->flags &= ~1;
    link_code_nodes((C2Record *)local->last, (C2Record *)list);
    local->last = end_label;
    finish_input_call_stack(local);
    if (second || first) {
        tree = copy_input_expression(temporary);
        local->last = tree;
        tree->previous = local->first;
        local->first->next = tree;
        if (indirect) convert_input_operand(local, type, 0, 0);
    }
}

C2Expression *__fastcall last_input_tree(C2Expression *tree)
{
    C2Expression *last = NULL;
    while (tree) {
        last = tree;
        tree = tree->next;
    }
    return last;
}
