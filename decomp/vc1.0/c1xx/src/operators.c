/* Builtin C++ operator candidates and their operand type patterns. */
#include "operators.h"
#include "builtin-operators.h"
#include "keywords.h"
#include "lexer.h"
#include "call-expressions.h"
#include "cpp-context.h"
#include "function-definitions.h"
#include "constructor-initializers.h"
#include "inline-vector.h"
#include "unary-expressions.h"
#include "numeric-conversion.h"
#include <recon.h>

/* Original [0x61540,0x615dc). Some class operations have a builtin form
   even after overload lookup fails; token categories supply the remainder. */
int __stdcall operator_has_builtin_fallback(unsigned char operation, unsigned char arity,
    unsigned classification)
{
    if (arity == 1) {
        if (operation == OP_BIT_AND || operation == OP_CALL) return 1;
        return token_categories[operation] == 0x46;
    }
    if (arity != 2) return 1;
    if (operation == OP_ASSIGN || operation == 0x3f || operation == 0x40 ||
        operation == 0x7e) return 1;
    if (operation == 0x5f) return classification == 0;
    return token_categories[operation] == 0x9f;
}

/* The pattern flag words are replaced in place by interned descriptors at
   initialization, matching the mutable operand union in the original table. */
#define BINARY(op, left, right, assign) { op, {{left}, {right}}, assign, 0 }
#define UNARY(op, type, assign) BINARY(op, type, 0, assign)
BuiltinOperator builtin_operators[] = { /* original 0xa7c70 */
    BINARY(OP_BIT_AND, 0x40, 0x40, 0),
    BINARY(OP_ASSIGN, 0x140, 0x81, 1),
    BINARY(OP_ASSIGN, 0x84, 0x81, 1),
    BINARY(OP_EQUAL, 0x140, 0x140, 0),
    BINARY(OP_EQUAL, 0x84, 0x81, 0),
    BINARY(OP_NOT_EQUAL, 0x140, 0x140, 0),
    BINARY(OP_NOT_EQUAL, 0x84, 0x81, 0),
    BINARY(OP_GREATER, 0x140, 0x140, 0),
    BINARY(OP_GREATER_EQUAL, 0x140, 0x140, 0),
    BINARY(OP_LESS, 0x140, 0x140, 0),
    BINARY(OP_LESS_EQUAL, 0x140, 0x140, 0),
    BINARY(OP_LOGICAL_AND, 0x40, 0x40, 0),
    BINARY(OP_LOGICAL_AND, 0x84, 0x40, 0),
    BINARY(OP_LOGICAL_AND, 0x40, 0x84, 0),
    BINARY(OP_LOGICAL_AND, 0x84, 0x84, 0),
    BINARY(OP_AND_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_BIT_OR, 0x40, 0x40, 0),
    BINARY(OP_OR_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_LOGICAL_OR, 0x40, 0x40, 0),
    BINARY(OP_LOGICAL_OR, 0x84, 0x40, 0),
    BINARY(OP_LOGICAL_OR, 0x40, 0x84, 0),
    BINARY(OP_LOGICAL_OR, 0x84, 0x84, 0),
    UNARY(OP_COMPLEMENT, 0x40, 0),
    BINARY(OP_BIT_XOR, 0x40, 0x40, 0),
    BINARY(OP_XOR_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_POST_DECREMENT, 0x40, 0x40, 1),
    UNARY(OP_PRE_DECREMENT, 0x40, 1),
    BINARY(OP_POST_INCREMENT, 0x40, 0x40, 1),
    UNARY(OP_PRE_INCREMENT, 0x40, 1),
    BINARY(OP_DIVIDE, 0x140, 0x140, 0),
    BINARY(OP_DIVIDE_ASSIGN, 0x140, 0x140, 1),
    UNARY(OP_NOT, 0x140, 0),
    UNARY(OP_NOT, 0x84, 0),
    UNARY(OP_NEGATE, 0x140, 0),
    BINARY(OP_SUBTRACT, 0x140, 0x140, 0),
    BINARY(OP_SUBTRACT, 0x88, 0x40, 0),
    BINARY(OP_SUBTRACT, 0x88, 0x81, 0),
    BINARY(OP_SUBTRACT_ASSIGN, 0x140, 0x140, 1),
    BINARY(OP_SUBTRACT_ASSIGN, 0x88, 0x40, 1),
    UNARY(OP_UNARY_PLUS, 0x140, 0),
    BINARY(OP_ADD, 0x140, 0x140, 0),
    BINARY(OP_ADD, 0x88, 0x40, 0),
    BINARY(OP_ADD, 0x40, 0x88, 0),
    BINARY(OP_ADD_ASSIGN, 0x140, 0x140, 1),
    BINARY(OP_ADD_ASSIGN, 0x88, 0x40, 1),
    BINARY(OP_REMAINDER, 0x140, 0x140, 0),
    BINARY(OP_REMAINDER_ASSIGN, 0x140, 0x140, 1),
    BINARY(OP_MULTIPLY, 0x140, 0x140, 0),
    UNARY(OP_DEREFERENCE, 0x88, 0),
    BINARY(OP_MULTIPLY_ASSIGN, 0x140, 0x140, 1),
    BINARY(OP_SHIFT_LEFT, 0x40, 0x40, 0),
    BINARY(OP_SHIFT_RIGHT, 0x40, 0x40, 0),
    BINARY(OP_SHIFT_LEFT_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_SHIFT_RIGHT_ASSIGN, 0x40, 0x40, 1),
    BINARY(OP_MEMBER_POINTER, 0x8c, 0x83, 0),
    BINARY(OP_SUBSCRIPT, 0x88, 0x40, 0),
    BINARY(OP_SUBSCRIPT, 0x40, 0x88, 0),
    {0}
};
#undef UNARY
#undef BINARY

