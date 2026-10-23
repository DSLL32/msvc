/* Selection of class special-member declarations. */
#include "special-member-lookup.h"
#include "class-lookup.h"
#include "cpp-context.h"
#include <recon.h>

/* Original 0x38780..0x38794, stdcall4. Preserve the independent
   vector deleting destructor lookup entry and its shared interned name. */
CppSymbol *__stdcall lookup_vector_delete_destructor(CType *type)
{
    return lookup_special_member(type, name_vector_delete_dtor, 0);
}

/* Original 0x37cd0..0x37db7, stdcall12. Among accepted declarations,
   choose the smallest available pointer model at least the requested model.
   An exact match ends the search; preserve the optional cdecl predicate. */
CppSymbol *__stdcall lookup_special_member(CType *type, char *name,
    SpecialMemberPredicate accept)
{
    CppSymbol *function, *candidate, *best = 0;
    CppSymbol *tag = cpp_context_stack->items[type->field08];
    int requested, selected = -1, model;
    unsigned kind;
    function = find_class_special_member(tag, name);
    if (!function) return 0;
    requested = special_member_model(type_pointer_model(type));
    kind = function->kind & 31;
    if (kind == 16 || kind == 17)
        function = ((OverloadSet *)function)->functions;
    while (function) {
        candidate = function;
        kind = candidate->kind & 31;
        if (kind != 6 && kind != 7) break;
        function = candidate->next;
        if (accept && !accept(candidate)) continue;
        model = generated_member_model(candidate);
        if (model < requested || (selected != -1 && selected <= model)) continue;
        best = candidate;
        selected = model;
        if (requested == model) break;
    }
    return best;
}
