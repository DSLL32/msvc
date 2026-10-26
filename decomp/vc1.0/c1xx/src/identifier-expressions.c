/* Identifier lookup at the expression boundary. */
#include "lexer.h"
#include "frontend-records.h"
#include "constructor-initializers.h"
#include "class-lookup.h"
#include "unary-expressions.h"
#include <recon.h>

IrToken *__stdcall resolve_symbol_expression(CppSymbol *symbol);

/* Original 0x3c330..0x3c37c. Ordinary names share the symbol-expression
   path with qualified identifiers, after deferred tag resolution. */
IrToken *__stdcall identifier_expression(char *name)
{
    CppSymbol *symbol = find_outer_name(name);
    if (!symbol) {
        if (name == name_most_derived) return most_derived_expression();
        symbol = recover_undeclared_identifier(name);
        if (!symbol) symbol = new_symbol(name, 0);
    }
    if ((symbol->flags & 0x10) && (symbol->kind & 0x80))
        symbol = find_tag_declaration(symbol);
    return symbol_expression(symbol);
}

/* Original 0x3c380..0x3c3dd. Browser references apply to ordinary symbols
   and qualified declarations, while overload wrappers defer their reference.
   Disabled browsing does not inspect the symbol before handing it onward. */
IrToken *__stdcall symbol_expression(CppSymbol *symbol)
{
    unsigned kind;
    if (browser_filename && (!symbol_scope_mode || options.option_FR)) {
        kind = symbol->kind & 31;
        if (kind != 16 && kind != 17) browser_type_reference(symbol);
    }
    return resolve_symbol_expression(symbol);
}

int __stdcall validate_expression_symbol(CppSymbol *symbol);
IrToken *__stdcall inherited_symbol_expression(CppSymbol *reference, CppSymbol *target);
IrToken *__stdcall enumerator_expression(CppSymbol *symbol);
IrToken *__stdcall finish_symbol_expression(CppSymbol *symbol, IrToken *expression);
CType *__stdcall make_expression_qualified_type(CType *base, unsigned short kind,
    unsigned short attributes, void *parameter);

/* Original 0x3c620..0x3c882. Unwrap qualified declarations, follow static
   member aliases, and form enum, bound-member, data or function values while
   retaining the caller's bare-symbol and inherited-qualification modes. */
IrToken *__stdcall resolve_symbol_expression(CppSymbol *symbol)
{
    CppSymbol *target, *object, *member;
    QualifiedDeclarator *qualified;
    IrToken *value, *member_value, *object_value;
    CType *type;
    unsigned kind;
    for (;;) {
        qualified = 0;
        target = symbol;
        if ((symbol->kind & 31) == 18) {
            qualified = (QualifiedDeclarator *)symbol;
            if (!qualified->resolution) return 0;
            target = (CppSymbol *)qualified->resolution->binding->target;
        }
        kind = target->kind & 31;
        if (kind == 5) {
            if (token_allocation_flags & 0x80) return symbol_reference(symbol);
            check_qualified_member_access(qualified);
            if (symbol->declaration_flags & 0x8000)
                return inherited_symbol_expression(symbol, target);
            return enumerator_expression(target);
        }
        if (kind == 16) return symbol_reference(target);
        if (kind != 2 && kind != 3 && kind != 17) break;
        if (token_allocation_flags & 0x80) return symbol_reference(symbol);
        if (!is_static_data_member(target)) {
            if (!validate_expression_symbol(target)) return 0;
            if ((symbol->declaration_flags & 0x8000) && !(target->declaration_flags & 0x1000))
                return inherited_symbol_expression(symbol, target);
            return symbol_reference(symbol);
        }
        check_qualified_member_access(qualified);
        symbol = static_member_alias(target);
    }
    if (target->flags & 0x10) return symbol_reference(symbol);
    if (kind == 9 && (target->attributes & 0x1000)) {
        object = lookup_symbol_binding(target, &member);
        if (!validate_expression_symbol(target)) return 0;
        qualified = lookup_class_name(cpp_context_stack->items[object->type->field08],
            member->name, 2, 0);
        member_value = symbol_reference((CppSymbol *)qualified);
        object_value = resolve_symbol_expression(object);
        return member_expression(0x59, object_value, member_value);
    }
    if (!validate_expression_symbol(target)) return 0;
    value = symbol_reference(target);
    kind = target->kind & 31;
    if (kind != 6 && kind != 7) {
        if ((target->attributes & 0x400) && kind != 10 && kind != 11) return value;
        if (value->kind == 0xa3) return value;
        return finish_symbol_expression(target, value);
    }
    type = strip_based_shape(target->type);
    if (!type || !type->related) return 0;
    value->type = make_expression_qualified_type(type, 11,
        (unsigned short)(type->related->attributes & 0xf77), 0);
    return value;
}