char *name_arrow_operator, *name_call_operator; /* original 0x9f20c/0x9f208 */
IrToken *operator_zero; /* original 0x9f3c8 */

/* Original 0x77d50..0x77dd6. */
void initialize_operators(void)
{
    BuiltinOperator *entry;
    OperatorOperand *operand;
    name_arrow_operator = intern_operator_name(OP_ARROW);
    name_call_operator = intern_operator_name(OP_CALL);
    for (entry = builtin_operators; entry->token; ++entry) {
        entry->name = intern_operator_name(entry->token);
        for (operand = entry->operands; operand < entry->operands + 2; ++operand)
            if ((unsigned short)operand->initial_flags)
                operand->type = make_tagged_type((unsigned short)operand->initial_flags);
    }
    token_allocation_flags |= 0x20;
    operator_zero = make_integer_constant(0);
    token_allocation_flags &= ~0x20;
    initialize_allocation_operators();
}

/* Original 0x77860..0x7789d; argument occupies a stack slot but is a byte. */
char *__stdcall intern_operator_name(unsigned char token)
{
    if (token == OP_SUBSCRIPT)
        return intern_identifier("[]");
    if (token == OP_CALL)
        return intern_identifier("()");
    return intern_identifier(token_spellings[token]);
}

/* Original 0x4f7b0..0x4f7bf. Default near allocation/deallocation forms. */
void initialize_allocation_operators(void)
{
    declare_operator_new(1);
    declare_operator_delete(1);
}

/* Original 0x4f4e0..0x4f60a. Declare the builtin operator-new signature. */
void __stdcall declare_operator_new(unsigned short attributes)
{
    unsigned char saved_mode;
    SymbolScope *saved_scope;
    CppSymbol *declaration, *parameters = 0;
    CType *base;
    DeclarationType result;
    ArenaPool *pool;

    save_declaration_context(&saved_mode, &saved_scope);
    declaration = new_declarator(name_new);
    declaration->cpp.parts.token = 151;
    declaration->type->attributes = 0x10;
    declaration_kind = 3;
    add_declaration_parameter(0, &parameters, 0, type_4064_second);
    if (attributes & 4)
        add_declaration_parameter(0, &parameters, 0, type_0802);
    declaration = attach_function_parameters(declaration, parameters);
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(CType))
        refill_arena(&compiler_arena, pool);
    base = (CType *)pool->next;
    pool->remaining -= sizeof(CType);
    pool->next += sizeof(CType);
    base->flags = 0x80;
    result.type = make_qualified_type(base, 1, attributes, 0);
    result.storage[0] = (declaration_storage[0] & ~7) | 1;
    result.storage[1] = declaration_storage[1];
    result.storage[2] = declaration_storage[2];
    apply_declaration_type(&result, declaration);
    find_scope_symbol(global_scope, name_new, 0);
    restore_declaration_context(saved_mode, saved_scope);
}

