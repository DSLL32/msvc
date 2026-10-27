#include <new.h>
#include <string.h>
#include "parser-actions.h"
#include "diagnostics.h"
#include "pointer-array.h"
#include <recon.h>
#pragma intrinsic(memcpy)

/* Two independent 48-byte stack matchers in the original. Only the match
 * list at +10 is consumed by this owner; other fields belong to the pending
 * constructor and matcher, with no default or whole-object copy claim. */
struct CompilerFunctionMatcher {
    BYTE unmodeled_00_0f[16];
    ParserDeferredList *parameters;
    BYTE unmodeled_14_2f[28];
    RECON_THISCALL CompilerFunctionMatcher(CompilerSymbol *, unsigned, unsigned, unsigned);
    int RECON_THISCALL Match(CompilerSymbol *);
};
typedef char CompilerFunctionMatcher_layout[(sizeof(CompilerFunctionMatcher) == 48 &&
    offsetof(CompilerFunctionMatcher, parameters) == 16) ? 1 : -1];


extern "C" {
int __cdecl compare_function_parameters(CompilerParameterTypes *, CompilerParameterTypes *); /*1e43a*/
WORD __cdecl classify_template_type(CompilerType *, unsigned, unsigned);
CompilerSymbol *__cdecl instantiate_declaration_class(CompilerSymbol *);
ParserDeferredList *__cdecl get_class_template_arguments(CompilerSymbol *);
int __cdecl compare_types(unsigned, CompilerType *, CompilerType *, unsigned); /*7769*/
int __cdecl compare_symbol_function_type(CompilerSymbol *, unsigned, CompilerType *, unsigned); /*108c6*/
void __cdecl merge_nested_declaration_values(void *, void *, CompilerSymbol *, unsigned);
void __cdecl record_template_match(CompilerSymbol *, CompilerSymbol *); /*4c731*/
void __cdecl install_function_template_parameters(CompilerSymbol *, ParserDeferredList *); /*4c770*/
void __cdecl describe_diagnostic_function(CompilerDiagnosticContext, CompilerFunctionType *, unsigned); /*8fcb3*/
void __cdecl compiler_warning(int, ...);
CompilerType *__cdecl intern_type(CompilerType *);
}

/* Both template arms contain this same inline allocation/copy sequence.
 * It stays a macro so no additional application call vertex is introduced. */
#define COPY_TEMPLATE_MATCHES(destination, source_list) do { \
    CompilerArena *copy_arena = &frontend_bss.arenas[0]; \
    BYTE *copy_memory = copy_arena->cursor; \
    copy_arena->cursor += 12; \
    if (copy_arena->cursor > copy_arena->committed_limit) copy_arena->CommitExtent(); \
    ParserDeferredList *copy = (ParserDeferredList *)copy_memory; \
    if (copy) { \
        copy->head = copy->tail = NULL; \
        ParserDeferredList *source = (source_list); \
        if (source->flags & 1) copy->flags |= 1; \
        ParserDeferredRecord *item = source->head; \
        do { \
            ParserDeferredRecord *cloned; \
            void *memory = copy_arena->Allocate(20); \
            if (item->flags & 3) \
                cloned = memory ? new(memory) ParserDeferredListItem(item) : NULL; \
            else \
                cloned = memory ? new(memory) ParserDeferredPointer(item) : NULL; \
            if (!copy->head) copy->head = cloned; \
            else { copy->tail->next = cloned; cloned->previous = copy->tail; } \
            copy->tail = cloned; \
            item = item->next; \
        } while (item); \
    } \
    install_function_template_parameters((destination), copy); \
} while (0)

/* Kind24 is an alias of the same function record, not a copied function. */
#define ACTUAL_FUNCTION(s) ((CompilerRuntimeFunction *)((s)->kind == 24 ? \
    ((CompilerAliasSymbol *)(s))->target->symbol : (s)))