/* Original 0x3c3e0..0x3c467. Permanent-expression context disallows data
   members and automatic locals, but permits functions and overload sets. */
int __stdcall validate_expression_symbol(CppSymbol *symbol)
{
    unsigned kind, storage;
    if (!(token_allocation_flags & 0x20)) return 1;
    kind = symbol->kind & 31;
    if (kind == 16 || kind == 17 || kind == 6 || kind == 7) return 1;
    if (kind == 2 || kind == 3) {
        report_syntax_error(648, symbol->name);
        return 0;
    }
    storage = symbol->storage[0] & 7;
    if (storage >= 4 && storage <= 6) {
        report_syntax_error(587, symbol->name);
        return 0;
    }
    return 1;
}

/* Original 0x5ef70..0x5f06d, stdcall 4. Convert an array/reference
   expression to its argument pointer model, retaining based-symbol context.
   The original pushes &base before normalizing attributes and leaves it on
   the stack as the fourth argument to replace_first_type_shape. */
CType *__stdcall parameter_reference_type(IrToken *reference)
{
    IrToken *base_expression, *call;
    CppSymbol *symbol, *base;
    TypeShape *shape;
    unsigned short attributes;
    if (reference->kind == 0x59) {
        call = reference->value.pointer;
        if (call->kind == 0x7a && !call->type->related)
            compiler_warning(options.configuration.extensions_enabled ? 4 : 1, 0xdf);
    }
    if ((reference->flags & 7) == 5) return reference->type;
    symbol = find_expression_symbol(reference);
    if (symbol) {
        if (symbol->attributes & 2) {
            shape = symbol->type->related;
            if (!(shape->flags & 0x77) && (shape->attributes & 4)) {
                attributes = shape->attributes;
                base = shape->data.based_symbol;
                goto replace;
            }
        }
        attributes = symbol->type->attributes;
        base = reference->type->related->data.based_symbol;
    } else {
        base_expression = find_expression_type_base(reference);
        if (base_expression && base_expression->type &&
            (shape = base_expression->type->related) != 0 &&
            (shape->flags & 0x77) != 2) {
            attributes = shape->attributes;
            base = shape->data.based_symbol;
        } else {
            base = reference->type->related->data.based_symbol;
            attributes = reference->type->attributes;
        }
    }
replace:
    attributes = normalize_type_attributes(1, attributes);
    return replace_first_type_shape(reference->type, 1, attributes & 15, &base);
}
IrToken *__stdcall based_type_expression(CType *type);

/* Original 0x61310..0x61371, stdcall12. Construct an explicitly
   typed symbol operand without completing or classifying the symbol type. */
IrToken *__stdcall new_symbol_reference(unsigned char operation,
    CppSymbol *symbol, CType *type)
{
    ArenaPool *pool;
    IrToken *reference;
    if (token_allocation_flags & 0x20000020UL)
        reference = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
        reference = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    reference->flags = 4;
    reference->kind = operation;
    reference->value.pointer = symbol;
    reference->type = type;
    return reference;
}

/* Original0x612f0..0x61306, stdcall8. Infer the declared symbol
   type, leaving the explicit constructor's other fields and ABI unchanged. */