/* Original 0x4f610..0x4f749. The runtime delete declaration takes void *;
   based-pointer configurations additionally supply the segment parameter. */
void __stdcall declare_operator_delete(unsigned short attributes)
{
    unsigned char saved_mode;
    SymbolScope *saved_scope;
    CppSymbol *declaration, *parameters = 0;
    CType *parameter_type;
    DeclarationType result;
    ArenaPool *pool;
    save_declaration_context(&saved_mode, &saved_scope);
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena, pool);
    parameter_type = (CType *)pool->next;
    pool->remaining -= sizeof(CType);
    pool->next += sizeof(CType);
    parameter_type->flags = 0x80;
    parameter_type->related = intern_type_shape(new_type_shape(1, attributes, 0));
    add_declaration_parameter(0, &parameters, 0, intern_type((TypeDescription *)parameter_type));
    if (attributes & 4) {
        parameter_type->flags = 0x802;
        parameter_type->related = 0;
        add_declaration_parameter(0, &parameters, 0, intern_type((TypeDescription *)parameter_type));
    }
    declaration = new_declarator(name_delete);
    declaration->cpp.parts.token = 152;
    declaration->type->attributes = 0x10;
    declaration_kind = 3;
    declaration = attach_function_parameters(declaration, parameters);
    result.storage[0] = (declaration_storage[0] & ~7) | 1;
    result.storage[1] = declaration_storage[1];
    result.storage[2] = declaration_storage[2];
    result.type = (CType *)copy_type_description(type_0080);
    apply_declaration_type(&result, declaration);
    --type_description_depth;
    restore_declaration_context(saved_mode, saved_scope);
}

IrToken *__stdcall resolve_operator_overload(unsigned char operation, IrToken *left,
    IrToken *right, unsigned arity, int diagnose, unsigned *failure);

/* Original 0x77d00..0x77d22. The public expression path enables the
   diagnostic/fallback policy while retaining the optional failure result. */
IrToken *__stdcall resolve_operator_expression(unsigned char operation, IrToken *left,
    IrToken *right, unsigned arity, unsigned *failure)
{
    return resolve_operator_overload(operation, left, right, arity, 1, failure);
}

IrToken *__stdcall resolve_overloaded_arrow(IrToken *object);
OverloadSearch *__stdcall new_operator_search(unsigned char operation, IrToken *left,
    IrToken *right, CType *object_type);
int __stdcall rank_operator_operands(OverloadSearch *search, IrToken *left, IrToken *right);
IrToken *__stdcall make_selected_operator_call(OverloadCandidate *candidate,
    IrToken *left, IrToken *right);
OverloadCandidate *__stdcall report_operator_overload_failure(OverloadSearch *search);

/* Original 0x77aa0..0x77cf4. Postfix operators can fall back to prefix
   overloads under extensions, with a class-name diagnostic. Other operators
   retain their exclusions, arrow recursion, candidate-count/rank stages and
   distinct failure codes. Failed stages retain their original cleanup policy. */
IrToken *__stdcall resolve_operator_overload(unsigned char operation, IrToken *left,
    IrToken *right, unsigned arity, int diagnose, unsigned *failure)
{
    IrToken *result;
    OverloadSearch *search;
    OverloadCandidate *candidate;
    CType *object_type;
    switch (operation) {
    case 0x3f: case 0x59: case 0x5f: case 0x80: case 0x97: case 0x98: case 0x9d:
        return 0;
    case OP_POST_DECREMENT: case OP_POST_INCREMENT:
        if (options.configuration.extensions_enabled && arity && diagnose) {
            result = resolve_operator_overload(operation, left, operator_zero, 1, 0, 0);
            if (result) return result;
            result = resolve_operator_overload(operation == OP_POST_INCREMENT ?
                OP_PRE_INCREMENT : OP_PRE_DECREMENT, left, right, 1, 0, 0);
            if (result) {
                compiler_warning(1, operation == OP_POST_INCREMENT ? 620 : 621,
                    ((CppSymbol *)cpp_context_stack->items[left->type->field08])->name);
                return result;
            }
        }
        right = operator_zero;
        break;
    case OP_PRE_DECREMENT: case OP_PRE_INCREMENT:
        right = 0;
        break;
    case OP_ARROW:
        return resolve_overloaded_arrow(left);
    }
    object_type = arity ? left->type : 0;
    search = new_operator_search(operation, left, right, object_type);
    if (!search) {
        if (failure) *failure = 0x2009;
        return 0;
    }
    if (!validate_overload_counts(search)) {
        if (failure) *failure = 0x2009;
        return 0;
    }
    if (!rank_operator_operands(search, left, right)) {
        if (failure) *failure = 0x200a;
        return 0;
    }
    candidate = select_overload_candidate(search, failure, report_operator_overload_failure);
    if (!candidate) {
        if (failure && !*failure) *failure = 0x200b;
        return 0;
    }
    result = make_selected_operator_call(candidate, left, right);
    destroy_overload_search(search);
    return result;
}

