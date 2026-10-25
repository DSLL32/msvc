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
 * No row has been encountered yet. These reserved bytes are NOT reconstructed
 * defaults: native guards precede every lookup until its row is established.
 * The original WORDs, including unmodeled low bits, stay in analysis evidence. */
static unsigned char input_opcode_properties[158];
static unsigned char input_inverse_condition[16];
static unsigned input_register_opcode[17];
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
static unsigned char input_opcode_known[158], input_condition_known[16];
static unsigned char input_register_opcode_known[17];
#define INPUT_PROPERTIES(op) \
    ((unsigned)(op) < 158 && input_opcode_known[(op)] ? \
     input_opcode_properties[(op)] : \
     (recon_unsupported("unmodeled input opcode properties"), 0))
#define INPUT_INVERSE(condition) \
    ((unsigned)(condition) < 16 && input_condition_known[(condition)] ? \
     input_inverse_condition[(condition)] : \
     (recon_unsupported("unmodeled input comparison inverse"), 0))
#define INPUT_REGISTER_OPCODE(kind) \
    ((unsigned)(kind) < 17 && input_register_opcode_known[(kind)] ? \
     input_register_opcode[(kind)] : \
     (recon_unsupported("unmodeled input register opcode"), 0))
#else
#define INPUT_PROPERTIES(op) input_opcode_properties[(op)]
#define INPUT_INVERSE(condition) input_inverse_condition[(condition)]
#define INPUT_REGISTER_OPCODE(kind) input_register_opcode[(kind)]
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
                reg = new_address_register(reg, 0x5000, 0, (C2FunctionRegister *)saved);
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
