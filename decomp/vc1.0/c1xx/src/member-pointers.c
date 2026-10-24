/* C++ pointers to nonstatic members and their inheritance representations. */
#include "lexer.h"
#include "call-expressions.h"
#include "expression-records.h"
#include "special-members.h"
#include "numeric-conversion.h"
#include "constructor-initializers.h"
#include "parameters.h"
#include "member-pointers.h"
#include "member-expressions.h"
#include "hash-table.h"
#include "function-finalization.h"
#include "global-ir.h"
#include <recon.h>

int __stdcall equal_data_member_null_representation(CType *target, CType *source);
int __stdcall recognize_compact_method_function(IrToken *expression, IrToken **function);
IrToken *__stdcall member_pointer_function_value(IrToken *storage, IrToken *expression);
IrToken *__stdcall member_pointer_adjustment_value(IrToken *storage, IrToken *expression);
IrToken *__stdcall adjust_member_pointer_displacement(IrToken *adjustment, int displacement);
IrToken *__stdcall member_pointer_component(IrToken *expression, int component, IrToken **saved);
IrToken *__stdcall preserve_null_data_member_conversion(IrToken *converted,
    IrToken *original, CType *source);

typedef IrToken *(__stdcall *MemberPointerConversion)(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_compact_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_compact_to_virtual_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_compact_to_general_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_virtual_to_compact_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_virtual_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_virtual_to_general_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_general_to_compact_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_general_to_virtual_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_general_method_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_compact_data_member_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_compact_to_virtual_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_compact_to_general_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_virtual_to_compact_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_virtual_data_member_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_virtual_to_general_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_general_to_compact_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_general_to_virtual_data_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);
IrToken *__stdcall convert_general_data_member_pointer(IrToken *expression, CType *type, QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base);

/* Original 0xa0b38..0xa0bb7: method/data, source model1..4,
   target model1..4. The machine uses a base five words before this table;
   subtracting one from each model recovers the same32 slots. */
MemberPointerConversion member_pointer_conversions[2][4][4] = {
    {
        { convert_compact_method_pointer, convert_compact_method_pointer, convert_compact_to_virtual_method_pointer, convert_compact_to_general_method_pointer },
        { convert_compact_method_pointer, convert_compact_method_pointer, convert_compact_to_virtual_method_pointer, convert_compact_to_general_method_pointer },
        { convert_virtual_to_compact_method_pointer, convert_virtual_to_compact_method_pointer, convert_virtual_method_pointer, convert_virtual_to_general_method_pointer },
        { convert_general_to_compact_method_pointer, convert_general_to_compact_method_pointer, convert_general_to_virtual_method_pointer, convert_general_method_pointer },
    },
    {
        { convert_compact_data_member_pointer, convert_compact_data_member_pointer, convert_compact_to_virtual_data_pointer, convert_compact_to_general_data_pointer },
        { convert_compact_data_member_pointer, convert_compact_data_member_pointer, convert_compact_to_virtual_data_pointer, convert_compact_to_general_data_pointer },
        { convert_virtual_to_compact_data_pointer, convert_virtual_to_compact_data_pointer, convert_virtual_data_member_pointer, convert_virtual_to_general_data_pointer },
        { convert_general_to_compact_data_pointer, convert_general_to_compact_data_pointer, convert_general_to_virtual_data_pointer, convert_general_data_member_pointer },
    },
};

IrToken *__stdcall make_multiple_inheritance_method_pointer(CType *type,
    IrToken *function, IrToken *adjustment);
CppSymbol *__stdcall enclosing_qualifier_class(QualifierBinding *binding);
int __stdcall resolve_member_pointer_path(QualifierBinding *path,
    BaseClassLink **virtual_base, int *displacement);
CppSymbol *__stdcall member_pointer_owner_class(CType *type);
unsigned __stdcall class_virtual_base_pointer_offset(CppSymbol *tag);
CppSymbol *__stdcall member_pointer_virtual_function(CppSymbol *member);
IrToken *__stdcall make_compact_method_pointer(CType *type, IrToken *function,
    IrToken *adjustment);