extern "C" CompilerSymbol *__cdecl resolve_single_function_declaration(CompilerSymbol *input,
    CompilerSymbol *declaration, CompilerType *type, unsigned mode)
{
    CompilerTypeShape *requested_shape = type->shape;
    CompilerFunctionType *requested;
    CompilerSymbolLink *cursor, *link;
    CompilerSymbol *candidate;
    CompilerRuntimeFunction *function, *new_function = (CompilerRuntimeFunction *)declaration;
    CompilerTypeShape *candidate_shape;
    CompilerFunctionType *existing;
    CompilerType *requested_result, *existing_result;
    int comparison;
    if (!requested_shape) return NULL;
    requested = requested_shape->payload.function;
    requested_result = requested->type;
    cursor = &((CompilerNamedSymbol *)input)->link;
    if (!input->IsFunction() && input->kind != 24)
        cursor = ((CompilerMemberGroupSymbol *)input)->variants;
    while (cursor) {
        link = cursor;
        if (input->IsFunction() || input->kind == 24) cursor = NULL;
        else cursor = cursor->next;
        candidate = (CompilerSymbol *)((BYTE *)link - 8);
        function = ACTUAL_FUNCTION(candidate);
        if (declaration) {
            unsigned source_mode = frontend_data.source_mode_flags;
            if ((source_mode & 0x40) && (function->linkage_flags & 0x20)) continue;
            if ((source_mode & 8) && !(source_mode & 0x20000) &&
                (new_function->linkage_flags & 0x40) && (function->linkage_flags & 0x20)) continue;
            if (!(source_mode & 0x200000) && !(new_function->linkage_flags & 0x60) &&
                (function->linkage_flags & 0x20)) {
                CompilerFunctionMatcher matcher((CompilerSymbol *)function, 0, 0, 0);
                if (matcher.Match(declaration)) {
                    if ((frontend_data.source_mode_flags & 8) && (new_function->function_flags & 4))
                        new_function->linkage_flags |= 0x40;
                    else new_function->linkage_flags |= 0x80;
                    function = ACTUAL_FUNCTION(candidate);
                    unsigned merge_mode = mode == 1 ? 2 : mode;
                    merge_nested_declaration_values(function->type->shape->payload.function,
                        requested, declaration, merge_mode);
                    function = ACTUAL_FUNCTION(candidate);
                    record_template_match(declaration, (CompilerSymbol *)function);
                    COPY_TEMPLATE_MATCHES(declaration, matcher.parameters);
                    continue;
                }
            }
            function = ACTUAL_FUNCTION(candidate);
            if ((frontend_data.source_mode_flags & 0x20) && !(function->linkage_flags & 0x60) &&
                function->kind != 8) {
                CompilerFunctionMatcher matcher(declaration, 0, 0, 0);
                if (matcher.Match((CompilerSymbol *)function)) {
                    function = ACTUAL_FUNCTION(candidate);
                    if (!(function->definition_flags & 4) && (function->function_flags & 4))
                        function->linkage_flags |= 0x40;
                    else function->linkage_flags |= 0x80;
                    record_template_match((CompilerSymbol *)function, declaration);
                    COPY_TEMPLATE_MATCHES((CompilerSymbol *)function, matcher.parameters);
                }
                continue;
            }
        }
        function = ACTUAL_FUNCTION(candidate);
        candidate_shape = function->type->shape;
        existing = candidate_shape->payload.function;
        existing_result = existing->type;
        if (requested_shape != candidate_shape) {
            if (requested_result != existing_result) {
                if (!requested_result) {
                    /* The original consumes bit0 of record byte+5 here.
                     * Its wider writer/meaning is not recovered. */
                    __asm { int 3 }
                    if (!(((const BYTE *)existing)[5] & 1)) continue;
                } else {
                    if (!existing_result) continue;
                    if ((requested_result->attributes ^ existing_result->attributes) & 0x1f00) continue;
                    if ((requested_result->shape->attributes ^ existing_result->shape->attributes) & 3) continue;
                }
            }
            if (requested->parameters != existing->parameters ||
                (requested->symbol_count && existing->symbol_count && requested->symbol_count <= existing->symbol_count)) {
                comparison = compare_function_parameters(requested->parameters, existing->parameters);
                if (comparison == 1) {
                    if (!declaration) continue;
                    function = ACTUAL_FUNCTION(candidate);
                    if (!(new_function->linkage_flags & 0x20) && (function->linkage_flags & 0x40))
                        goto matching_parameters;
                    begin_declaration_diagnostic();
                    CompilerDiagnosticContext first = allocate_diagnostic_context();
                    CompilerDiagnosticContext second = allocate_diagnostic_context();
                    describe_diagnostic_function(first, existing, 0);
                    describe_diagnostic_function(second, requested, 0);
                    const char *second_text = format_declaration_context(second);
                    const char *first_text = format_declaration_context(first);
                    function = ACTUAL_FUNCTION(candidate);
                    compiler_warning(0x243, (CompilerSymbol *)function, second_text,
                        (CompilerSymbol *)function, first_text);
                    function = ACTUAL_FUNCTION(candidate);
                    function->scope_flags |= 4;
                    end_declaration_diagnostic();
                    return (CompilerSymbol *)ACTUAL_FUNCTION(candidate);
                }
                if (comparison == 2) {
                    function = ACTUAL_FUNCTION(candidate);
                    if ((function->link.name == frontend_data.builtin_names.operator_delete ||
                         function->link.name == frontend_data.builtin_names.operator_delete_array) &&
                        (function->type_flags & 4) && requested->parameters->count != existing->parameters->count) {
                        compiler_warning(0x29d);
                        function = ACTUAL_FUNCTION(candidate);
                        function->scope_flags |= 4;
                        return (CompilerSymbol *)ACTUAL_FUNCTION(candidate);
                    }
                    continue;
                }
            }
        }
matching_parameters:
        if (declaration) {
            if (frontend_bss.declaration_scope_mode >= 2) {
                if (candidate->kind == 24)
                    compiler_warning(0x373, declaration, (CompilerSymbol *)ACTUAL_FUNCTION(candidate));
            } else if (frontend_data.translation_unit->current->level == 0xff && candidate->kind == 24) {
                ((CompilerAliasSymbol *)candidate)->target->selected = 1;
                continue;
            }
        }
        function = ACTUAL_FUNCTION(candidate);
        candidate_shape = function->type->shape;
        CompilerType *candidate_base = function->type->StripOuterShape();
        CompilerType *requested_base = type->StripOuterShape();
        if (!compare_types(0, requested_base, candidate_base, 1)) {
            if (requested_shape->base != candidate_shape->base) {
                if (!declaration) continue;
                function = ACTUAL_FUNCTION(candidate);
                if (compare_symbol_function_type((CompilerSymbol *)function, 0, type, 0)) goto incompatible_type;
                return (CompilerSymbol *)ACTUAL_FUNCTION(candidate);
            }
            if (declaration && !(frontend_data.source_mode_flags & 0x40)) {
                function = ACTUAL_FUNCTION(candidate);
                if ((function->linkage_flags & 0x80) && !(new_function->reference_flags & 0x40))
                    new_function->linkage_flags |= 0x80;
            }
            if (declaration && !(frontend_data.source_mode_flags & 0x20041)) {
                function = ACTUAL_FUNCTION(candidate);
                merge_nested_declaration_values(existing, requested, (CompilerSymbol *)function, mode);
            }
            return (CompilerSymbol *)ACTUAL_FUNCTION(candidate);
        }
incompatible_type:
        if (declaration) {
            function = ACTUAL_FUNCTION(candidate);
            if (!(function->linkage_flags & 0x40) && !(new_function->linkage_flags & 0x40) &&
                !(frontend_data.source_mode_flags & 0x300000)) {
                if (!(function->scope_flags & 4)) {
                    compiler_warning(0x22c, (CompilerSymbol *)function);
                    function->scope_flags |= 4;
                }
                return (CompilerSymbol *)ACTUAL_FUNCTION(candidate);
            }
        }
    }
    DeferredAction *action = frontend_bss.deferred_action_head;
    if (action) {
        if (frontend_bss.deferred_action_depth <= 0) {
            do { action->Execute(); action = action->next; } while (action);
        }
        frontend_bss.deferred_action_tail = NULL;
        frontend_bss.deferred_action_head = NULL;
    }
    return NULL;
}

