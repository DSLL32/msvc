/* C++ pointers to nonstatic members and their inheritance representations. */
#include "lexer.h"
#include "call-expressions.h"
#include "expression-records.h"
#include "special-members.h"
#include "numeric-conversion.h"
#include "constructor-initializers.h"
#include "parameters.h"
#include "member-pointers.h"
#include "hash-table.h"
#include "function-finalization.h"
#include "global-ir.h"
#include <recon.h>

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