IrToken *__stdcall make_virtual_method_pointer(CType *type, IrToken *function,
    IrToken *adjustment, BaseClassLink *virtual_base);
IrToken *__stdcall make_general_method_pointer(CType *type, IrToken *function,
    IrToken *adjustment, int vbptr_offset, IrToken *vbtable_offset);
IrToken *__stdcall make_virtual_data_member_pointer(CType *type, IrToken *offset,
    IrToken *virtual_offset);
IrToken *__stdcall make_general_data_member_pointer(CType *type, IrToken *offset,
    int vbptr_offset, IrToken *vbtable_offset);

int __stdcall convert_member_pointer_object(IrToken *expression);
IrToken *__stdcall apply_simple_data_member_pointer(IrToken *expression);
IrToken *__stdcall apply_virtual_data_member_pointer(IrToken *expression);
IrToken *__stdcall apply_general_data_member_pointer(IrToken *expression);

IrToken *__stdcall lower_multiple_inheritance_member_call(IrToken *object,
    IrToken *member, CType *type, IrToken **prefix);
IrToken *__stdcall lower_virtual_inheritance_member_call(IrToken *object,
    IrToken *member, CType *type, IrToken **prefix);
IrToken *__stdcall lower_general_inheritance_member_call(IrToken *object,
    IrToken *member, CType *type, IrToken **prefix);

int __stdcall data_member_pointer_uses_zero_null(CType *type);
CType *__stdcall member_pointer_function_type(CType *type);
IrToken *__stdcall make_null_member_function(CType *type);
IrToken *__stdcall make_null_compact_method_pointer(CType *type);
IrToken *__stdcall make_null_simple_data_pointer(CType *type);
IrToken *__stdcall make_null_virtual_data_pointer(CType *type);
IrToken *__stdcall make_null_general_data_pointer(CType *type);
IrToken *__stdcall make_null_virtual_method_pointer(CType *type);
IrToken *__stdcall make_null_general_method_pointer(CType *type);

int __stdcall tag_has_base_relation(CppSymbol *derived, CppSymbol *base);
int __stdcall equal_member_function_parameters(FunctionParameters *source,
    FunctionParameters *target);

/* Original 0x4e0c0..0x4e127. Virtual/general member pointers and
   multiple-inheritance method pointers need aggregate storage. */
unsigned char __stdcall member_pointer_is_aggregate(CType *type)
{
    TypeShape *shape = type->related;
    unsigned model;
    if (!shape) return 0;
    if (!(shape->flags & 0x77)) shape = shape->related;
    if (!shape || !(shape->flags & 0x30)) return 0;
    model = member_pointer_inheritance_model(type);
    switch (model) {
    case 2: return (type->related->flags & 0x20) != 0;
    case 3: case 4: return 1;
    default: return 0;
    }
}

/* Original 0x4df40..0x4df98. The cache retains a separate key
   with the original allocation policy and all three defined words. */
MemberPointerThunkKey *__stdcall copy_member_pointer_thunk_key(MemberPointerThunkKey *key,
    short arena)
{
    MemberPointerThunkKey *copy;
    ArenaPool *pool;
    if (!arena) copy = allocate_permanent(sizeof(MemberPointerThunkKey));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(MemberPointerThunkKey))
            refill_arena(&compiler_arena, pool);
        copy = (MemberPointerThunkKey *)pool->next;
        pool->remaining -= sizeof(MemberPointerThunkKey);
        pool->next += sizeof(MemberPointerThunkKey);
    }
    *copy = *key;
    return copy;
}

/* Original 0x4dc90..0x4dd52. Clone a virtual member's symbol with
   the generated-function identity; finish_declarations emits it later. */
