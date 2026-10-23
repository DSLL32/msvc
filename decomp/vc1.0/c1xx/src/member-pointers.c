/* C++ pointers to nonstatic members and their inheritance representations. */
#include "lexer.h"
#include "call-expressions.h"
#include "expression-records.h"
#include "special-members.h"
#include "numeric-conversion.h"
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
