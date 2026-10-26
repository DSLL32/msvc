/* C++ pointers to nonstatic members and their inheritance representations. */
#include "lexer.h"
#include "call-expressions.h"
#include "expression-records.h"
#include "special-members.h"
#include "numeric-conversion.h"
#include "constructor-initializers.h"
#include "parameters.h"
#include "member-pointers.h"
#include "class-lookup.h"
#include "segments.h"
#include "braced-initializers.h"
#include "macros.h"
#include "mangling.h"
#include "object-initializers.h"
#include "member-expressions.h"
#include "expression-preparation.h"
#include "type-sizes.h"
#include "return-statements.h"
#include "hash-table.h"
#include "function-finalization.h"
#include "global-ir.h"
#include "virtual-table-output.h"
#include <recon.h>

int __stdcall equal_data_member_null_representation(CType *target, CType *source);
int __stdcall can_reuse_member_pointer_storage(IrToken *expression);
int __stdcall recognize_compact_method_function(IrToken *expression, IrToken **function);
IrToken *__stdcall member_pointer_function_value(IrToken *storage, IrToken *expression);
IrToken *__stdcall member_pointer_adjustment_value(IrToken *storage, IrToken *expression);
IrToken *__stdcall member_pointer_virtual_offset_value(IrToken *storage, IrToken *expression);
IrToken *__stdcall member_pointer_vbptr_offset_value(IrToken *storage, IrToken *expression);
IrToken *__stdcall member_pointer_general_virtual_offset_value(IrToken *storage, IrToken *expression);
IrToken *__stdcall virtual_data_member_offset(IrToken *storage, IrToken *expression);
IrToken *__stdcall virtual_data_table_offset(IrToken *storage, IrToken *expression);
IrToken *__stdcall data_member_integer_field(IrToken *storage, unsigned component);
IrToken *__stdcall general_data_member_offset(IrToken *storage, IrToken *expression);
IrToken *__stdcall general_data_vbptr_offset(IrToken *storage, IrToken *expression);
IrToken *__stdcall general_data_table_offset(IrToken *storage, IrToken *expression);
IrToken *__stdcall member_pointer_integer_field(IrToken *storage, IrToken *expression,
    unsigned component);
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
IrToken *__stdcall build_virtual_method_pointer(CType *type, IrToken *function,
    IrToken *adjustment, IrToken *table_offset);
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

void __stdcall class_virtual_base_pointer_layout(CppSymbol *tag,
    unsigned *offset, CType **pointer_type);
IrToken *__stdcall adjust_general_member_pointer_object(IrToken *object,
    IrToken *address, IrToken *adjustment, IrToken *pointer_offset,
    IrToken *virtual_offset, CType *type);
void __stdcall prepare_data_member_object(IrToken *object, IrToken **value,
    IrToken **address);
void __stdcall prepare_member_pointer_object(IrToken *object, IrToken **value,
    IrToken **address, IrToken **prefix);
IrToken *__stdcall adjust_virtual_member_pointer_object(IrToken *object,
    IrToken *address, IrToken *adjustment, IrToken *virtual_offset, CType *type);

IrToken *__stdcall make_adjusted_member_call(IrToken *function, IrToken *object,
    CType *type);
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
/* Original 0x4b1b0..0x4b1de. Bind the adjusted object to the
   selected member function using the owning class type. */
IrToken *__stdcall make_adjusted_member_call(IrToken *function, IrToken *object,
    CType *type)
{
    CType *object_type = type->related->related->data.parameters->associated_type;
    IrToken *bound_object = new_typed_unary_expression(0x46, object_type, object);
    return new_left_typed_expression(0xa4, function, bound_object);
}

/* Original 0x4aa00..0x4aa7b, stdcall4. Apply both virtual-data
   representation fields, then expose the pointed-to member's value type. */
IrToken *__stdcall apply_virtual_data_member_pointer(IrToken *expression)
{
    IrToken *value, *address, *member, *saved, *storage, *virtual_offset, *offset, *result;
    prepare_data_member_object(expression->value.pointer, &value, &address);
    member = expression->tail.right;
    storage = member_pointer_component(member, 0, &saved);
    virtual_offset = virtual_data_table_offset(storage, member);
    storage = member_pointer_component(member, 1, &saved);
    offset = virtual_data_member_offset(storage, member);
    result = adjust_virtual_member_pointer_object(value, address, offset,
        virtual_offset, member->type);
    result = indirect_storage_expression(result);
    result->type = drop_first_type_shape(member->type);
    return result;
}

/* Original 0x4ab90..0x4acd7, stdcall4. Resolve general data
   members with the direct offset where the complete class permits it, otherwise
   select between ordinary and virtual-base address calculations. */
IrToken *__stdcall apply_general_data_member_pointer(IrToken *expression)
{
    IrToken *member = expression->tail.right;
    CppSymbol *owner = member_pointer_owner_class(member->type);
    IrToken *saved, *storage, *value, *address, *offset, *pointer_offset, *virtual_offset;
    IrToken *first, *second, *fallback, *result, *zero, *test;
    if ((owner->flags & 4) && !(owner->detail.class_info->flags & 0x10)) {
        storage = member_pointer_component(member, 0, &saved);
        offset = general_data_member_offset(storage, member);
        result = new_left_typed_expression(0x5a, expression->value.pointer, offset);
    } else {
        prepare_data_member_object(expression->value.pointer, &value, &address);
        storage = member_pointer_component(member, 0, &saved);
        virtual_offset = general_data_table_offset(storage, member);
        storage = member_pointer_component(member, 1, &saved);
        pointer_offset = general_data_vbptr_offset(storage, member);
        storage = member_pointer_component(member, 1, &saved);
        offset = general_data_member_offset(storage, member);
        second = clone_expression(offset, 0);
        first = clone_expression(address, 0);
        fallback = new_left_typed_expression(0x5a, first, second);
        if (owner->flags & 4)
            result = adjust_virtual_member_pointer_object(value, address,
                offset, virtual_offset, member->type);
        else
            result = adjust_general_member_pointer_object(value, address,
                offset, pointer_offset, virtual_offset, member->type);
        result = new_left_typed_expression(0x3f, fallback, result);
        zero = make_integer_constant(0);
        first = clone_expression(virtual_offset, 0);
        test = new_right_typed_expression(0x44, first, zero);
        result = new_right_typed_expression(0x5f, test, result);
    }
    result = indirect_storage_expression(result);
    result->type = drop_first_type_shape(member->type);
    return result;
}

/* Original 0x4ae20..0x4ae9d. Reuse an addressable member pointer,
   or retain its initial evaluation separately in the caller's prefix. */
IrToken *__stdcall lower_multiple_inheritance_member_call(IrToken *object,
    IrToken *member, CType *type, IrToken **prefix)
{
    IrToken *saved, *storage, *function, *adjustment, *adjusted;
    if (can_reuse_member_pointer_storage(member)) {
        storage = member_pointer_component(member, 0, &saved);
    } else {
        *prefix = member_pointer_component(member, 0, &saved);
        storage = member_pointer_component(member, 1, &saved);
    }
    function = member_pointer_function_value(storage, member);
    storage = member_pointer_component(member, 1, &saved);
    adjustment = member_pointer_adjustment_value(storage, member);
    adjusted = new_left_typed_expression(0x5a, object, adjustment);
    return make_adjusted_member_call(function, adjusted, type);
}

/* Original 0x4aea0..0x4af58, stdcall16. Retain the object's
   evaluation, then adjust its address through the virtual-base table. */
IrToken *__stdcall lower_virtual_inheritance_member_call(IrToken *object,
    IrToken *member, CType *type, IrToken **prefix)
{
    IrToken *saved, *storage, *function, *adjustment, *virtual_offset;
    IrToken *value, *address, *adjusted;
    if (can_reuse_member_pointer_storage(member)) {
        storage = member_pointer_component(member, 0, &saved);
    } else {
        *prefix = member_pointer_component(member, 0, &saved);
        storage = member_pointer_component(member, 1, &saved);
    }
    function = member_pointer_function_value(storage, member);
    storage = member_pointer_component(member, 1, &saved);
    adjustment = member_pointer_adjustment_value(storage, member);
    storage = member_pointer_component(member, 1, &saved);
    virtual_offset = member_pointer_virtual_offset_value(storage, member);
    prepare_member_pointer_object(object, &value, &address, prefix);
    adjusted = adjust_virtual_member_pointer_object(value, address, adjustment,
        virtual_offset, member->type);
    return make_adjusted_member_call(function, adjusted, type);
}

/* Original 0x4af60..0x4b0de, stdcall16. The general model
   conditionally applies a virtual-base adjustment, retaining object evaluation
   and the nonvirtual fallback as distinct expression branches. */
IrToken *__stdcall lower_general_inheritance_member_call(IrToken *object,
    IrToken *member, CType *type, IrToken **prefix)
{
    CppSymbol *owner = member_pointer_owner_class(member->type);
    IrToken *saved, *storage, *function, *adjustment, *value, *address;
    IrToken *virtual_offset, *pointer_offset, *fallback, *adjusted, *test, *zero;
    IrToken *first, *second;
    if (can_reuse_member_pointer_storage(member)) {
        storage = member_pointer_component(member, 0, &saved);
        function = member_pointer_function_value(storage, member);
    } else {
        *prefix = member_pointer_component(member, 0, &saved);
        storage = member_pointer_component(member, 1, &saved);
        function = member_pointer_function_value(storage, member);
    }
    storage = member_pointer_component(member, 1, &saved);
    adjustment = member_pointer_adjustment_value(storage, member);
    if ((owner->flags & 4) && !(owner->detail.class_info->flags & 0x10)) {
        adjusted = new_left_typed_expression(0x5a, object, adjustment);
    } else {
        prepare_member_pointer_object(object, &value, &address, prefix);
        second = clone_expression(adjustment, 0);
        first = clone_expression(address, 0);
        fallback = new_left_typed_expression(0x5a, first, second);
        storage = member_pointer_component(member, 1, &saved);
        virtual_offset = member_pointer_general_virtual_offset_value(storage, member);
        if (owner->flags & 4) {
            adjusted = adjust_virtual_member_pointer_object(value, address,
                adjustment, virtual_offset, member->type);
        } else {
            storage = member_pointer_component(member, 1, &saved);
            pointer_offset = member_pointer_vbptr_offset_value(storage, member);
            adjusted = adjust_general_member_pointer_object(value, address,
                adjustment, pointer_offset, virtual_offset, member->type);
        }
        adjusted = new_left_typed_expression(0x3f, fallback, adjusted);
        zero = make_integer_constant(0);
        first = clone_expression(virtual_offset, 0);
        test = new_right_typed_expression(0x44, first, zero);
        adjusted = new_right_typed_expression(0x5f, test, adjusted);
    }
    return make_adjusted_member_call(function, adjusted, type);
}

