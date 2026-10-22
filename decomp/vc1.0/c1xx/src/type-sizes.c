/* Sizes of base types, arrays, pointers and C++ member pointers. */
#include "type-sizes.h"

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