#undef COPY_TEMPLATE_MATCHES
#undef ACTUAL_FUNCTION

int RECON_THISCALL CompilerSymbol::IsFunction(void)
{
    return kind == 7 || kind == 8 || kind == 6;
}
RECON_THISCALL CompilerFunctionMatcher::CompilerFunctionMatcher(CompilerSymbol *, unsigned, unsigned, unsigned) { __asm { int 3 } }
int RECON_THISCALL CompilerFunctionMatcher::Match(CompilerSymbol *) { __asm { int 3 } return 0; }
RECON_THISCALL ParserDeferredRecord::ParserDeferredRecord(const ParserDeferredRecord *) { __asm { int 3 } }
RECON_THISCALL ParserDeferredListItem::ParserDeferredListItem(const ParserDeferredRecord *) { __asm { int 3 } }
extern "C" int __cdecl compare_function_parameters(CompilerParameterTypes *first, CompilerParameterTypes *second)
{
    if (first->count != second->count) return 2;
    int count = (int)first->count;
    int result = 0, index;
    for (index = 0; index < count; ++index) {
        CompilerType *left = first->types[index];
        CompilerType *right = second->types[index];
        if (left) left = left->StripQualifiers();
        if (right) right = right->StripQualifiers();
        result = compare_types(result, left, right, 0);
        if (result == 2) return 2;
    }
    return result;
}
extern "C" WORD __cdecl classify_template_type(CompilerType *type, unsigned mode, unsigned selector)
{
    WORD selected = (WORD)selector, result;
    while (type) {
        while (type->shape) {
            CompilerTypeShape *shape = type->shape;
            unsigned kind = shape->flags & 0x77;
            if (kind == 4) {
                CompilerFunctionType *function = shape->payload.function;
                if (function->type) {
                    result = classify_template_type(function->type, mode, selected);
                    if (result && (!selected || selected == result)) return result;
                }
                CompilerParameterTypes *parameters = function->parameters;
                int count = (int)parameters->count;
                int index;
                for (index = 0; index < count; ++index) {
                    if (!parameters->types[index]) continue;
                    result = classify_template_type(parameters->types[index], mode, selected);
                    if (result && (!selected || selected == result)) return result;
                }
            } else if (kind == 0x10 || kind == 0x20) {
                CompilerSymbol *symbol = shape->payload.symbol;
                CompilerType *member_type = ((CompilerIrSymbol *)symbol)->type;
                if (member_type && (short)member_type->auxiliary < 0 &&
                    (!selected || selected == member_type->auxiliary)) return member_type->auxiliary;
            }
            type = type->RemoveOuterShape();
        }
        if (!(type->flags & 0x400)) {
            unsigned kind = type->flags & 0x2fdf;
            if (kind != 0x11 && (kind != 0x12 || mode)) return 0;
        }
        short index = (short)type->auxiliary;
        if (index < 0) return !selected || selected == type->auxiliary ? type->auxiliary : 0;
        CompilerGlobalSymbolView *owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
        if (owner->class_detail->completion_flags & 1) {
            ParserDeferredList *arguments = get_class_template_arguments((CompilerSymbol *)owner);
            ParserDeferredRecord *argument = arguments->head;
            do {
                if (!(argument->flags & 3)) {
                    CompilerType *argument_type = (CompilerType *)((ParserDeferredPointer *)argument)->payload;
                    result = classify_template_type(argument_type, mode, selected);
                    if (result && (!selected || selected == result)) return result;
                }
                argument = argument->next;
            } while (argument);
            return 0;
        }
        if (owner->class_detail->completion_flags & 2) return 0xffff;
        index = (short)owner->declaration_qualifiers;
        if (!index) return 0;
        owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
        if (owner->kind != 4) return 0;
        type = owner->type;
    }
    return 0;
}
CompilerType *RECON_THISCALL CompilerType::RemoveOuterShape(void)
{
    CompilerType value;
    memcpy(&value, this, 16);
    value.shape = shape->base;
    return intern_type(&value);
}
extern "C" int __cdecl compare_types(unsigned status, CompilerType *left, CompilerType *right, unsigned ignore_shape)
{
    CompilerGlobalSymbolView *left_owner, *right_owner;
    CompilerTypeShape *first, *second;
    int related = 0;
    WORD first_kind, second_kind;
    if (left == right) return status;
    if (!left || !right) return 2;
    if (classify_template_type(left, 0, 0)) {
        if (!classify_template_type(right, 0, 0)) return 2;
        if ((frontend_data.source_mode_flags & 0x200000) && (left->flags & 0x400) && (right->flags & 0x400)) {
            right_owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)right->auxiliary];
            left_owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)left->auxiliary];
            if ((right_owner->class_detail->completion_flags & 2) &&
                (left_owner->class_detail->completion_flags & 1) &&
                (CompilerSymbol *)right_owner == instantiate_declaration_class((CompilerSymbol *)left_owner)) {
                related = 1;
                goto shapes;
            }
        }
        return left->Matches(right, 0) ? 0 : 2;
    }
    if (classify_template_type(right, 0, 0)) {
        if (!(frontend_data.source_mode_flags & 0x200000) || !(left->flags & 0x400) || !(right->flags & 0x400)) return 2;
        right_owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)right->auxiliary];
        left_owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)left->auxiliary];
        if (!(right_owner->class_detail->completion_flags & 2) ||
            !(left_owner->class_detail->completion_flags & 1) ||
            (CompilerSymbol *)right_owner != instantiate_declaration_class((CompilerSymbol *)left_owner)) return 2;
        related = 1;
        goto shapes;
    }
    if ((left->flags ^ right->flags) & 0xefdf) return 2;
    if (left->auxiliary != right->auxiliary) {
        left_owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)left->auxiliary];
        right_owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)right->auxiliary];
        if (!(left_owner->class_detail->completion_flags & 1) || !(right_owner->class_detail->completion_flags & 1)) return 2;
        CompilerSymbol *canonical = instantiate_declaration_class((CompilerSymbol *)left_owner);
        if (canonical != instantiate_declaration_class((CompilerSymbol *)right_owner)) return 2;
        ParserDeferredList *arguments = get_class_template_arguments((CompilerSymbol *)right_owner);
        ParserDeferredRecord *argument = arguments->head;
        do {
            if (argument->flags & 3) return 2;
            CompilerType *type = (CompilerType *)((ParserDeferredPointer *)argument)->payload;
            if (!classify_template_type(type, 0, 0)) return 2;
            argument = argument->next;
        } while (argument);
        related = 1;
    }