IrToken *__stdcall new_declared_symbol_reference(unsigned char operation, CppSymbol *symbol)
{
    return new_symbol_reference(operation, symbol, symbol->type);
}

/* Original 0x61380..0x614f7. Preserve a qualified wrapper in the operand
   while using its target for type classification. Member/overload forms
   deliberately leave type completion to their later resolution path. */
IrToken *__stdcall symbol_reference(CppSymbol *symbol)
{
    ArenaPool *pool;
    IrToken *reference, *based;
    CType *type, *unbased;
    unsigned kind;
    if (!symbol) return 0;
    if (token_allocation_flags & 0x20000020UL)
        reference = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
        reference = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    reference->flags = 4;
    reference->kind = 2;
    reference->value.pointer = symbol;
    if ((symbol->kind & 31) == 18)
        symbol = (CppSymbol *)((QualifiedDeclarator *)symbol)->resolution->binding->target;
    if (symbol->flags & 0x10) {
        reference->flags = 0;
        reference->kind = 0xa3;
        reference->type = symbol->type;
        return reference;
    }
    kind = symbol->kind & 31;
    if (kind == 16 || kind == 17 || kind == 3 || kind == 15 || kind == 2)
        return reference;
    if (kind == 5) {
        reference->type = symbol->type;
        return reference;
    }
    if ((symbol->attributes & 0x400) && (kind == 10 || kind == 11)) {
        reference->type = symbol->type;
        reference->type = parameter_reference_type(reference);
        return reference;
    }
    reference->type = type = symbol->type;
    if (symbol->attributes & 2) {
        if (kind != 7 && (type->related->attributes & 7) >= 5 &&
            (((CppSymbol *)type->related->data.based_symbol)->kind & 31) != 13) {
            token_allocation_flags |= 0x800;
            unbased = drop_first_type_shape(type);
            based = based_type_expression(type);
            return new_ir_expression(0x67, reference, based, unbased);
        }
        reference->type = drop_first_type_shape(type);
    }
    return reference;
}

IrToken *__stdcall indirect_storage_expression(IrToken *reference);

/* Original 0x3c890..0x3c8ba. A data symbol first becomes an indirect
   storage operand; reference declarations require a further language-level
   dereference through the general unary-expression path. */
IrToken *__stdcall finish_symbol_expression(CppSymbol *symbol, IrToken *expression)
{
    expression = indirect_storage_expression(expression);
    if (symbol_has_reference_shape(symbol))
        expression = builtin_unary_expression(0x46, expression);
    return expression;
}

IrToken *__stdcall new_typed_unary_expression(unsigned char operation, CType *type, IrToken *operand);
IrToken *__stdcall qualify_based_storage(IrToken *expression);

/* Original 0x5d9d0..0x5d9fd. Internal storage indirection retains the
   declared value type. Based-pointer adjustment is enabled by the shared
   expression-allocation mode, rather than by this node's flags. */
IrToken *__stdcall indirect_storage_expression(IrToken *reference)
{
    IrToken *expression;
    if (!reference) return 0;
    expression = new_typed_unary_expression(0x46, reference->type, reference);
    if (token_allocation_flags & 0x800)
        expression = qualify_based_storage(expression);
    return expression;
}

/* Original 0x5da00..0x5da61. A typed unary node uses the expression arena
   unless permanent or extended-permanent expression allocation is active.
   Other node fields remain untouched, matching the original constructor. */
IrToken *__stdcall new_typed_unary_expression(unsigned char operation, CType *type, IrToken *operand)
{
    ArenaPool *pool;
    IrToken *expression;
    if (token_allocation_flags & 0x20000020UL)
        expression = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
        expression = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    expression->flags = 1;
    expression->kind = operation;
    expression->value.pointer = operand;
    expression->type = type;
    return expression;
}

IrToken *__stdcall convert_based_expression(CType *type, IrToken *expression);

/* Original 0x39dc0..0x39e46. Argument/declaration expression lists retain
   a circular head link alongside the reverse operand chain. The contained
   expression points back to its list node through the same linkage word. */