BuiltinOperator *__stdcall find_builtin_operator(unsigned char operation);
unsigned __stdcall count_builtin_operator_candidates(BuiltinOperator *rule,
    IrToken *left, IrToken *right);
void __stdcall append_builtin_operator_candidates(OverloadSearch *search,
    BuiltinOperator *rule, IrToken *left, IrToken *right, unsigned *count);

/* Original 0x76b70..0x76d51. Reserve global/member/builtin candidate
   slots, retaining the member qualification in each member candidate. Builtin
   generation can reduce its estimate, so update both counts after appending. */
OverloadSearch *__stdcall new_operator_search(unsigned char operation, IrToken *left,
    IrToken *right, CType *object_type)
{
    unsigned argument_count = right ? 2 : 1;
    unsigned count = 0, builtin_count, actual_builtin_count;
    BuiltinOperator *rule = find_builtin_operator(operation);
    char *name = rule ? rule->name : intern_operator_name(operation);
    CppSymbol *global_functions, *member_functions = 0;
    QualifiedDeclarator *qualification = 0;
    OverloadSearch *search;
    unsigned kind;
    if (operation == OP_ASSIGN) {
        if (object_type && !class_tag_for_type(object_type)) return 0;
        global_functions = 0;
    } else {
        global_functions = find_scope_symbol(global_scope, name, 0);
        if (global_functions) {
            kind = global_functions->kind & 0x1f;
            if (kind != 16 && kind != 17) return 0;
            count = count_overload_functions(global_functions);
        }
    }
    if (object_type) {
        qualification = lookup_class_name(cpp_context_stack->items[object_type->field08],
            name, 2, 0);
        if (qualification) {
            member_functions = (CppSymbol *)qualification->resolution->binding->target;
            kind = member_functions->kind & 0x1f;
            if (kind != 16 && kind != 17) return 0;
            count += count_overload_functions(member_functions);
        }
    }
    builtin_count = count_builtin_operator_candidates(rule, left, right);
    count += builtin_count;
    if (!count) return 0;
    search = new_overload_search(0, argument_count, 3, count);
    if (!search) return 0;
    search->operation = operation;
    if (member_functions && !add_overload_candidates(search, member_functions, qualification)) {
        destroy_overload_search(search);
        return 0;
    }
    if (global_functions && !add_overload_candidates(search, global_functions, 0)) {
        destroy_overload_search(search);
        return 0;
    }
    if (builtin_count) {
        append_builtin_operator_candidates(search, rule, left, right, &actual_builtin_count);
        count = count - builtin_count + actual_builtin_count;
        if (!count) {
            destroy_overload_search(search);
            return 0;
        }
        search->total_count = count;
        search->viable_count = count;
    }
    return search;
}

/* Original 0x778a0..0x778c4. Check a row before checking its sentinel
   token: operation zero therefore selects the terminating row itself. */
BuiltinOperator *__stdcall find_builtin_operator(unsigned char operation)
{
    BuiltinOperator *rule = builtin_operators;
    if (rule->token == operation) return rule;
    while (rule->token) {
        ++rule;
        if (rule->token == operation) return rule;
    }
    return 0;
}

/* Original 0x75d10..0x75e23. Consecutive rows describe alternatives
   for one operator. Class operands contribute their conversion-type counts;
   the void-pointer pattern sums rather than multiplies those choices. Native
   short counts are sign-extended before the wrapping 32-bit arithmetic. */