CppSymbol *__stdcall emit_member_pointer_virtual_thunk(MemberPointerThunkKey *key)
{
    CppSymbol *thunk = allocate_permanent(sizeof(CppSymbol));
    unsigned char storage_kind;
    *thunk = *key->function;
    thunk->declaration_flags &= ~0x400;
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    thunk->index = next_symbol_index++;
    thunk->next = 0;
    thunk->cpp.parts.member_qualifiers |= 6;
    thunk->this_displacement = key->displacement;
    thunk->cpp.parts.member_qualifiers =
        (thunk->cpp.parts.member_qualifiers & ~0x38) | ((key->kind << 3) & 0x38);
    thunk->flags |= 4;
    thunk->storage[0] = (thunk->storage[0] & 0xf9) | 1;
    storage_kind = thunk->storage[0] & 7;
    thunk->storage[0] = declaration_storage[0];
    thunk->storage[1] = declaration_storage[1];
    thunk->storage[2] = declaration_storage[2];
    thunk->storage[0] = (thunk->storage[0] & 0xf8) | storage_kind;
    thunk->storage[0] |= 0x40;
    return thunk;
}

/* Original 0x4dfa0..0x4dfab, cdecl contextual hash callback.
   Member identity is deliberately absent from this cache's hash. */
unsigned __cdecl hash_tail_word_pair(void *key)
{
    MemberPointerThunkKey *pair = key;
    return pair->kind + (unsigned)pair->displacement;
}

/* Original 0x4dee0..0x4df37. Lookup keys use the requested compiler
   arena; arena zero retains a permanent key and -1 selects pool zero. */
MemberPointerThunkKey *__stdcall new_member_pointer_thunk_key(CppSymbol *member,
    short arena)
{
    MemberPointerThunkKey *key;
    ArenaPool *pool;
    if (!arena) key = allocate_permanent(sizeof(MemberPointerThunkKey));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(MemberPointerThunkKey))
            refill_arena(&compiler_arena, pool);
        key = (MemberPointerThunkKey *)pool->next;
        pool->remaining -= sizeof(MemberPointerThunkKey);
        pool->next += sizeof(MemberPointerThunkKey);
    }
    key->function = member;
    key->kind = 0;
    key->displacement = member->detail.thunk_virtual_displacement;
    return key;
}

/* Original 0x4dbf0..0x4dc51. Reuse a virtual member's generated
   function, keeping an independent permanent key for a new cache entry. */
CppSymbol *__stdcall member_pointer_virtual_function(CppSymbol *member)
{
    MemberPointerThunkKey *key = new_member_pointer_thunk_key(member, 2);
    CppSymbol *function;
    if (!declaration_thunks)
        declaration_thunks = new_hash_table(32, 0, 5, 4);
    function = find_hash_value(declaration_thunks, key);
    if (!function) {
        function = emit_member_pointer_virtual_thunk(key);
        set_hash_value(declaration_thunks, copy_member_pointer_thunk_key(key, 0), function);
    }
    return function;
}

/* Original 0x64da0..0x64ee8. Rank conversion between member-pointer
   shapes, their class relation and any nested member-function signature. */
unsigned __stdcall classify_member_pointer_conversion(CType *source, CType *target,
    int reverse)
{
    TypeShape *source_shape = source->related, *target_shape = target->related;
    unsigned short source_flags = source_shape->flags, target_flags = target_shape->flags;
    unsigned short source_attributes = source_shape->attributes;
    unsigned short target_attributes = target_shape->attributes, difference;
    FunctionParameters *source_parameters;
    CppSymbol *source_class = source_shape->data.tag_symbol;
    CppSymbol *target_class = target_shape->data.tag_symbol;

    if (source == target) return 1;
    if (((source->flags ^ target->flags) & 0xefff) ||
        source->field08 != target->field08 || ((source_flags ^ target_flags) & 0x10))
        return 0x2000;
    difference = source_attributes ^ target_attributes;
    if (difference & 7) return 0x2007;
    if (difference & 0x70) return 0x2008;
    if (reverse) {
        if (!tag_has_base_relation(source_class, target_class)) return 0x2000;
    } else if (!tag_has_base_relation(target_class, source_class)) return 0x2000;
    if ((target_flags & 0x20) && source_shape->related != target_shape->related) {
        source_parameters = source_shape->related->data.parameters;
        if (!equal_member_function_parameters(source_parameters,
            target_shape->related->data.parameters)) return 0x2000;
    }
    return source_class == target_class ? 1 : 0x20;
}