IrToken *__stdcall append_expression(IrToken *list, IrToken *expression, int mode)
{
    CType *type;
    IrToken *node;
    expression = validate_value_expression(expression);
    type = mode == 1 ? type_0000 : (expression ? expression->type : 0);
    if (expression && expression->type && expression->type->related &&
        (expression->type->related->attributes & 4))
        expression = convert_based_expression(type, expression);
    node = new_ir_expression(0x6a, expression, list, type);
    if (expression && (expression->flags & 8)) node->flags |= 8;
    if (!list) node->linkage.list_head = node;
    else {
        node->linkage.list_head = list->linkage.list_head;
        list->linkage.list_head = node;
    }
    if (expression) expression->linkage.list_head = node;
    return node;
}

/* Original 0x3b280..0x3b346. Check the underlying value after deferred
   expression wrappers, propagating diagnostic state to both wrapper and
   operand. A qualified overload set may remain unresolved at this point. */
IrToken *__stdcall validate_value_expression(IrToken *expression)
{
    IrToken *value;
    CppSymbol *symbol;
    QualifierBinding *binding;
    if (!expression) return 0;
    if (expression->flags & 8) return expression;
    value = expression;
    while ((value->flags & 7) == 7) {
        if (value->kind >= 0xc8 && value->kind <= 0xca)
            value = value->value.pointer;
    }
    if (!(value->flags & 7) && value->kind == 0xa3) {
        report_type_error(0x113, value->type);
        value->flags |= 8;
        expression->flags |= 8;
        return expression;
    }
    if (value->kind == 2) {
        symbol = value->value.pointer;
        if ((symbol->kind & 31) == 18) {
            binding = ((QualifiedDeclarator *)symbol)->resolution->binding;
            if ((binding->target->kind & 31) != 17) {
                report_syntax_error(0x255, format_qualified_name(binding, 0));
                value->flags |= 8;
                expression->flags |= 8;
                return expression;
            }
        }
    }
    return expression;
}

/* Original 0x61280..0x612e8. Binary/list nodes have two expression
   operands; the right operand shares the final word with literal metadata. */
IrToken *__stdcall new_ir_expression(unsigned char operation, IrToken *left, IrToken *right, CType *type)
{
    ArenaPool *pool;
    IrToken *expression;
    if (token_allocation_flags & 0x20000020UL)
        expression = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
        expression = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    expression->flags = 2;
    expression->kind = operation;
    expression->value.pointer = left;
    expression->tail.right = right;
    expression->type = type;
    return expression;
}

/* Original 0x36610..0x36663. Constructor initializers retain any trailing
   symbol records on their expression before joining the circular list. */
IrToken *__stdcall append_constructor_initializer(IrToken *list, IrToken *initializer)
{
    ExpressionSymbolList *symbols;
    IrToken *node;
    if (!initializer) return list;
    symbols = detach_expression_symbols();
    if (symbols) initializer = attach_expression_symbols(initializer, symbols);
    node = new_ir_expression(0x6a, initializer, list, 0);
    if (!list) node->linkage.list_head = node;
    else {
        node->linkage.list_head = list->linkage.list_head;
        list->linkage.list_head = node;
    }
    initializer->linkage.list_head = node;
    return node;
}

/* Original 0x61240..0x6125a. The operation preserves its left operand's
   type while retaining the ordinary two-operand node constructor. */
IrToken *__stdcall new_left_typed_expression(unsigned char operation, IrToken *left, IrToken *right)
{
    return new_ir_expression(operation, left, right, left->type);
}

/* Original0x5cfd0..0x5cfdc, cdecl. Retain the ordinary zero constant
   owner and mark its original recovery bit. */
IrToken *make_error_expression(void)
{
    IrToken *expression = make_integer_constant(0);
    expression->flags |= 8;
    return expression;
}