/* Original 0x7bea0..0x7bed0, stdcall12. This class's first
   virtual-base group supplies the pointer type; its class record owns offset. */
void __stdcall class_virtual_base_pointer_layout(CppSymbol *tag,
    unsigned *offset, CType **pointer_type)
{
    VirtualTableGroup *group = first_vector_item(tag->detail.class_info->virtual_table_groups);
    *offset = tag->detail.class_info->virtual_base_pointer_offset;
    if (pointer_type) *pointer_type = group->virtual_base_pointer->type;
}

/* Original 0x4aa80..0x4ab0d, stdcall20. Load the displacement
   through this class's vbptr and add the ordinary member adjustment. */
IrToken *__stdcall adjust_virtual_member_pointer_object(IrToken *object,
    IrToken *address, IrToken *adjustment, IrToken *virtual_offset, CType *type)
{
    unsigned pointer_offset;
    CType *pointer_type;
    IrToken *base, *table, *offset, *result;
    class_virtual_base_pointer_layout(member_pointer_owner_class(type),
        &pointer_offset, &pointer_type);
    offset = make_integer_constant(pointer_offset);
    base = new_ir_expression(0x5a, object, offset, pointer_type);
    offset = make_integer_constant(pointer_offset);
    table = new_ir_expression(0x5a, address, offset, pointer_type);
    table = indirect_storage_expression(table);
    table = new_right_typed_expression(0x5a, table, virtual_offset);
    table = indirect_storage_expression(table);
    result = new_left_typed_expression(0x5a, base, table);
    return new_left_typed_expression(0x5a, result, adjustment);
}

/* Original 0x4ace0..0x4ad7f, stdcall12. Preserve an object's
   first evaluation in the value expression and reuse its temporary thereafter. */
void __stdcall prepare_data_member_object(IrToken *object, IrToken **value,
    IrToken **address)
{
    IrToken *temporary;
    unsigned index;
    if (expression_can_be_repeated(object)) {
        *value = object;
        *address = clone_expression(object, 0);
        return;
    }
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    temporary = new_temporary_reference(index, object->type);
    *value = new_left_typed_expression(0x3d, temporary, object);
    *address = indirect_storage_expression(clone_expression(temporary, 0));
}

/* Original 0x4b0e0..0x4b1ab, stdcall16. Make two repeatable
   object expressions, retaining any one-time temporary assignment in prefix. */
void __stdcall prepare_member_pointer_object(IrToken *object, IrToken **value,
    IrToken **address, IrToken **prefix)
{
    IrToken *temporary, *assignment;
    unsigned index;
    if (expression_can_be_repeated(object)) {
        *value = object;
        *address = clone_expression(object, 0);
        return;
    }
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    temporary = new_temporary_reference(index, object->type);
    assignment = new_left_typed_expression(0x3d, temporary, object);
    *value = indirect_storage_expression(clone_expression(temporary, 0));
    *address = indirect_storage_expression(clone_expression(temporary, 0));
    if (*prefix) *prefix = new_left_typed_expression(0x40, *prefix, assignment);
    else *prefix = assignment;
}

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
    IrToken *function, *saved, *adjustment, *storage;
    if (!displacement && source_model == target_model) {
        expression->type = type;
        return expression;
    }
    if (source_model == 1)
        return make_compact_method_pointer(type, expression,
            make_integer_constant(displacement));
    if (target_model == 1) {
        if (!recognize_compact_method_function(expression, &function)) {
            storage = builtin_unary_expression(OP_BIT_AND, expression);
            function = member_pointer_function_value(storage, expression);
        }
        return make_compact_method_pointer(type, function, 0);
    }
    function = member_pointer_function_value(
        member_pointer_component(expression, 0, &saved), expression);
    adjustment = member_pointer_adjustment_value(
        member_pointer_component(expression, 1, &saved), expression);
    adjustment = adjust_member_pointer_displacement(adjustment, displacement);
    return make_compact_method_pointer(type, function, adjustment);
}

/* Original 0x4a2a0..0x4a39c, stdcall12. Store the function and
   this-adjustment in a fresh aggregate temporary, then yield that value. */
IrToken *__stdcall make_multiple_inheritance_method_pointer(CType *type,
    IrToken *function, IrToken *adjustment)
{
    unsigned index, displacement;
    CType *function_type, *pointer_type, *integer_type;
    IrToken *temporary, *offset, *storage, *first, *second, *sequence;
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    function_type = drop_first_type_shape(type);
    pointer_type = make_qualified_type(function_type, 3,
        function_type->related->attributes & 0x0f77, 0);
    displacement = pointer_kind_size(function_type->related->attributes & 0x0f77);
    temporary = new_temporary_reference(index, type);
    offset = make_integer_constant(0);
    storage = clone_expression(temporary, 0);
    first = new_ir_expression(0x5c, storage, offset, pointer_type);
    first = new_ir_expression(0x3d, first, function, pointer_type);
    integer_type = builtin_integer_type();
    offset = make_integer_constant(displacement);
    storage = clone_expression(temporary, 0);
    second = new_ir_expression(0x5c, storage, offset, integer_type);
    integer_type = builtin_integer_type();
    second = new_ir_expression(0x3d, second, adjustment, integer_type);
    sequence = new_right_typed_expression(0x40, first, second);
    return new_right_typed_expression(0x40, sequence,
        indirect_storage_expression(temporary));
}

/* Original 0x4bb10..0x4bbe3, stdcall12. Component zero captures
   reusable storage or assigns a temporary; subsequent components clone it. */
IrToken *__stdcall member_pointer_component(IrToken *expression, int component,
    IrToken **saved)
{
    IrToken *value;
    CType *type;
    unsigned index;
    if (component) return clone_expression(*saved, 0);
    if (can_reuse_member_pointer_storage(expression)) {
        value = expression->kind == 0x40 ? expression->tail.right : expression;
        value = modifiable_lvalue(value, 0);
        *saved = value;
        if (expression->kind == 0x40) {
            type = value->type;
            value = clone_expression(value, 0);
            return new_ir_expression(0x40, expression->value.pointer, value, type);
        }
        return clone_expression(*saved, 0);
    }
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    *saved = new_temporary_reference(index, expression->type);
    value = clone_expression(*saved, 0);
    return new_left_typed_expression(0x3d, value, expression);
}

/* Original 0x4bbf0..0x4bc26, stdcall4. A trailing storage
   expression can be reused only when repetition and lvalue probes agree. */
int __stdcall can_reuse_member_pointer_storage(IrToken *expression)
{
    if (expression->kind == 0x40) expression = expression->tail.right;
    return expression->kind == 0x46 && expression_can_be_repeated(expression) &&
        modifiable_lvalue(expression, 0) != 0;
}

/* Original 0x4c2d0..0x4c315, stdcall12. A base adjustment must
   preserve the source representation's reserved null data-member value. */
IrToken *__stdcall preserve_null_data_member_conversion(IrToken *converted,
    IrToken *original, CType *source)
{
    IrToken *null_value, *condition, *alternatives;
    null_value = make_null_simple_data_pointer(source);
    condition = new_ir_expression(0x44, original, null_value, type_0064);
    null_value = make_null_member_pointer(converted->type);
    alternatives = new_left_typed_expression(0x3f, null_value, converted);
    return new_right_typed_expression(0x5f, condition, alternatives);
}

/* Original 0x64ef0..0x64f13, stdcall12. Explicit casts accept
   every member-pointer conversion rank below the incompatible threshold. */
int __stdcall member_pointer_cast_compatible(CType *source, CType *target, int reverse)
{
    return classify_member_pointer_conversion(source, target, reverse) < 0x400;
}

/* Original 0x4c040..0x4c17a, stdcall8. Recognize the temporary
   sequence used to package a constant compact method pointer. The output
   is assigned before the later shape checks, including some false returns. */
int __stdcall recognize_compact_method_function(IrToken *expression,
    IrToken **function)
{
    IrToken *sequence, *assignment, *storage, *result;
    if (!expression) return 0;
    while (expression->kind == 0x6e) {
        expression = expression->value.pointer;
        if (!expression) break;
    }
    if (!expression || expression->kind != 0x40) return 0;
    sequence = expression->value.pointer;
    if (sequence->kind != 0x40) return 0;
    assignment = sequence->value.pointer;
    if (assignment->kind != 0x3d) return 0;
    storage = assignment->value.pointer;
    if (storage->kind != 0x5c || ((IrToken *)storage->value.pointer)->kind != 0xa7 ||
        storage->tail.right->kind != 7) return 0;
    *function = assignment->tail.right;
    if ((*function)->kind != 2) return 0;
    assignment = sequence->tail.right;
    if (assignment->kind != 0x3d) return 0;
    storage = assignment->value.pointer;
    if (storage->kind != 0x5c || ((IrToken *)storage->value.pointer)->kind != 0xa7 ||
        storage->tail.right->kind != 7 || assignment->tail.right->kind != 7)
        return 0;
    result = expression->tail.right;
    return result->kind == 0x46 && ((IrToken *)result->value.pointer)->kind == 0xa7;
}

/* Original 0x4bcc0..0x4bcea, stdcall8. The function address is
   the first field of the member-pointer storage, viewed as a callable pointer. */
IrToken *__stdcall member_pointer_function_value(IrToken *storage,
    IrToken *expression)
{
    CType *type = member_pointer_function_type(expression->type);
    IrToken *offset = make_integer_constant(0);
    return indirect_storage_expression(new_ir_expression(0x5c, storage, offset, type));
}

/* Original 0x4b4e0..0x4b512, stdcall8. Aggregate comparisons
   combine equal fields with AND and unequal fields with OR. */