unsigned __stdcall count_builtin_operator_candidates(BuiltinOperator *rule,
    IrToken *left, IrToken *right)
{
    CType *left_type = left->type, *right_type = right ? right->type : 0;
    unsigned total = 0, left_count, right_count;
    unsigned char operation;
    if (!rule) return 0;
    operation = rule->token;
    do {
        if ((rule->operands[1].type != 0) == (right != 0)) {
            if (left_type && is_class_or_reference_type(left_type))
                left_count = (unsigned)(int)inline_vector_count(
                    ((CppSymbol *)cpp_context_stack->items[left_type->field08])->detail.class_info->conversion_types);
            else left_count = 1;
            if (right_type && is_class_or_reference_type(right_type))
                right_count = (unsigned)(int)inline_vector_count(
                    ((CppSymbol *)cpp_context_stack->items[right_type->field08])->detail.class_info->conversion_types);
            else right_count = 1;
            if (right_type) {
                if ((rule->operands[1].type->flags & 0x2fdf) == 0x81)
                    total += left_count + right_count;
                else total += left_count * right_count;
            } else total += left_count;
        }
        ++rule;
    } while (rule->token == operation);
    return total;
}

/* Original 0x76d60..0x76f57, stdcall 20. Unary rules consume one
   operand iterator. The 0x81 pattern uses one common type on both sides;
   ordinary binary rules form the product of the two permitted type sets. */
void __stdcall append_builtin_operator_candidates(OverloadSearch *search,
    BuiltinOperator *rule, IrToken *left, IrToken *right, unsigned *count)
{
    BuiltinOperandIterator left_types, right_types;
    CType *left_pattern, *right_pattern;
    unsigned char operation;
    unsigned found;
    *count = 0;
    if (!rule) return;
    operation = rule->token;
    do {
        left_pattern = rule->operands[0].type;
        right_pattern = rule->operands[1].type;
        if ((right_pattern != 0) == (right != 0)) {
            if (!right_pattern) {
                begin_builtin_operand_types(&left_types, left, left_pattern,
                    (unsigned char)rule->assignment);
                while (next_builtin_operand_type(&left_types))
                    add_builtin_operator_candidate(search, rule, left_types.type, 0, count);
            } else if ((right_pattern->flags & 0x2fdf) == 0x81) {
                found = 0;
                begin_builtin_operand_types(&left_types, left, left_pattern,
                    (unsigned char)rule->assignment);
                while (next_builtin_operand_type(&left_types)) {
                    ++found;
                    add_builtin_operator_candidate(search, rule,
                        left_types.type, left_types.type, count);
                }
                if (found) {
                    begin_builtin_operand_types(&right_types, right, left_pattern,
                        (unsigned char)rule->assignment);
                    while (next_builtin_operand_type(&right_types))
                        add_builtin_operator_candidate(search, rule,
                            right_types.type, right_types.type, count);
                }
            } else {
                begin_builtin_operand_types(&left_types, left, left_pattern,
                    (unsigned char)rule->assignment);
                while (next_builtin_operand_type(&left_types)) {
                    begin_builtin_operand_types(&right_types, right, right_pattern, 0);
                    while (next_builtin_operand_type(&right_types))
                        add_builtin_operator_candidate(search, rule,
                            left_types.type, right_types.type, count);
                }
            }
        }
        ++rule;
    } while (rule->token == operation);
}

/* Original 0x76f60..0x76fc2, stdcall 16. Class operands expose
   their conversion-target set; scalar operands are returned only once. */
void __stdcall begin_builtin_operand_types(BuiltinOperandIterator *iterator,
    IrToken *expression, CType *category, unsigned require_lvalue)
{
    iterator->expression = expression;
    iterator->class_conversion = expression->type &&
        is_class_or_reference_type(expression->type) ? 1 : 0;
    if (iterator->class_conversion)
        begin_conversion_types(&iterator->conversions,
            cpp_context_stack->items[expression->type->field08]);
    iterator->category = category;
    iterator->require_lvalue = require_lvalue;
}

/* Original 0x76fd0..0x77060, stdcall 4. Scalar operands are consumed
   once; class operands skip conversion targets outside this rule's category. */