/* Original 0x3c470..0x3c5a8. Implicit class qualification selects a member
   through this when a nonstatic member function provides an object. Preserve
   the qualified wrapper's identity for self-linked inherited lookup paths. */
IrToken *__stdcall inherited_symbol_expression(CppSymbol *reference, CppSymbol *target)
{
    IrToken *object, *member, *result;
    QualifiedDeclarator *qualified;
    QualifierBinding *binding, *following;
    unsigned kind;
    if (!function_symbol) {
        kind = target->kind & 0x1f;
        if (kind == 17) return symbol_reference(reference);
        if (kind == 5) {
            check_qualified_member_access(reference);
            return enumerator_expression(target);
        }
        report_syntax_error(654, target->name);
        return 0;
    }
    if (!(function_symbol->cpp.parts.member_flags & 1)) {
        kind = target->kind & 0x1f;
        if (kind == 5) {
            check_qualified_member_access(reference);
            return enumerator_expression(target);
        }
        if (kind == 17 || kind == 3) return symbol_reference(reference);
        report_syntax_error(597, target->name);
        return make_error_expression();
    }
    object = adjusted_this_expression(0);
    member = symbol_reference(reference);
    result = member_expression(0x5c, object, member);
    if (result->kind == 0x46 && (reference->kind & 0x1f) == 18) {
        qualified = (QualifiedDeclarator *)reference;
        following = qualified->resolution->binding->following;
        if (following && following->following == following) {
            result->tail.symbol = reference;
            return result;
        }
    }
    if (result->kind == 0x5c && (reference->kind & 0x1f) == 18) {
        qualified = (QualifiedDeclarator *)reference;
        binding = qualified->resolution->binding;
        if ((binding->target->kind & 0x1f) == 17) {
            following = binding->following;
            if (following && following->following == following)
                result->tail.right->tail.symbol = reference;
        }
    }
    return result;
}

typedef char QualifierFollowingOffsetMustBe4[
    (offsetof(QualifierBinding, following) == 4) ? 1 : -1];
typedef char QualifiedResolutionOffsetMustBe12[
    (offsetof(QualifiedDeclarator, resolution) == 12) ? 1 : -1];

/* Original0x39f80..0x39f9f, stdcall4. Remove the ordered address,
   member-selection and indirection wrappers from an untyped designator. */
IrToken *__stdcall strip_function_designator(IrToken *expression)
{
    if (expression->kind == 0x3a) expression = expression->value.pointer;
    if (expression->kind == 0x5c) expression = expression->tail.right;
    if (expression->kind == 0x46) expression = expression->value.pointer;
    return expression;
}
IrToken *__stdcall resolve_qualified_assignment_operand(CType *type, QualifiedDeclarator *reference);

/* Original 0x39e50..0x39f73. Typed values pass through unchanged; untyped
   function/member designators select the overload matching the target type. */
IrToken *__stdcall resolve_assignment_operand(CType *type, IrToken *expression)
{
    CppSymbol *symbol;
    TypeShape *shape;
    CType *function_type;
    unsigned kind;
    if (!expression) return 0;
    if (expression->type) return expression;
    expression = strip_function_designator(expression);
    symbol = expression->value.pointer;
    if (symbol->declaration_flags & 0x1000) {
        report_syntax_error(277, format_symbol_name(symbol));
        return 0;
    }
    kind = symbol->kind & 0x1f;
    if (kind == 18)
        return resolve_qualified_assignment_operand(type, (QualifiedDeclarator *)symbol);
    if (kind != 16 && kind != 17) return expression;
    if (has_single_function(symbol)) symbol = ((OverloadSet *)symbol)->functions;
    else {
        if (!type || !type->related) return 0;
        shape = type->related;
        if ((shape->flags & 0x77) != 3 && (shape->flags & 0x44) != 0x44) return 0;
        function_type = drop_first_type_shape(type);
        symbol = find_matching_overload(symbol, function_type, 1);
    }
    if (!symbol) {
        report_syntax_error(563);
        return 0;
    }
    if (browser_filename) browser_type_reference(symbol);
    expression->value.pointer = symbol;
    function_type = strip_based_shape(symbol->type);
    expression->type = make_expression_qualified_type(function_type, 0xb,
        (unsigned short)(function_type->related->attributes & 0xf77), 0);
    return expression;
}