IrToken *__stdcall dispatch_member_pointer_comparison(IrToken *expression,
    MemberPointerComparison compare)
{
    switch (expression->kind) {
    case 0x44: return compare(expression, 0x44, 0x3b, 0x58);
    case 0x55: return compare(expression, 0x55, 0x58, 0x3b);
    default: return 0;
    }
}

/* Original 0x4b520..0x4b61d, stdcall16. Compare both stored
   function and adjustment fields, ignoring adjustment differences for nulls. */
IrToken *__stdcall compare_multiple_method_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine)
{
    IrToken *left = expression->value.pointer, *right, *saved, *storage;
    IrToken *left_function, *left_repeated, *left_adjustment, *null_function;
    IrToken *right_function, *right_adjustment, *function_test, *null_test;
    IrToken *adjustment_test, *result, *zero;
    CType *result_type;
    storage = member_pointer_component(left, 0, &saved);
    left_function = member_pointer_function_value(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_repeated = member_pointer_function_value(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_adjustment = member_pointer_adjustment_value(storage, left);
    null_function = make_null_member_function(left->type);
    right = expression->tail.right;
    storage = member_pointer_component(right, 0, &saved);
    right_function = member_pointer_function_value(storage, right);
    storage = member_pointer_component(right, 1, &saved);
    right_adjustment = member_pointer_adjustment_value(storage, right);
    function_test = new_ir_expression(comparison, left_function, right_function, type_0064);
    null_test = new_ir_expression(comparison, left_repeated, null_function, type_0064);
    adjustment_test = new_ir_expression(comparison, left_adjustment, right_adjustment, type_0064);
    result = new_left_typed_expression(null_combine, null_test, adjustment_test);
    result = new_left_typed_expression(combine, function_test, result);
    result_type = result->type;
    zero = make_integer_constant(0);
    return new_ir_expression(0x40, zero, result, result_type);
}

/* Original 0x4b620..0x4b6d0, stdcall16. A virtual data member
   pointer consists of the member offset and vbtable displacement. The final
   null_combine formal is retained in the ABI but is not read or forwarded. */
IrToken *__stdcall compare_virtual_data_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine)
{
    IrToken *left = expression->value.pointer, *right, *saved, *storage;
    IrToken *left_offset, *left_virtual, *right_offset, *right_virtual;
    IrToken *offset_test, *virtual_test, *result, *zero;
    storage = member_pointer_component(left, 0, &saved);
    left_offset = virtual_data_member_offset(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_virtual = virtual_data_table_offset(storage, left);
    right = expression->tail.right;
    storage = member_pointer_component(right, 0, &saved);
    right_offset = virtual_data_member_offset(storage, right);
    storage = member_pointer_component(right, 1, &saved);
    right_virtual = virtual_data_table_offset(storage, right);
    offset_test = new_ir_expression(comparison, left_offset, right_offset, type_0064);
    virtual_test = new_ir_expression(comparison, left_virtual, right_virtual, type_0064);
    result = new_left_typed_expression(combine, offset_test, virtual_test);
    zero = make_integer_constant(0);
    return new_right_typed_expression(0x40, zero, result);
}

/* Original 0x4b6e0..0x4b7e2, stdcall16. General data pointers
   compare the member offset, vbptr offset and vbtable displacement. The fourth
   formal is retained but the whole body neither reads nor forwards it. */
IrToken *__stdcall compare_general_data_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine)
{
    IrToken *left = expression->value.pointer, *right, *saved, *storage;
    IrToken *left_offset, *left_vbptr, *left_virtual, *right_offset, *right_vbptr, *right_virtual;
    IrToken *offset_test, *vbptr_test, *virtual_test, *result, *zero;
    storage = member_pointer_component(left, 0, &saved);
    left_offset = general_data_member_offset(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_vbptr = general_data_vbptr_offset(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_virtual = general_data_table_offset(storage, left);
    right = expression->tail.right;
    storage = member_pointer_component(right, 0, &saved);
    right_offset = general_data_member_offset(storage, right);
    storage = member_pointer_component(right, 1, &saved);
    right_vbptr = general_data_vbptr_offset(storage, right);
    storage = member_pointer_component(right, 1, &saved);
    right_virtual = general_data_table_offset(storage, right);
    offset_test = new_ir_expression(comparison, left_offset, right_offset, type_0064);
    vbptr_test = new_ir_expression(comparison, left_vbptr, right_vbptr, type_0064);
    virtual_test = new_ir_expression(comparison, left_virtual, right_virtual, type_0064);
    result = new_left_typed_expression(combine, offset_test, vbptr_test);
    result = new_left_typed_expression(combine, result, virtual_test);
    zero = make_integer_constant(0);
    return new_right_typed_expression(0x40, zero, result);
}

/* Original 0x4b7f0..0x4b94c, stdcall16. Null function pointers
   compare independently of this and virtual-base adjustments. */
IrToken *__stdcall compare_virtual_method_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine)
{
    IrToken *left = expression->value.pointer, *right, *saved, *storage;
    IrToken *left_function, *left_repeated, *left_adjustment, *left_virtual;
    IrToken *right_function, *right_adjustment, *right_virtual, *null_function;
    IrToken *function_test, *null_test, *adjustment_test, *virtual_test, *result, *zero;
    storage = member_pointer_component(left, 0, &saved);
    left_function = member_pointer_function_value(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_repeated = member_pointer_function_value(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_adjustment = member_pointer_adjustment_value(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_virtual = member_pointer_virtual_offset_value(storage, left);
    null_function = make_null_member_function(left->type);
    right = expression->tail.right;
    storage = member_pointer_component(right, 0, &saved);
    right_function = member_pointer_function_value(storage, right);
    storage = member_pointer_component(right, 1, &saved);
    right_adjustment = member_pointer_adjustment_value(storage, right);
    storage = member_pointer_component(right, 1, &saved);
    /* Original4b89e deliberately supplies the left expression's type basis. */
    right_virtual = member_pointer_virtual_offset_value(storage, left);
    function_test = new_ir_expression(comparison, left_function, right_function, type_0064);
    null_test = new_ir_expression(comparison, left_repeated, null_function, type_0064);
    adjustment_test = new_ir_expression(comparison, left_adjustment, right_adjustment, type_0064);
    virtual_test = new_ir_expression(comparison, left_virtual, right_virtual, type_0064);
    result = new_ir_expression(combine, adjustment_test, virtual_test, type_0064);
    result = new_left_typed_expression(null_combine, null_test, result);
    result = new_left_typed_expression(combine, function_test, result);
    zero = make_integer_constant(0);
    return new_right_typed_expression(0x40, zero, result);
}

/* Original 0x4b950..0x4bb0a, stdcall16. General method pointers
   compare function, this adjustment, vbptr location and vbtable displacement. */
IrToken *__stdcall compare_general_method_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine)
{
    IrToken *left = expression->value.pointer, *right, *saved, *storage;
    IrToken *left_function, *left_repeated, *left_adjustment, *left_vbptr, *left_virtual;
    IrToken *right_function, *right_adjustment, *right_vbptr, *right_virtual, *null_function;
    IrToken *function_test, *null_test, *adjustment_test, *vbptr_test, *virtual_test;
    IrToken *result, *zero;
    storage = member_pointer_component(left, 0, &saved);
    left_function = member_pointer_function_value(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_repeated = member_pointer_function_value(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_adjustment = member_pointer_adjustment_value(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_vbptr = member_pointer_vbptr_offset_value(storage, left);
    storage = member_pointer_component(left, 1, &saved);
    left_virtual = member_pointer_general_virtual_offset_value(storage, left);
    null_function = make_null_member_function(left->type);
    right = expression->tail.right;
    storage = member_pointer_component(right, 0, &saved);
    right_function = member_pointer_function_value(storage, right);
    storage = member_pointer_component(right, 1, &saved);
    right_adjustment = member_pointer_adjustment_value(storage, right);
    storage = member_pointer_component(right, 1, &saved);
    /* Both original4ba16/4ba2e use the left expression's field type basis. */
    right_vbptr = member_pointer_vbptr_offset_value(storage, left);
    storage = member_pointer_component(right, 1, &saved);
    right_virtual = member_pointer_general_virtual_offset_value(storage, left);
    function_test = new_ir_expression(comparison, left_function, right_function, type_0064);
    null_test = new_ir_expression(comparison, left_repeated, null_function, type_0064);
    adjustment_test = new_ir_expression(comparison, left_adjustment, right_adjustment, type_0064);
    vbptr_test = new_ir_expression(comparison, left_vbptr, right_vbptr, type_0064);
    virtual_test = new_ir_expression(comparison, left_virtual, right_virtual, type_0064);
    result = new_ir_expression(combine, adjustment_test, vbptr_test, type_0064);
    result = new_ir_expression(combine, result, virtual_test, type_0064);
    result = new_left_typed_expression(null_combine, null_test, result);
    result = new_left_typed_expression(combine, function_test, result);
    zero = make_integer_constant(0);
    return new_right_typed_expression(0x40, zero, result);
}

/* Original 0x4bc30..0x4bc3e, stdcall8. The second expression
   formal is unused; data-member offsets use the target int representation. */
IrToken *__stdcall virtual_data_member_offset(IrToken *storage, IrToken *expression)
{
    return data_member_integer_field(storage, 0);
}

/* Original 0x4bc40..0x4bc4e, stdcall8. As above, the expression
   formal is unused; the virtual data model stores vbtable offset in field one. */
IrToken *__stdcall virtual_data_table_offset(IrToken *storage, IrToken *expression)
{
    return data_member_integer_field(storage, 1);
}

/* Original 0x4bc50..0x4bc5e, stdcall8. The expression
   formal is unused; integer field 0 supplies the general data member byte offset. */
IrToken *__stdcall general_data_member_offset(IrToken *storage, IrToken *expression)
{
    return data_member_integer_field(storage, 0);
}

/* Original 0x4bc60..0x4bc6e, stdcall8. The expression
   formal is unused; integer field 1 supplies the general data vbptr location. */
IrToken *__stdcall general_data_vbptr_offset(IrToken *storage, IrToken *expression)
{
    return data_member_integer_field(storage, 1);
}

/* Original 0x4bc70..0x4bc7e, stdcall8. The expression
   formal is unused; integer field 2 supplies the general data vbtable displacement. */
IrToken *__stdcall general_data_table_offset(IrToken *storage, IrToken *expression)
{
    return data_member_integer_field(storage, 2);
}

/* Original 0x4bc80..0x4bcb3, stdcall8. Data-member aggregate
   fields start at byte zero and use the target integer width. */
IrToken *__stdcall data_member_integer_field(IrToken *storage, unsigned component)
{
    CType *integer_type = builtin_integer_type();
    unsigned offset = (integer_type->flags & 15) * component;
    IrToken *position;
    integer_type = builtin_integer_type();
    position = make_integer_constant(offset);
    return indirect_storage_expression(new_ir_expression(0x5c, storage,
        position, integer_type));
}

/* Original 0x4bd20..0x4bd60, stdcall8. Single inheritance has
   no stored this-adjustment; the larger representations use integer field zero. */
IrToken *__stdcall member_pointer_adjustment_value(IrToken *storage,
    IrToken *expression)
{
    switch (member_pointer_inheritance_model(expression->type)) {
    case 1: return make_integer_constant(0);
    case 2: case 3: case 4:
        return member_pointer_integer_field(storage, expression, 0);
    default: return 0;
    }
}

/* Original 0x4bd70..0x4bd83, stdcall8. The virtual model's
   second integer field is its vbtable displacement. */
IrToken *__stdcall member_pointer_virtual_offset_value(IrToken *storage,
    IrToken *expression)
{
    return member_pointer_integer_field(storage, expression, 1);
}

/* Original 0x4bdb0..0x4bdc3, stdcall8. General method
   representation integer field one holds the vbptr offset. */
IrToken *__stdcall member_pointer_vbptr_offset_value(IrToken *storage,
    IrToken *expression)
{
    return member_pointer_integer_field(storage, expression, 1);
}

/* Original 0x4bdd0..0x4bde3, stdcall8. General method
   representation integer field two holds the vbtable displacement. */
IrToken *__stdcall member_pointer_general_virtual_offset_value(IrToken *storage,
    IrToken *expression)
{
    return member_pointer_integer_field(storage, expression, 2);
}

/* Original 0x4bdf0..0x4be4e, stdcall12. Integer representation
   fields follow the function address and are spaced by the target int width. */
IrToken *__stdcall member_pointer_integer_field(IrToken *storage,
    IrToken *expression, unsigned component)
{
    TypeShape *shape = expression->type->related;
    CType *integer_type;
    unsigned offset;
    IrToken *position;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    offset = pointer_kind_size(shape->related->attributes & 0x0f77);
    integer_type = builtin_integer_type();
    offset += (integer_type->flags & 15) * component;
    integer_type = builtin_integer_type();
    position = make_integer_constant(offset);
    return indirect_storage_expression(new_ir_expression(0x5c, storage,
        position, integer_type));
}

/* Original 0x4a3a0..0x4a421. The virtual model stores function, this
   adjustment and vbtable displacement; its vbptr offset belongs to the class. */
IrToken *__stdcall make_virtual_method_pointer(CType *type, IrToken *function,
    IrToken *adjustment, BaseClassLink *virtual_base)
{
    CppSymbol *base, *owner;
    unsigned table_offset, displacement;
    IrToken *offset;
    if (virtual_base) {
        base = cpp_context_stack->items[virtual_base->context_index];
        owner = member_pointer_owner_class(type);
#ifdef RECON_NATIVE_TEST
        /* Original false query leaves entryESP-4 unwritten before4a3d5. */
        if (!query_virtual_base_pointer(base, owner, 0, &table_offset, 0)) { __asm { int 3 } }
#else
        query_virtual_base_pointer(base, owner, 0, &table_offset, 0);
#endif
        offset = make_integer_constant(table_offset);
    } else {
        displacement = adjustment->value.integer;
        owner = member_pointer_owner_class(type);
        displacement -= class_virtual_base_pointer_offset(owner);
        adjustment = make_integer_constant(displacement);
        offset = make_integer_constant(0);
    }
    return build_virtual_method_pointer(type, function, adjustment, offset);
}

/* Original 0x7bee0..0x7bf0f. The first vbtable's object location and
   its first entry together locate the class's virtual-base pointer. */
unsigned __stdcall class_virtual_base_pointer_offset(CppSymbol *tag)
{
    VirtualTable *table = first_vector_item(tag->detail.class_info->virtual_base_tables);
    unsigned offset = virtual_table_object_offset(table);
    return offset + virtual_base_table_entry_offset((VirtualBaseEntry *)table->entries, tag, table);
}

/* Original 0x4a430..0x4a56c, stdcall16. Materialize the virtual model's
   three fields in a temporary, then return its aggregate value. */
IrToken *__stdcall build_virtual_method_pointer(CType *type, IrToken *function,
    IrToken *adjustment, IrToken *table_offset)
{
    unsigned index, function_bytes, table_displacement;
    CType *function_type, *pointer_type, *integer_type;
    IrToken *temporary, *offset, *storage, *first, *second, *third, *sequence;
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    function_type = drop_first_type_shape(type);
    pointer_type = make_qualified_type(function_type, 3,
        function_type->related->attributes & 0x0f77, 0);
    function_bytes = pointer_kind_size(function_type->related->attributes & 0x0f77);
    integer_type = builtin_integer_type();
    table_displacement = function_bytes + (integer_type->flags & 15);
    temporary = new_temporary_reference(index, type);
    offset = make_integer_constant(0);
    storage = clone_expression(temporary, 0);
    first = new_ir_expression(0x5c, storage, offset, pointer_type);
    first = new_left_typed_expression(0x3d, first, function);
    integer_type = builtin_integer_type();
    offset = make_integer_constant(function_bytes);
    storage = clone_expression(temporary, 0);
    second = new_ir_expression(0x5c, storage, offset, integer_type);
    second = new_left_typed_expression(0x3d, second, adjustment);
    integer_type = builtin_integer_type();
    offset = make_integer_constant(table_displacement);
    storage = clone_expression(temporary, 0);
    third = new_ir_expression(0x5c, storage, offset, integer_type);
    third = new_left_typed_expression(0x3d, third, table_offset);
    sequence = new_right_typed_expression(0x40, first, second);
    sequence = new_right_typed_expression(0x40, sequence, third);
    return new_right_typed_expression(0x40, sequence,
        indirect_storage_expression(temporary));
}

/* Original 0x4b3b0..0x4b3d5. Each field is constructed explicitly;
   the function component retains the target's ordinary null-function type. */
IrToken *__stdcall make_null_virtual_method_pointer(CType *type)
{
    IrToken *table_offset = make_integer_constant(0);
    IrToken *adjustment = make_integer_constant(0);
    IrToken *function = make_null_member_function(type);
    return build_virtual_method_pointer(type, function, adjustment, table_offset);
}

/* Original 0x4a570..0x4a709, stdcall20. The general representation
   stores the function, this adjustment, vbptr offset and vbtable offset. */
IrToken *__stdcall make_general_method_pointer(CType *type, IrToken *function,
    IrToken *adjustment, int vbptr_offset, IrToken *vbtable_offset)
{
    unsigned index, function_bytes, pointer_displacement, table_displacement;
    CType *function_type, *pointer_type, *integer_type;
    IrToken *temporary, *offset, *storage, *first, *second, *third, *fourth, *sequence;
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    function_type = drop_first_type_shape(type);
    pointer_type = make_qualified_type(function_type, 3,
        function_type->related->attributes & 0x0f77, 0);
    function_bytes = pointer_kind_size(function_type->related->attributes & 0x0f77);
    integer_type = builtin_integer_type();
    pointer_displacement = function_bytes + (integer_type->flags & 15);
    integer_type = builtin_integer_type();
    table_displacement = pointer_displacement + (integer_type->flags & 15);
    temporary = new_temporary_reference(index, type);
    offset = make_integer_constant(0);
    storage = clone_expression(temporary, 0);
    first = new_ir_expression(0x5c, storage, offset, pointer_type);
    first = new_left_typed_expression(0x3d, first, function);
    integer_type = builtin_integer_type();
    offset = make_integer_constant(function_bytes);
    storage = clone_expression(temporary, 0);
    second = new_ir_expression(0x5c, storage, offset, integer_type);
    second = new_left_typed_expression(0x3d, second, adjustment);
    integer_type = builtin_integer_type();
    offset = make_integer_constant(pointer_displacement);
    storage = clone_expression(temporary, 0);
    third = new_ir_expression(0x5c, storage, offset, integer_type);
    offset = make_integer_constant(vbptr_offset);
    third = new_left_typed_expression(0x3d, third, offset);
    integer_type = builtin_integer_type();
    offset = make_integer_constant(table_displacement);
    storage = clone_expression(temporary, 0);
    fourth = new_ir_expression(0x5c, storage, offset, integer_type);
    fourth = new_left_typed_expression(0x3d, fourth, vbtable_offset);
    sequence = new_right_typed_expression(0x40, first, second);
    sequence = new_right_typed_expression(0x40, sequence, third);
    sequence = new_right_typed_expression(0x40, sequence, fourth);
    return new_right_typed_expression(0x40, sequence,
        indirect_storage_expression(temporary));
}

/* Original 0x4b3e0..0x4b407. A general null pointer uses the explicit
   -1 vbtable sentinel, with zero this/vbptr adjustments. */
IrToken *__stdcall make_null_general_method_pointer(CType *type)
{
    IrToken *table_offset = make_integer_constant(-1);
    IrToken *adjustment = make_integer_constant(0);
    IrToken *function = make_null_member_function(type);
    return make_general_method_pointer(type, function, adjustment, 0, table_offset);
}

/* Original 0x4a710..0x4a7de, stdcall12. The virtual data-member
   representation contains a byte displacement and a vbtable displacement. */
IrToken *__stdcall make_virtual_data_member_pointer(CType *type, IrToken *offset,
    IrToken *virtual_offset)
{
    unsigned index, displacement;
    CType *integer_type;
    IrToken *temporary, *constant, *storage, *first, *second, *sequence;
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    integer_type = builtin_integer_type();
    displacement = integer_type->flags & 15;
    temporary = new_temporary_reference(index, type);
    integer_type = builtin_integer_type();
    constant = make_integer_constant(0);
    storage = clone_expression(temporary, 0);
    first = new_ir_expression(0x5c, storage, constant, integer_type);
    first = new_left_typed_expression(0x3d, first, offset);
    integer_type = builtin_integer_type();
    constant = make_integer_constant(displacement);
    storage = clone_expression(temporary, 0);
    second = new_ir_expression(0x5c, storage, constant, integer_type);
    second = new_left_typed_expression(0x3d, second, virtual_offset);
    sequence = new_right_typed_expression(0x40, first, second);
    return new_right_typed_expression(0x40, sequence,
        indirect_storage_expression(temporary));
}

/* Original 0x4b370..0x4b38c. Zero data displacement and a -1 virtual
   displacement distinguish the null virtual data-member representation. */
IrToken *__stdcall make_null_virtual_data_pointer(CType *type)
{
    IrToken *virtual_offset = make_integer_constant(-1);
    IrToken *offset = make_integer_constant(0);
    return make_virtual_data_member_pointer(type, offset, virtual_offset);
}

/* Original 0x4a7e0..0x4a901, stdcall16. General data-member pointers
   store displacement, vbptr displacement and vbtable displacement. */
IrToken *__stdcall make_general_data_member_pointer(CType *type, IrToken *offset,
    int vbptr_offset, IrToken *vbtable_offset)
{
    unsigned index, pointer_displacement, table_displacement;
    CType *integer_type;
    IrToken *temporary, *constant, *storage, *first, *second, *third, *sequence;
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    integer_type = builtin_integer_type();
    pointer_displacement = integer_type->flags & 15;
    integer_type = builtin_integer_type();
    table_displacement = 2 * (integer_type->flags & 15);
    temporary = new_temporary_reference(index, type);
    integer_type = builtin_integer_type();
    constant = make_integer_constant(0);
    storage = clone_expression(temporary, 0);
    first = new_ir_expression(0x5c, storage, constant, integer_type);
    first = new_left_typed_expression(0x3d, first, offset);
    integer_type = builtin_integer_type();
    constant = make_integer_constant(pointer_displacement);
    storage = clone_expression(temporary, 0);
    second = new_ir_expression(0x5c, storage, constant, integer_type);
    constant = make_integer_constant(vbptr_offset);
    second = new_left_typed_expression(0x3d, second, constant);
    integer_type = builtin_integer_type();
    constant = make_integer_constant(table_displacement);
    storage = clone_expression(temporary, 0);
    third = new_ir_expression(0x5c, storage, constant, integer_type);
    third = new_left_typed_expression(0x3d, third, vbtable_offset);
    sequence = new_right_typed_expression(0x40, first, second);
    sequence = new_right_typed_expression(0x40, sequence, third);
    return new_right_typed_expression(0x40, sequence,
        indirect_storage_expression(temporary));
}

/* Original 0x4b390..0x4b3ae. The general null data-member pointer
   retains the -1 vbtable sentinel and both zero displacement fields. */
IrToken *__stdcall make_null_general_data_pointer(CType *type)
{
    IrToken *virtual_offset = make_integer_constant(-1);
    IrToken *offset = make_integer_constant(0);
    return make_general_data_member_pointer(type, offset, 0, virtual_offset);
}

/* Original0x4d390..0x4d4b6, stdcall24. Preserve the source evaluation
   while adapting the stored this/vbtable displacements to the target owner. */
void __stdcall compute_member_pointer_adjustment(CppSymbol *target, CppSymbol *source,
    int displacement, BaseClassLink *virtual_base, int *adjustment, int *table_offset);
IrToken *__stdcall adjust_virtual_member_displacement(IrToken *table_offset,
    IrToken *adjustment, int displacement);
IrToken *__stdcall member_conversion_table_reference(QualifierBinding *path, int forward);
IrToken *__stdcall convert_virtual_method_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    IrToken *saved, *prefix = 0, *storage, *function, *table_offset, *adjustment;
    IrToken *table, *result;
    CppSymbol *source_owner, *target_owner;
    int owner_adjustment;
    if (!virtual_base && !displacement) {
        expression->type = type;
        return expression;
    }
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    function = member_pointer_function_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    table_offset = member_pointer_virtual_offset_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    adjustment = member_pointer_adjustment_value(storage, expression);
    source_owner = member_pointer_owner_class(expression->type);
    target_owner = member_pointer_owner_class(type);
    compute_member_pointer_adjustment(target_owner, source_owner, displacement,
        virtual_base, &owner_adjustment, 0);
    adjustment = adjust_virtual_member_displacement(clone_expression(table_offset, 0),
        adjustment, owner_adjustment);
    table = member_conversion_table_reference(path, forward);
    if (table)
        table_offset = indirect_storage_expression(
            new_left_typed_expression(0x5a, table, table_offset));
    result = build_virtual_method_pointer(type, function, adjustment, table_offset);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4c240..0x4c29f, stdcall24. The caller owns both optional
   output words; the adjustment is always written before querying a vbtable. */
void __stdcall compute_member_pointer_adjustment(CppSymbol *target, CppSymbol *source,
    int displacement, BaseClassLink *virtual_base, int *adjustment, int *table_offset)
{
    *adjustment = displacement;
    if (source)
        *adjustment = (int)((unsigned)*adjustment + class_virtual_base_pointer_offset(source));
    if (!virtual_base)
        *adjustment = (int)((unsigned)*adjustment - class_virtual_base_pointer_offset(target));
    if (table_offset && virtual_base)
        query_virtual_base_pointer(cpp_context_stack->items[virtual_base->context_index],
            target, 0, (unsigned *)table_offset, 0);
}

/* Original0x4d320..0x4d388, stdcall12. A zero vbtable selector
   contributes no owner adjustment; retain the original conditional IR shape. */
IrToken *__stdcall adjust_virtual_member_displacement(IrToken *table_offset,
    IrToken *adjustment, int displacement)
{
    IrToken *condition, *alternatives, *choice, *zero, *offset;
    CType *integer_type;
    if (!displacement) return adjustment;
    condition = new_ir_expression(0x44, table_offset, make_integer_constant(0), type_0064);
    zero = make_integer_constant(0);
    offset = make_integer_constant(displacement);
    alternatives = new_left_typed_expression(0x3f, offset, zero);
    choice = new_right_typed_expression(0x5f, condition, alternatives);
    integer_type = builtin_integer_type();
    return new_ir_expression(0x5a, adjustment, choice, integer_type);
}

QualifierBinding *__stdcall first_virtual_conversion_path(QualifierBinding *path);
QualifierBinding *__stdcall last_virtual_conversion_path(QualifierBinding *path);
MemberConversionKey *__stdcall new_member_conversion_key(CppSymbol *target,
    CppSymbol *source, int forward, short arena);
MemberConversionKey *__stdcall copy_member_conversion_key(MemberConversionKey *key, short arena);
CppSymbol *__stdcall new_member_conversion_table(MemberConversionKey *key);

/* Original0x4d4c0..0x4d56e, stdcall8. Retain generated conversion tables
   by their actual class-pair/direction key, materializing them at finish_types. */
IrToken *__stdcall member_conversion_table_reference(QualifierBinding *path, int forward)
{
    QualifierBinding *first, *last;
    CppSymbol *target, *source, *symbol;
    BaseClassLink *base;
    MemberConversionKey *key;
    first = first_virtual_conversion_path(path);
    if (!first) return 0;
    last = last_virtual_conversion_path(path);
    base = (BaseClassLink *)first->target;
    target = cpp_context_stack->items[base->context_index];
    source = (CppSymbol *)last->target;
    if ((source->kind & 31) == 15)
        source = cpp_context_stack->items[((BaseClassLink *)source)->context_index];
    key = new_member_conversion_key(target, source, forward, 2);
    if (!deferred_type_tables) deferred_type_tables = new_hash_table(32, 0, 9, 8);
    symbol = find_hash_value(deferred_type_tables, key);
    if (!symbol) {
        symbol = new_member_conversion_table(key);
        set_hash_value(deferred_type_tables, copy_member_conversion_key(key, 0), symbol);
    }
    return symbol_reference(symbol);
}

int __stdcall member_conversion_preserves_virtual_layout(QualifierBinding *path);

/* Original0x4da50..0x4daa0, stdcall4. Find the first base edge
   needing a virtual-layout conversion, stopping at the owning declaration. */
QualifierBinding *__stdcall first_virtual_conversion_path(QualifierBinding *path)
{
    BaseClassLink *base;
    for (; path; path = path->next) {
        base = (BaseClassLink *)path->target;
        if ((base->kind & 31) != 15) return 0;
        if (base->state & 1) return path;
        if (base->displacement && !member_conversion_preserves_virtual_layout(path))
            return path;
    }
    return 0;
}

int __stdcall same_virtual_base_layout(CppSymbol *first, CppSymbol *second);

/* Original0x4daf0..0x4db2d, stdcall4. Compare the actual two class
   owners joined by this base edge, unwrapping the next base-list record. */
int __stdcall member_conversion_preserves_virtual_layout(QualifierBinding *path)
{
    BaseClassLink *base = (BaseClassLink *)path->target;
    CppSymbol *first = cpp_context_stack->items[base->context_index];
    CppSymbol *second = (CppSymbol *)path->next->target;
    if ((second->kind & 31) != 4)
        second = cpp_context_stack->items[((BaseClassLink *)second)->context_index];
    return same_virtual_base_layout(first, second);
}

/* Original0x4db30..0x4dbad, stdcall8. Compare each first owner's
   virtual-base table position with the second owner's corresponding entry. */
int __stdcall same_virtual_base_layout(CppSymbol *first, CppSymbol *second)
{
    VirtualBaseSearch search;
    unsigned first_pointer, second_pointer, first_table, second_table;
    CType *pointer_type;
    begin_virtual_base_search(&search, first);
    while (next_virtual_base_search(&search)) {
        /* Original ignores both query returns. A failed query leaves the
           corresponding output local unwritten; preserve that defect/read. */
        query_virtual_base_pointer(search.tag, first, &first_pointer, &first_table, &pointer_type);
        query_virtual_base_pointer(search.tag, second, &second_pointer, &second_table, &pointer_type);
        if (first_table != second_table) return 0;
    }
    return 1;
}

IrToken *__stdcall preserve_virtual_data_null(IrToken *converted, IrToken *original_table);

/* Original0x4c590..0x4c6bc, stdcall24. Convert the data offset and
   virtual selector together while retaining the source's null selector. */
IrToken *__stdcall convert_virtual_data_member_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    IrToken *saved, *prefix = 0, *storage, *original_table, *table_offset, *adjustment;
    IrToken *table, *result;
    CppSymbol *source_owner, *target_owner;
    int owner_adjustment;
    if (!virtual_base && !displacement) {
        expression->type = type;
        return expression;
    }
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    original_table = virtual_data_table_offset(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    table_offset = virtual_data_table_offset(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    adjustment = virtual_data_member_offset(storage, expression);
    source_owner = member_pointer_owner_class(expression->type);
    target_owner = member_pointer_owner_class(type);
    compute_member_pointer_adjustment(target_owner, source_owner, displacement,
        virtual_base, &owner_adjustment, 0);
    adjustment = adjust_virtual_member_displacement(clone_expression(table_offset, 0),
        adjustment, owner_adjustment);
    table = member_conversion_table_reference(path, forward);
    if (table)
        table_offset = indirect_storage_expression(
            new_left_typed_expression(0x5a, table, table_offset));
    result = make_virtual_data_member_pointer(type, adjustment, table_offset);
    result = preserve_virtual_data_null(result, original_table);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4c320..0x4c362, stdcall8. The aggregate data-member
   null value is selected by the source's reserved vbtable index minus one. */
IrToken *__stdcall preserve_virtual_data_null(IrToken *converted, IrToken *original_table)
{
    IrToken *null_index, *condition, *null_value, *alternatives;
    null_index = make_integer_constant(-1);
    condition = new_ir_expression(0x44, original_table, null_index, type_0064);
    null_value = make_null_member_pointer(converted->type);
    alternatives = new_left_typed_expression(0x3f, null_value, converted);
    return new_right_typed_expression(0x5f, condition, alternatives);
}

IrToken *__stdcall general_method_adjustment_value(IrToken *storage, IrToken *expression);

/* Original0x4d0f0..0x4d2e5, stdcall24. Adapt the general method
   record according to the two owners' actual virtual-base layouts. */
IrToken *__stdcall convert_general_method_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    CppSymbol *source = member_pointer_owner_class(expression->type);
    CppSymbol *target = member_pointer_owner_class(type);
    IrToken *saved, *prefix = 0, *storage, *function, *adjustment, *table_offset;
    IrToken *table, *result;
    unsigned pointer_offset, table_index;
    int source_virtual, target_virtual;
    if (!virtual_base && !displacement) {
        expression->type = type;
        return expression;
    }
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    function = member_pointer_function_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    adjustment = general_method_adjustment_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    member_pointer_vbptr_offset_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    table_offset = member_pointer_general_virtual_offset_value(storage, expression);
    source_virtual = source->detail.class_info->flags & 0x10;
    target_virtual = target->detail.class_info->flags & 0x10;
    if (!source_virtual && !target_virtual) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        table_offset = make_integer_constant(0);
        pointer_offset = 0;
    } else if (!source_virtual && target_virtual) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        /* Original ignores failure and then consumes its two output locals;
           preserve those unwritten locals rather than inventing a fallback. */
        query_virtual_base_pointer(cpp_context_stack->items[virtual_base->context_index],
            target, &pointer_offset, &table_index, 0);
        table_offset = make_integer_constant(table_index);
    } else if (source_virtual && !target_virtual) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        table_offset = make_integer_constant(0);
        pointer_offset = 0;
    } else {
        adjustment = adjust_virtual_member_displacement(clone_expression(table_offset, 0),
            adjustment, displacement);
        if (forward) source = target;
        class_virtual_base_pointer_layout(source, &pointer_offset, 0);
        table = member_conversion_table_reference(path, forward);
        if (table)
            table_offset = indirect_storage_expression(
                new_left_typed_expression(0x5a, table, table_offset));
    }
    result = make_general_method_pointer(type, function, adjustment, pointer_offset, table_offset);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4bd90..0x4bda4, stdcall8. General representation always
   stores the this adjustment in integer field zero, without a model switch. */
IrToken *__stdcall general_method_adjustment_value(IrToken *storage, IrToken *expression)
{
    return member_pointer_integer_field(storage, expression, 0);
}

IrToken *__stdcall preserve_general_data_null(IrToken *converted, IrToken *original_table);

/* Original0x4cee0..0x4d0e8, stdcall24. General data-member records
   retain their own reserved-null selector across the owner-layout adaptation. */
IrToken *__stdcall convert_general_data_member_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    CppSymbol *source = member_pointer_owner_class(expression->type);
    CppSymbol *target = member_pointer_owner_class(type);
    IrToken *saved, *prefix = 0, *storage, *original_table, *table_offset, *adjustment;
    IrToken *table, *result;
    unsigned pointer_offset, table_index;
    int source_virtual, target_virtual;
    if (!virtual_base && !displacement) {
        expression->type = type;
        return expression;
    }
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    original_table = general_data_table_offset(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    table_offset = general_data_table_offset(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    general_data_vbptr_offset(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    adjustment = general_data_member_offset(storage, expression);
    source_virtual = source->detail.class_info->flags & 0x10;
    target_virtual = target->detail.class_info->flags & 0x10;
    if (!source_virtual && !target_virtual) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        table_offset = make_integer_constant(0);
        pointer_offset = 0;
    } else if (!source_virtual && target_virtual) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        /* Original ignores failure and then reads these output locals. */
        query_virtual_base_pointer(cpp_context_stack->items[virtual_base->context_index],
            target, &pointer_offset, &table_index, 0);
        table_offset = make_integer_constant(table_index);
    } else if (source_virtual && !target_virtual) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        table_offset = make_integer_constant(0);
        pointer_offset = 0;
    } else {
        adjustment = adjust_virtual_member_displacement(clone_expression(table_offset, 0),
            adjustment, displacement);
        if (forward) source = target;
        class_virtual_base_pointer_layout(source, &pointer_offset, 0);
        table = member_conversion_table_reference(path, forward);
        if (table)
            table_offset = indirect_storage_expression(
                new_left_typed_expression(0x5a, table, table_offset));
    }
    result = make_general_data_member_pointer(type, adjustment, pointer_offset, table_offset);
    result = preserve_general_data_null(result, original_table);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4c370..0x4c382, stdcall8. Keep the distinct general-data
   wrapper and its call edge even though it shares the virtual null selector. */
IrToken *__stdcall preserve_general_data_null(IrToken *converted, IrToken *original_table)
{
    return preserve_virtual_data_null(converted, original_table);
}

/* Original0x4cda0..0x4ceda, stdcall24. Widen a virtual method
   record into the general layout using the source vbptr's signed direction. */
IrToken *__stdcall convert_virtual_to_general_method_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    CppSymbol *source = member_pointer_owner_class(expression->type);
    CppSymbol *target = member_pointer_owner_class(type);
    unsigned source_pointer = class_virtual_base_pointer_offset(source);
    IrToken *saved, *prefix = 0, *storage, *function, *adjustment, *table_offset;
    IrToken *table, *result;
    unsigned pointer_offset;
    displacement = (int)((unsigned)displacement + (forward ? source_pointer : 0U - source_pointer));
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    function = member_pointer_function_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    adjustment = member_pointer_adjustment_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    table_offset = member_pointer_virtual_offset_value(storage, expression);
    if (!(target->detail.class_info->flags & 0x10)) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        table_offset = make_integer_constant(0);
        pointer_offset = 0;
    } else {
        adjustment = adjust_virtual_member_displacement(clone_expression(table_offset, 0),
            adjustment, displacement);
        class_virtual_base_pointer_layout(target, &pointer_offset, 0);
        table = member_conversion_table_reference(path, forward);
        if (table)
            table_offset = indirect_storage_expression(
                new_left_typed_expression(0x5a, table, table_offset));
    }
    result = make_general_method_pointer(type, function, adjustment, pointer_offset, table_offset);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4cb00..0x4cc42, stdcall24. Widen data-member storage
   while checking the original selector separately for the reserved null value. */
IrToken *__stdcall convert_virtual_to_general_data_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    CppSymbol *source = member_pointer_owner_class(expression->type);
    CppSymbol *target = member_pointer_owner_class(type);
    unsigned source_pointer = class_virtual_base_pointer_offset(source);
    IrToken *saved, *prefix = 0, *storage, *original_table, *adjustment, *table_offset;
    IrToken *table, *result;
    unsigned pointer_offset;
    displacement = (int)((unsigned)displacement + (forward ? source_pointer : 0U - source_pointer));
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    original_table = virtual_data_table_offset(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    adjustment = virtual_data_member_offset(storage, expression);
    if (!(target->detail.class_info->flags & 0x10)) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        table_offset = make_integer_constant(0);
        pointer_offset = 0;
    } else {
        storage = member_pointer_component(expression, 1, &saved);
        table_offset = virtual_data_table_offset(storage, expression);
        adjustment = adjust_virtual_member_displacement(clone_expression(table_offset, 0),
            adjustment, displacement);
        class_virtual_base_pointer_layout(target, &pointer_offset, 0);
        table = member_conversion_table_reference(path, forward);
        if (table)
            table_offset = indirect_storage_expression(
                new_left_typed_expression(0x5a, table, table_offset));
    }
    result = make_general_data_member_pointer(type, adjustment, pointer_offset, table_offset);
    result = preserve_general_data_null(result, original_table);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4cc50..0x4cd9e, stdcall24. Narrow the general method
   record after removing the target owner's direction-dependent vbptr offset. */
IrToken *__stdcall convert_general_to_virtual_method_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    CppSymbol *source = member_pointer_owner_class(expression->type);
    CppSymbol *target = member_pointer_owner_class(type);
    unsigned target_pointer = class_virtual_base_pointer_offset(target), table_index;
    IrToken *saved, *prefix = 0, *storage, *function, *adjustment, *table_offset;
    IrToken *table, *result;
    displacement = (int)((unsigned)displacement - (forward ? target_pointer : 0U - target_pointer));
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    function = member_pointer_function_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    adjustment = member_pointer_adjustment_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    table_offset = member_pointer_general_virtual_offset_value(storage, expression);
    if (!(source->detail.class_info->flags & 0x10)) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        /* The ignored failed-query output remains an original unwritten local. */
        query_virtual_base_pointer(cpp_context_stack->items[virtual_base->context_index],
            target, 0, &table_index, 0);
        table_offset = make_integer_constant(table_index);
    } else {
        adjustment = adjust_virtual_member_displacement(clone_expression(table_offset, 0),
            adjustment, displacement);
        table = member_conversion_table_reference(path, forward);
        if (table)
            table_offset = indirect_storage_expression(
                new_left_typed_expression(0x5a, table, table_offset));
    }
    result = build_virtual_method_pointer(type, function, adjustment, table_offset);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4c9a0..0x4cafa, stdcall24. Narrow general data storage,
   preserving the original selector independently of any converted table load. */
IrToken *__stdcall convert_general_to_virtual_data_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    CppSymbol *source = member_pointer_owner_class(expression->type);
    CppSymbol *target = member_pointer_owner_class(type);
    unsigned target_pointer = class_virtual_base_pointer_offset(target), table_index;
    IrToken *saved, *prefix = 0, *storage, *original_table, *adjustment, *table_offset;
    IrToken *table, *result;
    displacement = (int)((unsigned)displacement - (forward ? target_pointer : 0U - target_pointer));
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    original_table = general_data_table_offset(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    adjustment = general_data_member_offset(storage, expression);
    if (!(source->detail.class_info->flags & 0x10)) {
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
        /* Original failure leaves this output local unwritten before use. */
        query_virtual_base_pointer(cpp_context_stack->items[virtual_base->context_index],
            target, 0, &table_index, 0);
        table_offset = make_integer_constant(table_index);
    } else {
        storage = member_pointer_component(expression, 1, &saved);
        table_offset = general_data_table_offset(storage, expression);
        adjustment = adjust_virtual_member_displacement(clone_expression(table_offset, 0),
            adjustment, displacement);
        table = member_conversion_table_reference(path, forward);
        if (table)
            table_offset = indirect_storage_expression(
                new_left_typed_expression(0x5a, table, table_offset));
    }
    result = make_virtual_data_member_pointer(type, adjustment, table_offset);
    result = preserve_virtual_data_null(result, original_table);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4daa0..0x4daec, stdcall4. Search from the enclosing
   declaration back toward the source; yield the owner after the last edge. */
QualifierBinding *__stdcall last_virtual_conversion_path(QualifierBinding *path)
{
    QualifierBinding *last;
    BaseClassLink *base;
    if (!path || (((CppSymbol *)path->target)->kind & 31) == 4) return 0;
    last = last_virtual_conversion_path(path->next);
    if (last) return last;
    base = (BaseClassLink *)path->target;
    if (base->state & 1) return path->next;
    if (base->displacement && !member_conversion_preserves_virtual_layout(path))
        return path->next;
    return 0;
}

/* Original0x4d710..0x4d768, stdcall16. Keep the three initialized
   class-pair/direction words in the requested actual compiler arena. */
MemberConversionKey *__stdcall new_member_conversion_key(CppSymbol *target,
    CppSymbol *source, int forward, short arena)
{
    MemberConversionKey *key;
    ArenaPool *pool;
    if (!arena) key = allocate_permanent(sizeof(MemberConversionKey));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(MemberConversionKey))
            refill_arena(&compiler_arena, pool);
        key = (MemberConversionKey *)pool->next;
        pool->remaining -= sizeof(MemberConversionKey);
        pool->next += sizeof(MemberConversionKey);
    }
    key->target = target;
    key->source = source;
    key->forward = forward;
    return key;
}

char *__stdcall name_member_conversion_table(CppSymbol *symbol, MemberConversionKey *key);

/* Original0x4d570..0x4d6c0, stdcall4. Declare the generated vdispmap
   as a retained integer array, sized from the selected owner's virtual groups. */
CppSymbol *__stdcall new_member_conversion_table(MemberConversionKey *key)
{
    CppSymbol *symbol = allocate_permanent(sizeof(CppSymbol));
    ArenaPool *pool = &compiler_arena.pools[2];
    DeclarationType context;
    CppSymbol *owner;
    TypeShape *array;
    unsigned short attributes;
    int count;
    symbol->kind = 1;
    symbol->name = name_vdispmap;
    if (token_allocation_flags & 0x20) symbol->type = allocate_permanent(sizeof(CType));
    else {
        if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena, pool);
        symbol->type = (CType *)pool->next;
        pool->remaining -= sizeof(CType);
        pool->next += sizeof(CType);
    }
    attributes = normalize_type_attributes(2, 0);
    symbol->type->attributes = attributes;
    symbol->flags |= 0x80;
    symbol->flags |= 4;
    symbol->name = name_member_conversion_table(symbol, key);
    if ((symbol->type->attributes & 7) == 2)
        apply_symbol_based_type(symbol, (CppSymbol *)(default_far_code_segment ?
            default_far_code_segment : default_far_data_segment));
    owner = key->forward ? key->target : key->source;
    count = vector_count(owner->detail.class_info->virtual_table_groups) + 1;
    array = new_type_shape(2, attributes, &count);
    attach_type_shape(symbol, array);
    context.type = builtin_integer_type();
    /* The three copied storage bytes replace all prior local bits except
       the already determined kind1; no undefined high local bits survive. */
    context.storage[0] = (declaration_storage[0] & ~7) | 0x41;
    context.storage[1] = declaration_storage[1] | 0x20;
    context.storage[2] = declaration_storage[2];
    symbol = apply_declaration_type(&context, symbol);
    symbol->declaration_flags = (symbol->declaration_flags & ~0x100) | 0x200;
    return symbol;
}

/* Original0x4d6c0..0x4d704, stdcall8. Decorate vdispmap with both
   real class owners while preserving its pending declaration type. */
char *__stdcall name_member_conversion_table(CppSymbol *symbol, MemberConversionKey *key)
{
    CType *saved = symbol->type;
    char *name;
    symbol->type = type_0064_q1000;
    symbol->storage[0] = (symbol->storage[0] & ~5) | 2;
    symbol->declaration_flags &= ~0x300;
    symbol->auxiliary.conversion_source = key->source;
    name = intern_identifier(write_decorated_name(0, symbol, key->target, 0));
    symbol->type = saved;
    return name;
}

/* Original0x4d770..0x4d7c8, stdcall8. Retain all three key words
   independently of the temporary lookup key and its arena lifetime. */
MemberConversionKey *__stdcall copy_member_conversion_key(MemberConversionKey *key, short arena)
{
    MemberConversionKey *copy;
    ArenaPool *pool;
    if (!arena) copy = allocate_permanent(sizeof(MemberConversionKey));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(MemberConversionKey)) refill_arena(&compiler_arena, pool);
        copy = (MemberConversionKey *)pool->next;
        pool->remaining -= sizeof(MemberConversionKey);
        pool->next += sizeof(MemberConversionKey);
    }
    *copy = *key;
    return copy;
}