/* Original 0x4a200..0x4a256. Sum nonvirtual base displacements,
   or return the first virtual base for its separate adjustment. */
int __stdcall resolve_member_pointer_path(QualifierBinding *path,
    BaseClassLink **virtual_base, int *displacement)
{
    BaseClassLink *base;
    *displacement = 0;
    *virtual_base = 0;
    for (; path; path = path->next) {
        base = (BaseClassLink *)path->target;
        if ((base->kind & 31) == 4) break;
        if (base->state & 1) {
            *virtual_base = base;
            return 0;
        }
        *displacement += base->displacement;
    }
    return 1;
}

/* Original 0x4a260..0x4a29c. A zero-adjustment class needs only
   the function address; multiple inheritance stores the adjustment too. */
IrToken *__stdcall make_compact_method_pointer(CType *type, IrToken *function,
    IrToken *adjustment)
{
    unsigned model = member_pointer_inheritance_model(type);
    if (model == 1) return new_cast_expression(type, function);
    if (model == 2)
        return make_multiple_inheritance_method_pointer(type, function, adjustment);
    return (IrToken *)model;
}

/* Original 0x4dfe0..0x4dff9. An optional leading qualifier precedes
   the member-pointer shape whose data holds the owner class. */
CppSymbol *__stdcall member_pointer_owner_class(CType *type)
{
    TypeShape *shape = type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    return shape->data.tag_symbol;
}

/* Original 0x49e10..0x49e23. Resolve the class embedded in the
   member-pointer type and obtain its fixed inheritance representation. */
unsigned __stdcall member_pointer_inheritance_model(CType *type)
{
    return member_pointer_class_kind(member_pointer_owner_class(type));
}

/* Original 0x49fc0..0x4a1f7. Construct the expression using the class's
   fixed compact, virtual-base or general member-pointer representation. */
IrToken *__stdcall member_pointer_expression(QualifiedDeclarator *qualifier)
{
    QualifierBinding *binding, *scope;
    CppSymbol *member, *owner, *target;
    BaseClassLink *virtual_base = 0;
    int displacement = 0, model;
    unsigned offset;
    CType *type, *base_type;
    IrToken *expression, *first, *second, *unsupported_result;

    binding = qualifier->resolution->binding;
    scope = binding->next;
    member = (CppSymbol *)binding->target;
    if ((member->kind & 31) == 17) return 0;
    check_qualified_member_access(qualifier);
    owner = (CppSymbol *)scope->target;
    if ((owner->kind & 31) != 4)
        owner = cpp_context_stack->items[((BaseClassLink *)owner)->context_index];
    if (symbol_has_reference_shape(member))
        report_syntax_error(634, enclosing_qualifier_class(binding)->name, member->name);
    if ((member->storage[0] & 7) == 3) return resolve_symbol_expression(member);
    if (member->storage[1] & 2)
        resolve_member_pointer_path(find_member_call_binding(1, owner, member, 0),
            &virtual_base, &displacement);
    if ((member->kind & 31) == 7) {
        defer_generated_member(member);
        base_type = member->type;
        if (member->attributes & 2) base_type = drop_first_type_shape(base_type);
        type = make_qualified_type(base_type, 0x20,
            base_type->related->attributes & 0xf77, &owner);
    } else type = make_qualified_type(member->type, 0x10, 0, &owner);

    if ((member->kind & 31) == 3) {
        model = member_pointer_inheritance_model(type);
        switch (model) {
        case 1: case 2:
            expression = new_integer_constant(member->detail.value);
            expression->type = type;
            return expression;
        case 3:
            offset = member->detail.value -
                class_virtual_base_pointer_offset(member_pointer_owner_class(type));
            first = make_integer_constant(offset);
            second = make_integer_constant(0);
            return make_virtual_data_member_pointer(type, first, second);
        case 4:
            second = make_integer_constant(0);
            first = make_integer_constant(member->detail.value);
            return make_general_data_member_pointer(type, first, 0, second);
        default:
#ifdef RECON_NATIVE_TEST
            /* Original 0x4a0e4 reads the unwritten entryESP-4 local.
               Preserve that Win32 path; native equivalence is unsupported. */
            __asm { int 3 }
#endif
            return unsupported_result;
        }
    }
    target = member;
    if (target->storage[1] & 2) target = member_pointer_virtual_function(target);
    expression = symbol_reference(target);
    model = member_pointer_inheritance_model(type);
    switch (model) {
    case 1: case 2:
        first = make_integer_constant(displacement);
        return make_compact_method_pointer(type, expression, first);
    case 3:
        first = make_integer_constant(displacement);
        return make_virtual_method_pointer(type, expression, first, virtual_base);
    case 4:
        second = make_integer_constant(0);
        first = make_integer_constant(displacement);
        return make_general_method_pointer(type, expression, first, 0, second);
    default:
#ifdef RECON_NATIVE_TEST
        /* Same unwritten local, read at original 0x4a18a. */
        __asm { int 3 }
#endif
        return unsupported_result;
    }
}