shapes:
    if (ignore_shape) return status;
    first = left->shape;
    second = right->shape;
    if (!first) return second ? 2 : !related;
    if (!second) return 2;
    if (first == second) {
        if ((left->attributes ^ right->attributes) & 0x1f00) return 2;
        return !related;
    }
    if (first->base != second->base) return 2;
    first_kind = first->flags;
    second_kind = second->flags;
    if ((first_kind ^ second_kind) & 0x40) return 2;
    if (((first->attributes & 3) != 0) != ((second->attributes & 3) != 0)) return 2;
    if (!(second->attributes & 3) && ((first->attributes ^ second->attributes) & 3)) return 2;
    if (first_kind == second_kind) return 2;
    if ((first_kind & 0x77) == 1 && (second_kind & 0x77) == 1 && ((first_kind | second_kind) & 0x400)) {
        if ((left->attributes ^ right->attributes) & 0x1f00) return 2;
        if (status != 1) return 0;
    }
    return !related;
}
int RECON_THISCALL CompilerType::Matches(CompilerType *, unsigned) { __asm { int 3 } return 0; }
extern "C" int __cdecl compare_symbol_function_type(CompilerSymbol *, unsigned, CompilerType *, unsigned) { __asm { int 3 } return 0; }
extern "C" void __cdecl record_template_match(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl install_function_template_parameters(CompilerSymbol *, ParserDeferredList *) { __asm { int 3 } }
extern "C" void __cdecl describe_diagnostic_function(CompilerDiagnosticContext, CompilerFunctionType *, unsigned) { __asm { int 3 } }