int __stdcall next_builtin_operand_type(BuiltinOperandIterator *iterator)
{
    if (!iterator->class_conversion) {
        if (!iterator->expression) return 0;
        if (iterator->require_lvalue && !builtin_operand_is_lvalue(iterator->expression))
            return 0;
        iterator->type = iterator->expression->type;
        iterator->expression = 0;
        return builtin_operand_type_matches(iterator->type, iterator->category,
            iterator->require_lvalue, 0);
    }
    while (next_conversion_type(&iterator->conversions)) {
        if (builtin_operand_type_matches(iterator->conversions.type, iterator->category,
            iterator->require_lvalue, 1)) {
            iterator->type = iterator->conversions.type;
            return 1;
        }
    }
    return 0;
}

TypeShape *__stdcall skip_reference_shape(TypeShape *shape);
/* Original 0x77070..0x771e9, stdcall 16. Match the rule's scalar,
   pointer and class-pointer patterns after removing a based prefix and one
   reference shape. Assignment through a conversion requires a reference. */
int __stdcall builtin_operand_type_matches(CType *type, CType *category,
    unsigned require_lvalue, int from_conversion)
{
    TypeShape *shape;
    unsigned short flags, category_flags;
    if (!type) return 0;
    flags = type->flags;
    shape = type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (require_lvalue && from_conversion && (!shape || !(shape->flags & 0x40)))
        return 0;
    shape = skip_reference_shape(shape);
    category_flags = category->flags & 0x2fdf;
    switch (category_flags) {
    case 0x40:
        return ((flags & 0x840) || (!require_lvalue && (flags & 0x200))) && !shape;
    case 0x83:
        return shape && (shape->flags & 0x30) ? 1 : 0;
    case 0x84:
    case 0x88:
        if (!shape || !(shape->flags & 3)) return 0;
        if (category_flags == 0x88 && (flags & 0x80) && !shape->related) return 0;
        return 1;
    case 0x8c:
        if (!shape || (shape->flags & 0x77) != 1) return 0;
        return is_unqualified_class_type(dereference_pointer_type(type)) ? 1 : 0;
    case 0x140:
        return ((flags & 0x940) || (!require_lvalue && (flags & 0x200))) && !shape;
    default:
        return 0;
    }
}

/* Original 0x77300..0x77438, stdcall 12. Member operators rank their
   implicit object before the explicit right operand. Builtin candidates carry
   their two parameter types directly instead of a function parameter cursor. */
int __stdcall rank_operator_operands(OverloadSearch *search, IrToken *left, IrToken *right)
{
    OverloadCandidate *candidate;
    OverloadArgumentRank *ranks;
    CppSymbol *function;
    CType *target;
    IrToken *operand;
    unsigned flags;
    for (candidate = search->candidates; candidate; candidate = candidate->next) {
        ranks = candidate->ranks;
        flags = (search->flags & 4) ? 1 : 0;
        operand = left;
        set_overload_phase(search, 8);
        if (!(candidate->flags & 0x10)) {
            function = candidate->function;
            if ((function->declaration_flags & 0x400) &&
                (function->cpp.parts.member_flags & 1)) {
                target = candidate->this_type;
                operand = builtin_unary_expression(OP_BIT_AND, left);
            } else target = *candidate->parameter_cursor++;
        } else target = candidate->builtin_left_type;
        ranks->parameter_type = target;
        ranks->rank = classify_expression_conversion(target, operand, flags, candidate);
        if (ranks->rank & 0x2000) {
            if (reject_overload_candidate(candidate, search, operand)) return 0;
        } else if (right) {
            set_overload_phase(search, 9);
            ++ranks;
            ++candidate->conversion_index;
            /* The original reads the right type without advancing this cursor. */
            target = (candidate->flags & 0x10) ? candidate->builtin_right_type :
                *candidate->parameter_cursor;
            ranks->parameter_type = target;
            ranks->rank = classify_expression_conversion(target, right, flags, candidate);
            if ((ranks->rank & 0x2000) &&
                reject_overload_candidate(candidate, search, right)) return 0;
        }
    }
    return 1;
}

/* Original 0x77900..0x77a0b, stdcall 12. Builtin winners become
   ordinary expression nodes; function winners retain their member qualifier
   and use the normal resolved-call ABI. */