/* Original 0x625f0..0x62616, stdcall4. Function designators retain
   the normalized calling convention/distance under an expression-local shape. */
CType *__stdcall function_designator_type(CType *type)
{
    unsigned short attributes = normalize_type_attributes(0xb,
        (unsigned short)(type->related->attributes & 0xf77));
    return make_expression_qualified_type(type, 0xb, attributes, 0);
}

IrToken *__stdcall make_most_derived_test(IrToken *object);

/* Original0x7c120..0x7c12e, cdecl. Use adjusted this before constructing
   the target's virtual-base identity test. */
IrToken *most_derived_expression(void)
{
    return make_most_derived_test(adjusted_this_expression(0));
}

/* Original0x39fb0..0x3a06e, stdcall8. Qualified assignment needs a
   member overload: bind a unique function or match the expected member type,
   then retain that exact binding for the member-pointer expression. */
IrToken *__stdcall resolve_qualified_assignment_operand(CType *type,
    QualifiedDeclarator *reference)
{
    QualifierBinding *binding = reference->resolution->binding;
    CppSymbol *symbol = (CppSymbol *)binding->target;
    TypeShape *shape;
    if ((symbol->kind & 31) != 17) {
        report_syntax_error(597, format_qualified_name(binding, 0));
        return make_error_expression();
    }
    if (has_single_function(symbol)) symbol = ((OverloadSet *)symbol)->functions;
    else {
        if (!type || !(shape = type->related)) return make_error_expression();
        if (!(shape->flags & 0x20) && (shape->flags & 0x77) != 3)
            return make_error_expression();
        symbol = find_matching_overload(symbol, drop_first_type_shape(type), 1);
    }
    if (!symbol) {
        report_syntax_error(563);
        return make_error_expression();
    }
    binding->target = (ClassReference *)symbol;
    return member_pointer_expression(reference);
}

/* Original 0x3c2b0..0x3c32a. A prior class-lookup diagnostic suppresses
   one duplicate; otherwise insert the ordinary recovery symbol in its real
   extension-dependent scope and report its browser declaration. */
CppSymbol *__stdcall recover_undeclared_identifier(char *name)
{
    CppSymbol *symbol;
    if (token_allocation_flags & 0x80)
        return 0;
    if (class_lookup_error) {
        class_lookup_error = 0;
        return 0;
    }
    report_syntax_error(65, name);
    symbol = new_symbol(name, options.configuration.extensions_enabled ? 0 : declaration_mode);
    if (browser_filename && (!symbol_scope_mode || options.option_FR))
        browser_declaration(symbol);
    return symbol;
}

/* Original0x61160..0x611c7, stdcall8. Deferred default arguments
   retain their detached symbol list without inventing a declared result type. */
IrToken *__stdcall wrap_default_argument_symbols(IrToken *expression,
    ExpressionSymbolList *symbols)
{
    IrToken *result;
    ArenaPool *pool;
    if (token_allocation_flags & 0x20000020UL)
        result = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
        result = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    result->flags = 7;
    result->kind = 0xc9;
    result->value.pointer = expression;
    result->tail.right = (IrToken *)symbols;
    result->type = 0;
    return result;
}

/* Original0x610f0..0x61157, stdcall8. Constructor initializer
   expressions retain the same symbol-list owner under their distinct tag. */
IrToken *__stdcall attach_expression_symbols(IrToken *expression,
    ExpressionSymbolList *symbols)
{
    IrToken *result;
    ArenaPool *pool;
    if (token_allocation_flags & 0x20000020UL)
        result = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
        result = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    result->flags = 7;
    result->kind = 0xc8;
    result->value.pointer = expression;
    result->tail.right = (IrToken *)symbols;
    result->type = 0;
    return result;
}