/* Original0x4c420..0x4c4be, stdcall24. The original tests the TARGET
   model for2, although this dispatch normally targets3. Preserve that check:
   replacing it with a source-model test would repair the historical2-to3 bug. */
IrToken *__stdcall convert_compact_to_virtual_method_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    int adjustment_word, table_word;
    IrToken *saved, *storage, *function, *adjustment, *table_offset;
    CppSymbol *owner = member_pointer_owner_class(type);
    compute_member_pointer_adjustment(owner, 0, displacement, virtual_base,
        &adjustment_word, &table_word);
    if (member_pointer_inheritance_model(type) == 2) {
        storage = member_pointer_component(expression, 0, &saved);
        function = member_pointer_function_value(storage, expression);
        storage = member_pointer_component(expression, 1, &saved);
        adjustment = member_pointer_adjustment_value(storage, expression);
        adjustment = adjust_member_pointer_displacement(adjustment, adjustment_word);
    } else {
        function = expression;
        adjustment = make_integer_constant(adjustment_word);
    }
    /* No virtual base, or an ignored failed query in the helper, leaves
       table_word unwritten. Preserve that local/read without a fallback. */
    table_offset = make_integer_constant(table_word);
    return build_virtual_method_pointer(type, function, adjustment, table_offset);
}

