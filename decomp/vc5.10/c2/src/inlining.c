#include "inlining.h"
#include "function-reader.h"
#include "function-records.h"
#include <recon.h>

extern int input_temporary_count;
extern int c2_global_optimize, c2_favor_speed, c2_float_precision;
extern int c2_global_optimization, input_global_optimization_active;
extern int input_no_global_optimization, c2_internal_listing;

#define INLINE_REGISTER(node) ((1U << ((node)->kind & 31)) & 6U)

int __fastcall inline_function_calls(C2Function *function, int depth,
                                     int budget, int force)
{
    C2InlineCall *pending;
    C2InlineBinding *bindings, *binding;
    C2Expression *call, *target, *operand, *before, *entry, *record, *next;
    C2Expression *result, *value, *assignment, *return_record;
    C2Function *callee;
    C2FunctionSymbol *symbol;
    C2FunctionRegister *result_register, *reg, *coerced;
    int original_budget = budget, count, arguments, return_count, constants, i;
    int needs_context;
    unsigned return_storage, bytes, callee_options;
    unsigned short type, kind;

    function->symbol->additional_flags |= 0x10;
    pending = collect_inline_calls(function, &count,
        force != 0 || (function->symbol->symbol.flags & 0x1000) != 0);
    for (; pending; pending = pending->next, --count) {
        call = pending->call;
        if (call->location.line) {
            record_source_line = function->source_line_base + call->location.line;
            record_line_delta = call->location.line;
        }
        target = call->left;
        if (target->kind == 4)
            symbol = (C2FunctionSymbol *)target->value.symbol;
        else
            symbol = (C2FunctionSymbol *)target->value.reg->canonical->symbol;
        arguments = 0;
        for (operand = target->next; operand; operand = operand->next)
            if (INLINE_REGISTER(operand) && operand->value.reg->value &&
                (operand->value.reg->value->opcode == 0x130 ||
                 operand->value.reg->value->opcode == 0x13e)) ++arguments;
        bytes = symbol->return_bytes;
        type = adjust_input_return_size(symbol->attributes & 15, bytes);
        return_storage = needs_input_return_storage(symbol->symbol.flags & 0x1c,
                                                     type, bytes);
        if (arguments != symbol->parameter_count + return_storage ||
            depth > pending->allowed_depth ||
            (symbol->temporary_count > budget && symbol->temporary_count >= 41) ||
            input_temporary_count > 35000) goto rejected;

        before = call->previous;
        if (call->control_flags) {
            needs_context = 0;
            while (before) {
                if (before->flags & 1) {
                    if ((before->kind == 17 &&
                        (before->detail.intrinsic == 0xdf || before->detail.intrinsic == 0xde)) ||
                        before->opcode == 0x177) goto rejected;
                    operand = before->tail.right;
                    if (!before->location.expression.properties) {
                        if (!operand || !INLINE_REGISTER(operand) ||
                            !operand->value.reg->value) break;
                    } else if (operand &&
                        (!INLINE_REGISTER(operand) || !operand->value.reg->value))
                        needs_context = 1;
                }
                before = before->previous;
            }
            if (!before) before = (C2Expression *)function->flow->last->records;
            record = before->next;
            while (!(record->flags & 1)) record = record->next;
            before = record->previous;
            if (needs_context) before = save_inline_argument_context(before, call);
        }
        if (symbol->temporary_count > 40) budget -= symbol->temporary_count;
        input_temporary_count += symbol->temporary_count;
        if (!(symbol->additional_flags & 0x80)) {
            set_diagnostic_function(symbol, 1);
            compiler_warning(4, 0x2c7);
        }
        callee = read_function(symbol);
        if (call->location.line) {
            record_source_line = function->source_line_base + call->location.line;
            record_line_delta = call->location.line;
        }
        if (!c2_global_optimize && (callee->options & 0x200000)) goto incompatible;
        for (entry = (C2Expression *)callee->flow->last->records;
             entry && entry->kind != 22; entry = entry->next) {}
        if ((callee->flags & 0x8000) || (callee->symbol->additional_flags & 0x200)) {
            operand = entry->left;
            for (i = 0; i < 2; ++i) {
                if (operand) {
                    if (!strcmp(operand->value.reg->symbol->name, "___$ReturnUdt")) {
                        if (symbol->additional_flags & 0x80) {
                            set_diagnostic_function(symbol, 1);
                            compiler_warning(4, 0x2c6);
                        }
                        discard_inline_function(callee);
                        goto next_call;
                    }
                    operand = operand->next;
                }
            }
        }
        if ((callee->flags & 1) && c2_global_optimize && skip_global_optimization(callee))
            goto incompatible;
        if (callee->symbol->additional_flags & 0x600) {
            if (c2_global_optimize && (callee->symbol->additional_flags & 0x200))
                goto incompatible;
            function_pass_456857(callee, 1);
            function_pass_456b13(callee);
            function_pass_4570e0(callee, 0);
        }
        constants = mark_inline_arguments(callee, entry, call);
        if (constants > 0 && propagate_inline_constants(&entry->next, constants) > 0)
            fold_inline_constants(entry);
        constants = inline_function_calls(callee, depth + 1, budget / count,
            force != 0 || (function->symbol->symbol.flags & 0x1000) != 0);
        if (!c2_favor_speed && reject_inline_cost(callee, call, entry)) goto discard;
        budget -= constants;
        input_temporary_count += constants;
        if (call->location.line) {
            record_source_line = function->source_line_base + call->location.line;
            record_line_delta = call->location.line;
        }
        merge_inline_symbols(function->symbols, callee->symbols);
        if (function->symbol->additional_flags & 0x300) prepare_inline_call(call);
        bindings = make_inline_bindings(callee, entry, call);
        return_count = rewrite_inline_returns(entry, bindings, &return_record);
        for (binding = bindings; binding; binding = binding->next)
            if (binding->value) {
                materialize_inline_arguments(callee);
                break;
            }
        before = splice_inline_arguments(call, bindings, before,
            (C2Expression *)callee->flow->first->last_record);
        result_register = NULL;
        value = NULL;
        if (call->tail.right) {
            if (return_count == 1 && !(callee->flags & 1) &&
                is_direct_inline_return(return_record, before, call)) {
                value = return_record->left;
                if (!INLINE_REGISTER(value) || !value->value.reg->value)
                    value = copy_input_expression(value);
                else {
                    return_record->left = NULL;
                    value->location.expression.properties &= ~0x10;
                }
            } else {
                result_register = find_inline_result_register(call, &before, entry);
                if (!result_register) {
                    if (return_storage) {
                        kind = 3;
                        bytes = call->tail.right->type;
                    } else if ((symbol->attributes & 15) == 7) {
                        bytes = call->tail.right->type;
                        kind = (unsigned short)input_type_alignment(bytes);
                    } else {
                        kind = symbol->attributes & 15;
                        bytes = adjust_input_return_size(kind, symbol->return_bytes);
                    }
                    result_register = input_symbol_register(
                        new_inline_result_symbol(kind, bytes & 0xfff, 1, 1, 4, 0));
                }
            }
        }
        rewrite_inline_result(callee, result_register, call);
        function->flags |= callee->flags & 0x80c9;
        callee_options = callee->options;
        if (callee_options & 0x40000) c2_float_precision = 1;
        if (input_global_optimization_active) {
            if (!c2_global_optimization) {
                if (!(callee_options & 0x2000000)) input_global_optimization_active = 0;
            } else if (!(callee_options & 0x1000000)) {
                input_global_optimization_active = 0;
                c2_global_optimization = 0;
                input_no_global_optimization = 1;
            }
        }
        result = call->tail.right;
        if (result) {
            if (result_register)
                value = new_typed_register_expression(0x120, result->type, result_register);
            if (!INLINE_REGISTER(result) || !result->value.reg->value ||
                !can_forward_inline_result(call, value)) {
                call->tail.right = NULL;
                result->location.expression.properties &= ~0x10;
                if (!INLINE_REGISTER(value) || !(assignment = value->value.reg->value) ||
                    !INLINE_REGISTER(result) || !result->value.reg->value) {
                    assignment = new_binary_expression(0x131, result->type, result, value,
                        insert_input_record, (C2Record *)call);
                } else {
                    type = assignment->tail.right->type;
                    if (type != result->type) {
                        reg = result->value.reg;
                        coerced = new_address_register(reg, type, type & 0xfff, 0);
                        operand = new_temporary_register_expression(0x11f, type, coerced, reg);
                        release_small_node(result);
                        result = operand;
                    }
                    replace_input_result(assignment, assignment->tail.right, result);
                    unlink_input_record((C2Record *)assignment);
                    insert_input_record((C2Record *)call, (C2Record *)assignment);
                }
                assignment->location.line = call->location.line;
                assignment->control_flags = call->control_flags;
                if (INLINE_REGISTER(result) && result->value.reg->value)
                    bind_input_temporary(result->value.reg, assignment);
                if (INLINE_REGISTER(value) && value->value.reg->value)
                    raise_input_control_level(assignment, (signed char)assignment->control_flags);
                if (result_register) goto splice_body;
            } else
                assignment = forward_inline_result(call, value);
            insert_inline_result(entry, assignment);
        }
splice_body:
        record = (C2Expression *)callee->flow->last->records;
        link_code_nodes((C2Record *)before, (C2Record *)record);
        operand = entry->next;
        while (record != operand) {
            next = record->next;
            unlink_and_release_input_record((C2Record *)record);
            record = next;
        }
        record = (C2Expression *)callee->flow->first->last_record;
        while (record) {
            next = record->kind == 23 ? NULL : record->previous;
            unlink_and_release_input_record((C2Record *)record);
            record = next;
        }
        unlink_and_release_input_record((C2Record *)call);
        if (c2_internal_listing) {
            print_inline_function(symbol->symbol.serial);
            symbol->additional_flags |= 0x1000;
        }
        goto next_call;
incompatible:
        if (symbol->additional_flags & 0x80) {
            set_diagnostic_function(symbol, 1);
            compiler_warning(4, 0x2c6);
        }
discard:
        discard_inline_function(callee);
        goto next_call;
rejected:
        if (symbol->additional_flags & 0x80) {
            set_diagnostic_function(symbol, 1);
            compiler_warning(4, 0x2c6);
        }
next_call: ;
    }
    function->symbol->additional_flags &= ~0x10;
    return original_budget - budget;
}

