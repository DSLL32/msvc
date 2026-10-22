/* Identifier lookup at the expression boundary. */
#include "lexer.h"
#include "frontend-records.h"

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
            resolve_deferred_declaration(qualified);
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
        resolve_deferred_declaration(qualified);
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
        type->related->attributes & 0xf77, 0);
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

CType *__stdcall parameter_reference_type(IrToken *reference);
IrToken *__stdcall based_type_expression(CType *type);

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
        expression = postfix_expression(0x46, expression);
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