int __stdcall data_member_conversion_needs_null_check(IrToken *expression);

/* Original0x4c180..0x4c23e, stdcall24. Widen a compact data offset
   and preserve null through the real one-evaluation object/condition pair. */
IrToken *__stdcall convert_compact_to_virtual_data_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    CppSymbol *owner = member_pointer_owner_class(type);
    int adjustment_word, table_word;
    IrToken *value, *address, *offset, *table_offset, *converted;
    CType *integer_type;
    compute_member_pointer_adjustment(owner, 0, displacement, virtual_base,
        &adjustment_word, &table_word);
    if (data_member_conversion_needs_null_check(expression)) {
        prepare_data_member_object(expression, &value, &address);
        integer_type = builtin_integer_type();
        offset = make_member_at_offset(address, adjustment_word, integer_type);
        /* As in the method converter, absent/failed query leaves the original
           table_word local unwritten; do not initialize a replacement value. */
        table_offset = make_integer_constant(table_word);
        converted = make_virtual_data_member_pointer(type, offset, table_offset);
        return preserve_null_data_member_conversion(converted, value, expression->type);
    }
    integer_type = builtin_integer_type();
    offset = make_member_at_offset(expression, adjustment_word, integer_type);
    table_offset = make_integer_constant(table_word);
    return make_virtual_data_member_pointer(type, offset, table_offset);
}