extern C2Function *current_function;
extern int c2_inline_limit;

C2InlineCall *__fastcall collect_inline_calls(C2Function *function, int *count,
                                             int force)
{
    C2InlineCall *first = NULL, *last = NULL, *pending;
    C2Expression *record, *next, *target, *operand, *assignment, *value, *context;
    C2FunctionRegister *reg;
    C2FunctionSymbol *symbol;
    int live_temporaries = 0;
    unsigned control = 0;
    *count = 0;
    record = (C2Expression *)function->flow->last->records;
    while (record) {
        next = record->next;
        if (!(record->flags & 1)) {
            if (record->kind == 21) {
                if (record->opcode == 0x182) {
                    control = record->value.control;
                    unlink_and_release_input_record((C2Record *)record);
                }
            } else if (record->kind == 25 &&
                       record->declaration->symbol.source_kind == 'T') {
                while (next->opcode != 0x174) next = next->next;
                next = next->next;
            }
            goto next_record;
        }
        target = record->left;
        for (operand = target; operand; operand = operand->next) {
            if (INLINE_REGISTER(operand) && operand->value.reg->value)
                --live_temporaries;
            else if (operand->kind == 3 && record->opcode != 0x151)
                for (reg = operand->value.reg->canonical; reg; reg = reg->variant_next)
                    reg->view_flags |= 2;
        }
        record->control_flags = (unsigned char)live_temporaries;
        for (operand = record->tail.right; operand; operand = operand->next)
            if (INLINE_REGISTER(operand) && operand->value.reg->value) ++live_temporaries;
        if (record->kind != 13) {
            if (record->kind == 20)
                scan_inline_assembly(record->detail.assembly);
            goto next_record;
        }
        if (record->location.line) {
            record_source_line = function->source_line_base + record->location.line;
            record_line_delta = record->location.line;
        }
        symbol = NULL;
        if (target->kind == 4) {
            symbol = (C2FunctionSymbol *)target->value.symbol;
            if (symbol->symbol.kind != 4 || !(symbol->attributes & 0x4000)) symbol = NULL;
        } else if (target->kind == 2) {
            symbol = (C2FunctionSymbol *)target->value.reg->canonical->symbol;
            if (!symbol || symbol->symbol.kind != 4 ||
                (symbol->attributes & 0x5000) != 0x5000) symbol = NULL;
        }
        if (!symbol) goto next_record;
        if ((force && (symbol->additional_flags & 0x100)) ||
            (!(current_function->flags & 0x8000) &&
             !(current_function->symbol->additional_flags & 0x200) &&
             (symbol->additional_flags & 0x300)) ||
            (!c2_favor_speed && symbol->temporary_count >= c2_inline_limit) ||
            !function_retains_symbols(symbol, c2_global_optimize)) {
            if ((symbol->additional_flags & 0xc0) == 0xc0) {
                set_diagnostic_function(symbol, 1);
                compiler_warning(4, 0x2c6);
            }
            goto next_record;
        }
        for (operand = target->next; operand; operand = operand->next) {
            if (!INLINE_REGISTER(operand) || !(assignment = operand->value.reg->value) ||
                assignment->opcode != 0x13e) continue;
            value = assignment->left;
            if (INLINE_REGISTER(value) && value->value.reg->value)
                value = value->value.reg->value;
            else if (INLINE_REGISTER(value) && value->value.reg->kind == 3 &&
                     !value->value.reg->value && (value->value.reg->flags & 0x40))
                value = resolve_inline_argument(assignment, value, &context);
            else
                value = NULL;
            if (value && ((value->kind == 17 && value->detail.intrinsic == 0xdf) ||
                          value->opcode == 0x177)) goto next_record;
        }
        if (!(symbol->additional_flags & 0x10) || (control & 0xf00)) {
            pending = c2_pool_allocate(4, sizeof(C2InlineCall));
            pending->call = record;
            pending->control = control;
            if (last) last->next = pending;
            else first = pending;
            ++*count;
            last = pending;
        }
next_record:
        record = next;
    }
    return first;
}

void __fastcall insert_input_record(C2Record *prior, C2Record *record)
{
    C2Record *next = prior->next;
    prior->next = record;
    record->previous = prior;
    record->next = next;
    next->previous = record;
}
