/* Canonical type sets in the compiler's permanent inline vectors. */
#include "class-type-sets.h"
#include <recon.h>

/* Original 0x4e370..0x4e37a. Keep the empty-set factory as a separate call. */
TypeSet *new_type_set(void)
{
    return new_inline_vector(0);
}

/* Original 0x309d0..0x309e3. Only the list and discriminator are initialized;
   the result fields become valid when the iterator advances. */
void __stdcall begin_conversion_functions(ConversionIterator *iterator, CppSymbol *tag)
{
    iterator->next = tag->auxiliary.scope_members;
    iterator->kind = 17;
}

/* Original 0x309f0..0x30a48. Class overload sets with the conversion flag
   supply their first function and its canonical target type. */
int __stdcall next_conversion_function(ConversionIterator *iterator)
{
    for (;;) {
        iterator->current = (OverloadSet *)iterator->next;
        if (!iterator->current) return 0;
        iterator->next = iterator->current->next;
        if ((iterator->current->kind ^ iterator->kind) & 31) continue;
        if (iterator->current->flags & 0x80) continue;
        if (!(iterator->current->declaration_flags & 0x4000)) continue;
        iterator->target = iterator->current->functions;
        iterator->type = canonical_conversion_type(iterator->target);
        return 1;
    }
}

/* Original 0x668c0..0x669aa. Empty sets share the existing permanent empty
   vector; nonempty sets can share either inherited conversion-target list. */
TypeSet *__stdcall share_inherited_type_set(TypeSet *set, CppSymbol *tag)
{
    TypeSet *shared = 0;
    BaseClassLink *base;
    CppSymbol *base_tag;
    if (!set) return 0;
    if (!inline_vector_count(set)) shared = empty_inline_vector;
    else {
        for (base = (BaseClassLink *)tag->auxiliary.scope_members; base;
                base = (BaseClassLink *)base->next) {
            if ((base->kind & 31) != 15 || (base->flags & 0x80)) continue;
            base_tag = cpp_context_stack->items[base->context_index];
            if (shared) break;
            if (equal_inline_vectors(set, base_tag->detail.class_info->conversion_types))
                shared = base_tag->detail.class_info->conversion_types;
            else if (equal_inline_vectors(set, base_tag->detail.class_info->ambiguous_conversion_types))
                shared = base_tag->detail.class_info->ambiguous_conversion_types;
        }
    }
    if (!shared) return set;
    release_inline_vector(set);
    return shared;
}
