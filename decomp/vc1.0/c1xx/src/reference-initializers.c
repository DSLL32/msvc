/* Reference binding and temporary lifetime. */
#include "reference-initializers.h"
#include "constructor-initializers.h"
#include "unary-expressions.h"
#include "frontend-records.h"
#include "parser.h"
#include "operators.h"
#include "expression-preparation.h"
#include <recon.h>

/* Original 0x67100..0x671ec, stdcall 8. */
IrToken *__stdcall convert_reference_initializer(CType *type, IrToken *expression)
{
    int extension_binding = 0;
    int is_const;
    CType *referent = dereference_pointer_type(type);
    IrToken *result;

    if (!reference_requires_temporary(referent, expression, &extension_binding))
        return bind_reference_lvalue(referent, type, expression);
    is_const = (type_qualifiers(referent) & 0x1000) != 0;
    if (extension_binding && options.configuration.extensions_enabled) {
        if (!is_const)
            report_type_conversion(270, 2, 0x7e, type, expression->type);
        if (initializing_reference)
            expression = retain_reference_temporary(referent, type, expression);
        return bind_reference_lvalue(referent, type, expression);
    }
    result = try_user_defined_conversion(type, expression, 0);
    if (!result) {
        if (is_const)
            return initialize_reference_temporary(referent, type, expression);
        report_type_conversion(extension_binding ? 609 : 607, 0, 0x7e,
            expression->type, type);
    }
    return result;
}

/* Original 0x671f0..0x67227, stdcall 4. */
int __stdcall reference_source_is_array(CType *type)
{
    if (!type || !type->related) return 0;
    return (type->related->flags & 0x77) == 2;
}

/* Original 0x67230..0x6727a, stdcall 12. */
int __stdcall reference_requires_temporary(CType *referent, IrToken *expression,
    int *extension_binding)
{
    if (!reference_types_compatible(referent, expression->type)) return 1;
    if (reference_lvalue_bindable(expression, extension_binding)) return 0;
    if (reference_source_is_array(expression->type)) return 0;
    return 1;
}

/* Original 0x67350..0x6742a, stdcall 8. Source flags are saved before
   reference stripping, as in the original comparison. */
int __stdcall reference_types_compatible(CType *referent, CType *source)
{
    unsigned short target_flags = referent->flags;
    unsigned short source_flags = source->flags;
    CType *value = strip_reference_type(source);
    TypeShape *shape = referent->related;
    if (value->related != shape) return 0;
    if (shape)
        return !((source_flags ^ target_flags) & 0x2fdf) &&
            value->field08 == referent->field08;
    if ((value->attributes & 0x1000) && !(referent->attributes & 0x1000))
        return 0;
    if (!(referent->flags & 0x9c0) && !shape) {
        if ((source_flags & 0x600) && reference_tag_compatible(referent, value))
            return 1;
        return 0;
    }
    if (((value->flags & 0x9c0) || value->related) && source_flags == target_flags)
        return 1;
    return 0;
}

/* Original 0x62360..0x6237c, stdcall 4. */
CType *__stdcall strip_reference_type(CType *type)
{
    if (type && type->related && (type->related->flags & 0x40))
        type = dereference_pointer_type(type);
    return type;
}

/* Original 0x67430..0x67459, stdcall 8. The ancestry helper's
   arguments are source first, target second. */
int __stdcall reference_tag_compatible(CType *referent, CType *source)
{
    if (source->field08 != referent->field08 &&
        !type_has_base_relation(source, referent)) return 0;
    return 1;
}

/* Original 0x67280..0x672b9, stdcall 8. */
int __stdcall reference_lvalue_bindable(IrToken *expression, int *extension_binding)
{
    if (!modifiable_lvalue(expression, 0)) return 0;
    if (reference_lvalue_needs_extension(expression)) {
        *extension_binding = 1;
        return 0;
    }
    return 1;
}

/* Original 0x672c0..0x67347, stdcall 4. This exceptional lvalue
   form is an indirection of a call whose function shape carries bit 0x200. */
int __stdcall reference_lvalue_needs_extension(IrToken *expression)
{
    IrToken *call, *callee;
    TypeShape *shape;
    if (!expression || expression->kind != OP_DEREFERENCE) return 0;
    call = expression->value.pointer;
    if (!call || call->kind != 0x7a) return 0;
    callee = call->value.pointer;
    if (!callee || !callee->type) return 0;
    shape = callee->type->related;
    if (!shape || (shape->flags & 0x77) != 4) return 0;
    return (shape->flags & 0x200) ? 1 : 0;
}

/* Original 0x674c0..0x67546, stdcall 12. */
IrToken *__stdcall bind_reference_lvalue(CType *referent, CType *reference,
    IrToken *expression)
{
    IrToken *address, *result;
    TypeShape *shape;
    unsigned rank;
    if ((referent->flags & 0x400) && !referent->related) {
        address = try_class_pointer_initializer(reference, expression);
    } else {
        address = builtin_unary_expression(OP_BIT_AND, expression);
        shape = address->type->related;
        rank = rank_pointer_attributes(1,
            (unsigned char)reference->related->attributes,
            (unsigned char)shape->attributes, shape);
        if (rank & 0x2000) {
            report_type_conversion(440, 0, 0x7e, address->type, reference);
            return 0;
        }
        address = new_cast_expression(reference, address);
    }
    result = indirect_storage_expression(address);
    result->type = referent;
    return result;
}