/* Original0x4c2a0..0x4c2ce, stdcall4. A known non-null constant
   needs no conditional preservation; unknown values and null constants do. */
int __stdcall data_member_conversion_needs_null_check(IrToken *expression)
{
    IrToken *null_value;
    if (expression && expression->kind == 7) {
        null_value = make_null_member_pointer(expression->type);
        if (null_value->value.integer != expression->value.integer) return 0;
    }
    return 1;
}

int __stdcall find_virtual_base_pointer(CppSymbol *base, CppSymbol *derived,
    unsigned *pointer_offset, unsigned *table_offset, CType **pointer_type);
void __stdcall write_member_conversion_table(CppSymbol *symbol, unsigned *values, int count);

/* Original0x4d870..0x4d9b8, stdcall8. Materialize the deferred class
   map using the original direction, table-byte indices and integer width. */
void __stdcall emit_deferred_type_table(void *description, CppSymbol *symbol)
{
    MemberConversionKey *key = description;
    unsigned width = builtin_integer_type()->flags & 15;
    CppSymbol *owner = key->forward ? key->target : key->source;
    int count = vector_count(owner->detail.class_info->virtual_table_groups) + 1;
    unsigned bytes = ((unsigned)count * sizeof(unsigned) + 3) & ~3U;
    ArenaPool *pool = &compiler_arena.pools[2];
    unsigned *values;
    VirtualBaseSearch search;
    unsigned source_pointer, target_pointer, source_index, target_index;
    if (pool->remaining < (int)bytes) refill_arena(&compiler_arena, pool);
    values = (unsigned *)pool->next;
    pool->remaining -= bytes;
    pool->next += bytes;
    if (key->forward) {
        if (find_virtual_base_pointer(key->target, key->source,
            &source_pointer, &source_index, 0)) values[0] = source_index;
        else values[0] = 0;
    }
    begin_virtual_base_search(&search, key->forward ? key->target : key->source);
    while (next_virtual_base_search(&search)) {
        /* The first query's ignored failure leaves its output words untouched,
           whereas the second explicitly substitutes a zero table index. */
        query_virtual_base_pointer(search.tag, key->source,
            &source_pointer, &source_index, 0);
        if (!find_virtual_base_pointer(search.tag, key->target,
            &target_pointer, &target_index, 0)) target_index = 0;
        if (key->forward) values[target_index / width] = source_index;
        else values[source_index / width] = target_index;
    }
    write_member_conversion_table(symbol, values, count);
}

