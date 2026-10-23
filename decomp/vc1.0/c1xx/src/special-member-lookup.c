/* Selection of class special-member declarations. */
#include "special-member-lookup.h"
#include "class-lookup.h"
#include "cpp-context.h"
#include "initialization.h"
#include <recon.h>

typedef char ConstructorMarkerOffsetMustBe50[
    (offsetof(CppSymbol, cpp) + offsetof(CppSymbolState, parts) +
     offsetof(CppSymbolStateParts, member_flags) == 50) ? 1 : -1];

/* Original 0x37f50..0x37f61, cdecl predicate supplied to lookup. */
int default_constructor_candidate(CppSymbol *candidate)
{
    return (candidate->cpp.parts.member_flags & 0x10) >> 4;
}

/* Original 0x37f90..0x37fd4, stdcall 4. A default-call candidate
   needs no closure when its sole declaration parameter is canonical void. */
int __stdcall is_void_parameter_constructor(CppSymbol *function)
{
    TypeShape *shape;
    DeclarationParameterInfo *parameters;
    if (!(function->cpp.parts.member_flags & 0x10)) return 0;
    shape = function->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    parameters = shape->data.parameters->declarations;
    return parameters->count == 1 && parameters->types[0] == type_0080;
}

/* Original 0x37f70..0x37f87, stdcall 4. The ordinary special-member
   search retains only constructors carrying the default-call marker. */
CppSymbol *__stdcall find_default_constructor(CType *type)
{
    return lookup_special_member(type, name_ctor, default_constructor_candidate);
}

/* Original 0x38720..0x38752, stdcall 4. Complete array elements use
   the virtual-base destructor variant when the class requires it. */
CppSymbol *__stdcall lookup_complete_object_destructor(CType *type)
{
    CppSymbol *tag = cpp_context_stack->items[type->field08];
    char *name = (tag->detail.class_info->flags & 0x20) ? name_vbase_dtor : name_dtor;
    return lookup_special_member(type, name, 0);
}

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
