#include "code-input.h"
#include <recon.h>

extern int c2_basic_mode, c2_codeview_types, c2_global_optimize;
extern int retain_function_input_pool, input_error_seen, c2_il_extended_header;
extern C2InputSymbol *pending_optimized_input;
extern C2SymbolListScope *current_symbol_list_scope;
typedef struct C2PendingInput C2PendingInput;
extern C2PendingInput *pending_input_records;

int input_preserve_operand, input_address_mode;
int input_end_of_module;
int input_saved_global_optimize = 1;
unsigned input_aggregate_bytes, input_last_source_index;
C2InputClause *input_clauses;

/* Only the two high-byte properties read by this dispatcher are represented.
 * Encountered function-entry70, parameter-end76, pragma79, scope-begin83 and filename86
 * are boundaries; variable38 has neither consumed property.
 * Reserved bytes are NOT reconstructed defaults; guards precede unknown reads.
 * The original WORDs, including unmodeled low bits, stay in analysis evidence. */
static struct {
    unsigned char pending00[38], variable, pending39[9], conversion, pending49[21];
    unsigned char function_entry, pending71[5];
    unsigned char parameter_end, pending77[2];
    unsigned char pragma, pending80[3], scope_begin, pending84[2], filename, pending87[71];
} input_opcode_properties = {{0}, 0, {0}, 0, {0}, 0x10, {0}, 0x10, {0}, 0x10, {0}, 0x10, {0}, 0x10, {0}};
static unsigned char input_inverse_condition[16];
/* Only encountered tracked-register kind4 and parameter kind5 are modeled. */
static union {
    struct { unsigned pending00[4], tracked, parameter, pending06[11]; } row;
    unsigned values[17];
} input_register_opcodes = {{{0}, 0x120, 0x120, {0}}};
/* Typed operand kind5 is the encountered local-register opcode. All other
 * backing rows remain UNMODELED, and native guards precede their reads. */
static union {
    struct { unsigned pending00[5], local, pending06[11]; } row;
    unsigned values[17];
} input_operand_opcodes = {{{0}, 0x120, {0}}};
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
static unsigned char input_condition_known[16];
#define INPUT_PROPERTIES(op) \
    ((unsigned)(op) == 38 ? input_opcode_properties.variable : \
     (unsigned)(op) == 48 ? input_opcode_properties.conversion : \
     (unsigned)(op) == 70 ? input_opcode_properties.function_entry : \
     (unsigned)(op) == 76 ? input_opcode_properties.parameter_end : \
     (unsigned)(op) == 79 ? input_opcode_properties.pragma : \
     (unsigned)(op) == 83 ? input_opcode_properties.scope_begin : \
     (unsigned)(op) == 86 ? input_opcode_properties.filename : \
     (recon_unsupported("unmodeled input opcode properties"), 0))
#define INPUT_INVERSE(condition) \
    ((unsigned)(condition) < 16 && input_condition_known[(condition)] ? \
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
            input_shift_operator(&stack, &token);
            break;
        case 51:
            if ((token.type & 0xf000) == 0x4000)
                value = new_real_expression(token.type, (C2Real *)&token.value.scalar.low);
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
                        NULL, NULL, value, NULL, 0);
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
            input_pragma(stack, &token);
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
                attach_input_expression(left, &left->left->next, value);
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
static struct {
    unsigned char pending00[38], variable, pending39[6], parameter, pending46[2];
    unsigned char conversion, pending49[21];
    unsigned char function_entry, pending71[5], parameter_end, pending77[2], pragma, pending80[3], scope_begin;
} input_token_forms = {{0}, INPUT_FORM_SYMBOL, {0}, INPUT_FORM_SYMBOL, {0},
                       INPUT_FORM_TYPE, {0}, INPUT_FORM_NONE, {0}, INPUT_FORM_NONE, {0}, INPUT_FORM_PRAGMA, {0}, INPUT_FORM_NONE};
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define INPUT_TOKEN_FORM(op) ((unsigned)(op) == 38 ? input_token_forms.variable : \
    (unsigned)(op) == 45 ? input_token_forms.parameter : \
    (unsigned)(op) == 48 ? input_token_forms.conversion : \
    (unsigned)(op) == 70 ? input_token_forms.function_entry : \
    (unsigned)(op) == 76 ? input_token_forms.parameter_end : \
    (unsigned)(op) == 79 ? input_token_forms.pragma : \
    (unsigned)(op) == 83 ? input_token_forms.scope_begin : \
    (recon_unsupported("unmodeled input token form"), 0))
#else
#define INPUT_TOKEN_FORM(op) (((unsigned char *)&input_token_forms)[(op)])
#endif

C2ILToken *__fastcall decode_input_token(unsigned opcode, C2Function *function,
                                        C2ILToken *token)
{
    unsigned form = INPUT_TOKEN_FORM(opcode);
    unsigned serial;
    int kind;
    C2ScopeHash *scope;
    C2Real real, temporary, *storage;
    union { unsigned __int64 bits; double real; } number;
    float single;
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
        token->value.scalar.low = (unsigned)storage;
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
        tree = new_aggregate_expression(0x122, type, NULL, NULL,
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
        tree = new_aggregate_expression(0x122, type, NULL, NULL, tree, NULL, 0);
        break;
    default:
        break;
    }
    link_code_nodes((C2Record *)stack->last, (C2Record *)tree);
    stack->last = tree;
    if (flags) tree->location.expression.properties |= 0x40;
    return tree;
}