/* Original0x4d9c0..0x4da4b, stdcall12. Emit each retained map word
   through the ordinary initializer stream and its actual integer type. */
void __stdcall write_member_conversion_table(CppSymbol *symbol, unsigned *values, int count)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    CppSymbol *element;
    IrToken *value;
    if (pool->remaining < (int)sizeof(CppSymbol)) refill_arena(&compiler_arena, pool);
    element = (CppSymbol *)pool->next;
    pool->remaining -= sizeof(CppSymbol);
    pool->next += sizeof(CppSymbol);
    symbol->attributes |= 4;
    symbol->storage[0] = (symbol->storage[0] & ~5) | 2;
    write_literal_initializer(symbol, 0, 0);
    element->kind = 9;
    element->name = 0;
    while (count > 0) {
        value = make_integer_constant(*values++);
        element->type = value->type;
        write_initializer_value(element, value, 0);
        --count;
    }
    flush_initializer_records();
}

/* Original0x4ab10..0x4ab8f, stdcall24. An incomplete general owner
   carries its vbptr and table offsets in the runtime member-pointer value.
   The shared six-word ABI retains unused address/type formals. */
IrToken *__stdcall adjust_general_member_pointer_object(IrToken *object,
    IrToken *address, IrToken *adjustment, IrToken *pointer_offset,
    IrToken *virtual_offset, CType *type)
{
    CType *integer_type = builtin_integer_type();
    CType *array_type = make_qualified_type(integer_type, 2, 0, 0);
    unsigned short attributes = normalize_type_attributes(1, 0);
    CType *pointer_type = make_qualified_type(array_type, 1, attributes, 0);
    IrToken *offset = clone_expression(pointer_offset, 0);
    IrToken *base = new_ir_expression(0x5a, object, offset, pointer_type);
    IrToken *table = indirect_storage_expression(clone_expression(base, 0));
    IrToken *result;
    table = new_right_typed_expression(0x5a, table, virtual_offset);
    table = indirect_storage_expression(table);
    result = new_left_typed_expression(0x5a, base, table);
    return new_left_typed_expression(0x5a, result, adjustment);
}