/* Original 0x4a910..0x4a9cf, stdcall4. Normalize object/member
   application before selecting its data-member inheritance representation.
   Function members retain this expression for later call lowering. */
IrToken *__stdcall apply_member_pointer(IrToken *expression)
{
    IrToken *left = (IrToken *)expression->value.pointer;
    IrToken *right = expression->tail.right;
    unsigned char operation = expression->kind;
    if (operation == 0x9d) {
        expression->kind = 0x9e;
        expression->value.pointer = builtin_unary_expression(0x3a, left);
    }
    if (!convert_member_pointer_object(expression)) {
        report_type_conversion(647, 0, operation, right->type, left->type);
        return 0;
    }
    if (right->type->related->flags & 0x10) {
        switch (member_pointer_inheritance_model(right->type)) {
        case 1:
        case 2:
            return apply_simple_data_member_pointer(expression);
        case 3:
            return apply_virtual_data_member_pointer(expression);
        case 4:
            return apply_general_data_member_pointer(expression);
        default:
            return 0;
        }
    }
    expression->type = 0;
    return expression;
}

/* Original 0x4b1e0..0x4b25f, stdcall4. The object's pointer shape
   is retained when a member requires conversion to its owning class. */
int __stdcall convert_member_pointer_object(IrToken *expression)
{
    CType *object_type = ((IrToken *)expression->value.pointer)->type;
    CppSymbol *owner = expression->tail.right->type->related->data.tag_symbol;
    TypeShape *pointer;
    CType *target;
    if (owner == cpp_context_stack->items[object_type->field08]) return 1;
    pointer = object_type->related;
    target = make_qualified_type(owner->type, pointer->flags,
        pointer->attributes, &pointer->data);
    if (!can_initialize(target, (IrToken *)expression->value.pointer)) return 0;
    expression->value.pointer = convert_initializer(1, target,
        (IrToken *)expression->value.pointer);
    return expression->value.pointer != 0;
}

/* Original 0x4ad80..0x4ae16, stdcall8. Separate evaluation required
   before an indirect member call from its object and callable expression. */
