/* Sizes of base types, arrays, pointers and C++ member pointers. */
#include "type-sizes.h"
#include "ir.h"
#include <recon.h>

/* Original 0x4f2d0..0x4f321, stdcall4. Skip an indirect declaration's
   leading shape, then ignore array dimensions to measure one scalar element. */
int __stdcall symbol_element_size(CppSymbol *symbol)
{
    CType *type = symbol->type;
    TypeShape *shape = type->related;
    if (symbol->attributes & 2) shape = shape->related;
    while (shape && !(shape->flags & 1)) shape = shape->related;
    if (shape) return (int)pointer_kind_size(shape->attributes);
    return (int)base_type_size(type->flags, cpp_context_stack->items[type->field08]);
}

/* Original 0x4f160..0x4f215. Array multiplication wraps in the original
   32-bit IMUL before its signed comparison. Recovery changes the bound to
   one while returning that already computed product. */
int __stdcall type_size(CType *type)
{
    TypeShape *shape = type->related;
    unsigned bytes;
    int element;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (!shape) return (int)base_type_size(type->flags, cpp_context_stack->items[type->field08]);
    if ((shape->flags & 0x77) == 2) {
        element = (int)inner_type_size(type->flags, cpp_context_stack->items[type->field08], shape);
        bytes = (unsigned)element * (unsigned)shape->data.array_bound;
        if ((int)bytes < element && shape->data.array_bound) {
            report_syntax_error(148);
            shape->data.array_bound = 1;
        }
        return (int)bytes;
    }
    if (shape->flags & 0x10) return (int)data_member_pointer_size(shape);
    if (shape->flags & 0x20) return (int)member_function_pointer_size(shape);
    return (int)pointer_kind_size(shape->attributes);
}

/* Original 0x4f280..0x4f2cc. ClassInfo's first word is complete-object size;
   its separate nonvirtual_size word is used for base subobjects. */
unsigned __stdcall base_type_size(unsigned short flags, CppSymbol *tag)
{
    if (flags & 0x400) {
        if ((tag->kind & 31) != 4) tag = cpp_context_stack->items[tag->type->field08];
        return tag->detail.class_info->size;
    }
    if (flags & 0x200) return 4;
    return flags & 15;
}

/* Original 0x4f330..0x4f370. Pointer representation sizes in the NT model. */
unsigned __stdcall pointer_kind_size(unsigned short attributes)
{
    switch (attributes & 7) {
    case 0: case 1: case 3: case 4: case 5: case 6: case 7:
        return 4;
    case 2:
        return 8;
    }
    return 0;
}

/* Original 0x565b0..0x565f9. Arrays retain the base object's alignment;
   other shape prefixes denote a four-byte pointer representation here. */
unsigned short __stdcall type_alignment(CType *type)
{
    TypeShape *shape = type->related;
    if (shape && (shape->flags & 0x77) != 2) return 4;
    if (type->flags & 0x400)
        return ((CppSymbol *)cpp_context_stack->items[type->field08])->detail.class_info->alignment_flags & 0x3f;
    return type->flags & 15;
}

typedef char ClassAlignmentOffsetMustBe50[
    (offsetof(ClassInfo, alignment_flags) == 50) ? 1 : -1];

/* Original 0x4ef30..0x4ef64. Diagnose zero-sized/incomplete types but
   retain the size computed before the diagnostic helper. */
unsigned __stdcall complete_type_size(CType *type)
{
    unsigned bytes = inner_type_size(type->flags,
        cpp_context_stack->items[type->field08], type->related);
    if (!bytes) require_complete_type(type);
    return bytes;
}

/* Original 0x4ef70..0x4f15a. Measure the type inside the outer shape:
   pointer and array prefixes inspect their next shape before choosing a
   representation size. Only the array-prefix multiplication diagnoses wrapped
   signed overflow; the nested-array branch beneath a pointer multiplies raw. */
unsigned __stdcall inner_type_size(unsigned short flags, CppSymbol *tag, TypeShape *shape)
{
    TypeShape *element;
    unsigned bytes, product;
    for (;;) {
        if (!shape) return base_type_size(flags, tag);
        switch (shape->flags & 0x77) {
        case 1:
            element = shape->related;
            if (!element) return base_type_size(flags, tag);
            switch (element->flags & 0x77) {
            case 1: case 3: case 0x40: case 0x44:
                return pointer_kind_size(element->attributes);
            case 2:
                bytes = inner_type_size(flags, tag, element);
                return bytes * (unsigned)element->data.array_bound;
            case 0x10:
                return data_member_pointer_size(element);
            case 0x20:
                return member_function_pointer_size(element);
            }
            return 0;
        case 2:
            element = shape->related;
            if (element) {
                if ((element->flags & 1) || (element->flags & 0x40))
                    return pointer_kind_size(element->attributes);
                if (element->flags & 0x20) return member_function_pointer_size(element);
                if (element->flags & 0x10) return data_member_pointer_size(element);
            }
            bytes = inner_type_size(flags, tag, element);
            if (!element) return bytes;
            product = bytes * (unsigned)element->data.array_bound;
            if ((int)product < (int)bytes && element->data.array_bound) {
                report_syntax_error(148);
                element->data.array_bound = 1;
            }
            return product;
        case 4:
            shape = shape->related;
            if (!shape) return base_type_size(flags, tag);
            if (shape->flags & 1) return pointer_kind_size(shape->attributes);
            break;
        case 3: case 0x40: case 0x44:
            return pointer_kind_size(shape->attributes);
        case 0x10:
            return data_member_pointer_size(shape);
        case 0x20:
            return member_function_pointer_size(shape);
        default:
            return 0;
        }
    }
}

/* Original 0x4e1f0..0x4e26a, stdcall4. A method pointer stores its
   callable address followed by the inheritance model's integer adjustments. */
unsigned __stdcall member_function_pointer_size(TypeShape *shape)
{
    unsigned bytes = pointer_kind_size(shape->attributes);
    switch (member_pointer_class_kind(shape->data.tag_symbol)) {
    case 0:
    case 1:
        return bytes;
    case 2:
        return bytes + (builtin_integer_type()->flags & 15);
    case 3:
        return bytes + 2 * (builtin_integer_type()->flags & 15);
    case 4:
        return bytes + 3 * (builtin_integer_type()->flags & 15);
    default:
        return 0;
    }
}