/* Original0x4c8d0..0x4c996, stdcall24. As in its virtual sibling,
   this function tests the target model for2; retain that historical check. */
IrToken *__stdcall convert_compact_to_general_method_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    unsigned pointer_offset = 0, table_index = 0;
    CppSymbol *base, *owner;
    IrToken *saved, *storage, *function, *adjustment, *table_offset;
    if (virtual_base) {
        base = cpp_context_stack->items[virtual_base->context_index];
        owner = member_pointer_owner_class(type);
        query_virtual_base_pointer(base, owner, &pointer_offset, &table_index, 0);
    }
    if (member_pointer_inheritance_model(type) == 2) {
        storage = member_pointer_component(expression, 0, &saved);
        function = member_pointer_function_value(storage, expression);
        storage = member_pointer_component(expression, 1, &saved);
        adjustment = member_pointer_adjustment_value(storage, expression);
        adjustment = adjust_member_pointer_displacement(adjustment, displacement);
    } else {
        function = expression;
        adjustment = make_integer_constant(displacement);
    }
    table_offset = make_integer_constant(table_index);
    return make_general_method_pointer(type, function, adjustment, pointer_offset, table_offset);
}

/* Original0x4c750..0x4c83b, stdcall24. The general representation
   starts with explicit zero vbptr/selector words before any optional lookup. */
IrToken *__stdcall convert_compact_to_general_data_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    unsigned pointer_offset = 0, table_index = 0;
    CppSymbol *base, *owner;
    IrToken *value, *address, *offset, *table_offset, *converted;
    CType *integer_type;
    if (virtual_base) {
        base = cpp_context_stack->items[virtual_base->context_index];
        owner = member_pointer_owner_class(type);
        query_virtual_base_pointer(base, owner, &pointer_offset, &table_index, 0);
    }
    if (data_member_conversion_needs_null_check(expression)) {
        prepare_data_member_object(expression, &value, &address);
        integer_type = builtin_integer_type();
        offset = make_member_at_offset(address, displacement, integer_type);
        table_offset = make_integer_constant(table_index);
        converted = make_general_data_member_pointer(type, offset, pointer_offset, table_offset);
        return preserve_null_data_member_conversion(converted, value, expression->type);
    }
    integer_type = builtin_integer_type();
    offset = make_member_at_offset(expression, displacement, integer_type);
    table_offset = make_integer_constant(table_index);
    return make_general_data_member_pointer(type, offset, pointer_offset, table_offset);
}

/* Original0x4c840..0x4c8c4, stdcall24. Retain the source evaluation
   and computed adjustment even when the compact target subsequently drops it. */
IrToken *__stdcall convert_general_to_compact_method_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    IrToken *saved, *prefix = 0, *storage, *function, *adjustment, *result;
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    function = member_pointer_function_value(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    adjustment = general_method_adjustment_value(storage, expression);
    adjustment = adjust_member_pointer_displacement(adjustment, displacement);
    result = make_compact_method_pointer(type, function, adjustment);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4d2f0..0x4d317, stdcall8. Reuse the incoming expression
   for a zero displacement; otherwise retain ordinary typed addition. */
IrToken *__stdcall adjust_member_pointer_displacement(IrToken *adjustment, int displacement)
{
    IrToken *offset;
    if (!displacement) return adjustment;
    offset = make_integer_constant(displacement);
    return new_left_typed_expression(0x5a, adjustment, offset);
}

/* Original0x4c6c0..0x4c746, stdcall24. Narrow the stored data offset,
   retaining a separate read of the general representation's null selector. */
IrToken *__stdcall convert_general_to_compact_data_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    IrToken *saved, *prefix = 0, *storage, *table_offset, *offset, *result;
    if (!can_reuse_member_pointer_storage(expression))
        prefix = member_pointer_component(expression, 0, &saved);
    storage = member_pointer_component(expression, prefix != 0, &saved);
    table_offset = general_data_table_offset(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    offset = general_data_member_offset(storage, expression);
    offset = adjust_member_pointer_displacement(offset, displacement);
    offset->type = type;
    result = preserve_general_data_null(offset, table_offset);
    if (prefix) result = new_right_typed_expression(0x40, prefix, result);
    return result;
}

/* Original0x4c4c0..0x4c58f, stdcall24. Single inheritance keeps
   only the function address; multiple inheritance subtracts the source layout. */
IrToken *__stdcall convert_virtual_to_compact_method_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    unsigned model = member_pointer_inheritance_model(type);
    IrToken *saved, *storage, *function, *adjustment, *offset, *result;
    CppSymbol *owner;
    int owner_adjustment;
    if (model == 1) {
        storage = modifiable_lvalue(expression, 0);
        function = member_pointer_function_value(storage, expression);
        return make_compact_method_pointer(type, function, 0);
    }
    if (model == 2) {
        owner = member_pointer_owner_class(expression->type);
        compute_member_pointer_adjustment(owner, 0, displacement, virtual_base,
            &owner_adjustment, 0);
        storage = member_pointer_component(expression, 0, &saved);
        function = member_pointer_function_value(storage, expression);
        storage = member_pointer_component(expression, 1, &saved);
        adjustment = member_pointer_adjustment_value(storage, expression);
        if (owner_adjustment) {
            offset = make_integer_constant(owner_adjustment);
            adjustment = new_left_typed_expression(0x4f, adjustment, offset);
        }
        return make_compact_method_pointer(type, function, adjustment);
    }
    /* Original defect: other models return the unwritten entryESP-4 local.
       The conversion table selects this helper for compact targets only. */
    return result;
}

/* Original0x4c390..0x4c419, stdcall24. Subtract the source virtual
   layout adjustment before assigning the compact data-member type. */
IrToken *__stdcall convert_virtual_to_compact_data_pointer(IrToken *expression, CType *type,
    QualifierBinding *path, int forward, int displacement, BaseClassLink *virtual_base)
{
    CppSymbol *owner = member_pointer_owner_class(expression->type);
    IrToken *saved, *storage, *offset, *table_offset, *position;
    int owner_adjustment;
    compute_member_pointer_adjustment(owner, 0, displacement, virtual_base,
        &owner_adjustment, 0);
    storage = member_pointer_component(expression, 0, &saved);
    offset = virtual_data_member_offset(storage, expression);
    storage = member_pointer_component(expression, 1, &saved);
    table_offset = virtual_data_table_offset(storage, expression);
    if (owner_adjustment) {
        position = make_integer_constant(owner_adjustment);
        offset = new_left_typed_expression(0x4f, offset, position);
    }
    offset->type = type;
    return preserve_virtual_data_null(offset, table_offset);
}

/* Original0x787e0..0x78808, stdcall4. Skip the selected member
   and base-link components to find its actual enclosing class qualifier. */
CppSymbol *__stdcall enclosing_qualifier_class(QualifierBinding *binding)
{
    for (binding = binding->next; binding; binding = binding->next)
        if ((binding->target->kind & 31) == 4)
            return (CppSymbol *)binding->target;
    return 0;
}