IrToken *__stdcall lower_member_pointer_call(IrToken *expression, IrToken **prefix)
{
    IrToken *member = expression->tail.right;
    IrToken *object = (IrToken *)expression->value.pointer;
    CType *type = member->type;
    CType *object_type;
    *prefix = 0;
    switch (member_pointer_inheritance_model(type)) {
    case 1:
        object_type = type->related->related->data.parameters->associated_type;
        return new_left_typed_expression(0xa4, member,
            new_typed_unary_expression(0x46, object_type, object));
    case 2:
        return lower_multiple_inheritance_member_call(object, member, type, prefix);
    case 3:
        return lower_virtual_inheritance_member_call(object, member, type, prefix);
    case 4:
        return lower_general_inheritance_member_call(object, member, type, prefix);
    default:
        return 0;
    }
}

/* Original 0x4b440..0x4b4d9, stdcall4. Single-word representations
   compare directly; aggregate representations compare their component fields. */
IrToken *__stdcall compare_member_pointer_expression(IrToken *expression)
{
    CType *type = ((IrToken *)expression->value.pointer)->type;
    unsigned data_member = type->related->flags & 0x10;
    switch (member_pointer_inheritance_model(type)) {
    case 1:
        return expression;
    case 2:
        if (data_member) return expression;
        return dispatch_member_pointer_comparison(expression,
            compare_multiple_method_pointer);
    case 3:
        return dispatch_member_pointer_comparison(expression,
            data_member ? compare_virtual_data_pointer : compare_virtual_method_pointer);
    case 4:
        return dispatch_member_pointer_comparison(expression,
            data_member ? compare_general_data_pointer : compare_general_method_pointer);
    default:
        return 0;
    }
}

/* Original 0x4b260..0x4b2e2, stdcall4. Null representations depend
   on both the inheritance model and whether this is a data member. */
IrToken *__stdcall make_null_member_pointer(CType *type)
{
    unsigned data_member = (type->related->flags & 0x10) != 0;
    switch (member_pointer_inheritance_model(type)) {
    case 1:
    case 2:
        return data_member ? make_null_simple_data_pointer(type) :
            make_null_compact_method_pointer(type);
    case 3:
        return data_member ? make_null_virtual_data_pointer(type) :
            make_null_virtual_method_pointer(type);
    case 4:
        return data_member ? make_null_general_data_pointer(type) :
            make_null_general_method_pointer(type);
    default:
        return 0;
    }
}

/* Original 0x4b2f0..0x4b30d, stdcall4. Create the adjustment before
   the function-address zero, retaining the original allocation order. */
IrToken *__stdcall make_null_compact_method_pointer(CType *type)
{
    IrToken *adjustment = make_integer_constant(0);
    IrToken *function = make_null_member_function(type);
    return make_compact_method_pointer(type, function, adjustment);
}

/* Original 0x4b310..0x4b32c, stdcall4. The function-address zero
   retains ordinary null flags while receiving its callable pointer type. */
IrToken *__stdcall make_null_member_function(CType *type)
{
    IrToken *function = make_integer_constant(0);
    function->type = member_pointer_function_type(type);
    return function;
}

/* Original 0x4bcf0..0x4bd13, stdcall4. Replace the member-pointer
   shape with an ordinary callable pointer, clearing member-only attributes. */
CType *__stdcall member_pointer_function_type(CType *type)
{
    type = drop_first_type_shape(type);
    return make_qualified_type(type, 3, type->related->attributes & 0x0f77, 0);
}

/* Original 0x4b330..0x4b34c, stdcall4. This constructor deliberately
   uses the plain integer constructor: even zero has no null-literal flag. */
IrToken *__stdcall make_null_simple_data_pointer(CType *type)
{
    IrToken *value = new_integer_constant(
        data_member_pointer_uses_zero_null(type) ? 0 : (unsigned)-1);
    value->type = type;
    return value;
}

/* Original 0x4b350..0x4b369, stdcall4. The owning class fixes
   which integer representation is reserved for a null data-member pointer. */
int __stdcall data_member_pointer_uses_zero_null(CType *type)
{
    CppSymbol *owner = member_pointer_owner_class(type);
    return (owner->detail.class_info->alignment_flags & 0x40) != 0;
}