IrToken *__stdcall make_selected_operator_call(OverloadCandidate *candidate,
    IrToken *left, IrToken *right)
{
    unsigned char operation;
    IrToken *arguments = 0, *object, *converted_left, *converted_right;
    unsigned count;
    if (!candidate->function) {
        operation = candidate->builtin_rule->token;
        converted_left = convert_selected_operator_operand(candidate, 0, left);
        if (operation == OP_POST_INCREMENT || operation == OP_POST_DECREMENT ||
            operation == OP_PRE_INCREMENT || operation == OP_PRE_DECREMENT)
            return builtin_binary_expression(operation, converted_left, make_integer_constant(1));
        if (!right) return builtin_unary_expression(operation, converted_left);
        converted_right = convert_selected_operator_operand(candidate, 1, right);
        if (operation == OP_LOGICAL_AND || operation == OP_LOGICAL_OR)
            converted_right = prepare_expression(converted_right, 3);
        return builtin_binary_expression(operation, converted_left, converted_right);
    }
    if (!(candidate->function->declaration_flags & 0x400)) {
        arguments = append_expression(0, left, 0);
        object = 0;
        count = 1;
    } else {
        object = builtin_unary_expression(OP_BIT_AND, left);
        count = 0;
    }
    if (right) {
        arguments = append_expression(arguments, right, 0);
        ++count;
    }
    if (candidate->qualification)
        bind_selected_overload(candidate->qualification, candidate->function);
    return make_resolved_call(candidate->function, object, arguments, count, 0);
}

/* Original 0x771f0..0x772f5, stdcall 20. Avoid duplicate builtin
   operand pairs, then reserve the same candidate/rank storage used by function
   overloads. Unassigned candidate fields retain their allocator initialization. */
void __stdcall add_builtin_operator_candidate(OverloadSearch *search,
    BuiltinOperator *rule, CType *left, CType *right, unsigned *count)
{
    OverloadCandidate *candidate;
    OverloadArgumentRank *ranks;
    if (!rule) return;
    for (candidate = search->storage; candidate; candidate = candidate->next) {
        if ((candidate->flags & 0x10) &&
            equivalent_operand_types(candidate->builtin_left_type, left) &&
            equivalent_operand_types(candidate->builtin_right_type, right)) return;
    }
    ++*count;
    begin_overload_candidate_cursor(search, &candidate, &ranks);
    candidate = search->current;
    if (candidate->flags & 0x20) {
        candidate->next = candidate + 1;
        candidate->next->previous = candidate;
        candidate->ranks = search->rank_cursor;
        search->rank_cursor += search->argument_count;
        search->current = candidate->next;
    }
    search->current->flags |= 0x20;
    candidate = search->current;
    ranks = search->rank_cursor;
    candidate->ranks = ranks;
    candidate->builtin_rule = rule;
    candidate->builtin_left_type = left;
    candidate->builtin_right_type = right;
    candidate->flags |= 0x10;
    save_overload_candidate_cursor(search, candidate, ranks);
}

/* Original 0x77a10..0x77a9b, stdcall4. Follow overloaded arrows
   until a call yields an ordinary pointer or a nonclass expression. */
IrToken *__stdcall resolve_overloaded_arrow(IrToken *object)
{
    QualifiedDeclarator *qualified;
    OverloadSet *functions;
    TypeShape *shape;
    for (;;) {
        qualified = lookup_class_name(cpp_context_stack->items[object->type->field08],
            name_arrow_operator, 2, 0);
        if (!qualified) return 0;
        functions = (OverloadSet *)qualified->resolution->binding->target;
        object = make_resolved_call(functions->functions,
            builtin_unary_expression(OP_BIT_AND, object), 0, 0, 0);
        if (!cpp_context_stack->items[object->type->field08]) return object;
        shape = object->type->related;
        if (shape && (shape->flags & 0x77) == 1) return object;
    }
}

/* Original 0x77440..0x77454, stdcall4. Probe without diagnostics;
   only the existence of a modifiable storage expression matters here. */
int __stdcall builtin_operand_is_lvalue(IrToken *expression)
{
    return modifiable_lvalue(expression, 0) != 0;
}

/* Original 0x778d0..0x778fc, cdecl12. Only operands with a
   selected conversion member need another call; preserve the other operand. */
IrToken *convert_selected_operator_operand(OverloadCandidate *candidate,
    unsigned index, IrToken *operand)
{
    OverloadArgumentRank *rank = &candidate->ranks[index];
    if (rank->conversion)
        return invoke_user_conversion(rank->conversion, operand, rank->parameter_type);
    return operand;
}