/* Original 0x4a9d0..0x4a9f6, stdcall4. Interpret the member offset
   as pointer addition, then dereference using the member's unwrapped type. */
IrToken *__stdcall apply_simple_data_member_pointer(IrToken *expression)
{
    CType *type = intern_type((TypeDescription *)drop_first_type_shape(
        expression->tail.right->type));
    expression->kind = 0x5a;
    return new_typed_unary_expression(0x46, type, expression);
}

/* Original 0x4be50..0x4bede, stdcall16. Select the conversion for
   the source/target representations after resolving its base-class path.
   This case establishes model1..4 only; other model values remain unvalidated. */
IrToken *__stdcall convert_member_pointer_representation(IrToken *expression,
    CType *type, QualifiedDeclarator *path, int forward)
{
    unsigned target_model = member_pointer_inheritance_model(type);
    unsigned source_model = member_pointer_inheritance_model(expression->type);
    unsigned data_member = (type->related->flags & 0x10) != 0;
    QualifierBinding *binding = path->resolution->binding;
    BaseClassLink *virtual_base;
    int displacement;
    resolve_member_pointer_path(binding, &virtual_base, &displacement);
    if (!forward) displacement = (int)(0U - (unsigned)displacement);
    return member_pointer_conversions[data_member][source_model - 1][target_model - 1](
        expression, type, binding, forward, displacement, virtual_base);
}

/* Original 0x4bee0..0x4bf5e, stdcall24. Only the value, target
   type and displacement participate; the shared dispatch contract retains
   the unused path, direction and virtual-base words. */
IrToken *__stdcall convert_compact_data_member_pointer(IrToken *expression,
    CType *type, QualifierBinding *path, int forward, int displacement,
    BaseClassLink *virtual_base)
{
    IrToken *saved, *original, *converted;
    if (!displacement && equal_data_member_null_representation(type, expression->type))
        return new_cast_expression(type, expression);
    original = indirect_storage_expression(member_pointer_component(expression, 0, &saved));
    converted = indirect_storage_expression(member_pointer_component(expression, 1, &saved));
    converted = make_member_at_offset(converted, displacement, type);
    return preserve_null_data_member_conversion(converted, original, expression->type);
}

/* Original 0x4b410..0x4b433, stdcall8. Both predicates return
   zero or one; equal null encodings allow the ordinary cast path. */
int __stdcall equal_data_member_null_representation(CType *target, CType *source)
{
    int source_zero = data_member_pointer_uses_zero_null(source);
    int target_zero = data_member_pointer_uses_zero_null(target);
    return source_zero == target_zero;
}

/* Original 0x4bf60..0x4c038, stdcall24. Compact method values
   either retain their representation or materialize function/this-adjustment. */
IrToken *__stdcall convert_compact_method_pointer(IrToken *expression,
    CType *type, QualifierBinding *path, int forward, int displacement,
    BaseClassLink *virtual_base)
{
    unsigned target_model = member_pointer_inheritance_model(type);
    unsigned source_model = member_pointer_inheritance_model(expression->type);
    IrToken *function, *saved, *adjustment;
    if (!displacement && source_model == target_model) {
        expression->type = type;
        return expression;
    }
    if (source_model == 1)
        return make_compact_method_pointer(type, expression,
            make_integer_constant(displacement));
    if (target_model == 1) {
        if (!recognize_compact_method_function(expression, &function))
            function = member_pointer_function_value(
                builtin_unary_expression(OP_BIT_AND, expression), expression);
        return make_compact_method_pointer(type, function, 0);
    }
    function = member_pointer_function_value(
        member_pointer_component(expression, 0, &saved), expression);
    adjustment = member_pointer_adjustment_value(
        member_pointer_component(expression, 1, &saved), expression);
    adjustment = adjust_member_pointer_displacement(adjustment, displacement);
    return make_compact_method_pointer(type, function, adjustment);
}
