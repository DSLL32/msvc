#define RECON_LEGACY_JMP_WORDS 16
#include <setjmp.h>
#include <stdio.h>
#include <new.h>
#include <stddef.h>
#include <string.h>
#include "declaration.h"
#include "global-output.h"
#include "pointer-array.h"
#include "diagnostics.h"
#include "pragma.h"
#include "parser-actions.h"
#include "parser-grammar.h"
#include "lexer.h"
#pragma intrinsic(memcpy, memset)
#include <recon.h>

/* Wider declaration paths are source-only until their real owners are
 * reached. These are separate views of existing owners, not new native
 * copies of their unknown bytes. */
typedef CompilerGlobalClassDetail DeclarationClassDetail;
typedef CompilerRuntimeFunction DeclarationFunctionView;
struct DeclarationBindingScope { void *entries; /* original+0 */ };
struct DeclarationBindingOwner { DeclarationBindingScope *scope; /* original+0 */ };
struct DeclarationBinding : CompilerNamedSymbol {
    DeclarationBindingOwner *owner; /* original+10, actual common header */
};
#ifndef RECON_NATIVE_TEST
DeclarationSourceState declaration_source_state;
CompilerType declaration_type_scratch;
int publish_browser_local; /* c0838 */
#endif

extern "C" {
CompilerSymbol *__cdecl resolve_symbol_reference(CompilerSymbol *);
int __cdecl is_scope_declaration(CompilerSymbol *);
CompilerDeclarationMetadata *__cdecl get_declaration_metadata(CompilerSymbol *);
CompilerFunctionContext *__cdecl get_declaration_function_context(CompilerSymbol *);
CompilerDeclarationAttachment *__cdecl get_declaration_attachment(CompilerSymbol *);
void __cdecl check_qualified_declaration(CompilerSymbol *);
void __cdecl validate_declaration_type(CompilerSymbol *, int);
CompilerSymbol *__cdecl instantiate_declaration_class(CompilerSymbol *);
void __cdecl merge_declaration_classes(CompilerSymbol *, CompilerSymbol *);
void __cdecl diagnose_member_declaration(CompilerSymbol *);
int __cdecl is_current_declaration_class(CompilerSymbol *);
CompilerTypeShape *__cdecl qualify_declaration_shape(CompilerTypeShape *);
CompilerType *__cdecl intern_type(CompilerType *);
CompilerTypeShape *__cdecl intern_type_shape(CompilerTypeShape *);
void __cdecl require_current_declaration_class(void);
unsigned __cdecl get_function_calling_convention(CompilerType *);
WORD __cdecl get_type_classification(CompilerType *);
void __cdecl set_declaration_reference(CompilerSymbol *, CompilerSymbol *);
void __cdecl set_declaration_metadata(CompilerSymbol *, CompilerDeclarationMetadata *);
void __cdecl set_declaration_attachment(CompilerSymbol *, CompilerDeclarationAttachment *);
void __cdecl compiler_warning(int, ...);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
}

/* These macros name already-modeled owners; they introduce no graph vertex. */
#define DECL_OWNER(t) ((CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)(t)->auxiliary])
#define DECL_DETAIL(s) ((DeclarationClassDetail *)(s)->class_detail)
#define DECL_FLAGS(s) (*(DWORD *)&(s).context.class_flags)
#define DECL_CLASS(s) ((s).context.class_flags & 7)
#define DECL_CLASS_SET(s,v) ((s).context.class_flags = ((s).context.class_flags & ~7) | (v))
#define DECL_MODE() FRONTEND_LINKAGE_CONTEXT()

extern "C" {
void __cdecl combine_symbol_declaration(CompilerSymbol *, CompilerSymbol *);
void __cdecl process_external_recovery(unsigned);
CompilerSymbol *__cdecl append_implicit_parameter(unsigned, CompilerParameterSet **, CompilerNameEntry *, CompilerType *); /*21787*/
CompilerSymbol *__cdecl prepare_function_definition(CompilerSymbol *); /*404c*/
void __cdecl recover_function_declaration(void); /*89832*/
ParserTokenView *__cdecl current_declaration_lookahead(void); /*29469*/
void __cdecl set_function_segment(CompilerSymbol *, CompilerSymbol *); /*8e9f0*/
unsigned __cdecl global_type_size(CompilerType *);
CompilerType *__cdecl get_function_return_type(CompilerType *);
CompilerType *__cdecl qualify_type(CompilerSymbol *, unsigned, WORD);
void __cdecl validate_function_declaration(CompilerSymbol *, unsigned);
void __cdecl select_code_segment(unsigned);
}

extern "C" CompilerSymbol *__cdecl declare_symbol(CompilerDeclaration *declaration,
    CompilerSymbol *input, CompilerDeclarator *declarator)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    CompilerGlobalSymbolView *previous;
    CompilerBoundSymbol *bound;
    CompilerSymbolBindingScope *scope;
    CompilerSymbol *qualified_owner;
    CompilerDeclarationState state;
    CompilerSavedLookup *saved;
    CompilerTranslationUnit *unit;
    CompilerArena *arena;
    CompilerType *type;
    CompilerGlobalSymbolView *return_class;
    CompilerDeclarationMetadata *metadata;
    int index;
    if (!(frontend_data.source_mode_flags & 4) &&
        (frontend_data.compiler_flags & 0x20)) select_code_segment(1);
    if (frontend_data.declaration_context_index >= 0) return input;
    unit = frontend_data.translation_unit;
    arena = &frontend_bss.arenas[8]; /* original C6600 inline bump */
    saved = (CompilerSavedLookup *)arena->cursor;
    arena->cursor += 36;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (saved) saved = saved->Initialize(unit, unit->saved_lookup, 0);
    unit->saved_lookup = saved;
    bound = symbol->next ? (CompilerBoundSymbol *)((BYTE *)symbol->next - 8) : NULL;
    if (bound && bound->binding->owner->scope->entries && bound->binding) {
        scope = bound->binding->owner->scope;
        if (scope->entries) {
            frontend_data.declaration_lookup_owner = scope->symbol;
            unit = frontend_data.translation_unit;
            if (!unit->current->level && unit->current != unit->root)
                unit->PrepareScopeSymbol(bound->binding->owner->symbol);
            frontend_data.translation_unit->SelectScopeSymbol(frontend_data.declaration_lookup_owner);
            frontend_data.translation_unit->MarkDefinitionScopes();
        }
    }
    if (frontend_data.declaration_lookup_owner &&
        frontend_data.declaration_lookup_owner->kind != 23) {
        qualified_owner = bound->binding->owner->symbol;
        frontend_data.compiler_flags |= 0x100;
        symbol->kind = 3;
        memcpy(&state.context, &declaration->context, 28);
        *(DWORD *)&symbol->storage_class = *(DWORD *)&state.context.class_flags;
        if ((state.context.reference_flags & 1) && state.context.reference)
            set_declaration_reference(input, state.context.reference);
        else symbol->reference_flags &= ~1U;
        if (state.context.context_flags & 0x40) {
            if (state.context.metadata) {
                metadata = (CompilerDeclarationMetadata *)frontend_bss.arenas[0].Allocate(16);
                if (metadata) memcpy(metadata, state.context.metadata, 16);
                set_declaration_metadata(input, metadata);
            } else symbol->symbol_flags &= ~0x40U;
        } else input->SetDeclarationMetadata(NULL);
        if (state.context.attachment) set_declaration_attachment(input, state.context.attachment);
        else symbol->reference_flags &= ~0x80U;
        if (frontend_data.declaration_lookup_owner->kind != 4) {
            scope = bound->binding->owner->scope;
            while (scope->symbol->kind != 4)
                scope = (CompilerSymbolBindingScope *)scope->entries;
            symbol->next = NULL;
            frontend_data.declaration_lookup_owner = scope->symbol;
            compiler_warning(0x1fd, input, scope->symbol);
            recover_function_declaration();
            return input;
        }
        if (!(frontend_data.compiler_flags & 0x200)) {
            if (declaration->context.context_flags & 2) compiler_warning(0x2d3, input, 15);
            if ((declaration->context.class_flags & 7) == 3) compiler_warning(0x2d4, input);
        }
        previous = (CompilerGlobalSymbolView *)merge_symbol_declaration(declaration, input, declarator);
        if (qualified_owner->kind != 17 && qualified_owner->kind != 8) {
            if (symbol->function_class == 1) compiler_warning(0x1fd, input, frontend_data.declaration_lookup_owner);
            else compiler_warning(0x221, (unsigned)symbol->function_class);
            symbol->scope_flags |= 4;
            frontend_data.compiler_flags &= ~0x100U;
            process_external_recovery(0);
            goto validate_function;
        }
        if (!previous) {
            compiler_warning(0x1ff, input, frontend_data.declaration_lookup_owner);
            symbol->type_flags &= ~0x30U;
            symbol->scope_flags |= 4;
            symbol->function_class = 1;
            frontend_data.compiler_flags &= ~0x100U;
            if (!(frontend_data.source_mode_flags & 4)) process_external_recovery(0);
            goto validate_function;
        }
        if (previous->kind == 7 || previous->kind == 8) {
            combine_symbol_declaration((CompilerSymbol *)previous, input);
            symbol = previous;
            input = (CompilerSymbol *)symbol;
            if (!(frontend_data.source_mode_flags & 4)) {
                if (symbol->definition_flags & 4) {
                    compiler_warning((symbol->linkage_flags & 0x10) ? 0x258 : 0x54, input);
                    symbol->scope_flags |= 4;
                }
                symbol->definition_flags = (symbol->definition_flags & ~8U) | 4;
                frontend_data.translation_unit->MarkDefinitionScopes();
            }
        }
    } else {
        symbol->definition_flags |= 4;
        previous = (CompilerGlobalSymbolView *)merge_symbol_declaration(declaration, input, declarator);
        if (!previous) {
            compiler_warning(0xf4, input);
            symbol->scope_flags |= 4;
            return input;
        }
        if ((previous->reference_flags & 0x40) && !(previous->linkage_flags & 0x80))
            compiler_warning(0x390, previous);
        symbol = previous;
        input = (CompilerSymbol *)symbol;
        symbol->definition_flags &= ~8U;
    }
validate_function:
    validate_function_declaration(input, 1);
    if (frontend_data.source_mode_flags & 4) return input;
    if ((symbol->kind == 7 || symbol->kind == 8) && (symbol->linkage_flags & 1)) {
        type = get_function_return_type(symbol->type);
        if (get_type_classification(type) & 0x80) {
            index = (short)type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            return_class = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
        } else return_class = NULL;
        if ((return_class->class_detail->completion_flags & 1) &&
            !(return_class->class_detail->template_flags & 1) && !(return_class->definition_flags & 4) &&
            !(return_class->class_detail->completion_flags & 4)) {
            index = frontend_data.declaration_context_index;
            if (index >= 0) {
                if (index >= 16) { __asm { int 3 } }
                if (frontend_bss.class_scope_rows[index].symbol->name == return_class->name)
                    goto return_class_ready;
            }
            if (!(return_class->scope_flags & 4) && !(frontend_data.source_mode_flags & 0x200020)) {
                CompilerGlobalSymbolView *instance = (CompilerGlobalSymbolView *)instantiate_declaration_class((CompilerSymbol *)return_class);
                if (instance->class_detail->completion_flags & 8)
                    merge_declaration_classes((CompilerSymbol *)instance, (CompilerSymbol *)return_class);
            }
        }
return_class_ready:
        if (!(return_class->definition_flags & 4) && !frontend_bss.suppress_parser_codegen)
            compiler_warning(0x1b, return_class);
        frontend_bss.function_return_class = qualify_type((CompilerSymbol *)return_class, 0, 0);
        append_implicit_parameter(0, &frontend_bss.current_parameters,
            frontend_data.builtin_names.return_udt, frontend_bss.function_return_class);
    }
    if (frontend_data.compiler_flags & 0x100) {
        if ((symbol->type_flags & 0x10) &&
            (((CompilerGlobalSymbolView *)frontend_data.declaration_lookup_owner)->class_detail->context_flags & 0x10))
            append_implicit_parameter(1, &frontend_bss.current_parameters,
                frontend_data.builtin_names.initialize_virtual_bases, frontend_data.builtin_types.type_4063_first);
        if (!frontend_bss.current_parameter_kind) symbol->builtin_flags &= ~1U;
        if (symbol->builtin_flags & 1) {
            CompilerType *owner_type = symbol->type->shape->payload.function->type;
            append_implicit_parameter(0, &frontend_bss.current_parameters,
                frontend_data.builtin_names.this_parameter, owner_type);
        }
    }
    if (symbol->kind != 7 && symbol->kind != 8) {
        if (symbol->kind == 6) compiler_warning(0xa9, input);
        else {
            ParserTokenView *lookahead = current_declaration_lookahead();
            if (lookahead) compiler_warning(0xef, current_declaration_lookahead()->kind, input);
            else compiler_warning(0x36, 0x53, input);
        }
        symbol->scope_flags |= 4;
        recover_function_declaration();
        return input;
    }
    if (!(symbol->attributes & 0x80)) {
        compiler_warning(0xce, input);
        frontend_bss.current_parameters = NULL;
    }
    if (symbol->member_flags & 8) {
        if (!(symbol->symbol_flags & 1) && !(frontend_data.compiler_flags & 0x200))
            compiler_warning(0x1eb, input, 0x176);
    }
    if (frontend_data.default_code_segment) set_function_segment(input, frontend_data.default_code_segment);
    frontend_bss.current_function = prepare_function_definition(input);
    frontend_bss.active_parameter_kind = frontend_bss.current_parameter_kind;
    frontend_bss.active_parameters = frontend_bss.current_parameters;
    type = frontend_data.function_result_type;
    if (type->flags != 0x80 && !global_type_size(type)) {
        index = (short)type->auxiliary;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        compiler_warning(0x4f, frontend_bss.current_function, type,
            frontend_data.frontend_pointer_table->items[index]);
    }
    return frontend_bss.current_function;
}

extern "C" CompilerSymbol *__cdecl begin_function_body(CompilerSymbol *input)
{
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)input;
    CompilerTranslationUnit *unit;
    CompilerArena *arena;
    CompilerSavedLookup *saved;
    CompilerGlobalSymbolView *owner, *return_class, *instance;
    CompilerGlobalClassDetail *detail;
    CompilerType *type;
    int index;
    if (symbol->kind != 7 && symbol->kind != 8) return input;
    frontend_data.compiler_flags = (frontend_data.compiler_flags & ~0x01000000U) | 0x40000300;
    frontend_data.compiler_mode_flags &= ~2U;
    frontend_bss.declaration_nesting = 1;
    unit = frontend_data.translation_unit;
    arena = &frontend_bss.arenas[8];
    saved = (CompilerSavedLookup *)arena->cursor;
    arena->cursor += 36;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (saved) saved = saved->Initialize(unit, unit->saved_lookup, 0);
    unit->saved_lookup = saved;
    if (symbol->declaration_qualifiers) {
        index = (short)symbol->declaration_qualifiers;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        frontend_data.declaration_lookup_owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
        frontend_data.translation_unit->SelectScopeSymbol(frontend_data.declaration_lookup_owner);
        frontend_data.translation_unit->MarkDefinitionScopes();
    }
    symbol->definition_flags = (symbol->definition_flags & ~8U) | 4;
    validate_function_declaration(input, 1);
    symbol->attributes = (symbol->attributes & ~0x10) | 0x80;
    if (symbol->linkage_flags & 1) {
        type = get_function_return_type(symbol->type);
        if (get_type_classification(type) & 0x80) {
            index = (short)type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            return_class = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
        } else return_class = NULL;
        detail = return_class->class_detail;
        if ((detail->completion_flags & 1) && !(detail->template_flags & 1) &&
            !(return_class->definition_flags & 4) && !(detail->completion_flags & 4)) {
            index = frontend_data.declaration_context_index;
            if (index >= 0) {
                if (index >= 16) { __asm { int 3 } }
                if (frontend_bss.class_scope_rows[index].symbol->name == return_class->name)
                    goto return_class_ready;
            }
            if (!(return_class->scope_flags & 4) && !(frontend_data.source_mode_flags & 0x200020)) {
                instance = (CompilerGlobalSymbolView *)instantiate_declaration_class((CompilerSymbol *)return_class);
                if (instance->class_detail->completion_flags & 8)
                    merge_declaration_classes((CompilerSymbol *)instance, (CompilerSymbol *)return_class);
            }
        }
return_class_ready:
        if (!(return_class->definition_flags & 4)) compiler_warning(0x1b, return_class);
        frontend_bss.function_return_class = qualify_type((CompilerSymbol *)return_class, 0, 0);
        append_implicit_parameter(0, &frontend_bss.current_parameters,
            frontend_data.builtin_names.return_udt, frontend_bss.function_return_class);
    }
    if (symbol->type_flags & 4) {
        if (symbol->type_flags & 0x10) {
            index = (short)symbol->declaration_qualifiers;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
            if (owner->class_detail->context_flags & 0x10)
                append_implicit_parameter(1, &frontend_bss.current_parameters,
                    frontend_data.builtin_names.initialize_virtual_bases, frontend_data.builtin_types.type_4063_first);
        }
        if (symbol->builtin_flags & 1)
            append_implicit_parameter(0, &frontend_bss.current_parameters,
                frontend_data.builtin_names.this_parameter, symbol->type->shape->payload.function->type);
        if (symbol->type_flags & 0x70) frontend_data.compiler_flags |= 0x01000000;
        else frontend_data.compiler_flags &= ~0x01000000U;
    }
    if (frontend_data.default_code_segment) set_function_segment(input, frontend_data.default_code_segment);
    frontend_bss.current_function = prepare_function_definition(input);
    frontend_bss.active_parameters = frontend_bss.current_parameters;
    frontend_bss.active_parameter_kind = frontend_bss.current_parameter_kind;
    --frontend_bss.declaration_nesting;
    if (frontend_bss.parser_context_depth >= 60) { __asm { int 3 } }
    frontend_bss.declaration_controls[frontend_bss.parser_context_depth].flags &= ~3;
    return input;
}

void RECON_THISCALL CompilerTranslationUnit::SelectScopeSymbol(CompilerSymbol *symbol)
{
    /* The original reloads the active TU for either dispatch; its incoming
     * receiver is not used as the called method's receiver. */
    if (symbol->kind == 15) return;
    if (symbol->kind == 4) frontend_data.translation_unit->SelectClassSymbol(symbol);
    else if (symbol->kind == 23) frontend_data.translation_unit->SelectNamespaceSymbol(symbol);
}

void RECON_THISCALL CompilerTranslationUnit::SelectClassSymbol(CompilerSymbol *symbol)
{
    CompilerSymbol *enclosing = NULL;
    int index;
    if (!current->level && CurrentDeclarationOwner() == symbol) return;
    if (current->level == 0xff && ((CompilerClassScope *)current)->symbol == symbol) return;
    if (symbol->declaration_qualifiers) {
        index = (short)symbol->declaration_qualifiers;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        enclosing = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
        SelectScopeSymbol(enclosing);
    }
    if (symbol->kind == 7 || symbol->kind == 8 || symbol->kind == 6) return;
    EnterClassScope(symbol, ((CompilerNamedSymbol *)symbol)->link.name, enclosing == NULL);
}

void RECON_THISCALL CompilerTranslationUnit::EnterClassScope(CompilerSymbol *symbol,
        CompilerNameEntry *name, unsigned outermost)
{
    int forced = 0, index;
    CompilerScopeTable *scope;
    if (!symbol) return;
    if (symbol->kind == 15) {
        CompilerClassMemberSymbol *member = (CompilerClassMemberSymbol *)symbol;
        if (member->flags & 2) return;
        index = member->owner_index;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        symbol = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
        forced = 1;
    } else if (!is_scope_declaration(symbol)) return;
    scope = SetEnclosingClass(symbol, outermost ? 0xfc : 0xfe);
    if (scope && (forced || ((CompilerNamedSymbol *)symbol)->link.name == name))
        ((CompilerClassScope *)scope)->reset_value = 1;
}

void RECON_THISCALL CompilerTranslationUnit::MarkDefinitionScopes(void)
{
    CompilerScopeTable *scope = current;
    unsigned first = 1;
    while (scope != root) {
        if (scope->level == 0xfd) break;
        if (scope->level >= 0xfb) {
            ((CompilerClassScope *)scope)->reset_value = first;
            first = 0;
            scope->level = 0xff;
        }
        scope = scope->next;
    }
}
extern "C" CompilerSymbol *__cdecl make_parameter_symbol(CompilerNameEntry *, CompilerType *);
extern "C" CompilerSymbol *__cdecl make_parameter_sentinel(void);
extern "C" CompilerSymbol *__cdecl append_implicit_parameter(unsigned append,
        CompilerParameterSet **parameters, CompilerNameEntry *name, CompilerType *type)
{
    CompilerSymbol *symbol = make_parameter_symbol(name, type);
    CompilerParameterSet *set;
    CompilerSymbolLink *link;
    CompilerArena *arena;
    CompilerPool *pool;
    BYTE *memory;
    unsigned arena_index;
    if (!*parameters) {
        arena_index = (frontend_data.declaration_context_index < 0) + 3;
        if (!arena_index && frontend_data.free_parameter_sets) {
            set = frontend_data.free_parameter_sets;
            frontend_data.free_parameter_sets = set->next_free;
            set->arena_index = 0;
            set->head = set->tail = NULL;
        } else {
            if (arena_index) {
                arena = &frontend_bss.arenas[arena_index];
                memory = arena->cursor;
                arena->cursor += 12;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            } else {
                pool = &frontend_bss.pools[8]; /* actual c0b00 */
                if (pool->remaining < 12) pool->Refill(12);
                memory = pool->cursor;
                pool->cursor += 12;
                pool->remaining -= 12;
            }
            set = (CompilerParameterSet *)memory;
            if (set) {
                set->arena_index = arena_index;
                set->head = set->tail = NULL;
            }
        }
        *parameters = set;
    }
    set = *parameters;
    link = &((CompilerNamedSymbol *)symbol)->link;
    if (!append) {
        if (link) {
            link->name = link->name;
            link->next = set->head;
            ((CompilerParameterSymbol *)symbol)->previous_parameter = NULL;
        }
        if (!set->head) set->head = set->tail = link;
        else {
            if (!frontend_data.parameter_sentinel)
                frontend_data.parameter_sentinel = make_parameter_sentinel();
            if ((CompilerSymbol *)((BYTE *)set->head - 8) == frontend_data.parameter_sentinel)
                set->head = set->tail = link;
            else {
                ((CompilerParameterSymbol *)((BYTE *)set->head - 8))->previous_parameter = link;
                set->head = link;
            }
        }
    } else {
        if (link) {
            link->name = link->name;
            link->next = NULL;
            ((CompilerParameterSymbol *)symbol)->previous_parameter = set->tail;
        }
        if (!set->tail) set->head = set->tail = link;
        else {
            if (!frontend_data.parameter_sentinel)
                frontend_data.parameter_sentinel = make_parameter_sentinel();
            if ((CompilerSymbol *)((BYTE *)set->tail - 8) == frontend_data.parameter_sentinel)
                set->head = set->tail = link;
            else {
                set->tail->next = link;
                set->tail = link;
            }
        }
    }
    return symbol;
}
extern "C" void __cdecl reset_function_reference_counter(void); /*4128*/
extern "C" void __cdecl reset_function_locations(void); /*4133*/
extern "C" CompilerSymbol *__cdecl make_unnamed_function(void); /*47e24*/
extern "C" CompilerSymbol *__cdecl prepare_function_definition(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol;
    reset_function_reference_counter();
    if (!input) input = make_unnamed_function();
    symbol = (CompilerGlobalSymbolView *)input;
    if ((symbol->symbol_flags & 1) && (symbol->storage_class & 7) != 3) {
        frontend_bss.function_reset_state[0] = 0;
        frontend_bss.function_reset_state[1] = 0;
        frontend_bss.function_reset_state[2] = 0;
    }
    frontend_data.force_type_serialization = 0;
    frontend_data.type_debug_mode = frontend_data.auto_inline;
    frontend_data.disable_inline_records = 0;
    frontend_data.function_result_type = (symbol->linkage_flags & 1) ?
        frontend_bss.function_return_class : symbol->type->StripOuterShape();
    frontend_data.compiler_mode_flags &= ~0x800U;
    if (!(frontend_data.compiler_flags & 0x20))
        frontend_data.translation_unit->BeginParameterScope(frontend_bss.current_parameters);
    frontend_bss.global_output_active = 1;
    frontend_bss.declaration_scope_mode = 1;
    frontend_data.declaration_scope_number = 1;
    frontend_bss.function_body_state = 0;
    frontend_bss.function_source_position = frontend_bss.saved_context_stack.position;
    reset_function_locations();
    return input;
}
extern "C" void __cdecl reset_function_reference_counter(void)
{ frontend_data.function_reference_counter = 0x3fffff; }
extern "C" void __cdecl reset_function_locations(void)
{
    frontend_data.function_location_cache[0] = 0xffffffff;
    frontend_data.function_location_cache[1] = 0xffffffff;
}
extern "C" CompilerSymbol *__cdecl make_unnamed_function(void) { __asm { int 3 } return NULL; }
extern "C" void __cdecl recover_function_declaration(void) { __asm { int 3 } }
extern "C" ParserTokenView *__cdecl current_declaration_lookahead(void) { return frontend_bss.parser_lookahead; }
extern "C" void __cdecl set_function_segment(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }

extern "C" {
void __cdecl begin_function_output(void); /*42e2*/
void __cdecl emit_function_parameters(CompilerSymbol *, CompilerParameterSet *); /*4e15*/
int __cdecl initialize_debug_function_output(unsigned, unsigned); /*8087e*/
void __cdecl initialize_function_browser_output(void); /*8976b*/
unsigned __cdecl function_parameter_bytes(CompilerParameterSet *); /*46757*/
void __cdecl print_preprocessed_function(CompilerSymbol *, FILE *); /*8f0d9*/
void __cdecl emit_context_transition(unsigned);
}
extern "C" void __cdecl process_external_initializer(void)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    CompilerGlobalSymbolView *owner;
    unsigned owner_reference = 0, debug_first, debug_second, preserve, bytes, convention;
    int index;
    if (!symbol) return;
    if (((CompilerRecordedLexer *)frontend_bss.saved_context_stack.top->object)->active) {
        if (symbol->kind == 8) {
            index = (short)symbol->declaration_qualifiers;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
            owner_reference = (unsigned)owner->class_detail->declaration_reference;
        }
        if (!frontend_bss.debug_module->Slot14(owner_reference,
                frontend_bss.saved_context_stack.position, frontend_bss.lexical_skip_mode,
                &frontend_data.debug_module_kind, &debug_first, &debug_second, &preserve))
            report_compiler_diagnostic(73, ".\\funcs.c", 0x4bf);
        frontend_data.backend->SetDebugMode(preserve == 0);
        frontend_data.emit_global_locations = frontend_data.debug_module_kind == 4;
        if (frontend_data.emit_global_locations &&
            !initialize_debug_function_output(debug_first, debug_second))
            report_compiler_diagnostic(73, ".\\funcs.c", 0x4c6);
        if (frontend_bss.browser_filename) initialize_function_browser_output();
    }
    begin_function_output();
    symbol = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    if (!frontend_bss.stack_check_warning_emitted &&
        (symbol->type->attributes & 0x70) == 0x40 && frontend_data.normalized_stack_checks) {
        report_language_diagnostic(1, 0x7c);
        symbol = (CompilerGlobalSymbolView *)frontend_bss.current_function;
        frontend_bss.stack_check_warning_emitted = 1;
    }
    if (!frontend_bss.suppress_parser_codegen) {
        convention = symbol->type->attributes & 0x70;
        if ((convention == 0x60 || convention == 0x40) && (symbol->type_flags & 2)) {
            bytes = function_parameter_bytes(frontend_bss.active_parameters);
            symbol = (CompilerGlobalSymbolView *)frontend_bss.current_function;
            if (symbol->qualifiers & 0x40) {
                if (bytes != symbol->alternate_low) compiler_warning(0x2c5, symbol);
            } else {
                if (bytes > 0xffff) {
                    report_compiler_diagnostic(0x1fc, symbol);
                    symbol = (CompilerGlobalSymbolView *)frontend_bss.current_function;
                }
                symbol->alternate_low = (WORD)bytes;
                ((CompilerGlobalSymbolView *)frontend_bss.current_function)->qualifiers |= 0x40;
            }
            symbol = (CompilerGlobalSymbolView *)frontend_bss.current_function;
        }
    }
    if (frontend_bss.suppress_parser_codegen && !(symbol->type_flags & 4)) {
        begin_declaration_diagnostic();
        print_preprocessed_function((CompilerSymbol *)symbol, frontend_bss.preprocessor_output);
        end_declaration_diagnostic();
    } else if (!frontend_bss.suppress_parser_codegen && frontend_data.debug_module_kind != 4)
        emit_function_parameters((CompilerSymbol *)symbol, frontend_bss.current_parameters);
    if (frontend_data.options.debug_info && !frontend_bss.suppress_parser_codegen &&
        frontend_data.debug_module_kind != 4) emit_context_transition(17);
    frontend_data.compiler_flags |= 0x20000;
}
extern "C" {
void __cdecl initialize_function_code(unsigned); /*4417*/
CompilerExpression *__cdecl make_implicit_this_expression(CompilerSymbol *); /*21862*/
int __cdecl can_reference_symbol(CompilerSymbol *);
CompilerExpression *__cdecl make_ir_binary_node(unsigned, CompilerExpression *, CompilerExpression *); /*48df*/
void __cdecl emit_ir_expression(CompilerExpression *); /*48fb*/
void __cdecl emit_parameter_declaration(CompilerSymbol *); /*218fd*/
CompilerWorkLink *__cdecl prepend_work_link(CompilerWorkLink **, unsigned);
}
extern "C" void __cdecl begin_function_output(void)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    CompilerExpression *expression;
    CompilerSymbolLink *link;
    CompilerIrSymbol *parameter;
    unsigned storage_class;
    symbol->definition_flags |= 4;
    frontend_bss.function_parameter_count = 0;
    ((CompilerSymbol *)frontend_bss.current_function)->attributes &= ~0x10U;
    if (frontend_bss.suppress_parser_codegen) return;
    initialize_function_code(0);
    if (frontend_data.debug_module_kind != 4) {
        frontend_bss.function_entry_label = make_internal_label(0x1f);
        write_ir_record(0x53, NULL);
        write_ir_record(0x53, NULL);
        symbol = (CompilerGlobalSymbolView *)frontend_bss.current_function;
        if ((symbol->type_flags & 4) && (symbol->builtin_flags & 1)) {
            CompilerExpression *owner = make_implicit_this_expression(0);
            expression = symbol_expression((CompilerSymbol *)symbol, 1);
            expression = make_ir_binary_node(0x99, expression, owner);
        } else expression = symbol_expression((CompilerSymbol *)symbol, 1);
        emit_ir_expression(make_ir_binary_node(0x6c, expression, NULL));
    }
    link = frontend_bss.active_parameters ? frontend_bss.active_parameters->head : NULL;
    while (link) {
        parameter = (CompilerIrSymbol *)((BYTE *)link - 8);
        storage_class = parameter->storage_class & 7;
        if (storage_class >= 5 && storage_class <= 6 && frontend_data.debug_module_kind != 4)
            emit_parameter_declaration((CompilerSymbol *)parameter);
        if (parameter->type && parameter->type != frontend_data.builtin_types.type_0080)
            ++frontend_bss.function_parameter_count;
        link = link->next;
    }
}
extern "C" void __cdecl write_global_ir_record(unsigned, CompilerSymbol *);
extern "C" void __cdecl initialize_function_code(unsigned mode)
{
    OutputBuffer *buffer;
    unsigned bytes;
    int good, saved_stack_checks;
    frontend_data.source_mode_flags |= 0x1000;
    if (frontend_data.option_c9il) {
        frontend_bss.output_buffers[3].Flush(); /* C0360 */
        frontend_bss.saved_reference_count = ftell(frontend_bss.output_buffers[3].file);
        frontend_bss.output_buffers[1].Flush(); /* C0590 */
        frontend_bss.local_reference_count = ftell(frontend_bss.output_buffers[1].file);
    } else if (!(frontend_data.compiler_flags & 0x20)) {
        bytes = frontend_bss.output_buffers[5].cursor - frontend_bss.output_buffers[5].data;
        buffer = &frontend_bss.output_buffers[4];
        if (bytes) {
            good = buffer->Ensure(bytes + 1);
            buffer->ok &= good;
            if (buffer->ok) buffer->WriteBytes(frontend_bss.output_buffers[5].data, bytes);
            frontend_bss.output_buffers[5].cursor = frontend_bss.output_buffers[5].data;
        }
        if (frontend_bss.alternate_output_flush) buffer->FlushAlternate();
        else buffer->Flush();
    }
    frontend_bss.ir_record_count = 0;
    if (frontend_data.compiler_flags & 0x20) {
        /* The original has no intervening call between these two stores. */
        saved_stack_checks = frontend_data.normalized_stack_checks;
        frontend_data.normalized_stack_checks = 0;
        frontend_data.normalized_stack_checks = saved_stack_checks;
    }
    if (frontend_data.debug_module_kind == 4) return;
    emit_context_transition(0x1f);
    if (frontend_data.options.inline_level) emit_context_transition(0x20);
    if (mode) {
        ++frontend_bss.global_output_active;
        write_global_ir_record(6, NULL);
        --frontend_bss.global_output_active;
    } else {
        emit_context_transition(2);
        emit_context_transition(0x24);
        emit_context_transition(0x23);
    }
}
void RECON_THISCALL OutputBuffer::FlushAlternate(void) { __asm { int 3 } }
/* 21862..218fd, 6518d..651ff. Hidden return parameters can precede the
 * real this parameter; its existing name identity distinguishes it. */
extern "C" CompilerExpression *__cdecl make_implicit_this_expression(CompilerSymbol *parameter)
{
    if (!parameter) {
        CompilerParameterSet *parameters = frontend_bss.active_parameters;
        if (parameters) {
            CompilerSymbolLink *link = parameters->head;
            parameter = (CompilerSymbol *)((BYTE *)link - 8);
            if (((CompilerTypedSymbol *)parameter)->type == frontend_data.builtin_types.type_0080) {
                link = link->next;
                parameter = link ? (CompilerSymbol *)((BYTE *)link - 8) : NULL;
            }
            if (parameter && ((CompilerNamedSymbol *)parameter)->link.name !=
                frontend_data.builtin_names.this_parameter) parameter = NULL;
        }
        if (!parameter) {
            CompilerIrSymbol *function = (CompilerIrSymbol *)frontend_bss.current_function;
            if (!(function->type_flags & 4)) compiler_warning(0x2a1, function);
            else compiler_warning((function->storage_class & 7) == 3 ? 0x29f : 0x2a0, function);
            return NULL;
        }
    }
    CompilerExpression *expression = make_unary_expression(0xa6, symbol_expression(parameter, 0));
    CompilerType *type = ((CompilerIrSymbol *)frontend_bss.current_function)->type;
    expression->type = type->shape->payload.function->type;
    if (!can_reference_symbol(parameter)) return NULL;
    expression->flags |= 0x20;
    return expression;
}
extern "C" CompilerExpression *__cdecl make_ir_binary_node(unsigned code,
    CompilerExpression *first, CompilerExpression *second)
{
    return make_binary_node(code, first, second, first->type);
}
/* 218fd..2194d: output the parameter, then push its symbol/level pair
 * onto the existing alternating block-output work stack. */
extern "C" void __cdecl emit_parameter_declaration(CompilerSymbol *symbol)
{
    write_ir_record(0x2a, symbol_expression(symbol, 0));
    prepend_work_link(&frontend_bss.block_output_entries, 4);
    frontend_bss.block_output_entries->value.symbol = symbol;
    prepend_work_link(&frontend_bss.block_output_entries, 4);
    frontend_bss.block_output_entries->value.level = 1;
}
extern "C" int __cdecl initialize_debug_function_output(unsigned, unsigned) { __asm { int 3 } return 0; }
extern "C" void __cdecl initialize_function_browser_output(void) { __asm { int 3 } }
extern "C" unsigned __cdecl function_parameter_bytes(CompilerParameterSet *) { __asm { int 3 } return 0; }
extern "C" void __cdecl print_preprocessed_function(CompilerSymbol *, FILE *) { __asm { int 3 } }

extern "C" void __cdecl finalize_symbol_declaration(CompilerDeclarator *lookup, CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    CompilerDeclarationState initial, state;
    CompilerType *type, *declared_type;
    CompilerGlobalSymbolView *owner;
    DeclarationClassDetail *detail;
    CompilerSymbol *qualified_scope;
    int adjust_linkage = 0;
    int invalid_function = 0;
    unsigned classification, warning_class;
    CompilerDeclarationMetadata *metadata;
    CompilerArena *arena;
    BYTE *memory;
#ifdef RECON_NATIVE_TEST
    /* Unnamed parameters, unbound functions and the reached members
     * use this common header. Wider variants remain unsupported. */
    if (!((input->kind == 11 && (!lookup || !symbol->next)) ||
          (input->kind == 10 && lookup && !symbol->next) ||
          (input->kind == 3 && lookup && !symbol->next) ||
          (input->kind == 12 && lookup && !symbol->next)))
        recon_unsupported("declaration: unmodeled symbol/lookup domain");
#endif
    DECL_FLAGS(initial) = *(DWORD *)&symbol->storage_class;
    if (symbol->reference_flags & 1) {
        initial.context.reference = resolve_symbol_reference(input);
        if (initial.context.reference) initial.context.reference_flags |= 1;
        else initial.context.reference_flags &= ~1;
    } else initial.SetReference(NULL);
    initial.SetMetadata((symbol->symbol_flags & 0x40) ? get_declaration_metadata(input) : NULL);
    if ((symbol->kind == 7 || symbol->kind == 8) &&
        (((DeclarationFunctionView *)symbol)->queue_flags & 0x80))
        initial.SetFunctionContext(get_declaration_function_context(input));
    else initial.SetFunctionContext(NULL);
    initial.context.attachment = (symbol->reference_flags & 0x80) ? get_declaration_attachment(input) : NULL;
    initial.context.class_context = symbol->kind == 4 && (DECL_DETAIL(symbol)->declaration_flags & 0x80);
    /* The four flag BYTEs and six typed DWORDs are this local object's
     * actual complete28-byte transfer; this is not a global context claim. */
    memcpy(&state, &initial, 28);
    if (frontend_bss.parser_context_depth >= 60) { __asm { int 3 } }
    qualified_scope = frontend_bss.parser_qualification_scopes[frontend_bss.parser_context_depth];
    if (qualified_scope) check_qualified_declaration(qualified_scope);
    if (!(symbol->attributes & 0x40)) {
        if (frontend_data.declaration_context_index >= 0 && !(initial.context.context_flags & 4))
            adjust_linkage = 1;
        else if (symbol->next) {
#ifdef RECON_NATIVE_TEST
            recon_unsupported("declaration: unmodeled binding owner");
#endif
            DeclarationBinding *binding = (DeclarationBinding *)((BYTE *)symbol->next - 8);
            if (binding->owner->scope->entries && binding->owner) adjust_linkage = 1;
        }
    }
    if (lookup) {
        declared_type = lookup->type;
        if (adjust_linkage && declared_type->shape &&
            (declared_type->shape->flags & 0x77) == 4 && (lookup->flags & 4)) {
            if (!(declared_type->shape->flags & 0x2000)) {
#ifdef RECON_NATIVE_TEST
                recon_unsupported("declaration: unmodeled scratch index and type-copy bytes");
#endif
                ++declaration_type_scratch_index;
                CompilerType *scratch = &declaration_type_scratch + declaration_type_scratch_index;
                memcpy(scratch, declared_type, 16);
                scratch->shape = qualify_declaration_shape(declared_type->shape);
                declared_type = intern_type(scratch);
                --declaration_type_scratch_index;
            }
            symbol->attributes |= 0x80;
        }
        if (lookup->flags & 1) symbol->scope_flags |= 4;
        symbol->type = declared_type;
        classification = DECL_CLASS(initial);
        validate_declaration_type(input, classification == 7);
        if (!declared_type->shape && classification != 7 &&
            !(frontend_data.compiler_mode_flags & 0x20000000)) {
            if (declared_type->flags == 0x80) {
                if (symbol->kind != 12 && symbol->kind != 13) {
                    compiler_warning(0xb6, input);
                    symbol->type = frontend_data.builtin_types.type_0063;
                }
            } else {
                if ((declared_type->flags & 0x400) && (short)declared_type->auxiliary >= 0 &&
                    !(symbol->reference_flags & 0x80)) {
                    owner = DECL_OWNER(declared_type);
                    if (owner) {
                        detail = DECL_DETAIL(owner);
                        if (detail->completion_flags & 2) {
                            if (!(frontend_data.source_mode_flags & 0x300000)) {
                                compiler_warning(0x3bb, owner);
                                symbol->scope_flags |= 4;
                            }
                        }
                        if ((detail->completion_flags & 1) && !(detail->template_flags & 1) &&
                            !(owner->definition_flags & 4) && !(detail->completion_flags & 4) &&
                            !(owner->scope_flags & 4) && !(frontend_data.source_mode_flags & 0x200020)) {
                            int same_class = frontend_data.declaration_context_index >= 0 &&
                                frontend_bss.class_scope_rows[frontend_data.declaration_context_index].symbol->name == owner->name;
                            if (!same_class) {
                                CompilerSymbol *instantiated = instantiate_declaration_class((CompilerSymbol *)owner);
                                if (DECL_DETAIL((CompilerGlobalSymbolView *)instantiated)->completion_flags & 8)
                                    merge_declaration_classes(instantiated, (CompilerSymbol *)owner);
                            }
                        }
                        if ((owner->definition_flags & 4) && (detail->context_flags & 0x80) &&
                            !(symbol->scope_flags & 4) && !declaration_source_state.suppress_class_completion &&
                            (!(frontend_data.compiler_mode_flags & 0x10000000) || !(frontend_data.source_mode_flags & 0x10))) {
                            diagnose_member_declaration((CompilerSymbol *)owner);
                            symbol->scope_flags |= 4;
                        }
                    }
                }
                declared_type = symbol->type;
                if (classification != 1 && (declared_type->flags & 0x400) &&
                    (short)declared_type->auxiliary >= 0 && !(symbol->reference_flags & 0x80)) {
                    owner = DECL_OWNER(declared_type);
                    if (owner) {
                        detail = DECL_DETAIL(owner);
                        if (owner->kind == 4 && (detail->completion_flags & 1) &&
                            !(detail->template_flags & 1) && !(owner->definition_flags & 4) &&
                            !(detail->completion_flags & 4) && !(owner->scope_flags & 4) &&
                            !(frontend_data.source_mode_flags & 0x200020)) {
                            int same_class = frontend_data.declaration_context_index >= 0 &&
                                frontend_bss.class_scope_rows[frontend_data.declaration_context_index].symbol->name == owner->name;
                            if (!same_class) {
                                CompilerSymbol *instantiated = instantiate_declaration_class((CompilerSymbol *)owner);
                                if (DECL_DETAIL((CompilerGlobalSymbolView *)instantiated)->completion_flags & 8)
                                    merge_declaration_classes(instantiated, (CompilerSymbol *)owner);
                            }
                        }
                        if (!(owner->definition_flags & 4) && symbol->kind != 13 && symbol->kind != 12 &&
                            symbol->kind != 11 && symbol->kind != 22 &&
                            !((frontend_data.source_mode_flags & 0x200000) && symbol->kind == 10) &&
                            !((frontend_data.source_mode_flags & 0x100000) && symbol->kind == 3) &&
                            (symbol->storage_class & 7) != 3) {
                            if (is_current_declaration_class((CompilerSymbol *)owner))
                                compiler_warning(0x1cc, input, owner);
                            else compiler_warning(0x4f, input, declared_type, owner);
                            symbol->type = frontend_data.builtin_types.type_0063;
                        }
                    }
                }
            }
        }
    }
    type = symbol->type;
    if (symbol->scope_flags & 4) state.context.class_flags = initial.context.class_flags | 8;
    if (symbol->kind == 7 || symbol->kind == 8) {
        if (!(lookup->flags & 2)) {
            if ((frontend_data.options.fastcall_calls || frontend_data.options.pascal_calls || frontend_data.options.stdcall_calls) &&
                symbol->name == frontend_data.builtin_names.main_entry && !symbol->declaration_qualifiers) {
                type = type->WithCallingConvention(0x10);
                report_language_diagnostic(3, 7, input, 0x25);
            } else if ((frontend_data.options.fastcall_calls || frontend_data.options.pascal_calls || frontend_data.options.cdecl_calls) &&
                (symbol->name == frontend_data.builtin_names.winmain_entry || symbol->name == frontend_data.builtin_names.dllmain_entry) && !symbol->declaration_qualifiers) {
                type = type->WithCallingConvention(0x60);
                report_language_diagnostic(3, 7, input, 0x82);
            }
        } else if ((symbol->type_flags & 0x30) && !(frontend_data.compiler_flags & 0x200)) {
            unsigned convention = type->shape->attributes & 0x70;
            if (convention != 0x70 && convention != 0x60) report_language_diagnostic(1, 0xa6);
            type = type->WithCallingConvention((type->shape->flags & 0x8000) ? 0x10 : 0x70);
        }
        if (!(state.context.class_flags & 8)) {
            if (type->shape->flags & 0x200) ((DeclarationFunctionView *)symbol)->linkage_flags |= 1;
            else if ((type->shape->flags & 0x100) && DECL_MODE() == 2) {
                owner = DECL_OWNER(symbol->type);
                if (!(owner->type_flags & 2) &&
                    (!(owner->definition_flags & 4) || !(DECL_DETAIL(owner)->declaration_flags & 1))) {
                    compiler_warning(0x20e, input, owner);
                    invalid_function = 1;
                }
            }
            if (state.context.context_flags & 2) {
                if (!adjust_linkage) {
                    compiler_warning(0x23f, input);
                    state.context.context_flags &= ~2;
                } else if (symbol->name == frontend_data.builtin_names.operator_new ||
                    symbol->name == frontend_data.builtin_names.operator_delete ||
                    symbol->name == frontend_data.builtin_names.operator_new_array ||
                    symbol->name == frontend_data.builtin_names.operator_delete_array) {
                    compiler_warning(0x28a, input);
                    state.context.context_flags &= ~2;
                    invalid_function = 1;
                }
            }
            if (state.context.context_flags & 4) {
                if (frontend_data.declaration_context_index < 0) {
                    compiler_warning(0xff, input);
                    state.context.context_flags &= ~4;
                    invalid_function = 1;
                } else if (!adjust_linkage) require_current_declaration_class();
            }
            if (DECL_CLASS(state) == 1 && adjust_linkage && !(state.context.class_flags & 0x20)) {
                if (frontend_data.declaration_context_index < 0 && frontend_data.options.language_extensions)
                    report_language_diagnostic(1, 0x276, input, 10);
                else compiler_warning(0x2d0, input, "extern");
                DECL_CLASS_SET(state, 0);
            }
            if (DECL_CLASS(state) == 3) {
                if (symbol->type_flags & 0x20) {
                    compiler_warning(0x23e, input);
                    invalid_function = 1;
                }
                if (adjust_linkage) ((DeclarationFunctionView *)symbol)->builtin_flags &= ~1;
                else if (!(frontend_data.source_mode_flags & 0x40) && frontend_bss.declaration_scope_mode)
                    compiler_warning(0x10b, input);
            }
            if (state.context.class_flags & 0x10) {
                if (symbol->type_flags & 0x30) type = type->Substitute(frontend_data.builtin_types.type_0080);
            } else {
                if (symbol->type_flags & 0x20) {
                    compiler_warning(0x277, input); invalid_function = 1;
                } else if (symbol->type_flags & 0x10) {
                    compiler_warning(0x215, input); invalid_function = 1;
                }
                if (invalid_function) type = type->Substitute(frontend_data.builtin_types.type_0080);
            }
            if (invalid_function) {
                state.context.class_flags = (state.context.class_flags | 8) & ~7;
                symbol->scope_flags |= 4;
            }
        }
        switch (DECL_CLASS(state)) {
        case 0: case 2: state.context.class_flags = (state.context.class_flags & 0xf9) | 0x21; break;
        case 3:
            if (state.context.context_flags & 2) {
                compiler_warning(0x240, input); state.context.context_flags &= ~2;
            }
            if ((state.context.member_flags & 0xc) && !adjust_linkage) {
                compiler_warning(0xc9, input);
                state.context.class_flags = (state.context.class_flags & 0xf9) | 0x21;
            }
            break;
        case 4: case 5: case 6:
            report_language_diagnostic(1, 0x2a, input);
            state.context.class_flags = (state.context.class_flags & 0xf9) | 0x21;
            break;
        case 7:
            if (symbol->definition_flags & 4) {
                compiler_warning(0x47, input);
                state.context.class_flags = (state.context.class_flags & 0xf9) | 1;
            }
            symbol->kind = 10;
            break;
        }
    } else {
        classification = DECL_CLASS(state);
        if (((classification >= 1 && classification <= 6 && classification != 3) ||
            ((state.context.member_flags & 0xc) && classification != 3)) && (symbol->kind == 2 || symbol->kind == 3)) {
            compiler_warning(0x47, input);
            state.context.class_flags = (state.context.class_flags & ~7) | 8;
            symbol->scope_flags |= 4;
        }
        warning_class = 1;
        if (state.context.context_flags & 2) {
            state.context.context_flags &= ~2; warning_class = 15;
        }
        if (state.context.context_flags & 4) {
            state.context.context_flags &= ~4; warning_class = 13;
            state.context.class_flags |= 8; symbol->scope_flags |= 4;
            symbol->kind = 3; adjust_linkage = 1;
        }
        if (state.context.context_flags & 1) {
            state.context.context_flags &= ~1; warning_class = 14;
        }
        if (warning_class != 1) {
            if (symbol->kind == 12) compiler_warning(0x2df, warning_class);
            else if (symbol->kind == 13) compiler_warning(0x2e0, warning_class);
            else compiler_warning(0x1b1, input, warning_class);
        }
        if (symbol->kind == 13) {
            switch (DECL_CLASS(state)) {
            case 1: warning_class = 10; break;
            case 3: warning_class = 9; break;
            case 4: warning_class = 8; break;
            case 5: warning_class = 11; break;
            case 7: warning_class = 12; break;
            default: warning_class = 1; break;
            }
            state.context.class_flags &= ~7;
            if (warning_class != 1) compiler_warning(0x2e0, warning_class);
        }
        if (symbol->kind == 11 || symbol->kind == 12 || symbol->kind == 22) {
            classification = DECL_CLASS(state);
            if (classification != 5 && classification != 4) {
                if (classification) report_language_diagnostic(1, 0x2a, input);
                state.context.class_flags = (state.context.class_flags & 0xfc) | 4;
            }
            if ((state.context.context_flags & 0x47) || (state.context.member_flags & 0x4f) ||
                (state.context.reference_flags & 0x9d) || state.context.function_context_active || state.context.class_context) {
                report_language_diagnostic(1, 0x2a, input);
                classification = DECL_CLASS(state);
                memset(&state, 0, 28);
                DECL_CLASS_SET(state, classification);
            }
        } else if (symbol->kind == 10) {
            unsigned flags = frontend_data.compiler_flags;
            classification = DECL_CLASS(state);
            if ((flags & 0x20000) || ((flags & 0x100) && (frontend_data.compiler_mode_flags & 0x100))) {
                if (!classification) {
                    if (flags & 0x20) state.context.class_flags = (state.context.class_flags & 0xfa) | 2;
                    else if (state.context.member_flags & 8) state.context.class_flags = (state.context.class_flags & 0xf9) | 1;
                    else state.context.class_flags = (state.context.class_flags & 0xfc) | 4;
                }
            } else if (classification >= 4 && classification <= 6) {
                compiler_warning(0x186, input, classification < 5 ? 8 : 11);
                state.context.class_flags = (state.context.class_flags & 0xfa) | 2;
            } else if (!classification) {
                if (get_function_calling_convention(type) & 0x100)
                    state.context.class_flags = (state.context.class_flags & 0xfb) | 3;
                else if (((symbol->qualifiers & 5) == 5 && !(symbol->attributes & 4)) || (state.context.member_flags & 8))
                    state.context.class_flags = (state.context.class_flags & 0xf9) | 1;
                else state.context.class_flags = (state.context.class_flags & 0xfa) | 2;
            }
        }
        if (DECL_CLASS(state) == 3 && (state.context.member_flags & 0xc) && symbol->kind != 2 && symbol->kind != 3) {
            compiler_warning(0xc9, input);
            state.context.member_flags &= ~0xc; state.context.class_flags |= 8;
        }
        if ((state.context.member_flags & 0xc) && (frontend_data.compiler_flags & 0x20000) &&
            !(frontend_data.compiler_flags & 0x20) && DECL_CLASS(state) != 1)
            compiler_warning(0x47, input);
    }
    if (!adjust_linkage) {
        int builtin = (symbol->kind == 7 || symbol->kind == 8) &&
            (signed char)((DeclarationFunctionView *)symbol)->builtin_operation >= 1 &&
            (signed char)((DeclarationFunctionView *)symbol)->builtin_operation < 17;
        if (!builtin) {
            if (frontend_data.declaration_mode_index < -1 ||
                frontend_data.declaration_mode_index >= 10) { __asm { int 3 } }
            symbol->type_flags ^= (symbol->type_flags ^ DECL_MODE()) & 3;
            if (frontend_data.options.extern_c_no_throw && (symbol->kind == 7 || symbol->kind == 8) &&
                DECL_MODE() == 2 && !(state.context.reference_flags & 0x20)) state.context.reference_flags |= 0x10;
        }
    }
    if (symbol->kind != 22 && (type->flags & 0x2fdf) == 0x12 && (short)type->auxiliary >= 0 &&
        !(frontend_data.compiler_mode_flags & 0x10000000) && !(frontend_data.source_mode_flags & 0x100000))
        compiler_warning(0x3bb, input);
    if (state.context.reference_flags & 4) {
        if (!((symbol->kind == 2 || symbol->kind == 3) && !DECL_CLASS(state) &&
            !(type->GetCallingConvention() & 0x100))) {
            compiler_warning(0x47, input); state.context.reference_flags &= ~4;
        }
    }
    if ((state.context.reference_flags & 8) && !((symbol->kind == 7 || symbol->kind == 8) &&
        (symbol->type_flags & 0x10) && frontend_data.declaration_context_index >= 0)) {
        compiler_warning(0x47, input); state.context.reference_flags &= ~8;
    }
    *(DWORD *)&symbol->storage_class = DECL_FLAGS(state);
    if ((state.context.reference_flags & 1) && state.context.reference) set_declaration_reference(input, state.context.reference);
    else symbol->reference_flags &= ~1;
    if ((state.context.context_flags & 0x40) && state.context.metadata) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("declaration: unmodeled metadata transfer");
#endif
        arena = &frontend_bss.arenas[0]; /* original c6500 */
        memory = arena->cursor;
        arena->cursor += 16;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        metadata = (CompilerDeclarationMetadata *)memory;
        if (metadata) memcpy(metadata, state.context.metadata, 16);
        set_declaration_metadata(input, metadata);
    } else symbol->symbol_flags &= ~0x40;
    if (state.context.attachment) set_declaration_attachment(input, state.context.attachment);
    symbol->type = type;
    symbol->reference_flags &= ~0x80;
}

void RECON_THISCALL CompilerDeclarationState::SetReference(CompilerSymbol *symbol)
{
    context.reference = symbol;
    if (symbol) context.reference_flags |= 1;
    else context.reference_flags &= ~1;
}
void RECON_THISCALL CompilerDeclarationState::SetMetadata(CompilerDeclarationMetadata *value)
{
    context.metadata = value;
    if (value) context.context_flags |= 0x40;
    else context.context_flags &= ~0x40;
}
void RECON_THISCALL CompilerDeclarationState::SetFunctionContext(CompilerFunctionContext *value)
{
    context.function_context_active = value != NULL;
    context.function_context = value;
}
CompilerType *RECON_THISCALL CompilerType::Substitute(CompilerType *type)
{
    CompilerTypeShape outer;
    CompilerType value;
    memcpy(&outer, shape, 12);
    memcpy(&value, type, 16);
    outer.base = type->shape;
    value.shape = intern_type_shape(&outer);
    value.attributes = (value.attributes & 0xff8f) | (attributes & 0x70);
    return intern_type(&value);
}
CompilerType *RECON_THISCALL CompilerType::WithCallingConvention(unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerDeclarationMetadata *__cdecl get_declaration_metadata(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" CompilerFunctionContext *__cdecl get_declaration_function_context(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" CompilerDeclarationAttachment *__cdecl get_declaration_attachment(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" int __cdecl can_assign_bound_member(CompilerSymbol *);
extern "C" const char *__cdecl access_diagnostic_kind(CompilerSymbol *);
extern "C" const char *__cdecl access_diagnostic_name(BYTE);
extern "C" void __cdecl check_qualified_declaration(CompilerSymbol *input)
{
    const char *kind;
    const char *access;
    if (!input || (frontend_data.source_mode_flags & 0x300000)) return;
    if (can_assign_bound_member(input)) return;
    kind = access_diagnostic_kind(frontend_bss.access_diagnostic_symbol);
    if (!frontend_bss.access_diagnostic_owner) {
        if (frontend_data.compiler_mode_flags & 1)
            report_language_diagnostic(3, 0xf1, frontend_bss.access_diagnostic_symbol);
        else compiler_warning(0xf1, frontend_bss.access_diagnostic_symbol);
        return;
    }
    access = access_diagnostic_name(frontend_bss.access_diagnostic_code);
    if (!frontend_bss.access_diagnostic_base) {
        unsigned diagnostic = 0xf8 + (frontend_bss.access_diagnostic_ambiguous != 0);
        if (frontend_data.compiler_mode_flags & 1)
            report_language_diagnostic(3, diagnostic, frontend_bss.access_diagnostic_symbol,
                access, kind, frontend_bss.access_diagnostic_owner);
        else compiler_warning(diagnostic, frontend_bss.access_diagnostic_symbol,
                access, kind, frontend_bss.access_diagnostic_owner);
    } else {
        access = access_diagnostic_name(frontend_bss.access_diagnostic_base_code);
        if (frontend_data.compiler_mode_flags & 1)
            report_language_diagnostic(3, 0xf7, frontend_bss.access_diagnostic_symbol,
                frontend_bss.access_diagnostic_owner, access, frontend_bss.access_diagnostic_base);
        else compiler_warning(0xf7, frontend_bss.access_diagnostic_symbol,
                frontend_bss.access_diagnostic_owner, access, frontend_bss.access_diagnostic_base);
    }
}
extern "C" const char *__cdecl access_diagnostic_kind(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" const char *__cdecl access_diagnostic_name(BYTE) { __asm { int 3 } return NULL; }
extern "C" void __cdecl validate_declaration_type(CompilerSymbol *input, int is_typedef)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    CompilerType *type = symbol->type;
    CompilerTypeShape *shape = type->shape;
#ifdef RECON_NATIVE_TEST
    if (!((input->kind == 10 && (!shape || (shape->flags & 0x77) == 4 ||
                               (shape->flags & 0x77) == 3 || (shape->flags & 0x77) == 0x40)) ||
          (input->kind == 3 && (!shape || (shape->flags & 0x77) == 4 ||
                              (shape->flags & 0x77) == 1 || (shape->flags & 0x77) == 2 ||
                              (shape->flags & 0x77) == 3)) ||
          (input->kind == 11 && (!shape || (shape->flags & 0x77) == 0x40 ||
                                        (shape->flags & 0x77) == 1)) ||
          (input->kind == 12 && (!shape || (shape->flags & 0x77) == 1 ||
                               (shape->flags & 0x77) == 3 || (shape->flags & 0x77) == 0x40))))
        recon_unsupported("declaration type: unsupported symbol/shape variant");
#endif
    if (shape && (shape->flags & 0x77) == 4) {
        if (symbol->kind == 10) symbol->kind = 7;
        else if (symbol->kind == 3 || symbol->kind == 2) {
            if (symbol->kind == 2) compiler_warning(0x290, input);
            symbol->kind = 8;
        } else {
            if (symbol->kind == 13 && !(frontend_data.compiler_mode_flags & 0x20000000))
                compiler_warning(0x42);
            type = type->RemoveFunction();
        }
    } else if (shape && (shape->flags & 0x77) == 2) {
        if (symbol->kind == 11 || symbol->kind == 12 || symbol->kind == 22) {
            if (shape->flags & 0x1000) compiler_warning(0x21c);
            type = type->ConvertArray(1);
        } else if (!is_typedef) {
            symbol->qualifiers |= 4;
            CompilerTypeShape *element = shape;
            while (element && (element->flags & 0x77) == 2) element = element->base;
            symbol->definition_flags = (symbol->definition_flags & ~0x80) | (!element ? 0x80 : 0);
            if (!element && !(symbol->reference_flags & 0x80) && !(shape->flags & 0x1000) && !shape->payload.value)
                symbol->qualifiers |= 1;
        }
    } else if (shape && (shape->flags & 0x40) && symbol->kind == 2)
        compiler_warning(0x213, input);
    symbol->type = type;
}
CompilerType *RECON_THISCALL CompilerType::ConvertArray(unsigned) { __asm { int 3 } return NULL; }
CompilerType *RECON_THISCALL CompilerType::RemoveFunction(void) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl instantiate_declaration_class(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl merge_declaration_classes(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" int __cdecl is_current_declaration_class(CompilerSymbol *symbol)
{
    int index = frontend_data.declaration_context_index;
    if (index < 0) return 0;
    if (index >= 16) { __asm { int 3 } }
    return ((CompilerNamedSymbol *)symbol)->link.name ==
        frontend_bss.class_scope_rows[index].symbol->name;
}
extern "C" CompilerTypeShape *__cdecl qualify_declaration_shape(CompilerTypeShape *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl require_current_declaration_class(void) { __asm { int 3 } }
extern "C" void __cdecl set_declaration_reference(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl set_declaration_metadata(CompilerSymbol *, CompilerDeclarationMetadata *) { __asm { int 3 } }
extern "C" void __cdecl set_declaration_attachment(CompilerSymbol *, CompilerDeclarationAttachment *) { __asm { int 3 } }

/* Views of existing binding/declaration owners on publication paths. No
 * native instances, copied gaps or default-valued replacement objects. */
struct PublicationOwnerView {
    PublicationOwnerView *parent; /* original+0, namespace-owner chain */
    BYTE unmodeled_04_07[4];
    CompilerSymbol *symbol; /* original+8; intervening bytes unmodeled */
};
struct PublicationBindingView { PublicationOwnerView *owner; /* original+0 */ };
struct PublicationBoundSymbolView : CompilerNamedSymbol {
    PublicationBindingView *binding; /* original+10 */
};
extern "C" {
CompilerSymbol *__cdecl merge_symbol_declaration(CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *);
int __cdecl is_class_scope_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl define_class_member(CompilerSymbol *, CompilerSymbol *, CompilerSymbol *);
CompilerSymbol *__cdecl check_symbol_definition(CompilerSymbol *);
int __cdecl is_function_specialization(CompilerSymbol *);
void __cdecl record_local_declaration(CompilerSymbol *);
void __cdecl validate_function_declaration(CompilerSymbol *, unsigned);
void __cdecl emit_symbol_definition(CompilerSymbol *);
}

extern "C" CompilerSymbol *__cdecl declare_builtin(CompilerDeclaration *declaration,
    CompilerSymbol *input, CompilerDeclarator *lookup)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    CompilerGlobalSymbolView *owner;
    CompilerSymbol *original = input;
    PublicationBoundSymbolView *bound;
    CompilerDeclarationContext *flags = &declaration->context;
    unsigned mode;
    int special_scope = 0;
    int was_reference = (symbol->attributes >> 2) & 1;
#ifdef RECON_NATIVE_TEST
    if (input->kind != 1 || ((CompilerNamedSymbol *)input)->link.next)
        recon_unsupported("symbol publication: unmodeled input/binding domain");
#endif
    bound = symbol->next ? (PublicationBoundSymbolView *)((BYTE *)symbol->next -
        offsetof(CompilerNamedSymbol, link)) : NULL;
    if (bound) {
        if (!bound->binding) return NULL;
        if (is_class_scope_symbol(bound->binding->owner->symbol)) {
            if (frontend_bss.declaration_scope_mode) compiler_warning(0x28f, input);
        } else special_scope = 1;
    }
    if ((frontend_data.compiler_mode_flags & 0x20) && !frontend_bss.declaration_scope_mode &&
        !(flags->class_flags & 7)) {
        flags->class_flags = (flags->class_flags & 0xd9) | 1;
    }
    symbol = (CompilerGlobalSymbolView *)merge_symbol_declaration(declaration, input, lookup);
#ifdef RECON_NATIVE_TEST
    /* A merged class consumes only its common header before returning.
     * The other reached variants retain their original owner. */
    if (!(symbol && symbol->kind == 4) &&
        (symbol != (CompilerGlobalSymbolView *)input || !(symbol->kind == 7 || symbol->kind == 10)))
        recon_unsupported("symbol publication: unmodeled merged declaration");
#endif
    if (special_scope) frontend_data.translation_unit->SetScopeMode(1);
    if (!symbol) {
        if (original && ((CompilerNamedSymbol *)original)->link.name &&
            (((CompilerNamedSymbol *)original)->link.name->flags & 0x2000000)) return NULL;
        compiler_warning(0xf4, original);
        original->scope_flags |= 4;
        return NULL;
    }
    if (symbol->scope_flags & 4) return NULL;
    if (symbol->kind == 4) return (CompilerSymbol *)symbol;
    mode = frontend_data.source_mode_flags;
    if (!(mode & 0x24) && frontend_bss.browser_filename &&
        (!frontend_bss.declaration_scope_mode || publish_browser_local)) {
        if (!(symbol->record_index & 0xffffff)) {
            if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(55);
            unsigned index = frontend_data.next_symbol_record_id++;
            symbol->record_index = (symbol->record_index & 0xff000000) | (index & 0xffffff);
        }
        frontend_data.backend->Slot20((CompilerSymbol *)symbol);
        mode = frontend_data.source_mode_flags;
    }
    if (symbol->kind == 7 || symbol->kind == 8 || symbol->kind == 6) {
        if (was_reference) { compiler_warning(0x48, symbol); return NULL; }
        if (symbol->attributes & 0x10) compiler_warning(0x88, symbol);
        validate_function_declaration((CompilerSymbol *)symbol, 0);
        if (flags->member_flags & 1) compiler_warning(0x1e8, symbol);
        return (CompilerSymbol *)symbol;
    }
    if (!(mode & 0x200004) && !(symbol->member_flags & 8)) {
        if (was_reference) {
            if (frontend_bss.declaration_scope_mode && (symbol->storage_class & 7) == 1) {
                symbol->scope_flags |= 4;
                compiler_warning(0xcd, symbol);
            } else symbol->definition_flags |= 4;
        } else if ((symbol->storage_class & 7) == 1) {
            if (!(symbol->symbol_flags & 0x10) &&
                (!(symbol->qualifiers & 4) || !(symbol->qualifiers & 1))) symbol->definition_flags |= 4;
        } else symbol->definition_flags |= 4;
    }
    if (bound) {
        owner = (CompilerGlobalSymbolView *)bound->binding->owner->symbol;
        if (frontend_data.translation_unit->current != frontend_data.translation_unit->root)
            frontend_data.translation_unit->PrepareScopeSymbol((CompilerSymbol *)owner);
        CompilerGlobalSymbolView *class_symbol = (CompilerGlobalSymbolView *)
            frontend_data.frontend_pointer_table->items[(short)owner->declaration_qualifiers];
        if (class_symbol->kind == 4) {
            if (is_class_scope_symbol((CompilerSymbol *)owner)) {
                if (frontend_data.source_mode_flags & 0x200004) return (CompilerSymbol *)owner;
                return define_class_member((CompilerSymbol *)symbol, bound, (CompilerSymbol *)owner);
            }
            if (frontend_data.compiler_flags & 0x200) return (CompilerSymbol *)symbol;
            compiler_warning(0x15e, bound);
        }
    }
    if ((symbol->member_flags & 8) && ((symbol->definition_flags & 4) || was_reference)) {
        CompilerGlobalSymbolView *current = (CompilerGlobalSymbolView *)frontend_bss.current_function;
        if (!(frontend_data.compiler_flags & 0x20000) || !(current->member_flags & 8) ||
            !(current->symbol_flags & 1)) compiler_warning(0x1eb, symbol, 0x179);
    }
    if ((symbol->storage_class & 7) && (symbol->storage_class & 7) != 7 &&
        (flags->class_flags & 7) != 1 && !was_reference &&
        (!(symbol->definition_flags & 4) || !check_symbol_definition((CompilerSymbol *)symbol)) &&
        symbol->kind == 10) {
        if (is_function_specialization((CompilerSymbol *)symbol)) compiler_warning(0x212, symbol);
        if ((symbol->qualifiers & 4) && symbol->type->shape->base &&
            (symbol->type->shape->base->flags & 0x40)) compiler_warning(0x212, symbol);
        if (symbol->member_flags & 2) record_local_declaration((CompilerSymbol *)symbol);
    }
    emit_symbol_definition((CompilerSymbol *)symbol);
    frontend_bss.global_expression_scope_active = 1;
    return (CompilerSymbol *)symbol;
}

extern "C" int __cdecl is_class_scope_symbol(CompilerSymbol *symbol)
{
    if (!symbol || (symbol->kind != 2 && symbol->kind != 3)) return 0;
    return (((CompilerIrSymbol *)symbol)->storage_class & 7) == 3;
}
extern "C" CompilerSymbol *__cdecl define_class_member(CompilerSymbol *, CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" CompilerGlobalSymbolView *__cdecl enclosing_declaration_class(CompilerSymbol *);
extern "C" CompilerSymbol *__cdecl check_symbol_definition(CompilerSymbol *input)
{
    if ((input->definition_flags & 0x10) ||
        ((((CompilerIrSymbol *)input)->storage_class & 7) == 7)) return NULL;
    CompilerGlobalSymbolView *owner = enclosing_declaration_class(input);
    if (!owner) return NULL;
    CompilerGlobalClassDetail *detail = owner->class_detail;
    if ((detail->completion_flags & 1) && !(detail->template_flags & 1) &&
        !(owner->definition_flags & 4) && !(detail->completion_flags & 4)) {
        int current = 0;
        int index = frontend_data.declaration_context_index;
        if (index >= 0) {
            if ((unsigned)index >= 16) { __asm { int 3 } }
            current = frontend_bss.class_scope_rows[index].symbol->name == owner->name;
        }
        if (!current && !(owner->scope_flags & 4) && !(frontend_data.source_mode_flags & 0x200020)) {
            CompilerSymbol *instance = instantiate_declaration_class((CompilerSymbol *)owner);
            if (((CompilerGlobalSymbolView *)instance)->class_detail->completion_flags & 8)
                merge_declaration_classes(instance, (CompilerSymbol *)owner);
        }
    }
    detail = owner->class_detail;
    if ((detail->generated_member_flags & 0x1a) == 0x1a && (detail->declaration_flags & 1)) return NULL;
    return (CompilerSymbol *)owner;
}
extern "C" int __cdecl is_function_specialization(CompilerSymbol *input)
{
    if (!input) return 0;
    CompilerTypeShape *shape = ((CompilerIrSymbol *)input)->type->shape;
    return shape && (shape->flags & 0x40) ? 1 : 0;
}
extern "C" void __cdecl record_local_declaration(CompilerSymbol *) { __asm { int 3 } }
extern "C" CompilerTypeShape *__cdecl strip_declaration_shape(CompilerTypeShape *);
extern "C" void __cdecl validate_function_declaration(CompilerSymbol *input, unsigned check_member)
{
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)input;
    unsigned operation;
    CompilerFunctionType *function;
    CompilerParameterTypes *parameters;
    CompilerType **types, **parameter;
    CompilerType *type;
    CompilerTypeShape *shape;
    int count, related = 0, unknown = 0, named_count;
    int member = (symbol->type_flags & 4) != 0;
    if (symbol->kind != 7 && symbol->kind != 8 && symbol->kind != 6) return;
    if (symbol->type_flags & 0x30) return;
    operation = symbol->function_class;
    if (operation == 1) return;
    if (check_member && member) return;
#ifdef RECON_NATIVE_TEST
    if (operation != 0x34 && operation != 0x51 && operation != 0x8c && operation != 0x8d)
        recon_unsupported("function validation: other operator-specific owners remain unmodeled");
#endif
    switch (operation) {
    case 0x36: case 0x50: case 0x56: case 0x5e: case 0x81: case 0x92: case 0x95:
        compiler_warning(0x320, operation); symbol->scope_flags |= 4; return;
    case 0x3d: case 0x54: case 0x55: case 0x5a: case 0x86:
        return;
    case 0x34: case 0x41: case 0x53: case 0x76: case 0x8f:
        if (!member) { compiler_warning(0x321, operation, 0); symbol->scope_flags |= 4; return; }
        if (!(symbol->builtin_flags & 1)) { compiler_warning(0x321, operation, 0x152); symbol->scope_flags |= 4; return; }
        break;
    }
    function = symbol->type->shape->payload.function;
    parameters = function->parameters;
    types = parameters->types;
    count = (int)parameters->count;
    named_count = function->symbol_count;
    if (count == 1 && !types[0]->shape && (types[0]->flags & 0x80)) count = 0;
    for (unsigned i = 0; i < (unsigned)count; ++i) {
        type = types[i];
        if (!type) { unknown = 1; continue; }
        unsigned code = type->flags & 0x2fdf;
        if ((type->flags & 0x600) || code == 0x12 || code == 0x11) {
            shape = type->shape;
            if (!shape || ((shape->flags & 0x40) && !shape->base)) ++related;
        }
    }
    parameter = types;
    if (member && (symbol->builtin_flags & 1)) { ++related; ++count; }
    if (!count) {
        if (!member) { compiler_warning(0x329, operation); symbol->scope_flags |= 4; }
        else if (!(symbol->builtin_flags & 1)) { compiler_warning(0x322, operation); symbol->scope_flags |= 4; }
        return;
    }
    if (operation != 0x8c && operation != 0x8d && !related) {
        compiler_warning(0x323, operation); symbol->scope_flags |= 4;
    }
    if (unknown && operation != 0x8c && operation != 0x76) {
        compiler_warning(0x33d, operation); symbol->scope_flags |= 4;
    }
    switch (operation) {
    case 6: return;
    case 0x31: case 0x46: case 0x4a: case 0x51:
        if (count == 1) {
            if (operation == 0x46) symbol->function_class = 0x5a;
            else if (operation == 0x4a) symbol->function_class = 0x3d;
            else if (operation == 0x51) symbol->function_class = 0x86;
            break;
        }
        /* These forms also accept the binary arity. */
        if (count > 2) { compiler_warning(0x324, operation); symbol->scope_flags |= 4; }
        else if (count < 2) { compiler_warning(0x325, operation); symbol->scope_flags |= 4; }
        break;
    case 0x32: case 0x33: case 0x37: case 0x39: case 0x3a: case 0x3b:
    case 0x3e: case 0x3f: case 0x41: case 0x42: case 0x43: case 0x44:
    case 0x45: case 0x47: case 0x48: case 0x49: case 0x4b: case 0x4c:
    case 0x4d: case 0x4e: case 0x4f: case 0x52: case 0x57: case 0x58:
    case 0x5b: case 0x5c: case 0x93:
        if (count > 2) { compiler_warning(0x324, operation); symbol->scope_flags |= 4; }
        else if (count < 2) { compiler_warning(0x325, operation); symbol->scope_flags |= 4; }
        break;
    case 0x34:
        if (count > 2 && !(symbol->scope_flags & 1)) { compiler_warning(0x324, operation); symbol->scope_flags |= 4; }
        else if (count < 2) { compiler_warning(0x325, operation); symbol->scope_flags |= 4; }
        break;
    case 0x38: case 0x40:
        if (count > 2) { compiler_warning(0x326, operation); symbol->scope_flags |= 4; }
        else if (count == 2) {
            if (!member || !(symbol->builtin_flags & 1)) ++parameter;
            type = *parameter;
            if (!(((type->flags & 0x40) || (short)type->auxiliary < 0) &&
                  ((type->flags & 0x4f) == 0x43 || (short)type->auxiliary < 0) &&
                  !type->shape && !(type->flags & 0x4000))) {
                compiler_warning(0x327, operation); symbol->scope_flags |= 4;
            }
        } else symbol->function_class = operation == 0x38 ? 0x54 : 0x55;
        break;
    case 0x53: {
        CompilerSymbol *owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[(short)symbol->declaration_qualifiers];
        type = symbol->type;
        unsigned flags = type->flags;
        if ((flags & 0x408) != 0x408 && (flags & 0x402) != 0x402 &&
            (flags & 0x404) != 0x404 && (short)type->auxiliary >= 0)
            report_language_diagnostic(2, 0x11c, owner);
        else {
            shape = strip_declaration_shape(type->shape->base);
            if (shape) {
                if ((shape->flags & 0x77) != 1 || shape->base)
                    report_language_diagnostic(2, 0x11c, owner);
            } else if ((short)type->auxiliary >= 0 &&
                frontend_data.frontend_pointer_table->items[(short)type->auxiliary] == owner)
                report_language_diagnostic(2, 0x11d, owner);
        }
        if (count > 1) { compiler_warning(0x328, operation); symbol->scope_flags |= 4; }
        break;
    }
    case 0x3c: case 0x59: case 0x8f:
        if (count > 1) {
            if (operation == 0x8f) compiler_warning(0x343, input);
            else compiler_warning(0x328, operation);
            symbol->scope_flags |= 4;
        }
        break;
    case 0x8c:
        type = symbol->type; shape = type->shape;
        if (!(type->flags & 0x80) || !shape || (shape->flags & 0x77) != 4 ||
            !shape->base || (shape->base->flags & 0x40) || shape->base->base || (shape->base->attributes & 3)) {
            compiler_warning(0x338); symbol->scope_flags |= 4;
        }
        type = *parameter;
        if (type->flags != 0x4063 || type->shape) {
            compiler_warning(parameter == types ? 0x335 : 0x336); symbol->scope_flags |= 4;
        }
        break;
    case 0x8d:
        type = symbol->type; shape = type->shape;
        if (!(type->flags & 0x80) || !shape || shape->base || (shape->flags & 0x77) != 4) {
            compiler_warning(0x331); symbol->scope_flags |= 4;
        }
        if (count > 2) { compiler_warning(0x32d); symbol->scope_flags |= 4; }
        type = *parameter; shape = type->shape;
        if (!(type->flags & 0x80) || (type->attributes & 0x1f00) || !shape ||
            (shape->flags & 0x40) || shape->base || (shape->attributes & 3)) {
            compiler_warning(0x32f); symbol->scope_flags |= 4;
        }
        if (count >= 2) {
            if (!member) { compiler_warning(0x330); symbol->scope_flags |= 4; }
            type = parameter[1];
            if (type->flags != 0x4063 || type->shape) { compiler_warning(0x32a); symbol->scope_flags |= 4; }
        }
        break;
    }
    operation = symbol->function_class;
    if (operation == 0x76) return;
    if (operation == 0x8c) {
        if (count - named_count < 1) { compiler_warning(0x33e); symbol->scope_flags |= 4; }
    } else if (named_count) { compiler_warning(0x33f, operation); symbol->scope_flags |= 4; }
}
extern "C" CompilerTypeShape *__cdecl strip_declaration_shape(CompilerTypeShape *shape)
{
    return shape && (shape->flags & 0x40) ? shape->base : shape;
}

extern "C" {
void __cdecl apply_declaration_reference(CompilerSymbol *, CompilerSymbol *);
void __cdecl check_global_declaration(CompilerSymbol *);
int __cdecl check_type_declaration_attribute(CompilerType *);
unsigned __cdecl global_type_size(CompilerType *);
}
extern "C" void __cdecl emit_symbol_definition(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input, *owner;
    CompilerType *type;
    unsigned classification;
    if (symbol->definition_flags & 0x10) return;
    classification = symbol->storage_class & 7;
#ifdef RECON_NATIVE_TEST
    /* The reached variable path uses only the common header before its
     * calling-convention query. Wider reference owners remain pending. */
    if (symbol->kind != 10 || classification < 1 || classification > 3)
        recon_unsupported("symbol emission: unmodeled non-typedef domain");
#endif
    switch (classification) {
    case 1:
        if (!(symbol->attributes & 4)) return;
        /* An actual definition shares the reference selection below. */
    case 2: case 3: {
        CompilerSymbol *reference;
        if (symbol->type->GetCallingConvention() & 0x100)
            reference = declaration_reference_defaults.type_context;
        else reference = (symbol->attributes & 4) ? declaration_reference_defaults.definition :
            declaration_reference_defaults.fallback;
        apply_declaration_reference(input, reference);
        break;
    }
    case 4: case 5: case 6:
        if (symbol->reference_flags & 1) compiler_warning(0xb9, input);
        break;
    case 7: return;
    }
    check_global_declaration(input);
    if (symbol->attributes & 4) return;
    type = symbol->type;
    if ((get_type_classification(type) & 0x82) && (get_function_calling_convention(type) & 0x100)) {
        owner = DECL_OWNER(type);
        if ((DECL_DETAIL(owner)->declaration_flags & 0x40) && check_type_declaration_attribute(type)) {
            if (classification == 2 || classification == 3) report_language_diagnostic(4, 0x10c, input);
            else report_language_diagnostic(1, 0x10d, input);
        }
    }
    if (global_type_size(type) || classification == 1) return;
    if (type->flags & 0x400) {
        owner = DECL_OWNER(type);
        if (owner->kind != 4) owner = DECL_OWNER(owner->type);
        if (!(owner->definition_flags & 4)) {
            compiler_warning(0x4f, input, owner->type, owner);
            symbol->scope_flags |= 4;
            return;
        }
    }
    compiler_warning(0x85, input);
}
extern "C" void __cdecl apply_declaration_reference(CompilerSymbol *input, CompilerSymbol *reference)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    if (reference) {
        if (!(symbol->reference_flags & 1) || (symbol->reference_flags & 2)) {
            set_declaration_reference(input, reference);
            symbol->reference_flags |= 2;
        }
    } else if (symbol->reference_flags & 2) {
        symbol->reference_flags &= ~1;
    }
}
extern "C" int __cdecl check_global_type_completion(CompilerType *);
extern "C" void __cdecl defer_global_declaration(CompilerSymbol *);
extern "C" void __cdecl check_global_declaration(CompilerSymbol *input)
{
    if (frontend_bss.error_count || (input->scope_flags & 4) ||
        frontend_bss.declaration_scope_mode) return;
    if (!check_symbol_definition(input)) return;
    CompilerType *type = ((CompilerIrSymbol *)input)->type;
    if (check_global_type_completion(type)) {
        CompilerGlobalSymbolView *owner = DECL_OWNER(type);
        if (!(owner->class_detail->generated_member_flags & 0x10))
            defer_global_declaration(input);
    } else {
        compiler_warning(0x106, input);
        input->scope_flags |= 4;
    }
}
extern "C" int __cdecl check_global_type_completion(CompilerType *) { __asm { int 3 } return 0; }
extern "C" void __cdecl defer_global_declaration(CompilerSymbol *) { __asm { int 3 } }
extern "C" int __cdecl check_type_declaration_attribute(CompilerType *) { __asm { int 3 } return 0; }
extern "C" CompilerSymbol *__cdecl prepare_pending_initializer(CompilerSymbol *);
extern "C" void __cdecl record_initializer_context(CompilerSymbol *);
extern "C" CompilerSymbol *__cdecl prepare_pending_initializer(CompilerSymbol *input)
{
    frontend_bss.initializer_pending_state = 0;
    frontend_data.compiler_flags &= ~0x40000U;
    if (frontend_bss.declaration_scope_mode &&
        (((CompilerIrSymbol *)input)->storage_class & 7) >= 4 &&
        (((CompilerIrSymbol *)input)->storage_class & 7) <= 6)
        frontend_data.compiler_flags |= 0x100000;
    else frontend_data.compiler_flags &= ~0x100000U;
    record_initializer_context(input);
    frontend_bss.initializing_symbol = input;
    frontend_bss.initializer_value_state = 0;
    frontend_data.initializer_reset_values[0] = 0;
    frontend_data.initializer_reset_values[1] = 0;
    frontend_data.initializer_reset_values[2] = 0;
    return input;
}
extern "C" CompilerWorkLink *__cdecl prepend_work_link(CompilerWorkLink **, unsigned);
extern "C" void __cdecl record_initializer_context(CompilerSymbol *input)
{
    ++frontend_data.initializer_context_count;
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    CompilerType *type = symbol->type;
    if (!type->shape || (type->shape->flags & 0x77) != 2) return;
    unsigned storage = symbol->storage_class & 7;
    if (storage >= 1 && storage <= 3) return;
    if (!DECL_OWNER(type)) return;
    frontend_bss.initializer_context.symbol = input;
    frontend_bss.initializer_context.state_index = frontend_data.parameter_work_index;
    CompilerWorkLink *link = prepend_work_link(&frontend_bss.initializer_context.head, 4);
    link->value.kind = frontend_data.parameter_work_kind;
    frontend_data.parameter_work_kind = 7;
}
/* 165aa..165db: allocate one eight-byte link, retaining its untouched payload. */
extern "C" CompilerWorkLink *__cdecl prepend_work_link(CompilerWorkLink **head, unsigned index)
{
    if (index >= 9) { __asm { int 3 } }
    CompilerArena *arena = &frontend_bss.arenas[index];
    CompilerWorkLink *link = (CompilerWorkLink *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    link->next = *head;
    *head = link;
    return link;
}
extern "C" CompilerSymbol *__cdecl prepare_symbol_initializer(CompilerSymbol *input)
{
    if (frontend_bss.suppress_parser_codegen) return input;
    if (!input) return NULL;
    if (input->scope_flags & 4) return input;
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    unsigned storage = symbol->storage_class & 7;
    if (storage == 7) {
        compiler_warning(0x47, input);
        return NULL;
    }
    if (storage == 1) {
        if (frontend_bss.declaration_scope_mode) compiler_warning(0xcd, input);
        symbol->storage_class = (symbol->storage_class & ~7) | 2;
    }
    if (symbol->member_flags & 8) frontend_data.source_mode_flags |= 0x800;
    return prepare_pending_initializer(input);
}
extern "C" void __cdecl begin_symbol_initializer(CompilerSymbol *input)
{
    frontend_data.expression_target = input;
    if (!input) {
        frontend_data.expression_target_active = 0;
        return;
    }
    int active = is_function_specialization(input);
    frontend_data.expression_target_active = 1;
    if (!active) frontend_data.expression_target_active = 0;
}
extern "C" void __cdecl initialize_symbol_value(CompilerSymbol *, CompilerExpression *);
extern "C" void __cdecl initialize_symbol(CompilerSymbol *input, CompilerExpression *expression)
{
    initialize_symbol_value(input, expression);
    if (frontend_data.source_mode_flags & 0x800) {
        frontend_data.source_mode_flags &= ~0x800U;
        input->attributes &= ~4;
    }
}
void RECON_THISCALL CompilerTranslationUnit::SetScopeMode(unsigned mode)
{
    BYTE level = current->level;
    while (level >= 0xfb && level != 0xfd) {
        if (mode && level != 0xfe) {
            if (level == 0xfc) PopCurrentScope();
            return;
        }
        PopCurrentScope();
        level = current->level;
    }
}
void RECON_THISCALL CompilerTranslationUnit::PopCurrentScope(void)
{
    UnregisterCurrentScope();
    if (current->context) current->RestoreContext();
    current = current->next;
}
void RECON_THISCALL CompilerTranslationUnit::UnregisterCurrentScope(void)
{
    if (!current->HasBuckets()) return;
    if (depth >= 128) { __asm { int 3 } }
    if (scopes[depth] != current) {
        report_compiler_error(0x1fa);
        return;
    }
    scopes[depth] = NULL;
    --depth;
}
void RECON_THISCALL CompilerScopeTable::RestoreContext(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTranslationUnit::PrepareScopeSymbol(CompilerSymbol *) { __asm { int 3 } }

/* Additional source-only tagged views of existing owners. */
typedef CompilerNamespaceData PublicationNamespacePayload;
typedef CompilerScopeMatch PublicationLookupRecord;
extern "C" {
void __cdecl prepare_function_declaration(CompilerSymbol *, CompilerDeclarator *);
void __cdecl begin_declaration_diagnostic(void);
void __cdecl end_declaration_diagnostic(void);
CompilerDiagnosticContext __cdecl describe_declaration_context(unsigned, CompilerDeclarationContext);
const char *__cdecl format_declaration_context(CompilerDiagnosticContext);
WORD __cdecl get_type_classification(CompilerType *);
void __cdecl validate_symbol_attributes(CompilerSymbol *);
CompilerSymbol *__cdecl find_bound_declaration(CompilerSymbol *, CompilerSymbol *);
int __cdecl merge_function_declarations(CompilerSymbol *, CompilerSymbol *, CompilerDeclarator *);
CompilerSymbol *__cdecl resolve_generated_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl merge_scoped_declaration(CompilerSymbol *, CompilerSymbol *, CompilerDeclarator *);
void __cdecl discard_declaration_symbol(CompilerSymbol *);
void __cdecl record_scoped_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl get_function_template_owner(CompilerSymbol *);
}

extern "C" CompilerSymbol *__cdecl merge_symbol_declaration(CompilerDeclaration *declaration,
    CompilerSymbol *input, CompilerDeclarator *lookup)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    CompilerDeclarationContext state, original_context;
    PublicationBoundSymbolView *bound, *found;
    PublicationOwnerView *owner;
    CompilerGlobalSymbolView *target, *current;
    CompilerScopeMatch *result, *selected;
    CompilerScopeLookup *matches;
    CompilerScopeTable *scope;
    CompilerArena *arena;
    PublicationLookupRecord *created;
    CompilerType *type;
    unsigned classification, code;
    int type_declaration = 0;
    if (!input || !declaration->type) return NULL;
#ifdef RECON_NATIVE_TEST
    if ((symbol->kind != 1 && symbol->kind != 3 && symbol->kind != 11) ||
        symbol->next || frontend_bss.current_function)
        recon_unsupported("merge: unmodeled binding/current-function domain");
#endif
    declaration->context.class_flags &= 0x7f;
    state = declaration->context;
    if ((state.class_flags & 7) == 1) {
        state.context_flags |= 0x10;
        current = (CompilerGlobalSymbolView *)frontend_bss.current_function;
        if (current && current->declaration_qualifiers) {
            target = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)current->declaration_qualifiers];
            if (target->kind == 23)
                symbol->declaration_qualifiers = ((PublicationNamespacePayload *)target->type)->type_index;
        }
    }
    bound = symbol->next ? (PublicationBoundSymbolView *)((BYTE *)symbol->next - offsetof(CompilerNamedSymbol, link)) : NULL;
    if (bound && is_class_scope_symbol(bound->binding->owner->symbol) &&
        (!(state.class_flags & 0x20) || (state.context_flags & 6) ||
         (state.member_flags & 8) || (state.reference_flags & 0x80) || state.class_context)) {
        begin_declaration_diagnostic();
        compiler_warning(0x2d0, input, format_declaration_context(describe_declaration_context(0, state)));
        end_declaration_diagnostic();
        classification = state.class_flags & 7;
        memset(&state, 0, sizeof(state));
        state.class_flags = 8; /* original clears the low classification bits before reset */
        symbol->scope_flags |= 4;
    }
    if (lookup->flags & 8) {
        prepare_function_declaration(input, lookup);
        if (lookup->parameters) {
            unsigned mode = lookup->parameters->declaration_mode;
            if (mode == 1) state.reference_flags |= 0x30;
            else if (mode == 2) state.reference_flags |= 0x20;
        }
    }
    if (symbol->kind == 2 || symbol->kind == 3) type_declaration = 1;
    else if (symbol->kind == 1) symbol->kind = 10;
    *(DWORD *)&symbol->storage_class = *(DWORD *)&state.class_flags;
    if ((state.reference_flags & 1) && state.reference) set_declaration_reference(input, state.reference);
    else symbol->reference_flags &= ~1;
    input->SetDeclarationMetadata((state.context_flags & 0x40) ? state.metadata : NULL);
    if (state.attachment) set_declaration_attachment(input, state.attachment);
    else symbol->reference_flags &= ~0x80;
    finalize_symbol_declaration(lookup, input);

    classification = symbol->storage_class & 7;
    if (symbol->kind != 2 && symbol->kind != 3 && symbol->kind != 17 && symbol->kind != 8 &&
        !(symbol->kind == 7 && frontend_data.declaration_context_index > -1 && !(symbol->symbol_flags & 4)) &&
        classification != 7 && !(symbol->kind == 7 && (symbol->type_flags & 0x30)) &&
        symbol->name && (symbol->name->flags & 0x2000000)) {
        result = frontend_data.translation_unit->LookupName(symbol->name, 0x20);
        CompilerSymbol *previous = result ? result->symbol : NULL;
        if (symbol->kind == 7 || symbol->kind == 8) code = 0x3a7;
        else if (frontend_data.compiler_mode_flags & 0x10000000) code = 0x3b0;
        else if ((classification >= 1 && classification <= 3) || !frontend_bss.declaration_scope_mode) code = 0x3a8;
        else code = frontend_bss.declaration_scope_mode == 1 ? 0x3ae : 0x3ab;
        compiler_warning(code, previous);
    }
    original_context = declaration->context;
    if ((original_context.class_flags & 0x30) == 0x30 && !(symbol->type_flags & 0x30) &&
        ((symbol->kind != 7 && symbol->kind != 8) ||
         (!(symbol->type_flags & 0x40) && symbol->type->StripOuterShape()->shape)))
        compiler_warning(0x1f5, 0x53, input);
    if ((symbol->member_flags & 1) &&
        (!(symbol->symbol_flags & 1) || (symbol->scope_flags & 4)) &&
        (!((symbol->kind == 7 || symbol->kind == 8) && frontend_data.declaration_context_index < 0)) &&
        !(symbol->scope_flags & 4)) {
        compiler_warning(0x1e8, input);
        symbol->scope_flags |= 4;
    }
    current = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    classification = symbol->storage_class & 7;
    if (frontend_bss.declaration_scope_mode == 2 && current && (current->member_flags & 1) &&
        classification >= 4 && classification <= 6) {
        code = get_type_classification(symbol->type);
        if (((code & 0x80) && !(DECL_DETAIL(DECL_OWNER(symbol->type))->declaration_flags & 1)) ||
            (symbol->attributes & 4)) compiler_warning(0x1e9, input);
    }
    if (symbol->member_flags & 2) {
        if ((symbol->kind == 10 || symbol->kind == 3) && classification >= 1 && classification <= 3) {
            code = get_type_classification(symbol->type);
            if ((code == 0x80 || code == 2) && !(DECL_DETAIL(DECL_OWNER(symbol->type))->declaration_flags & 1) &&
                !(symbol->scope_flags & 4)) {
                compiler_warning(0x1e3, input); symbol->scope_flags |= 4;
            }
        } else {
            compiler_warning(0x1e0, input); symbol->member_flags &= ~2;
        }
        if (symbol->member_flags & 0xc) compiler_warning(0x1ec, input);
    }
    if ((symbol->reference_flags & 1) &&
        !((symbol->kind == 10 || symbol->kind == 3) && classification >= 1 && classification <= 3)) {
        compiler_warning(0x1df, input); symbol->reference_flags &= ~1;
    }
    if ((symbol->reference_flags & 0x10) && symbol->kind != 7 && symbol->kind != 8 && !(symbol->scope_flags & 4)) {
        compiler_warning(0x1ef, input); symbol->scope_flags |= 4;
    }
    if (symbol->member_flags & 0x40) {
        if (symbol->kind == 10 && (symbol->attributes & 4) &&
            (classification == 1 || classification == 2 || (classification == 3 && (symbol->storage_class & 0x20))))
            symbol->symbol_flags |= 0x80;
        else {
            compiler_warning(0x1f0, input);
            symbol->member_flags &= ~0x40; symbol->storage_class &= ~0x40;
        }
    }
    validate_symbol_attributes(input);
    if (state.function_context_active) {
        if (symbol->kind == 7 || symbol->kind == 8) input->SetFunctionContext(state.function_context);
        else if (!(symbol->scope_flags & 4)) { compiler_warning(0x1f1, input); symbol->scope_flags |= 4; }
    }
    if (state.class_context && symbol->kind != 4 && !(symbol->scope_flags & 4)) {
        compiler_warning(0x1f2, input); symbol->scope_flags |= 4;
    }
    if ((symbol->type_flags & 0x40) && (lookup->flags & 4)) compiler_warning(0x24a);
    if ((symbol->scope_flags & 4) && symbol->kind != 7 && symbol->kind != 8 && symbol->kind != 6 &&
        type_declaration)
        return input;
    found = bound;
    if (found) {
        found = (PublicationBoundSymbolView *)find_bound_declaration((CompilerSymbol *)found, input);
        if (!found) return NULL;
        target = (CompilerGlobalSymbolView *)found->binding->owner->symbol;
        if (symbol->kind == 7 || symbol->kind == 8) {
            merge_function_declarations((CompilerSymbol *)target, input, lookup);
            return found->binding->owner->symbol;
        }
        if ((target->attributes & 0x40) && target->kind == 3) {
            if (frontend_data.source_mode_flags & 0x200000) return (CompilerSymbol *)target;
            target = (CompilerGlobalSymbolView *)resolve_generated_symbol((CompilerSymbol *)target);
            symbol->name = target->name;
            symbol->storage_class = (symbol->storage_class & ~7) ^ 2;
        } else {
            if (frontend_data.compiler_flags & 0x200) {
                merge_function_declarations((CompilerSymbol *)target, input, lookup);
                return found->binding->owner->symbol;
            }
            return input;
        }
    }
    classification = symbol->storage_class & 7;
    if (classification == 7) {
        symbol->definition_flags |= 0x10;
        if (symbol->name) symbol->name->flags |= 0x20000;
        type = symbol->type;
        if (type && ((type->flags & 0x600) || (type->flags & 0x2fdf) == 0x12 || (type->flags & 0x2fdf) == 0x11) &&
            (!type->shape || ((type->shape->flags & 0x40) && !type->shape->base)) &&
            (type->flags & 0x2fdf) != 0x12 && (type->flags & 0x2fdf) != 0x11) {
            target = DECL_OWNER(type);
            if (!target) { symbol->scope_flags |= 4; return input; }
            if (target->scope_flags & 1) {
                if (symbol->kind == 3) symbol->name->flags |= 0x40000;
                target->name = symbol->name;
                target->name = symbol->name; /* actual repeated store */
                DECL_DETAIL(target)->declaration_flags |= 8;
                target->scope_flags &= ~1;
                if (!frontend_data.option_c9il) symbol->definition_flags |= 8;
            }
        }
        if (symbol->name && (symbol->name->flags & 0x2000000)) {
            result = frontend_data.translation_unit->LookupName(symbol->name, 0x20);
            CompilerSymbol *previous = result ? result->symbol : NULL;
            if (frontend_bss.declaration_scope_mode) compiler_warning(0x3ac, previous);
            else if (frontend_data.declaration_context_index == -1) compiler_warning(0x3a9, previous);
            else compiler_warning(0x3a5, previous,
                frontend_bss.class_scope_rows[frontend_data.declaration_context_index].symbol);
            return NULL;
        }
    }
    if (type_declaration) return input;
    if ((frontend_data.source_mode_flags & 1) && (symbol->kind == 7 || symbol->kind == 8))
        symbol->linkage_flags |= 0x40;
    target = (CompilerGlobalSymbolView *)frontend_data.translation_unit->CurrentDeclarationOwner();
    if (target) symbol->declaration_qualifiers = ((PublicationNamespacePayload *)target->type)->type_index;
    result = NULL;
    if (found && is_class_scope_symbol(found->binding->owner->symbol)) {
        owner = found->binding->owner->parent;
        target = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[
            (short)((CompilerGlobalSymbolView *)owner->symbol)->declaration_qualifiers];
        while (target && target->kind != 23) {
            owner = owner->parent;
            target = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[
                (short)((CompilerGlobalSymbolView *)owner->symbol)->declaration_qualifiers];
        }
        if (target) {
            scope = ((PublicationNamespacePayload *)target->type)->scope;
            matches = scope->Find(symbol->name, 1);
            if (matches) {
                selected = matches->Select(1);
                if (selected) {
                    CompilerSymbol *merged = merge_scoped_declaration(selected->symbol, input, 0);
                    if (merged) {
                        created = (PublicationLookupRecord *)frontend_bss.arenas[5].Allocate(12);
                        if (created) { created->symbol = merged; created->scope = NULL; created->next = NULL; result = created; }
                    }
                }
            } else {
                CompilerSymbol *merged = merge_scoped_declaration(NULL, input, 0);
                if (merged) {
                    arena = &frontend_bss.arenas[5];
                    created = (PublicationLookupRecord *)arena->cursor;
                    arena->cursor += 12;
                    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
                    if (created) { created->symbol = merged; created->scope = NULL; created->next = NULL; result = created; }
                }
            }
        }
    } else if (!((frontend_data.source_mode_flags & 0x100000) && (symbol->kind == 7 || symbol->kind == 8)))
        result = frontend_data.translation_unit->ProbeDeclaration(input);
    CompilerSymbol *existing = result ? result->symbol : NULL;
    if (!existing && frontend_bss.declaration_scope_mode >= 2) {
        result = frontend_data.translation_unit->FindScopedDeclaration(input);
        existing = result ? result->symbol : NULL;
    } else if (!existing && !frontend_data.options.language_extensions && frontend_data.translation_unit->current->level == 1) {
        result = frontend_data.translation_unit->LookupName(symbol->name, 0x132);
        if (result && result->symbol && (result->symbol->definition_flags & 0x10))
            report_language_diagnostic(1, 0xe0, input);
    }
    if (existing) {
        target = (CompilerGlobalSymbolView *)existing;
        if (symbol->attributes & 0x80) target->attributes |= 0x80;
        if (state.reference_flags & 0x40) {
            if (target->kind == 4) {
                if (!(DECL_DETAIL(target)->template_flags & 1)) compiler_warning(0x38f, target->name);
            } else if (target->kind == 7) {
                if (!(target->linkage_flags & 0x80)) {
                    if (target->linkage_flags & 0x40) compiler_warning(0x38c, existing, 0x44);
                    else compiler_warning(0x390, existing);
                }
            } else if (target->kind == 10) {
                CompilerGlobalSymbolView *template_owner = (CompilerGlobalSymbolView *)get_function_template_owner(existing);
                CompilerGlobalSymbolView *class_owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)template_owner->declaration_qualifiers];
                DeclarationClassDetail *class_detail = DECL_DETAIL(class_owner);
                if (!(class_detail->template_flags & 1)) {
                    if (class_detail->completion_flags & 1) {
                        if (target->references & 3) compiler_warning(0x38c, template_owner, 0x44);
                    } else compiler_warning(0x38b, template_owner);
                }
            }
        }
        if (input != existing) discard_declaration_symbol(input);
        return existing;
    }
    if (!(symbol->record_index & 0xffffff)) {
        if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(55);
        unsigned index = frontend_data.next_symbol_record_id++;
        symbol->record_index = (symbol->record_index & 0xff000000) | (index & 0xffffff);
    }
    if (frontend_bss.declaration_scope_mode >= 2) record_scoped_symbol(input);
    if (symbol->kind == 10)
        symbol->declaration_scope_number = (!(frontend_data.compiler_flags & 0x20) && (symbol->storage_class & 7) != 1) ?
            frontend_data.declaration_scope_number : 0;
    if ((frontend_data.source_mode_flags & 0x100000) && (symbol->kind == 7 || symbol->kind == 8)) return input;
    if (original_context.context_flags & 0x20) frontend_data.translation_unit->root->Insert(input);
    else frontend_data.translation_unit->DeclarationScope(1)->Insert(input);
    return input;
}

/* Genuine named entry traps for unreconstructed declaration dependencies. */
extern "C" void __cdecl prepare_function_declaration(CompilerSymbol *symbol, CompilerDeclarator *lookup)
{
    CompilerParameterDescription *parameters = lookup->parameters;
    CompilerType *type = lookup->type;
    if (!parameters) return;
    symbol->attributes |= 0x80;
    frontend_bss.current_parameters = parameters->parameters;
    frontend_bss.current_parameter_kind = parameters->kind;
    if (type->shape->flags & 0x4000)
        frontend_bss.current_parameter_kind = 5;
}
extern "C" CompilerDiagnosticContext __cdecl describe_declaration_context(unsigned, CompilerDeclarationContext) { __asm { int 3 } return 0; }
extern "C" const char *__cdecl format_declaration_context(CompilerDiagnosticContext) { __asm { int 3 } return NULL; }
extern "C" WORD __cdecl get_type_classification(CompilerType *type)
{
    CompilerTypeShape *shape, *base;
    WORD flags;
    if (!type || !(type->flags & 0x400)) return 0;
    shape = type->shape;
    if (!shape) return 0x80;
    flags = shape->flags;
    if ((flags & 0x40) && !shape->base) return 0x40;
    flags &= 0x77;
    if (flags == 1) return !shape->base ? 1 : 0;
    if (flags == 2) {
        base = shape->base;
        while (base && ((BYTE)base->flags & 0x77) == 2) base = base->base;
        if (!base) return 2;
    }
    return 0;
}
extern "C" void __cdecl validate_symbol_attributes(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    if (!(symbol->reference_flags & 0x80)) return;
    if (symbol->kind != 3 || (symbol->storage_class & 7) == 3) {
        compiler_warning(0x30b, input);
        symbol->reference_flags &= ~0x80;
    } else if (symbol->type->shape && (symbol->type->shape->flags & 0x77) == 2) {
        symbol->type = intern_type(symbol->type->StripArrayDimensions());
        symbol->qualifiers &= ~5;
    }
}
CompilerType *RECON_THISCALL CompilerType::StripArrayDimensions(void) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl resolve_function_declaration(CompilerSymbol *, CompilerSymbol *, CompilerType *, unsigned); /*7375*/
extern "C" CompilerSymbol *__cdecl find_bound_declaration(CompilerSymbol *input, CompilerSymbol *declaration)
{
    CompilerBoundSymbol *bound = NULL;
    CompilerSymbol *value = input, *selected;
    unsigned kind;
    if (!value) return NULL;
    if (value->kind == 18) {
        bound = (CompilerBoundSymbol *)value;
        value = bound->binding->owner->symbol;
    }
    selected = value;
    if (value->definition_flags & 0x20) {
        CompilerSymbol *linked = (CompilerSymbol *)((BYTE *)((CompilerNamedSymbol *)value)->link.next - 8);
        if (value->definition_flags & 0x10) selected = linked;
        else value = linked;
    }
    if (!(frontend_data.compiler_flags & 0x10) && declaration) {
        if (declaration->kind == 7 || declaration->kind == 8) {
            kind = selected->kind;
            if (kind == 24) kind = ((CompilerAliasSymbol *)selected)->target->symbol->kind;
            if (kind == 16 || kind == 7 || kind == 6 || kind == 17 || kind == 8 || kind == 25) {
                value = resolve_function_declaration(selected, declaration,
                    ((CompilerIrSymbol *)declaration)->type, 1);
                if (!value) return NULL;
            } else {
                compiler_warning(0x3f, selected);
                value = selected;
            }
        } else if (!(declaration->definition_flags & 0x10)) value = selected;
    }
    if (bound) {
        bound->binding->owner->symbol = value;
        return (CompilerSymbol *)bound;
    }
    return value;
}
extern "C" CompilerSymbol *__cdecl resolve_single_function_declaration(CompilerSymbol *, CompilerSymbol *, CompilerType *, unsigned); /*73a5*/
extern "C" CompilerSymbol *__cdecl resolve_function_declaration(CompilerSymbol *input,
    CompilerSymbol *declaration, CompilerType *type, unsigned mode)
{
    if (input->kind != 25)
        return resolve_single_function_declaration(input, declaration, type, mode);
    CompilerSymbol *selected = NULL;
    CompilerOverloadLink *entry = ((CompilerOverloadSymbol *)input)->group->head;
    do {
        CompilerSymbol *match = resolve_single_function_declaration(entry->symbol, declaration, type, mode);
        if (match) {
            if (selected && selected != match) return NULL;
            selected = match;
        }
        entry = entry->next;
    } while (entry);
    return selected;
}
extern "C" int __cdecl merge_function_declarations(CompilerSymbol *, CompilerSymbol *, CompilerDeclarator *) { __asm { int 3 } return 0; }
extern "C" {
CompilerSymbol *__cdecl merge_function_binding(CompilerSymbol *, CompilerSymbol *, CompilerDeclarator *);
CompilerSymbol *__cdecl merge_identical_declaration(CompilerSymbol *, CompilerSymbol *);
CompilerSymbol *__cdecl prepare_function_symbol(CompilerSymbol *);
void __cdecl combine_symbol_declaration(CompilerSymbol *, CompilerSymbol *);
void __cdecl merge_nested_declaration_values(void *, void *, CompilerSymbol *, unsigned);
int __cdecl is_complete_declaration_type(CompilerType *);
}
/* These kind tests classify the same original alias target; no extra helper
 * function or native alias payload is introduced by this expression. */
#define DECL_PROTECTED_KIND(k) ((k)==16 || (k)==7 || (k)==6 || (k)==17 || (k)==8 || (k)==25)
extern "C" CompilerSymbol *__cdecl merge_scoped_declaration(CompilerSymbol *previous,
    CompilerSymbol *input, CompilerDeclarator *lookup)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    CompilerGlobalSymbolView *existing = (CompilerGlobalSymbolView *)previous;
    CompilerGlobalSymbolView *original;
    unsigned kind = symbol->kind;
    if (frontend_data.compiler_mode_flags & 0x10000000) return NULL;
#ifdef RECON_NATIVE_TEST
    /* A first kind10 declaration consumes only its common header/type here;
     * the calling-convention query retains the next pending method. */
    if (!((kind == 7 || kind == 8) || (!previous && (kind == 11 || kind == 10 || kind == 5)) ||
        (!previous && kind == 4 && !(symbol->storage_class & 0x20)) ||
        (kind == 10 && (symbol->storage_class & 7) == 7 && !(symbol->storage_class & 0x20) &&
        ((!previous) ||
        (previous && existing->kind == 4 && !(existing->definition_flags & 0x20))))))
        recon_unsupported("scoped declaration: unmodeled previous symbol/record variant");
#endif
    if (kind != 11 && kind != 12 && kind != 22 && kind != 7 && kind != 8 &&
        kind >= 1 && kind <= 14 && kind != 5 && (symbol->storage_class & 0x20) &&
        (get_function_calling_convention(symbol->type) & 0x100)) {
        int valid = 1;
        if (!(symbol->attributes & 4) &&
            (!(get_type_classification(symbol->type) & 0x82) || !is_complete_declaration_type(symbol->type))) {
            compiler_warning(0x2de, input);
            valid = 0;
        }
        if (valid && !frontend_bss.declaration_scope_mode) {
            int retains_previous = 0;
            if (existing) {
                unsigned existing_kind = existing->kind;
                if (existing_kind == 24)
                    existing_kind = (*(CompilerSymbol **)(existing->type))->kind;
                retains_previous = DECL_PROTECTED_KIND(existing_kind) ||
                    (existing->storage_class & 0x20) || (existing->storage_class & 7) != 1;
                if (!retains_previous) {
                    symbol->storage_class = (symbol->storage_class & ~7) | 2;
                    symbol->symbol_flags |= 0x10;
                }
            }
            if (!existing) {
                symbol->storage_class = ((symbol->storage_class & ~7) | 3) & 0x9f;
                if (symbol->member_flags & 0x40) {
                    compiler_warning(0x1f0, input);
                    symbol->member_flags &= ~0x40;
                    symbol->symbol_flags &= ~0x80;
                    symbol->symbol_flags |= 0x10;
                }
            }
        }
    }
    if (!existing) return NULL;
    original = existing;
    existing = (CompilerGlobalSymbolView *)find_bound_declaration((CompilerSymbol *)existing, NULL);
    kind = existing->kind;
    if (kind == 24) kind = (*(CompilerSymbol **)(existing->type))->kind;
    if (DECL_PROTECTED_KIND(kind) && (symbol->kind == 7 || symbol->kind == 8))
        return merge_function_binding((CompilerSymbol *)existing, input, lookup);
    if (existing == original) {
        CompilerSymbol *merged = merge_identical_declaration((CompilerSymbol *)existing, input);
        if (merged) return merged;
    }
    if (!frontend_bss.declaration_scope_mode &&
        ((existing->storage_class & 7) == 1 || (symbol->storage_class & 7) == 1 || (existing->member_flags & 8))) {
        if (merge_function_declarations((CompilerSymbol *)existing, input, lookup)) {
            symbol->scope_flags |= 4;
            return input;
        }
        combine_symbol_declaration((CompilerSymbol *)existing, input);
        return (CompilerSymbol *)existing;
    }
    if ((existing->storage_class & 7) == 1 && (symbol->storage_class & 7) == 1) {
        if (merge_function_declarations((CompilerSymbol *)existing, input, lookup)) {
            symbol->scope_flags |= 4;
            return input;
        }
        CompilerTypeShape *shape = symbol->type->shape;
        kind = shape->flags & 0x77;
        if (kind == 3 || kind == 0x20 || kind == 0x44)
            merge_nested_declaration_values(shape->base->payload.type,
                existing->type->shape->base->payload.type, input, 1);
        return (CompilerSymbol *)existing;
    }
    if (!merge_function_declarations((CompilerSymbol *)existing, input, lookup)) compiler_warning(0x56, input);
    symbol->scope_flags |= 4;
    return input;
}
extern "C" {
int __cdecl is_ambiguous_function_binding(CompilerSymbol *); /*8d34d*/
void __cdecl insert_function_binding(CompilerSymbol **, CompilerSymbol *); /*2b4ab*/
int __cdecl find_builtin_function(CompilerNameEntry *);
int __cdecl builtin_function_enabled(unsigned);
void __cdecl bind_builtin_function(CompilerSymbol *, unsigned);
void __cdecl bind_type_declaration(CompilerFunctionType *, CompilerSymbol *);
}
extern "C" CompilerSymbol *__cdecl merge_function_binding(CompilerSymbol *previous,
    CompilerSymbol *input, CompilerDeclarator *lookup)
{
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)input;
    CompilerRuntimeFunction *selected = (CompilerRuntimeFunction *)resolve_function_declaration(
        previous, input, symbol->type, 1);
    if (frontend_bss.deferred_action_head) {
        frontend_bss.deferred_action_tail = NULL;
        frontend_bss.deferred_action_head = NULL;
    }
    if (selected) {
        if ((selected->scope_flags & 4) || (symbol->scope_flags & 4)) return (CompilerSymbol *)selected;
        if (merge_function_declarations((CompilerSymbol *)selected, input, lookup)) return NULL;
        if ((selected->kind != 7 && selected->kind != 8) || !(selected->linkage_flags & 0x80))
            combine_symbol_declaration((CompilerSymbol *)selected, input);
        return (CompilerSymbol *)selected;
    }
    if ((symbol->link.name == frontend_data.builtin_names.main_entry ||
         symbol->link.name == frontend_data.builtin_names.winmain_entry ||
         symbol->link.name == frontend_data.builtin_names.dllmain_entry) && !symbol->declaration_qualifiers)
        compiler_warning(0x2db, input);
    if (symbol->type_flags & 2) {
        if (is_ambiguous_function_binding(previous)) compiler_warning(0x2dd, input);
        else {
            int builtin = find_builtin_function(symbol->link.name);
            if (builtin != -1 && builtin_function_enabled(builtin)) bind_builtin_function(input, builtin);
        }
    }
    bind_type_declaration(symbol->type->shape->payload.function, input);
    insert_function_binding(&previous, input);
    if (frontend_bss.declaration_scope_mode >= 2 && !(frontend_data.source_mode_flags & 0x40)) {
        CompilerScopeLookup *matches = frontend_data.translation_unit->root->Find(symbol->link.name, 1);
        CompilerScopeMatch *match = matches ? matches->Select(1) : NULL;
        CompilerSymbol *candidate = match ? match->symbol : NULL;
        if (candidate) {
            unsigned kind = candidate->kind;
            if (kind == 24) kind = ((CompilerAliasSymbol *)candidate)->target->symbol->kind;
            if (DECL_PROTECTED_KIND(kind)) {
                selected = (CompilerRuntimeFunction *)resolve_function_declaration(candidate, input, symbol->type, 0);
                if (selected) {
                    symbol->encoded_index = (symbol->encoded_index & 0xff000000) | (selected->encoded_index & 0xffffff);
                    symbol->definition_flags |= 8;
                    selected->definition_flags &= ~8;
                    if (selected->reference_flags & 1) {
                        CompilerSymbol *reference = resolve_symbol_reference((CompilerSymbol *)selected);
                        if (reference) set_declaration_reference(input, reference);
                        else symbol->reference_flags &= ~1;
                    }
                    if ((selected->emission_state & 3) < 3)
                        selected->emission_state = (selected->emission_state & ~3U) | ((selected->emission_state + 1) & 3);
                    selected->emission_state = (selected->emission_state & ~4U) | ((frontend_data.reference_context & 1) << 2);
                    return input;
                }
            }
        }
    }
    if (!(symbol->encoded_index & 0xffffff)) {
        if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(0x37);
        unsigned index = frontend_data.next_symbol_record_id++;
        symbol->encoded_index = (symbol->encoded_index & 0xff000000) | (index & 0xffffff);
    }
    return input;
}
extern "C" int __cdecl is_ambiguous_function_binding(CompilerSymbol *) { __asm { int 3 } return 0; }
extern "C" void __cdecl insert_function_binding(CompilerSymbol **previous, CompilerSymbol *input)
{
    CompilerSymbol *symbol = *previous;
    if (symbol->kind == 7 || symbol->kind == 8 || symbol->kind == 6 || symbol->kind == 24) {
        *previous = frontend_data.translation_unit->CreateFunctionGroup(input, symbol);
    } else {
        CompilerSymbolLink *link = &((CompilerNamedSymbol *)input)->link;
        link->next = ((CompilerMemberGroupSymbol *)symbol)->variants;
        ((CompilerMemberGroupSymbol *)symbol)->variants = link;
    }
}
extern "C" CompilerSymbol *__cdecl instantiate_tag_definition(CompilerSymbol *);
extern "C" CompilerSymbol *__cdecl recycle_symbol(unsigned, CompilerNameEntry *, unsigned);
extern "C" void __cdecl link_tag_definition(CompilerSymbol *, CompilerSymbol *); /*8cdc1*/
CompilerSymbol *RECON_THISCALL CompilerTranslationUnit::CreateFunctionGroup(CompilerSymbol *input, CompilerSymbol *previous)
{
    CompilerSymbolLink *new_link = &((CompilerNamedSymbol *)input)->link;
    CompilerSymbolLink *old_link = &((CompilerNamedSymbol *)previous)->link;
    unsigned ordinary = previous->kind == 7 || previous->kind == 6 ||
        (previous->kind == 24 && ((CompilerAliasSymbol *)previous)->target->symbol->kind == 7);
    CompilerSymbolLink **slot = FindFunctionBindingSlot(old_link, ordinary);
    CompilerSmallSymbol *group;
    CompilerKind4Symbol *class_owner = NULL;
    int replace_tail = 0;
    unsigned kind = (frontend_data.declaration_context_index < 0 ||
        (input->kind != 24 && (((CompilerIrSymbol *)input)->function_flags & 4))) ? 16 : 17;
    CompilerNameEntry *name = old_link->name;
    group = (CompilerSmallSymbol *)recycle_symbol(kind, name, 0);
    if (!group) {
        BYTE *memory = frontend_bss.arenas[0].Allocate((frontend_data.symbol_record_bytes[kind] + 3) & ~3U);
        group = memory ? (CompilerSmallSymbol *)((CompilerSymbol *)memory)->Initialize(kind, name, 0) : NULL;
    }
    if (kind == 17) {
        short owner_index = previous->kind == 24 ?
            ((CompilerAliasSymbol *)previous)->target->owner_index :
            (short)previous->declaration_qualifiers;
        CompilerSymbol *owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[owner_index];
        if (owner->kind != 4) { __asm { int 3 } }
        class_owner = (CompilerKind4Symbol *)owner;
        replace_tail = class_owner->class_binding->last_member == old_link;
    }
    CompilerSymbol *source = previous->kind == 24 ? ((CompilerAliasSymbol *)previous)->target->symbol : previous;
    group->type_flags = (group->type_flags & ~0x40) | (source->type_flags & 0x40);
    source = previous->kind == 24 ? ((CompilerAliasSymbol *)previous)->target->symbol : previous;
    group->type_flags = (group->type_flags & ~0x10) | (source->type_flags & 0x10);
    source = previous->kind == 24 ? ((CompilerAliasSymbol *)previous)->target->symbol : previous;
    group->type_flags = (group->type_flags & ~3) | (source->type_flags & 3);
    if (previous->declaration_qualifiers) {
        CompilerSymbol *owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[(short)previous->declaration_qualifiers];
        if (owner) group->SetDeclarationOwner(owner);
    }
    if (previous->definition_flags & 0x20) {
        CompilerSymbol *target = instantiate_tag_definition(previous);
        group->definition_flags |= 0x20;
        link_tag_definition((CompilerSymbol *)group, target);
        previous->definition_flags &= ~0x20;
    } else if (previous->definition_flags & 0x40) {
        CompilerSymbol *target = instantiate_tag_definition(previous);
        group->definition_flags |= 0x40;
        link_tag_definition(target, (CompilerSymbol *)group);
        previous->definition_flags &= ~0x40;
    }
    group->link.next = old_link->next;
    *slot = &group->link;
    if (replace_tail) class_owner->class_binding->last_member = &group->link;
    group->variants = new_link;
    new_link->next = old_link;
    old_link->next = NULL;
    return (CompilerSymbol *)group;
}
CompilerSymbolLink **RECON_THISCALL CompilerClassScope::FindLink(CompilerSymbolLink *input)
{
    if (symbol->kind != 4) { __asm { int 3 } }
    CompilerSymbolLink **slot = &((CompilerKind4Symbol *)symbol)->class_binding->members;
    CompilerNameEntry *name = input->name;
    do {
        if ((*slot)->name == name) return slot;
        slot = &(*slot)->next;
    } while (*slot);
    return NULL;
}
CompilerSymbolLink **RECON_THISCALL CompilerTranslationUnit::FindFunctionBindingSlot(CompilerSymbolLink *input, unsigned ordinary)
{
    unsigned index = depth;
    if (index >= 128) { __asm { int 3 } }
    for (;;) {
        CompilerScopeTable *scope = scopes[index];
        unsigned level = scope->level;
        if (level != 1 && level != 0xfa &&
            ((level == 0xff && !ordinary) || (level == 0 && ordinary) ||
             (level > 1 && level < 0x80))) {
            CompilerSymbolLink **slot = scope->FindLink(input);
            if (slot) return slot;
        }
        if (!index) return NULL;
        --index;
    }
}
void RECON_THISCALL CompilerSymbol::SetDeclarationOwner(CompilerSymbol *owner)
{
    /* The original kind26 arm loops without changing its cached kind or
     * receiver. This arm has not been exercised by the current case. */
    if (owner->kind == 26) for (;;) {}
    if (owner->kind == 4) {
        declaration_qualifiers = ((CompilerGlobalSymbolView *)owner)->class_detail->scope_number;
        return;
    }
    if (owner->kind == 23) {
        declaration_qualifiers = ((CompilerNamespaceSymbol *)owner)->data->scope_number;
        return;
    }
    if (owner->kind != 7 && owner->kind != 8) return;
    CompilerGlobalSymbolView *function = (CompilerGlobalSymbolView *)owner;
    if (!function->declaration_scope_number) owner->SetDeclarationScopeNumber(7);
    declaration_qualifiers = function->declaration_scope_number;
    if (kind == 4)
        ((CompilerGlobalSymbolView *)this)->class_detail->nested_scope_number = frontend_data.declaration_scope_number;
}
extern "C" void __cdecl link_tag_definition(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" CompilerSymbol *__cdecl merge_identical_declaration(CompilerSymbol *previous, CompilerSymbol *input)
{
    CompilerGlobalSymbolView *existing = (CompilerGlobalSymbolView *)previous;
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    if ((existing->definition_flags & 0x10) && (symbol->definition_flags & 0x10)) {
        CompilerType *type = symbol->type;
        if (type == existing->type) return previous;
        if (existing->kind != 4) return NULL;
        short index = (short)type->auxiliary;
        CompilerPointerArray *table = frontend_data.frontend_pointer_table;
        if (index < 0 || index >= table->count) { __asm { int 3 } }
        if (table->items[index] != previous || type->shape) return NULL;
        return type->attributes == existing->type->attributes || !type->attributes ? previous : NULL;
    }
    if (!(existing->definition_flags & 0x10) && !(symbol->definition_flags & 0x10)) return NULL;
    if (existing->definition_flags & 0x10) {
        if ((existing->storage_class & 7) == 7) compiler_warning(0x179, previous);
    } else if ((symbol->storage_class & 7) == 7) compiler_warning(0x17a, previous);
    if (existing->definition_flags & 0x20) return previous;
    existing->definition_flags |= 0x20;
    CompilerSymbol *replacement;
    if (symbol->kind == 7 || symbol->kind == 8) replacement = prepare_function_symbol(input);
    else {
        if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(0x37);
        unsigned index = frontend_data.next_symbol_record_id++;
        symbol->record_index = (symbol->record_index & 0xff000000) | (index & 0xffffff);
        replacement = input;
    }
    CompilerSymbolLink *old_link = &((CompilerNamedSymbol *)previous)->link;
    CompilerSymbolLink *new_link = &((CompilerNamedSymbol *)replacement)->link;
    new_link->next = old_link->next;
    replacement->definition_flags |= 0x40;
    link_tag_definition(previous, replacement);
    old_link->next = new_link;
    int depth = frontend_data.declaration_context_index;
    if (depth >= 0) {
        if ((unsigned)depth >= 16) { __asm { int 3 } }
        CompilerGlobalSymbolView *owner = frontend_bss.class_scope_rows[depth].symbol;
        if (owner->kind != 4) { __asm { int 3 } }
        CompilerClassBinding *binding = ((CompilerKind4Symbol *)owner)->class_binding;
        if (binding->last_member == old_link) binding->last_member = new_link;
    }
    return input;
}
extern "C" void __cdecl combine_symbol_declaration(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl merge_nested_declaration_values(void *, void *, CompilerSymbol *, unsigned) { __asm { int 3 } }
extern "C" int __cdecl is_complete_declaration_type(CompilerType *) { __asm { int 3 } return 0; }
extern "C" void __cdecl discard_declaration_symbol(CompilerSymbol *symbol)
{
    if (frontend_bss.declaration_scope_mode && frontend_data.declaration_context_index < 0 &&
        !(frontend_data.compiler_mode_flags & 0x10000000)) {
        int reusable = 0;
        if ((frontend_data.compiler_mode_flags & 0x40000000) &&
            !(frontend_data.source_mode_flags & 8) && !(frontend_data.compiler_flags & 0x200))
            reusable = 1;
        if (!reusable && !(frontend_data.source_mode_flags & 0x50)) return;
    }
    unsigned kind = symbol->kind;
    if (kind >= 26) { __asm { int 3 } }
    unsigned size_class = frontend_data.symbol_size_class[kind];
    if (size_class >= 11) { __asm { int 3 } }
    CompilerSymbolLink *link = &((CompilerNamedSymbol *)symbol)->link;
    link->next = frontend_data.recycled_symbols[size_class];
    frontend_data.recycled_symbols[size_class] = link;
}
extern "C" void __cdecl record_scoped_symbol(CompilerSymbol *) { __asm { int 3 } }
extern "C" CompilerSymbol *__cdecl get_function_template_owner(CompilerSymbol *) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerSymbol::SetDeclarationMetadata(CompilerDeclarationMetadata *metadata)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)this;
    if (!metadata) { symbol->symbol_flags &= ~0x40; return; }
#ifdef RECON_NATIVE_TEST
    recon_unsupported("declaration metadata: unmodeled nonnull16-byte transfer");
#endif
    CompilerArena *arena = &frontend_bss.arenas[0];
    CompilerDeclarationMetadata *copy = (CompilerDeclarationMetadata *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (copy) memcpy(copy, metadata, 16);
    set_declaration_metadata(this, copy);
}
void RECON_THISCALL CompilerSymbol::SetFunctionContext(CompilerFunctionContext *) { __asm { int 3 } }
CompilerType *RECON_THISCALL CompilerType::StripOuterShape(void)
{
    CompilerType stripped;
    memcpy(&stripped, this, 16);
    stripped.attributes &= ~0x70U;
    stripped.shape = shape->base;
    return intern_type(&stripped);
}
CompilerScopeMatch *RECON_THISCALL CompilerScopeLookup::Select(unsigned diagnose)
{
    CompilerScopeMatch *first = head;
    if (!first) return NULL;
    if (first == tail) return first;
    int saw_functions = 0, saw_other = 0;
    CompilerSymbol *other = NULL;
    for (CompilerScopeMatch *match = first; match; match = match->next) {
        CompilerSymbol *symbol = match->symbol;
        BYTE kind = symbol->kind;
        if (kind == 24) kind = ((CompilerAliasSymbol *)symbol)->target->symbol->kind;
        if (kind == 16 || kind == 7 || kind == 6 || kind == 17 || kind == 8 || kind == 25) {
            if (saw_other) goto ambiguous;
            saw_functions = 1;
        } else {
            if (saw_functions || (other && other != symbol)) goto ambiguous;
            other = symbol;
            saw_other = 1;
        }
    }
    if (other) return first;
    return MergeFunctionMatches();
ambiguous:
    if (!diagnose) return NULL;
    compiler_warning(0x368, ((CompilerNamedSymbol *)first->symbol)->link.name);
    return head;
}
CompilerScopeMatch *RECON_THISCALL CompilerScopeLookup::MergeFunctionMatches(void) { __asm { int 3 } return NULL; }
/* Existing extended scope owner. The current root has no next scope and
 * never reads this source-only tail; no32-byte root allocation is enlarged. */
struct DeclarationOwnerScope : CompilerScope { CompilerSymbol *declaration_owner; /* original+20 */ };
extern "C" CompilerPointerArray *__cdecl initialize_type_aliases(void);
extern "C" CompilerPointerArray *__cdecl apply_type_aliases(CompilerPointerArray *, CompilerSymbol *);
extern "C" int __cdecl matches_scope_lookup_mode(CompilerSymbolLink *, unsigned);
extern "C" CompilerSymbolLink *__cdecl resolve_scope_lookup_link(CompilerSymbolLink *);
CompilerScopeLookup *RECON_THISCALL CompilerScope::Lookup(CompilerNameEntry *name,
    unsigned mode, unsigned *search_state)
{
    CompilerTranslationUnit *unit;
    CompilerSymbolLink *link = NULL;
    CompilerSymbol *found = NULL, *alias = NULL, *candidate, *target;
    CompilerScopeLookup *result;
    CompilerScopeMatch *match;
    CompilerArena *arena = &frontend_bss.arenas[5];
    if ((mode & 0x800) && !level) {
        unit = frontend_data.translation_unit;
        if (next) {
            if (!unit->type_aliases) unit->type_aliases = initialize_type_aliases();
            unit->type_aliases = apply_type_aliases(unit->type_aliases,
                ((DeclarationOwnerScope *)this)->declaration_owner);
        } else unit->ApplyNamespaceAliases(unit->global_namespace);
    }
    if (buckets) {
        link = buckets[(name->flags & bucket_mask) & 0xffff];
        *search_state = 1;
    }
    while (link) {
        if (link->name == name) {
            candidate = (CompilerSymbol *)((BYTE *)link - 8);
            if (candidate->kind == 24 && !frontend_data.translation_unit->lookup_override) {
                target = ((CompilerAliasSymbol *)candidate)->target->symbol;
                unsigned kind = target->kind;
                if (kind == 24)
                    kind = ((CompilerAliasSymbol *)target)->target->symbol->kind;
                if (kind == 16 || kind == 7 || kind == 6 || kind == 17 || kind == 8 || kind == 25)
                    alias = candidate;
                while (candidate->kind == 24)
                    candidate = ((CompilerAliasSymbol *)candidate)->target->symbol;
                link = &((CompilerNamedSymbol *)candidate)->link;
            }
            if (matches_scope_lookup_mode(link, mode)) {
                if (((CompilerSymbol *)((BYTE *)link - 8))->definition_flags & 0x20)
                    link = resolve_scope_lookup_link(link);
                found = (CompilerSymbol *)((BYTE *)link - 8);
                break;
            }
        }
        link = link->next;
    }
    if (secondary && !(mode & 0x200)) {
        result = (CompilerScopeLookup *)arena->cursor;
        arena->cursor += 8;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        if (result) {
            if (found) {
                match = (CompilerScopeMatch *)arena->cursor;
                arena->cursor += 12;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
                if (match) {
                    match->symbol = alias ? alias : found;
                    match->scope = this;
                    match->next = NULL;
                }
                result->head = result->tail = match;
            } else result->head = result->tail = NULL;
        }
        secondary->AppendLookup(name, mode | 0x200, search_state, result);
        return result->head ? result : NULL;
    }
    if (!found) return NULL;
    result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    match = (CompilerScopeMatch *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (match) {
        match->symbol = alias ? alias : found;
        match->scope = this;
        match->next = NULL;
    }
    result->head = result->tail = match;
    return result;
}
extern "C" CompilerPointerArray *__cdecl initialize_type_aliases(void) { __asm { int 3 } return NULL; }
extern "C" CompilerPointerArray *__cdecl apply_type_aliases(CompilerPointerArray *, CompilerSymbol *) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerTranslationUnit::ApplyNamespaceAliases(CompilerSymbol *) { __asm { int 3 } }
CompilerScopeLookup *RECON_THISCALL CompilerAuxiliaryScope::Lookup(CompilerNameEntry *name,
    unsigned mode, unsigned *search_state)
{
    CompilerSymbolLink *link = parameters->head;
    *search_state = 1;
    while (link) {
        if (link->name == name && matches_scope_lookup_mode(link, mode)) break;
        link = link->next;
    }
    if (!link) return NULL;
    CompilerArena *arena = &frontend_bss.arenas[5];
    CompilerScopeLookup *lookup = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!lookup) return NULL;
    CompilerScopeMatch *match = (CompilerScopeMatch *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (match) {
        match->symbol = (CompilerSymbol *)((BYTE *)link - 8);
        match->scope = this;
        match->next = NULL;
    }
    lookup->head = lookup->tail = match;
    return lookup;
}

extern "C" int __cdecl matches_scope_lookup_mode(CompilerSymbolLink *link, unsigned mode)
{
    CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)link - 8);
    unsigned kind = symbol->kind;
    if (kind == 4) return (mode >> 5) & 1;
    if (kind == 15) return mode & 1;
    if (kind == 23) return (mode >> 8) & 1;
    if (kind == 3 || kind == 2 || kind == 17 || kind == 8 || kind == 5 || kind == 24)
        return (mode >> 1) & 1;
    if (kind == 10 && (((CompilerIrSymbol *)symbol)->storage_class & 7) == 7)
        return (mode >> 1) & 1;
    return (mode >> 4) & 1;
}
extern "C" CompilerSymbolLink *__cdecl resolve_scope_lookup_link(CompilerSymbolLink *) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerSecondaryScopes::AppendLookup(CompilerNameEntry *, unsigned, unsigned *, CompilerScopeLookup *) { __asm { int 3 } }
CompilerSymbol *RECON_THISCALL CompilerTranslationUnit::CurrentDeclarationOwner(void)
{
    CompilerScopeTable *scope = current;
    if (scope->level == 0) {
        if (scope->next) {
#ifdef RECON_NATIVE_TEST
            recon_unsupported("declaration owner: unmodeled extended scope tail");
#endif
            return ((DeclarationOwnerScope *)scope)->declaration_owner;
        }
    } else if (scope->level == 0xfa) {
        scope = scope->next;
        if (scope->level == 0 && scope->next) {
#ifdef RECON_NATIVE_TEST
            recon_unsupported("declaration owner: unmodeled enclosing scope tail");
#endif
            return ((DeclarationOwnerScope *)scope)->declaration_owner;
        }
    }
    return NULL;
}
#ifndef RECON_NATIVE_TEST
static int declaration_probe_disabled; /* ce134; source-only until special scope is reached */
#else
#define declaration_probe_disabled (*(int *)(recon_unsupported("declaration probe: unmodeled special-scope switch"),0))
#endif
CompilerScopeMatch *RECON_THISCALL CompilerTranslationUnit::ProbeDeclaration(CompilerSymbol *symbol)
{
    CompilerScopeTable *scope = DeclarationScope(0);
    CompilerScopeLookup *matches = scope->Find(((CompilerNamedSymbol *)symbol)->link.name, 1);
    CompilerScopeMatch *match;
    CompilerSymbol *merged = NULL;
    PublicationLookupRecord *result = NULL;
    if (!matches) merged = merge_scoped_declaration(NULL, symbol, 0);
    else {
        match = matches->Select(1);
        if (match) merged = merge_scoped_declaration(match->symbol, symbol, 0);
    }
    if (merged) {
        result = (PublicationLookupRecord *)frontend_bss.arenas[5].Allocate(12);
        if (result) { result->symbol = merged; result->scope = NULL; result->next = NULL; }
    }
    if (!result && scope->level == 0xfa) {
        if (!declaration_probe_disabled && !(frontend_data.compiler_mode_flags & 0x10000000)) {
            scope = scope->next;
            if (!scope) return NULL;
            matches = scope->Find(((CompilerNamedSymbol *)symbol)->link.name, 1);
            merged = NULL;
            if (!matches) merged = merge_scoped_declaration(NULL, symbol, 0);
            else {
                match = matches->Select(1);
                if (match) merged = merge_scoped_declaration(match->symbol, symbol, 0);
            }
            if (merged) {
                CompilerArena *arena = &frontend_bss.arenas[5];
                result = (PublicationLookupRecord *)arena->cursor;
                arena->cursor += 12;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
                if (result) { result->symbol = merged; result->scope = NULL; result->next = NULL; }
            }
        }
    }
    return result;
}
CompilerScopeTable *RECON_THISCALL CompilerTranslationUnit::DeclarationScope(unsigned skip_special)
{
    CompilerScopeTable *scope = current;
    while (scope->level >= 0xfb || (scope->level == 0xfa && skip_special &&
        !declaration_probe_disabled && !(frontend_data.compiler_mode_flags & 0x10000000)))
        scope = scope->next;
    return scope;
}
CompilerScopeMatch *RECON_THISCALL CompilerTranslationUnit::FindScopedDeclaration(CompilerSymbol *) { __asm { int 3 } return NULL; }
CompilerScopeMatch *RECON_THISCALL CompilerTranslationUnit::LookupName(CompilerNameEntry *name, unsigned mode)
{
    CompilerScopeLookup *matches;
    CompilerSymbol *symbol;
    CompilerScopeMatch *result;
    unsigned search_state = 1;
    BYTE level;
    if (!name) return NULL;
    if (!opaque_lookup_state || (frontend_data.compiler_mode_flags & 8)) {
        level = depth;
        if (level >= 128) { __asm { int 3 } }
        do {
            matches = scopes[level]->Lookup(name, mode, &search_state);
            if (matches) return matches->Select(1);
        } while (level-- != 0);
        return NULL;
    }
    if (!retired_scopes) {
        if (suppress_lookup_errors) return NULL;
        compiler_warning(lookup_context == 0x53 ? 0xe3 : 0xe4, name);
        suppress_lookup_errors = 1;
        return NULL;
    }
    symbol = LookupScopeName(retired_scopes, name,
        mode == 0x32 ? 0x32 : mode | 0x40,
        suppress_lookup_errors ? 0 : 0x181);
    if (!symbol) {
        if (!suppress_lookup_errors)
            compiler_warning(0x27, 0x49, name, retired_scopes);
        return NULL;
    }
    CompilerArena *arena = &frontend_bss.arenas[5]; /* actual c65a0 */
    result = (CompilerScopeMatch *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (result) {
        result->symbol = find_bound_declaration(symbol, NULL);
        result->scope = NULL;
        result->next = NULL;
    }
    return result;
}
extern "C" CompilerSymbolBinding *__cdecl lookup_class_binding(CompilerSymbol *, CompilerNameEntry *, unsigned);
extern "C" CompilerSymbolBinding *__cdecl make_symbol_binding(CompilerSymbolBinding *, CompilerSymbolBindingOwner *, CompilerSymbol *);
extern "C" const char *__cdecl format_encoded_symbol_name(void *, int);
CompilerSymbol *RECON_THISCALL CompilerTranslationUnit::LookupScopeName(CompilerSymbol *input,
    CompilerNameEntry *name, unsigned mode, unsigned diagnostic)
{
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    CompilerSymbolBinding *binding;
    CompilerScopeMatch *match;
    CompilerSymbol *symbol, *owner;
    CompilerTranslationUnit *unit;
    CompilerSymbol *saved_owner;
    DWORD saved_active;
    int saved_errors;
    BYTE saved_context;
    unsigned filtered_mode = mode & ~0x40U;
    unsigned arena_kind;
    int diagnosed = 0;
    CompilerSymbolBindingOwner *alternative, *deepest;
    CompilerSymbolLink *link;
    CompilerBoundSymbol *result;
    CompilerArena *arena;
    if ((detail->completion_flags & 1) && !(detail->template_flags & 1) &&
        !(input->definition_flags & 4) && !(detail->completion_flags & 4)) {
        int index = frontend_data.declaration_context_index;
        if (index < 0 ||
            ((unsigned)index < 16 && frontend_bss.class_scope_rows[index].symbol->name !=
             ((CompilerNamedSymbol *)input)->link.name)) {
            if (!(input->scope_flags & 4) && !(frontend_data.source_mode_flags & 0x200020)) {
                symbol = instantiate_declaration_class(input);
                if (((CompilerGlobalSymbolView *)symbol)->class_detail->completion_flags & 8)
                    merge_declaration_classes(symbol, input);
            }
        } else if ((unsigned)index >= 16) { __asm { int 3 } }
    }
    if (((CompilerNamedSymbol *)input)->link.name == name && (mode & 0x20)) {
        binding = make_symbol_binding(NULL, NULL, input);
        goto make_reference;
    } else {
        binding = lookup_class_binding(input, name, filtered_mode);
        if (!binding && (mode & 0x40)) {
            unsigned prior_recursion = frontend_bss.qualified_lookup_recursion++;
            match = prior_recursion ? NULL : FindHiddenName(name);
            frontend_bss.qualified_lookup_recursion = 0;
            if (!match) {
                unit = frontend_data.translation_unit;
                saved_owner = unit->retired_scopes;
                saved_context = unit->lookup_context;
                saved_active = unit->opaque_lookup_state;
                saved_errors = unit->suppress_lookup_errors;
                unit->suppress_lookup_errors = 0;
                unit->retired_scopes = NULL;
                unit->opaque_lookup_state = 0;
                unit->lookup_context = 1;
                match = FindHiddenName(name);
                unit = frontend_data.translation_unit;
                if (saved_active) {
                    unit->retired_scopes = saved_owner;
                    unit->lookup_context = saved_context;
                    unit->opaque_lookup_state = 1;
                    unit->suppress_lookup_errors = saved_errors;
                } else {
                    unit->retired_scopes = NULL;
                    unit->lookup_context = 1;
                    unit->opaque_lookup_state = 0;
                    unit->suppress_lookup_errors = 0;
                }
            }
            if (match) {
                symbol = match->symbol;
                if (symbol->kind == 18)
                    symbol = ((CompilerBoundSymbol *)symbol)->binding->owner->symbol;
                if (symbol->kind >= 1 && symbol->kind <= 14 && symbol->kind != 5 &&
                    (((CompilerIrSymbol *)symbol)->storage_class & 7) == 7) {
                    CompilerType *type = ((CompilerIrSymbol *)symbol)->type;
                    if ((type->flags & 0x400) && !type->shape) {
                        int index = (short)type->auxiliary;
                        CompilerPointerArray *table = frontend_data.frontend_pointer_table;
                        if (!table || index < 0 || index >= table->count) { __asm { int 3 } }
                        CompilerNameEntry *type_name = ((CompilerNamedSymbol *)table->items[index])->link.name;
                        if (((CompilerNamedSymbol *)input)->link.name == type_name && (mode & 1))
                            binding = make_symbol_binding(NULL, NULL, input);
                        else binding = lookup_class_binding(input, type_name, filtered_mode);
                        if (binding) {
                            unit = frontend_data.translation_unit;
                            saved_owner = unit->retired_scopes;
                            saved_context = unit->lookup_context;
                            saved_active = unit->opaque_lookup_state;
                            saved_errors = unit->suppress_lookup_errors;
                            unit->suppress_lookup_errors = 0;
                            unit->retired_scopes = NULL;
                            unit->opaque_lookup_state = 0;
                            unit->lookup_context = 1;
                            LookupName(type_name, 0x132);
                            unit = frontend_data.translation_unit;
                            if (saved_active) {
                                unit->retired_scopes = saved_owner;
                                unit->lookup_context = saved_context;
                                unit->opaque_lookup_state = 1;
                                unit->suppress_lookup_errors = saved_errors;
                            } else {
                                unit->retired_scopes = NULL;
                                unit->lookup_context = 1;
                                unit->opaque_lookup_state = 0;
                                unit->suppress_lookup_errors = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!binding) return NULL;
    if (!((mode & 1) && ((CompilerNamedSymbol *)input)->link.name == name) &&
        binding->owner != binding->last && !(mode & 0x80)) {
        if (diagnostic) {
            diagnosed = 1;
            deepest = binding->owner;
            while (deepest->inner) deepest = deepest->inner;
            compiler_warning(diagnostic, deepest->symbol, name);
            for (alternative = binding->owner; alternative; alternative = alternative->next) {
                const char *text = format_encoded_symbol_name(alternative, 1);
                report_language_diagnostic(1, diagnostic,
                    alternative == binding->owner ? 0x17c : 0x17d, name, text);
            }
        }
        binding->last = binding->owner;
        binding->owner->next = NULL;
    }
make_reference:
    if (frontend_data.compiler_flags & 0x400)
        arena_kind = frontend_bss.symbol_arena_kind;
    else if (frontend_bss.declaration_scope_mode &&
        frontend_data.declaration_context_index < 0 &&
        !(frontend_data.compiler_mode_flags & 0x10000000) &&
        (((frontend_data.compiler_mode_flags & 0x40000000) &&
          !(frontend_data.source_mode_flags & 8) && !(frontend_data.compiler_flags & 0x200)) ||
         !(frontend_data.source_mode_flags & 0x50)))
        arena_kind = 4;
    else arena_kind = 0;
    result = NULL;
    if (!arena_kind) {
        unsigned size_class = frontend_data.symbol_size_class[18];
        if (size_class >= 11) { __asm { int 3 } }
        link = frontend_data.recycled_symbols[size_class];
        if (link) {
            frontend_data.recycled_symbols[size_class] = link->next;
            link->name = NULL; link->next = NULL;
            result = (CompilerBoundSymbol *)((BYTE *)link - 8);
            memset(result, 0, frontend_data.symbol_record_bytes[18]);
            result->link.name = name;
            result->kind = 18;
        }
    }
    if (!result) {
        if (arena_kind >= 9) { __asm { int 3 } }
        arena = &frontend_bss.arenas[arena_kind];
        result = (CompilerBoundSymbol *)arena->cursor;
        arena->cursor += (frontend_data.symbol_record_bytes[18] + 3) & ~3U;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        if (result) {
            result->kind = 18;
            result->link.name = name;
            result->link.next = NULL;
        }
    }
    /* The original unconditionally stores through the allocated result. */
    result->binding = binding;
    if (diagnosed) result->scope_flags |= 4;
    return result;
}
/* The five-DWORD search context holds two initially empty inherited-class
 * sets and the caller's collection of matching declarations. */
struct CompilerClassBindingSearch {
    CompilerNameEntry *name;
    unsigned mode;
    CompilerHashTable *seen_classes, *secondary_classes;
    CompilerVector *matches;
};
typedef char CompilerClassBindingSearch_layout[(sizeof(CompilerClassBindingSearch) == 20 &&
    offsetof(CompilerClassBindingSearch, seen_classes) == 8 &&
    offsetof(CompilerClassBindingSearch, secondary_classes) == 12 &&
    offsetof(CompilerClassBindingSearch, matches) == 16) ? 1 : -1];
extern "C" void __cdecl select_temporary_symbol_arena(unsigned);
extern "C" void __cdecl restore_temporary_symbol_arena(void);
extern "C" CompilerVector *__cdecl new_option_vector(unsigned, int);
extern "C" CompilerSymbolBinding *__cdecl collect_class_bindings(CompilerSymbolBinding *, CompilerClassBindingSearch *);
extern "C" void __cdecl finish_class_bindings(CompilerSymbolBinding *, CompilerVector *);
extern "C" void __cdecl record_class_name_reference(CompilerSymbol *, CompilerNameEntry *);
extern "C" CompilerSymbolBinding *__cdecl lookup_class_binding(CompilerSymbol *symbol,
    CompilerNameEntry *name, unsigned mode)
{
    if (name->flags & 0x40000) {
        CompilerClassBindingSearch search;
        CompilerSymbolBinding *binding;
        select_temporary_symbol_arena(
            (frontend_data.compiler_mode_flags & 0x40000000) ||
            (frontend_data.source_mode_flags & 0x10) ? 3 : 5);
        search.name = name; search.mode = mode;
        search.seen_classes = search.secondary_classes = NULL;
        search.matches = new_option_vector(1, 5);
        binding = collect_class_bindings(make_symbol_binding(NULL, NULL, symbol), &search);
        finish_class_bindings(binding, search.matches);
        restore_temporary_symbol_arena();
        return binding;
    }
    if (frontend_data.options.incremental_rebuild &&
        ((ClassDependencyRecordsView *)frontend_data.dependency->records)->record_members) {
        if ((symbol->definition_flags & 0x10) &&
            (((CompilerIrSymbol *)symbol)->type->flags & 0x200)) return NULL;
        if (!(((CompilerGlobalSymbolView *)symbol)->class_detail->completion_flags & 0x40))
            record_class_name_reference(symbol, name);
    }
    return NULL;
}
extern "C" void *__cdecl lookup_hash_value(CompilerHashTable *, const void *);
extern "C" int __cdecl accept_class_binding(CompilerSymbol *, CompilerVector *);
extern "C" CompilerHashTable *__cdecl create_hash_table(unsigned, unsigned, unsigned, unsigned, int, int);
extern "C" void __cdecl append_option_vector(CompilerVector *, void *);
/* 10e8d..1128b, 5a509..5a56e. Collect the class's own declaration,
 * then search its inherited class links without revisiting virtual bases. */
extern "C" CompilerSymbolBinding *__cdecl collect_class_bindings(
    CompilerSymbolBinding *binding, CompilerClassBindingSearch *search)
{
    CompilerSymbol *input = binding->owner->symbol;
    CompilerGlobalSymbolView *owner = (CompilerGlobalSymbolView *)input;
    CompilerSymbolBinding *result = NULL, *found;
    CompilerSymbolLink *entry = NULL;
    CompilerVector *additional;
    short index;
    if ((input->definition_flags & 0x10) && (((CompilerIrSymbol *)input)->type->flags & 0x200)) {
        CompilerSymbol *value = (CompilerSymbol *)lookup_hash_value(owner->lookup_table, search->name);
        return value ? make_symbol_binding(NULL, binding->owner, value) : NULL;
    }
    if (input->kind != 4 && input->kind != 23) {
        CompilerClassMemberSymbol *member = (CompilerClassMemberSymbol *)input;
        if (member->flags & 2) return NULL;
        int slot = member->owner_index;
        CompilerPointerArray *table = frontend_data.frontend_pointer_table;
        if (slot < 0 || slot >= table->count) { __asm { int 3 } }
        input = (CompilerSymbol *)table->items[slot];
        owner = (CompilerGlobalSymbolView *)input;
    }
    if (frontend_data.options.incremental_rebuild &&
        ((ClassDependencyRecordsView *)frontend_data.dependency->records)->record_members)
        frontend_data.dependency->LookupClassName(input, search->name);
    if (owner->class_detail->member_index) entry = owner->class_detail->member_names.Find(search->name);
    else {
        CompilerClassMemberIterator members;
        members.Initialize(&((CompilerNamedSymbol *)input)->link, 0);
        while (members.Next()) {
            if (members.entry->name == search->name) { entry = members.entry; break; }
        }
    }
    if (entry) {
        CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)entry - 8);
        if (matches_scope_lookup_mode(entry, search->mode) && accept_class_binding(symbol, search->matches)) {
            append_option_vector(search->matches, symbol);
            additional = owner->class_detail->additional_members;
            index = 0;
            while (additional && index < additional->count) {
                CompilerSymbol *related = ((InheritedMemberGroup *)additional->items[index++])->symbol;
                if (!search->seen_classes) {
                    search->seen_classes = create_hash_table(4, 5, 0, 0, 1, 1);
                    search->secondary_classes = create_hash_table(4, 5, 0, 0, 1, 1);
                }
                insert_hash_entry(search->seen_classes, related, (void *)1);
                insert_hash_entry(search->secondary_classes, related, (void *)1);
            }
            return make_symbol_binding(NULL, binding->owner, find_bound_declaration(symbol, NULL));
        }
    }
    if (search->name == frontend_data.builtin_names.assignment_operator) return NULL;
    if (!(input->definition_flags & 4) && (frontend_data.compiler_flags & 2)) return NULL;
    CompilerKind4Symbol *class_owner = (CompilerKind4Symbol *)input;
    CompilerClassMemberIterator bases;
    bases.InitializeHead(class_owner->class_binding ? class_owner->class_binding->members : NULL);
    bases.entry = NULL;
    bases.current = bases.valid ? bases.chain : NULL;
    bases.owner = &class_owner->link;
    bases.mode = 4;
    for (;;) {
        entry = bases.current;
        bases.entry = entry;
        bases.current = bases.current && bases.current->next ? bases.current->next : NULL;
        if (!entry) break;
        CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)entry - 8);
        if (symbol->kind == 3 && (entry->name == frontend_data.builtin_names.vbptr ||
            entry->name == frontend_data.builtin_names.vfptr)) continue;
        if (symbol->kind != 15) break;
        CompilerClassMemberSymbol *base = (CompilerClassMemberSymbol *)symbol;
        if ((frontend_data.source_mode_flags & 0x100000) && (base->flags & 2)) continue;
        if (base->flags & 1) {
            if (search->mode & 4) continue;
            if (!search->seen_classes) {
                search->seen_classes = create_hash_table(4, 5, 0, 0, 1, 1);
                search->secondary_classes = create_hash_table(4, 5, 0, 0, 1, 1);
            }
            int slot = base->owner_index;
            CompilerPointerArray *table = frontend_data.frontend_pointer_table;
            if (slot < 0 || slot >= table->count) { __asm { int 3 } }
            CompilerSymbol *target = (CompilerSymbol *)table->items[slot];
            if (lookup_hash_value(search->seen_classes, target) == (void *)1) continue;
            insert_hash_entry(search->seen_classes, target, (void *)1);
        }
        found = collect_class_bindings(make_symbol_binding(NULL, binding->owner, symbol), search);
        if (found) {
            if (!result) result = found;
            else {
                result->last->next = found->owner;
                result->last = found->last;
            }
            if (search->mode & 8) return result;
        }
    }
    return result;
}
extern "C" void *__cdecl lookup_hash_value(CompilerHashTable *, const void *) { __asm { int 3 } return NULL; }
/* 1898f..18a15, 5e23c..5e26e. Virtual-base declarations collapse to
 * one result; class-scope and deferred definitions reject repeated owners. */
extern "C" int __cdecl accept_class_binding(CompilerSymbol *symbol, CompilerVector *matches)
{
    short index = 0;
    if (symbol->kind == 15 && (((CompilerClassMemberSymbol *)symbol)->flags & 1)) {
        while (matches && index < matches->count) {
            CompilerSymbol *other = (CompilerSymbol *)matches->items[index++];
            if (other->kind == 15 && (((CompilerClassMemberSymbol *)other)->flags & 1)) return 0;
        }
    } else if (is_class_scope_symbol(symbol) || (symbol->definition_flags & 0x10)) {
        while (matches && index < matches->count)
            if (matches->items[index++] == symbol) return 0;
    }
    return 1;
}
int RECON_THISCALL CompilerDependency::LookupClassName(CompilerSymbol *, CompilerNameEntry *) { __asm { int 3 } return 0; }
extern "C" int __cdecl is_shadowed_class_binding(CompilerSymbolBindingOwner *, CompilerVector *);
/* 112e2..112fe, 5a583..5a5c7: unlink filtered alternatives and preserve
 * the final surviving owner as the list's tail. */
extern "C" void __cdecl finish_class_bindings(CompilerSymbolBinding *binding, CompilerVector *matches)
{
    if (!binding || binding->owner == binding->last || !matches) return;
    CompilerSymbolBindingOwner **cursor = &binding->owner;
    while (*cursor) {
        if (is_shadowed_class_binding(*cursor, matches))
            *cursor = (*cursor)->next;
        else {
            binding->last = *cursor;
            cursor = &(*cursor)->next;
        }
    }
}
extern "C" int __cdecl is_shadowed_class_binding(CompilerSymbolBindingOwner *, CompilerVector *) { __asm { int 3 } return 0; }
extern "C" void __cdecl record_class_name_reference(CompilerSymbol *, CompilerNameEntry *) { __asm { int 3 } }

CompilerScopeLookup *RECON_THISCALL CompilerClassScope::Lookup(CompilerNameEntry *name,
    unsigned mode, unsigned *search_state)
{
    CompilerTranslationUnit *unit = frontend_data.translation_unit;
    CompilerSymbol *found, *owner, *item;
    CompilerScopeLookup *result;
    CompilerScopeMatch *match;
    CompilerArena *arena;
    CompilerSymbolLink *link;
    int visible;
    if (mode & 0x400) return NULL;
    if (symbol->kind == 23)
        return ((CompilerNamespaceSymbol *)symbol)->data->scope->Lookup(name, mode, search_state);
    found = unit->LookupScopeName(symbol, name, mode,
        unit->suppress_lookup_errors ? 0 : 0x181);
    if (!found) {
        if (!reset_value) *search_state = 0;
        return auxiliary ? auxiliary->Lookup(name, mode, search_state) : NULL;
    }
    if (*search_state && reset_value) {
        owner = ((CompilerBoundSymbol *)found)->binding->owner->symbol;
        if (owner->kind != 4) found->type_flags |= 0x80;
    } else if (!reset_value && level != 0xfc) {
        owner = ((CompilerBoundSymbol *)found)->binding->owner->symbol;
        if ((((CompilerIrSymbol *)owner)->storage_class & 7) != 3 &&
            owner->kind != 5 && !(owner->definition_flags & 0x10) &&
            owner->kind != 15 && owner->kind != 24 && owner != symbol &&
            unit->current->level != 1) {
            if (owner->kind == 17 || owner->kind == 8) {
                visible = 0;
                if (owner->kind == 7 || owner->kind == 8 || owner->kind == 6 || owner->kind == 24)
                    link = &((CompilerNamedSymbol *)owner)->link;
                else link = ((CompilerMemberGroupSymbol *)owner)->variants;
                while (link) {
                    CompilerSymbolLink *current = link;
                    if (owner->kind == 7 || owner->kind == 8 || owner->kind == 6 || owner->kind == 24)
                        link = NULL;
                    else link = link->next;
                    item = (CompilerSymbol *)((BYTE *)current - 8);
                    if (item->kind == 24) item = ((CompilerAliasSymbol *)item)->target->symbol;
                    if (!(((CompilerRuntimeFunction *)item)->builtin_flags & 1)) visible = 1;
                }
            } else visible = (owner->definition_flags & 0x40) != 0;
            if (!visible) {
                if (!(frontend_data.compiler_flags & 0x10000000))
                    compiler_warning(0x147, found);
                return NULL;
            }
        }
    }
    arena = &frontend_bss.arenas[5];
    result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    match = (CompilerScopeMatch *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (match) {
        match->symbol = found;
        match->scope = this;
        match->next = NULL;
    }
    result->head = result->tail = match;
    return result;
}

extern "C" CompilerSymbol *__cdecl find_matching_symbol(CompilerSymbolLink *, CompilerNameEntry *);
CompilerScopeLookup *RECON_THISCALL CompilerScope::Find(CompilerNameEntry *name, unsigned context)
{
    CompilerSymbol *found = buckets ? find_matching_symbol(buckets[(bucket_mask & name->flags) & 0xffff], name) : NULL;
    CompilerArena *arena = &frontend_bss.arenas[5];
    CompilerScopeLookup *result;
    CompilerScopeMatch *match;
    if (secondary && (!found || !context)) {
        if (context && context != 2) return NULL;
        result = (CompilerScopeLookup *)arena->cursor;
        arena->cursor += 8;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        if (result) {
            if (found) {
                match = (CompilerScopeMatch *)arena->cursor;
                arena->cursor += 12;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
                if (match) { match->symbol = found; match->scope = this; match->next = NULL; }
                result->head = result->tail = match;
            } else result->head = result->tail = NULL;
        }
        secondary->AppendMatches(name, result);
        return result->head ? result : NULL;
    }
    if (!found) return NULL;
    result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    match = (CompilerScopeMatch *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (match) { match->symbol = found; match->scope = this; match->next = NULL; }
    result->head = result->tail = match;
    return result;
}
/* Alias kind24 borrows a record whose first field is its target symbol.
 * No wider target record, allocation or default is inferred here. */
typedef CompilerAliasTarget DeclarationAliasTarget;
typedef CompilerAliasSymbol DeclarationAliasSymbol;
extern "C" CompilerSymbol *__cdecl find_matching_symbol(CompilerSymbolLink *link, CompilerNameEntry *name)
{
    while (link && link->name != name) link = link->next;
    if (!link) return NULL;
    CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)link - offsetof(CompilerNamedSymbol, link));
    if (symbol->kind == 24 && !frontend_data.translation_unit->lookup_override) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("symbol lookup: unmodeled alias target owner");
#endif
        CompilerSymbol *target = ((DeclarationAliasSymbol *)symbol)->target->symbol;
        unsigned kind = target->kind;
        if (kind == 16 || kind == 7 || kind == 6 || kind == 17 || kind == 8 || kind == 25)
            return symbol;
        if (kind == 24) {
            kind = ((DeclarationAliasSymbol *)target)->target->symbol->kind;
            if (kind == 16 || kind == 7 || kind == 6 || kind == 17 || kind == 8 || kind == 25)
                return symbol;
        }
        do symbol = ((DeclarationAliasSymbol *)symbol)->target->symbol;
        while (symbol->kind == 24);
    }
    return symbol;
}
void RECON_THISCALL CompilerSecondaryScopes::AppendMatches(CompilerNameEntry *, CompilerScopeLookup *) { __asm { int 3 } }

void RECON_THISCALL CompilerScope::Insert(CompilerSymbol *input)
{
    CompilerSymbol *symbol = PrepareInsert(input);
    if (!symbol) return;
    int local_scope = level >= 1 && level < 0x80;
    symbol->scope_flags = (symbol->scope_flags & ~8) | (local_scope ? 8 : 0);
    int recording = !(symbol->scope_flags & 8) && frontend_bss.global_output_active && !frontend_data.option_c9il;
    symbol->scope_flags = (symbol->scope_flags & ~0x10) | (recording ? 0x10 : 0);
    if (recording && !frontend_bss.debug_module->Slot48())
        report_compiler_diagnostic(0x49, "p1inl.inl", 0x288);
    if (!buckets) {
        int had_buckets = HasBuckets();
        unsigned bytes = bucket_mask * 4 + 4;
        BYTE *memory;
        if (level && !(frontend_data.compiler_mode_flags & 0x50000000)) {
            CompilerArena *arena = &frontend_bss.arenas[4];
            memory = arena->cursor;
            arena->cursor += bytes;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        } else {
            CompilerPool *pool = &frontend_bss.pools[8]; /* actual c0b00 */
            if (pool->remaining < bytes) pool->Refill(bytes);
            memory = pool->cursor;
            pool->cursor += bytes;
            pool->remaining -= bytes;
        }
        buckets = (CompilerSymbolLink **)memory;
        if (!had_buckets) frontend_data.translation_unit->RegisterScope(this);
    }
    Link(symbol);
}
extern "C" void __cdecl bind_type_declaration(CompilerFunctionType *, CompilerSymbol *);
extern "C" CompilerSymbol *__cdecl prepare_function_symbol(CompilerSymbol *);
CompilerSymbol *RECON_THISCALL CompilerScopeTable::PrepareInsert(CompilerSymbol *symbol)
{
    if (symbol->kind != 23 && symbol->kind != 24) {
        CompilerType *type = ((CompilerIrSymbol *)symbol)->type;
        if (type && type->shape) {
            CompilerTypeShape *shape = type->shape;
            switch (shape->flags & 0x77) {
            case 3: case 0x20: case 0x44:
                bind_type_declaration(shape->base->payload.function, symbol);
                break;
            case 4:
                return prepare_function_symbol(symbol);
            }
        }
    }
    return symbol;
}
extern "C" void __cdecl bind_parameter_declarations(CompilerParameterLink *, unsigned,
    CompilerFunctionType *, unsigned, CompilerSymbol *);
extern "C" void __cdecl bind_type_declaration(CompilerFunctionType *function, CompilerSymbol *symbol)
{
    unsigned count = function->symbol_count;
    bind_parameter_declarations(function->symbols, count, function,
        function->parameters->count - count, symbol);
}
extern "C" void __cdecl bind_parameter_declarations(CompilerParameterLink *link, unsigned count,
    CompilerFunctionType *function, unsigned index, CompilerSymbol *symbol)
{
    /* The reached deferred-item payload is the existing expression-list
     * node. Append writes its first operand at+10; this consumer tests only
     * presence and follows the previous links established by the linker. */
    int missing = 0;
    while (count) {
        link = link->previous;
        ++index;
        if (!link->value) { compiler_warning(0x224, symbol, index); ++missing; }
        --count;
    }
    if (missing) { function->symbol_count = 0; function->symbols = NULL; }
}
extern "C" {
CompilerSymbol *__cdecl recycle_symbol(unsigned, CompilerNameEntry *, unsigned);
int __cdecl find_builtin_function(CompilerNameEntry *);
int __cdecl builtin_function_enabled(unsigned);
void __cdecl bind_builtin_function(CompilerSymbol *, unsigned);
}
extern "C" CompilerSymbol *__cdecl prepare_function_symbol(CompilerSymbol *input)
{
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)input;
    CompilerSymbol *result = input;
    if (input->kind != 7 && input->kind != 8 && input->kind != 6) return input;
    if (input->kind == 8 && (symbol->linkage_flags & 0x20)) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("function publication: unmodeled kind17 alias allocation");
#endif
        CompilerSmallSymbol *alias = (CompilerSmallSymbol *)recycle_symbol(17, symbol->link.name, 0);
        if (!alias) {
            BYTE *memory = frontend_bss.arenas[0].Allocate((frontend_data.symbol_record_bytes[17] + 3) & ~3U);
            alias = memory ? (CompilerSmallSymbol *)((CompilerSymbol *)memory)->Initialize(17, symbol->link.name, 0) : NULL;
        }
        alias->type_flags = (alias->type_flags & ~0x53) | (symbol->type_flags & 0x53);
        alias->target = &symbol->link;
        result = alias;
    } else if (symbol->type_flags & 2) {
        int index = find_builtin_function(symbol->link.name);
        if (index != -1 && builtin_function_enabled(index)) bind_builtin_function(input, index);
    }
    bind_type_declaration(symbol->type->shape->payload.function, input);
    if (!(symbol->encoded_index & 0xffffff)) {
        if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(55);
        unsigned index = frontend_data.next_symbol_record_id++;
        symbol->encoded_index = (symbol->encoded_index & 0xff000000) | (index & 0xffffff);
    }
    return result;
}
extern "C" int __cdecl find_builtin_function(CompilerNameEntry *identifier)
{
    /* The consumed prefix ranges cover '_' through 'z'; the final z+1
     * boundary establishes63 function names. These are semantic names and
     * stable indices, not a copy of the surrounding image. */
    static const char *const names[63] = {
        "_strset", "_setjmp", "_rotr", "_rotl", "_outpw", "_outpd", "_outp",
        "_lrotr", "_lrotl", "_inpw", "_inpd", "_inp", "_exception_info",
        "_exception_code", "_enable", "_disable", "_alloca", "_abnormal_termination",
        "__ull_rshift", "__ll_rshift", "__ll_lshift", "__emulu", "__emul",
        "_ReturnAddress", "_InterlockedIncrement", "_InterlockedExchangeAdd",
        "_InterlockedExchange", "_InterlockedDecrement", "_InterlockedCompareExchange",
        "_InterlockedAddLargeStatistic",
        "atan2", "atan", "asin", "acos", "abs", "cosh", "cos", "exp",
        "fmod", "fabs", "inpw", "inpd", "inp", "log10", "log", "labs",
        "memset", "memcpy", "memcmp", "outpw", "outpd", "outp", "pow",
        "strset", "strlen", "strcpy", "strcmp", "strcat", "sqrt", "sinh", "sin",
        "tanh", "tan"
    };
    struct NameRange { char initial; BYTE first, end; };
    static const NameRange ranges[] = {
        { '_', 0, 30 }, { 'a', 30, 35 }, { 'c', 35, 37 }, { 'e', 37, 38 },
        { 'f', 38, 40 }, { 'i', 40, 43 }, { 'l', 43, 46 }, { 'm', 46, 49 },
        { 'o', 49, 52 }, { 'p', 52, 53 }, { 's', 53, 61 }, { 't', 61, 63 }
    };
    const char *text = identifier->name;
    int initial = (signed char)*text;
    unsigned group, index;
    const char *left, *right;
    if (initial < '_') return -1;
    /* No identifier producer supplies '{'..DEL. Their neighboring original
     * prefix bytes have no recovered table meaning and must not be read. */
    if (initial > 'z') { __asm { int 3 } }
    for (group = 0; group < sizeof(ranges) / sizeof(ranges[0]); ++group) {
        if (ranges[group].initial != initial) continue;
        for (index = ranges[group].first; index < ranges[group].end; ++index) {
            left = names[index] + 1;
            right = text + 1;
            while (*left && *left == *right) { ++left; ++right; }
            if (*left == *right) return index;
        }
        break;
    }
    return -1;
}
extern "C" int __cdecl builtin_function_enabled(unsigned) { __asm { int 3 } return 0; }
extern "C" void __cdecl bind_builtin_function(CompilerSymbol *, unsigned) { __asm { int 3 } }
void RECON_THISCALL CompilerTranslationUnit::RegisterScope(CompilerScopeTable *scope)
{
    if (depth >= 128) { __asm { int 3 } }
    if (scopes[depth] == scope || !scope->HasBuckets()) return;
    ++depth;
    if (depth >= 128) report_compiler_error(0x3d);
    /* A returning failure reporter still cannot make slot128 a scope slot. */
    if (depth >= 128) { __asm { int 3 } }
    scopes[depth] = scope;
}

void RECON_THISCALL CompilerTranslationUnit::InsertCurrentScope(CompilerSymbol *symbol)
{
    current->Insert(symbol);
}

void RECON_THISCALL CompilerTranslationUnit::ActivateClassScope(CompilerScopeTable *scope)
{
    scope->next = current;
    current = scope;
    RegisterScope(scope);
}

void RECON_THISCALL CompilerSymbol::SetKind(BYTE value)
{
    kind = value;
}

CompilerClassMemberIterator *RECON_THISCALL CompilerClassMemberIterator::InitializeHead(CompilerSymbolLink *head)
{
    chain = head;
    valid = head != NULL;
    return this;
}
int RECON_THISCALL CompilerClassMemberIterator::Next(void)
{
    for (;;) {
        entry = current;
        if (!entry) return 0;
        if (chain) chain = chain->next;
        current = chain;
        CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)entry - 8);
        switch (mode) {
        case 0: return 1;
        case 1:
            if ((symbol->kind == 3 || symbol->kind == 2) && !(symbol->scope_flags & 1)) return 1;
            break;
        case 2:
            if (symbol->kind == 17 || symbol->kind == 8) return 1;
            break;
        case 3:
            if (symbol->kind == 4) return 1;
            break;
        case 5:
            if ((symbol->kind == 17 || symbol->kind == 8) && (symbol->type_flags & 0x40)) return 1;
            break;
        default: break;
        }
    }
}
RECON_THISCALL CompilerMemberVariantIterator::CompilerMemberVariantIterator(CompilerSymbolLink *link)
{
    chain = NULL;
    current = NULL;
    source = link;
    if (link) {
        CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)link - 8);
        if (symbol->kind == 7 || symbol->kind == 8 || symbol->kind == 6 || symbol->kind == 24)
            chain = link;
        else chain = ((CompilerMemberGroupSymbol *)symbol)->variants;
    }
}
void RECON_THISCALL CompilerMemberVariantIterator::SetLink(CompilerSymbolLink *link)
{
    current = NULL;
    source = link;
    CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)link - 8);
    if (symbol->kind == 7 || symbol->kind == 8 || symbol->kind == 6 || symbol->kind == 24)
        chain = link;
    else chain = ((CompilerMemberGroupSymbol *)symbol)->variants;
}
int RECON_THISCALL CompilerMemberVariantIterator::Next(void)
{
    if (!source || !chain) return 0;
    current = chain;
    BYTE kind = ((CompilerSymbol *)((BYTE *)source - 8))->kind;
    if (kind == 7 || kind == 8 || kind == 6 || kind == 24) chain = NULL;
    else chain = chain->next;
    return 1;
}
int RECON_THISCALL CompilerMemberVariantIterator::Advance(void)
{
    if (!source || !chain) return 0;
    while (((CompilerSymbol *)((BYTE *)chain - 8))->kind == 24) {
        chain = chain->next;
        if (!chain) return 0;
    }
    current = chain;
    BYTE kind = ((CompilerSymbol *)((BYTE *)source - 8))->kind;
    if (kind == 7 || kind == 8 || kind == 6) chain = NULL;
    else chain = chain->next;
    return 1;
}

extern "C" void __cdecl process_declaration_specifiers(CompilerDeclaration *declaration)
{
    CompilerType *type = declaration->type;
    int index = (short)type->auxiliary;
    if (index < 0) return;
    CompilerPointerArray *table = frontend_data.frontend_pointer_table;
    if (index >= table->count) { __asm { int 3 } }
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)table->items[index];
    if (!symbol || (symbol->type->flags & 0x200) ||
        (frontend_data.compiler_flags & 0x200) || !(symbol->scope_flags & 1)) return;
    CompilerKind4Symbol *owner = (CompilerKind4Symbol *)symbol;
    CompilerSymbolLink *head = owner->class_binding ? owner->class_binding->members : NULL;
    CompilerClassMemberIterator members;
    members.InitializeHead(head);
    members.reset_value = 0;
    members.current = members.valid ? members.chain : NULL;
    members.owner = &((CompilerNamedSymbol *)symbol)->link;
    members.mode = 2;
    CompilerMemberVariantIterator variants(NULL);
    CompilerMemberVariantIterator *cursor = &variants;
    for (;;) {
        while (cursor->Next()) {
            CompilerSymbol *member = (CompilerSymbol *)((BYTE *)variants.current - 8);
            if (member->kind == 24) member = ((DeclarationAliasSymbol *)member)->target->symbol;
            if (!(member->scope_flags & 1)) { compiler_warning(0x273); return; }
        }
        if (!members.Next()) break;
        variants.SetLink(members.entry);
    }
    static const unsigned special_member_categories[4] = {0x172, 0x173, 0x174, 0x175};
    for (unsigned kind = 1; kind <= 4; ++kind) {
        if (symbol->class_detail->inherited_member_flags & (1U << kind)) {
            compiler_warning(0x27f, special_member_categories[kind - 1]);
            return;
        }
    }
}

int RECON_THISCALL CompilerMemberVariantIterator::IsPrimary(void)
{
    CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)current - 8);
    if (symbol->kind == 24) {
        CompilerAliasTarget *target = ((CompilerAliasSymbol *)symbol)->target;
        if (target->selected) return 0;
        symbol = target->symbol;
    }
    return (((CompilerRuntimeFunction *)symbol)->linkage_flags & 0x40) == 0;
}
CompilerSymbol *RECON_THISCALL CompilerMemberVariantIterator::GetSymbol(void)
{
    CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)current - 8);
    return symbol->kind == 24 ? ((CompilerAliasSymbol *)symbol)->target->symbol : symbol;
}

int RECON_THISCALL CompilerAuxiliaryScope::HasBuckets(void)
{
    return value != NULL;
}

extern "C" unsigned __cdecl allocate_symbol_record_id(void)
{
    if (frontend_data.next_symbol_record_id >= 0x7fffffffU)
        report_compiler_error(0x37);
    return frontend_data.next_symbol_record_id++;
}

void RECON_THISCALL CompilerSymbol::SetName(CompilerNameEntry *name)
{
    ((CompilerNamedSymbol *)this)->link.name = name;
}

extern "C" void __cdecl set_symbol_declaration_context(
    CompilerSymbol *input, CompilerDeclarationContext context)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    CompilerDeclarationMetadata *metadata;
    CompilerArena *arena;
    /* These are the four actual flag BYTEs, not declaration tail storage. */
    memcpy(&symbol->declaration_state, &context.class_flags, 4);
    if ((context.reference_flags & 1) && context.reference)
        set_declaration_reference(input, context.reference);
    else symbol->reference_flags &= ~1U;
    if ((context.context_flags & 0x40) && context.metadata) {
        arena = &frontend_bss.arenas[0];
        metadata = (CompilerDeclarationMetadata *)arena->cursor;
        arena->cursor += 16;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        /* The original owns and copies exactly16 bytes here. Its contents
         * remain opaque; neither a field interpretation nor zero is inferred. */
        if (metadata) memcpy(metadata, context.metadata, 16);
        set_declaration_metadata(input, metadata);
    } else symbol->function_flags &= ~0x40U;
    if (context.attachment) set_declaration_attachment(input, context.attachment);
    else symbol->reference_flags &= ~0x80U;
}

void RECON_THISCALL CompilerAuxiliaryScope::Insert(CompilerSymbol *input)
{
    CompilerSymbol *symbol = PrepareInsert(input);
    if (!symbol) return;
    symbol->scope_flags |= 8;
    int first_insert = value == NULL;
    Link(symbol);
    if (first_insert) frontend_data.translation_unit->RegisterScope(this);
}

extern "C" CompilerSymbol *__cdecl make_parameter_sentinel(void);
void RECON_THISCALL CompilerAuxiliaryScope::Link(CompilerSymbol *symbol)
{
    CompilerParameterSet *set;
    CompilerSymbolLink *link;
    BYTE *memory;
    unsigned arena_index;
    if (!parameters) {
        arena_index = 0;
        if (frontend_bss.declaration_scope_mode &&
            frontend_data.declaration_context_index < 0 &&
            !(frontend_data.compiler_mode_flags & 0x10000000) &&
            !(frontend_data.source_mode_flags & 0x50) &&
            (!(frontend_data.compiler_mode_flags & 0x40000000) ||
             (frontend_data.source_mode_flags & 8) ||
             (frontend_data.compiler_flags & 0x200))) arena_index = 4;
        if (!arena_index && frontend_data.free_parameter_sets) {
            set = frontend_data.free_parameter_sets;
            frontend_data.free_parameter_sets = set->next_free;
            memset(set, 0, 12);
        } else {
            if (arena_index) {
                CompilerArena *arena = &frontend_bss.arenas[arena_index];
                memory = arena->cursor;
                arena->cursor += 12;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            } else {
                CompilerPool *pool = &frontend_bss.pools[8]; /* actual c0b00 */
                if (pool->remaining < 12) pool->Refill(12);
                memory = pool->cursor;
                pool->cursor += 12;
                pool->remaining -= 12;
            }
            set = (CompilerParameterSet *)memory;
        }
        if (set) {
            set->arena_index = arena_index;
            set->head = set->tail = NULL;
        }
        parameters = set;
    }
    set = parameters;
    link = &((CompilerNamedSymbol *)symbol)->link;
    if (link) {
        link->name = link->name;
        link->next = NULL;
        ((CompilerParameterSymbol *)symbol)->previous_parameter = set->tail;
    }
    if (!set->tail) set->head = set->tail = link;
    else {
        if (!frontend_data.parameter_sentinel)
            frontend_data.parameter_sentinel = make_parameter_sentinel();
        if ((CompilerSymbol *)((BYTE *)set->tail - offsetof(CompilerNamedSymbol, link)) ==
            frontend_data.parameter_sentinel) set->head = set->tail = link;
        else { set->tail->next = link; set->tail = link; }
    }
}

extern "C" BYTE *__cdecl allocate_parameter_description(unsigned)
{
    /* The genuine class allocator ignores its supplied sizeof argument. */
    CompilerArena *arena = &frontend_bss.arenas[5]; /* actual c65a0 */
    BYTE *memory = arena->cursor;
    arena->cursor += 28;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    return memory;
}
CompilerParameterSet *RECON_THISCALL CompilerAuxiliaryScope::ParameterTable(void)
{
    return parameters;
}
CompilerParameterDescription *RECON_THISCALL CompilerParameterDescription::Initialize(
    unsigned mode, CompilerParameterSet *set)
{
    kind = mode;
    scope_open = set != NULL;
    parameters = set;
    return this;
}

CompilerScopeTable *RECON_THISCALL CompilerTranslationUnit::CurrentScope(void)
{
    return current;
}

void RECON_THISCALL CompilerParameterDescription::SetQualifiers(CompilerDeclaratorQualifiers *input)
{
    /* Both original DWORD stores include the qualifier representation gap. */
    memcpy(&flags, input, 8);
}
void RECON_THISCALL CompilerParameterDescription::SetDeclarationMode(unsigned mode)
{
    declaration_mode = mode;
}

extern "C" CompilerExpression *__cdecl make_internal_label(unsigned code)
{
    CompilerArena *arena = &frontend_bss.arenas[4];
    BYTE *memory = arena->cursor;
    CompilerLabelSymbol *symbol;
    CompilerExpression *expression;
    arena->cursor += (frontend_data.symbol_record_bytes[9] + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    symbol = memory ? (CompilerLabelSymbol *)((CompilerSymbol *)memory)->Initialize(9, NULL, 4) : NULL;
    symbol->code = code;
    symbol->type = frontend_data.builtin_types.type_0063;
    emit_label_reference(symbol);
    expression = (CompilerExpression *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    expression->flags = 4;
    expression->kind = 6;
    expression->value.symbol = symbol;
    expression->type = symbol->type;
    return expression;
}

extern "C" void __cdecl emit_label_reference(CompilerSymbol *input)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    symbol->scope_flags |= 8;
    if (!(symbol->encoded_index & 0xffffff)) {
        if (frontend_data.next_symbol_record_id >= 0x7fffffff)
            report_compiler_error(0x37);
        unsigned index = frontend_data.next_symbol_record_id++;
        symbol->encoded_index = (symbol->encoded_index & 0xff000000) | (index & 0xffffff);
    }
    symbol->link.next = frontend_data.pending_label_head ?
        &((CompilerNamedSymbol *)frontend_data.pending_label_head)->link : NULL;
    frontend_data.pending_label_head = symbol;
}

extern "C" CompilerExpression *__cdecl symbol_expression(CompilerSymbol *input, unsigned value)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    CompilerArena *arena;
    CompilerExpression *expression;
    BYTE kind;
    if (!input) return NULL;
    arena = &frontend_bss.arenas[frontend_data.constant_arena_index];
    expression = (CompilerExpression *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    expression->kind = 6;
    expression->flags = 4;
    expression->value.symbol = input;
    if (symbol->kind == 18)
        symbol = (CompilerIrSymbol *)((CompilerBoundSymbol *)symbol)->binding->owner->symbol;
    if (symbol->scope_flags & 4) expression->flags = 12;
    if (symbol->definition_flags & 0x10) {
        expression->flags = 0;
        expression->kind = 0x98;
        expression->type = symbol->type;
        return expression;
    }
    kind = symbol->kind;
    if (kind == 24 || kind == 16 || kind == 17 || kind == 25)
        return expression;
    if ((kind == 7 || kind == 8) && !value) return expression;
    if (kind == 6 || kind == 3 || kind == 15 || kind == 2) return expression;
    if (kind == 5) { expression->type = symbol->type; return expression; }
    if ((symbol->qualifiers & 4) && (kind == 11 || kind == 12 || kind == 22)) {
        expression->type = symbol->type;
        expression->type = reference_expression_type(expression);
        return expression;
    }
    expression->type = symbol->type;
    if ((symbol->reference_flags & 1) && kind != 7 && kind != 8) {
        CompilerSymbol *resolved = resolve_symbol_reference(symbol);
        if (resolved->kind != 14) {
            /* The type is the fourth outer call argument, retained while
             * the two-argument symbol-expression constructor returns. */
            CompilerType *type = expression->type;
            resolved = resolve_symbol_reference(symbol);
            CompilerExpression *right = make_resolved_symbol_expression(resolved, 0);
            return make_binary_node(0x5d, expression, right, type);
        }
    }
    return expression;
}

extern "C" CompilerExpression *__cdecl make_binary_node(unsigned code,
    CompilerExpression *first, CompilerExpression *second, CompilerType *type)
{
    CompilerArena *arena = &frontend_bss.arenas[frontend_data.constant_arena_index];
    CompilerExpression *expression = (CompilerExpression *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    expression->kind = (BYTE)code;
    expression->flags = 2;
    expression->type = type;
    expression->value.operands.first = first;
    expression->value.operands.second = second;
    return expression;
}

extern "C" {
void __cdecl push_expression_work(const void *, unsigned); /*6323*/
void __cdecl begin_parameter_output(void); /*4ede*/
void __cdecl emit_parameter_output(CompilerSymbol *, unsigned); /*4f0a*/
void __cdecl finish_parameter_output(void); /*500f*/
void __cdecl reset_expression_work(void); /*654f*/
}
extern "C" void __cdecl emit_function_parameters(CompilerSymbol *symbol, CompilerParameterSet *parameters)
{
    CompilerExpressionWorkInput frame;
    CompilerSymbolLink *link;
    CompilerExpressionWork *current, *restored;
    CompilerExpressionWork *saved;
    memset(&frame, 0, sizeof(frame));
    frame.kind = 1;
    frame.symbol = symbol;
    frontend_data.saved_expression_work_nodes = frontend_data.expression_work_nodes;
    if (frontend_data.expression_work_reset_pending ||
        frontend_data.expression_work_error_count != frontend_bss.error_count)
        reset_expression_work();
    frontend_data.expression_work_reset_pending = 0;
    push_expression_work(&frame, 24);
    frontend_data.expression_work_error_count = frontend_bss.error_count;
    begin_parameter_output();
    if (symbol->scope_flags & 4) goto finish;
    if (parameters) {
        link = parameters->tail;
        while (link) {
            emit_parameter_output((CompilerSymbol *)((BYTE *)link - 8), 0);
            link = ((CompilerParameterSymbol *)((BYTE *)link - 8))->previous_parameter;
        }
    }
finish:
    finish_parameter_output();
    saved = frontend_data.saved_expression_work_nodes;
    if (saved) {
        current = frontend_data.expression_work_current;
        restored = saved->next;
        current->previous = frontend_data.expression_work_nodes;
        frontend_data.expression_work_nodes->next = current;
        restored->previous = NULL;
        frontend_data.saved_expression_work_nodes->next = NULL;
        frontend_data.expression_work_current = restored;
        frontend_data.expression_work_nodes = frontend_data.saved_expression_work_nodes;
        frontend_data.saved_expression_work_nodes = current->previous;
    }
}
extern "C" void __cdecl begin_parameter_output(void)
{
    CompilerExpressionWork frame;
    memset(&frame, 0, sizeof(frame));
    frame.kind = 2;
    push_expression_work(&frame, 16);
}
extern "C" void __cdecl pop_expression_work(void); /*643f*/
extern "C" void __cdecl finish_parameter_output(void)
{
    CompilerExpressionWork frame;
    if (frontend_data.expression_work_nodes && frontend_data.expression_work_nodes->kind == 2) {
        pop_expression_work();
        return;
    }
    memset(&frame, 0, sizeof(frame));
    frame.kind = 3;
    push_expression_work(&frame, 16);
}
extern "C" void __cdecl pop_expression_work(void)
{
    CompilerExpressionWork *record = frontend_data.expression_work_nodes;
    CompilerExpressionWork *previous = record->previous;
    record->previous = frontend_data.expression_work_head;
    record->next = NULL;
    if (frontend_data.expression_work_head) frontend_data.expression_work_head->next = record;
    frontend_data.expression_work_head = record;
    if (previous) {
        frontend_data.expression_work_nodes = previous;
        previous->next = NULL;
    } else {
        frontend_data.expression_work_nodes = NULL;
        frontend_data.expression_work_current = NULL;
    }
    --frontend_data.expression_work_depth;
}

extern "C" {
#ifndef __linux__
int __cdecl _setjmp3(jmp_buf, int, ...);
#endif
CompilerExpressionWork *__cdecl reuse_expression_work(unsigned); /*64b0*/
CompilerExpressionWork *__cdecl find_expression_work(CompilerExpressionWork *, unsigned, unsigned); /*173a6*/
}
extern "C" void __cdecl push_expression_work(const void *input, unsigned bytes)
{
    CompilerExpressionWork *record = NULL;
    CompilerArena *arena;
    unsigned size_class;
    jmp_buf recovery;
    if (frontend_data.expression_work_reset_pending) return;
    if (frontend_data.expression_work_depth == 0xffffffff) {
        if (frontend_bss.expression_work_recovery_mode && !frontend_data.expression_work_recovery) {
            frontend_data.expression_work_recovery = recovery;
            if (_setjmp3(recovery, 0)) {
                frontend_data.expression_work_recovery = NULL;
                return;
            }
        }
        CompilerExpressionWorkInput *scope = (CompilerExpressionWorkInput *)find_expression_work(frontend_data.expression_work_nodes, 1, 1);
        report_compiler_diagnostic(0x26, scope->symbol);
        if (frontend_data.expression_work_recovery == recovery)
            frontend_data.expression_work_recovery = NULL;
    }
    /* The typed domain supplies the complete16-byte header and fits the
     * recovered four allocation classes. Other spans remain unsupported. */
    if (bytes < 16 || bytes > 64) { __asm { int 3 } }
    if (bytes <= 16) size_class = 0;
    else if (bytes <= 32) size_class = 1;
    else size_class = bytes > 48 ? 3 : 2;
    if (frontend_data.expression_work_head) record = reuse_expression_work(size_class);
    if (!record) {
        frontend_data.expression_work_units += size_class + 1;
        arena = &frontend_bss.arenas[4];
        record = (CompilerExpressionWork *)arena->cursor;
        arena->cursor += (size_class << 4) + 16;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    }
    memcpy(record, input, bytes);
    if (frontend_data.expression_work_nodes) frontend_data.expression_work_nodes->next = record;
    else frontend_data.expression_work_current = record;
    record->previous = frontend_data.expression_work_nodes;
    frontend_data.expression_work_nodes = record;
    record->flags = (record->flags & 0x3f) | (size_class << 6);
    frontend_data.expression_work_nodes->depth = (WORD)frontend_data.expression_work_depth;
    frontend_data.expression_work_nodes->next = NULL;
    ++frontend_data.expression_work_depth;
}
extern "C" CompilerExpressionWork *__cdecl reuse_expression_work(unsigned size_class)
{
    BYTE requested = (BYTE)size_class;
    CompilerExpressionWork *record = frontend_data.expression_work_head;
    CompilerExpressionWork *tail;
    unsigned available;
    while (record && (record->flags >> 6) < requested) record = record->previous;
    if (!record) return NULL;
    if (record->previous) record->previous->next = record->next;
    if (record->next) record->next->previous = record->previous;
    else frontend_data.expression_work_head = record->previous;
    available = record->flags >> 6;
    if (available > requested) {
        tail = (CompilerExpressionWork *)((BYTE *)record + ((unsigned)requested + 1) * 16);
        tail->flags = (tail->flags & 0x3f) | ((available - requested - 1) << 6);
        tail->next = NULL;
        tail->previous = frontend_data.expression_work_head;
        if (frontend_data.expression_work_head) frontend_data.expression_work_head->next = tail;
        frontend_data.expression_work_head = tail;
    }
    return record;
}


extern "C" {
CompilerExpression *__cdecl prepare_parameter_expression(CompilerExpression *, unsigned); /*1a528*/
CompilerExpression *__cdecl make_parameter_operation(CompilerSymbol *, CompilerExpression *, unsigned, unsigned, unsigned); /*21198*/
CompilerExpression *__cdecl make_parameter_statement(unsigned, CompilerExpression *, CompilerSymbol *); /*212d2*/
}
extern "C" void __cdecl emit_parameter_output(CompilerSymbol *input, unsigned flags)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    CompilerGlobalSymbolView *owner;
    CompilerParameterWorkInput frame;
    CompilerExpression *expression;
    if (!(get_type_classification(symbol->type) & 0x80)) return;
    owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)symbol->type->auxiliary];
    if (!(owner->type_flags & 0x20) || (owner->class_detail->generated_member_flags & 0x10)) return;
    memset(&frame, 0, sizeof(frame));
    frame.kind = 5;
    frame.flags |= 2;
    frame.symbol = symbol;
    frontend_data.parameter_work_kind = 5;
    expression = make_resolved_symbol_expression(symbol, 0);
    expression = prepare_parameter_expression(expression, 4);
    expression = make_parameter_operation((CompilerSymbol *)owner, expression, 3, 1, 0);
    expression = make_parameter_statement(0, expression, (CompilerSymbol *)owner);
    emit_statement_expression(expression);
    frame.source_state = frontend_bss.initializer_context.source_state;
    frame.parameter_flags = (frame.parameter_flags & ~1U) | (flags & 1);
    frame.state_index = frontend_data.parameter_work_index;
    frame.active = 1;
    frontend_data.parameter_work_kind = 1;
    /* The actual producer2132d allocates16*capacity bytes and grows by128
     * rows at equality. It remains a pending child; no row payload defaults
     * are inferred from the pointer/capacity cells. */
    if (!frontend_data.parameter_work_states ||
        frontend_data.parameter_work_index >= frontend_data.parameter_work_capacity) { __asm { int 3 } }
    frontend_data.parameter_work_states[frontend_data.parameter_work_index].active = 1;
    push_expression_work(&frame, 32);
}

extern "C" CompilerExpression *__cdecl make_parameter_operation(CompilerSymbol *, CompilerExpression *, unsigned, unsigned, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_parameter_statement(unsigned, CompilerExpression *, CompilerSymbol *) { __asm { int 3 } return NULL; }

extern "C" {
void __cdecl begin_nested_block(void); /*51db*/
int __cdecl emit_member_initializers(CompilerSymbol *, CompilerExpression *); /*2277c*/
void __cdecl emit_destructor_body(CompilerSymbol *); /*2306d*/
void __cdecl emit_deleting_destructor_body(CompilerSymbol *); /*3eb4e*/
void __cdecl emit_vector_deleting_destructor_body(CompilerSymbol *); /*b4174*/
void __cdecl emit_default_constructor_closure(CompilerSymbol *); /*b3f18*/
void __cdecl emit_vftable_constructor_closure(CompilerSymbol *); /*b3f5e*/
void __cdecl emit_copy_constructor_closure(CompilerSymbol *); /*b4072*/
}
extern "C" void __cdecl begin_compound_statement(void)
{
    CompilerSymbol *function;
    frontend_data.backend->Slot0c();
    if (frontend_bss.declaration_scope_mode >= 0xf9) report_compiler_error(0x3d);
    ++frontend_bss.declaration_scope_mode;
    ++frontend_data.declaration_scope_number;
    frontend_bss.global_output_active = frontend_bss.declaration_scope_mode;
    if (frontend_data.debug_module_kind == 4) return;
    if (!(frontend_data.compiler_flags & 0x20)) frontend_data.translation_unit->BeginLocalScope();
    if (!frontend_bss.suppress_parser_codegen) write_ir_record(0x53, NULL);
    if (frontend_bss.declaration_scope_mode >= 2) begin_nested_block();
    if (frontend_bss.suppress_parser_codegen || frontend_bss.declaration_scope_mode != 2) return;
    function = frontend_bss.current_function;
    if (frontend_bss.expression_work_recovery_mode && !function) return;
    if (function->type_flags & 0x10) {
        emit_member_initializers(frontend_data.declaration_lookup_owner, frontend_bss.current_parser_expression);
        frontend_bss.current_parser_expression = NULL;
        return;
    }
    if (frontend_bss.current_parser_expression) { compiler_warning(0x226, function); return; }
    if (function->type_flags & 0x20) { emit_destructor_body(frontend_data.declaration_lookup_owner); return; }
    CompilerNameEntry *name = ((CompilerNamedSymbol *)function)->link.name;
    if (name == frontend_data.builtin_names.deleting_destructor)
        emit_deleting_destructor_body(frontend_data.declaration_lookup_owner);
    else if (name == frontend_data.builtin_names.vector_deleting_destructor)
        emit_vector_deleting_destructor_body(frontend_data.declaration_lookup_owner);
    else if (name == frontend_data.builtin_names.default_constructor_closure)
        emit_default_constructor_closure(frontend_data.declaration_lookup_owner);
    else if (name == frontend_data.builtin_names.local_vftable_constructor_closure)
        emit_vftable_constructor_closure(frontend_data.declaration_lookup_owner);
    else if (name == frontend_data.builtin_names.copy_constructor_closure)
        emit_copy_constructor_closure(frontend_data.declaration_lookup_owner);
}
void RECON_THISCALL CompilerTranslationUnit::BeginLocalScope(void)
{
    CompilerArena *arena = &frontend_bss.arenas[4];
    BYTE *memory = arena->cursor;
    CompilerScope *scope;
    arena->cursor += 32;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    scope = memory ? new(memory) CompilerScope(frontend_bss.declaration_scope_mode, current, 15) : NULL;
    ActivateClassScope(scope);
}
extern "C" void __cdecl begin_nested_block(void)
{
    CompilerBlockWorkInput frame;
    CompilerExpressionWork *record, *next, *previous;
    memset(&frame, 0, sizeof(frame));
    frame.kind = 15;
    if (frontend_data.expression_work_reset_pending) return;
    frame.flags &= ~4U;
    frame.state_index = frontend_data.parameter_work_index;
    push_expression_work(&frame, 20);
    if (frontend_data.saved_expression_work_nodes) {
        record = frontend_data.expression_work_nodes;
        next = frontend_data.saved_expression_work_nodes->next;
        previous = record->previous;
        record->next = next;
        next->previous = frontend_data.expression_work_nodes;
        record->previous = frontend_data.saved_expression_work_nodes;
        frontend_data.saved_expression_work_nodes->next = frontend_data.expression_work_nodes;
        previous->next = NULL;
        frontend_data.expression_work_nodes = previous;
        frontend_data.saved_expression_work_nodes = NULL;
    }
}
extern "C" {
unsigned __cdecl constructor_this_qualifiers(CompilerSymbol *); /*22add*/
void __cdecl begin_member_initializers(void); /*22af6*/
void __cdecl prepare_virtual_base_initializers(CompilerSymbol *); /*22554*/
void __cdecl finish_member_initializers(void); /*22c99*/
void __cdecl emit_constructor_cleanup(CompilerSymbol *, unsigned); /*22659*/
CompilerExpression *__cdecl take_member_initializer(CompilerExpression **, CompilerSymbol *); /*22b3f*/
int __cdecl query_initializer_mode(void); /*1b288*/
void __cdecl set_initializer_mode(unsigned); /*23333*/
void __cdecl emit_virtual_base_initializers(CompilerSymbol *, CompilerExpression **, unsigned); /*4782d*/
CompilerExpression *__cdecl make_base_initializer(CompilerSymbol *, unsigned, int); /*22db4*/
int __cdecl initialize_member_expression(int, CompilerType *, CompilerExpression *, CompilerSymbol *); /*3235b*/
void __cdecl emit_copy_member_initializer(CompilerSymbol *, CompilerSymbol *, CompilerExpression *, int, unsigned, unsigned); /*89a9b*/
void __cdecl emit_special_member_initializer(CompilerSymbol *, int, CompilerExpression *, unsigned); /*899d6*/
CompilerExpression *__cdecl make_return_object_call(unsigned, CompilerSymbol *, CompilerExpression *, CompilerExpression *, unsigned, unsigned, unsigned);
}

/* 2277c..22add, 65c83..65e08. Explicit initializers are removed from
 * their expression chain as the real class/member order is traversed. */
extern "C" int __cdecl emit_member_initializers(CompilerSymbol *input,
    CompilerExpression *initializers)
{
    CompilerGlobalSymbolView *owner = (CompilerGlobalSymbolView *)input;
    unsigned qualifiers = constructor_this_qualifiers(frontend_bss.current_function);
    CompilerSymbolLink *link;
    CompilerGlobalSymbolView *member, *scope;
    CompilerExpression *arguments, *target, *operation;
    CompilerType *type;
    CompilerTypeShape *shape;
    CompilerPointerArray *table;
    int offset, is_member, index, missing, mode;
    begin_member_initializers();
    prepare_virtual_base_initializers(input);
    if (owner->class_detail->context_flags & 0x10) {
        mode = query_initializer_mode();
        if (!mode) set_initializer_mode(1);
        emit_virtual_base_initializers(input, &initializers, qualifiers);
        if (!mode) set_initializer_mode(0);
    }
    CompilerClassBinding *binding = ((CompilerKind4Symbol *)input)->class_binding;
    for (link = binding ? binding->members : NULL; link; link = link->next) {
        member = (CompilerGlobalSymbolView *)((BYTE *)link - 8);
        if (member->kind == 17 || member->kind == 8 || (member->reference_flags & 0x80) ||
            member->name == frontend_data.builtin_names.alignment_member ||
            is_class_scope_symbol((CompilerSymbol *)member) || (member->definition_flags & 0x10) ||
            member->kind == 5 || member->kind == 24) continue;
        if (member->kind == 15) {
            CompilerClassMemberSymbol *base = (CompilerClassMemberSymbol *)member;
            if (base->flags & 1) continue;
            index = base->owner_index;
            table = frontend_data.frontend_pointer_table;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            scope = (CompilerGlobalSymbolView *)table->items[index];
            offset = base->base_offset;
            is_member = 0;
        } else {
            scope = enclosing_declaration_class((CompilerSymbol *)member);
            offset = member->member_offset;
            if (scope) is_member = 1;
        }
        arguments = take_member_initializer(&initializers, (CompilerSymbol *)member);
        if (arguments) {
            if (member->kind == 3 && member->type->shape &&
                (member->type->shape->flags & 0x77) == 2 &&
                (!scope || (scope->type_flags & 0x10))) {
                CompilerRuntimeFunction *function = (CompilerRuntimeFunction *)frontend_bss.current_function;
                if (!((function->builtin_flags & 0x20) && (function->scope_flags & 1)))
                    compiler_warning(0x218, input, member);
                else if (scope && (member->definition_flags & 0x80))
                    emit_copy_member_initializer((CompilerSymbol *)member, (CompilerSymbol *)scope,
                        arguments, offset, 1, qualifiers);
                else emit_special_member_initializer((CompilerSymbol *)member, offset, arguments, qualifiers);
                continue;
            }
            if (!scope) {
                arguments = check_statement_expression(arguments, 1);
                begin_symbol_initializer((CompilerSymbol *)member);
                frontend_data.assignment_target_active = 1;
                if (!initialize_member_expression(offset, member->type, arguments, (CompilerSymbol *)member))
                    compiler_warning(0x1b7, member);
                frontend_data.assignment_target_active = 0;
                frontend_data.expression_target = NULL;
                frontend_data.expression_target_active = 0;
            } else if (!(scope->type_flags & 0x10)) compiler_warning(0x21e, member);
            else {
                if (!is_member) set_initializer_mode(1);
                target = make_base_initializer((CompilerSymbol *)scope, qualifiers, offset);
                operation = make_return_object_call(0, (CompilerSymbol *)scope,
                    target, arguments, 0, is_member, 1);
                operation = check_statement_expression(operation, 0);
                if (operation) emit_statement_expression(operation);
                set_initializer_mode(0);
            }
            continue;
        }
        if (scope) {
            type = qualify_type((CompilerSymbol *)scope, qualifiers, 0);
            if (!is_complete_declaration_type(type)) {
                compiler_warning(0x200, scope);
                continue;
            }
            if (member->definition_flags & 0x80) {
                emit_copy_member_initializer((CompilerSymbol *)member, (CompilerSymbol *)scope,
                    NULL, offset, 0, qualifiers);
                continue;
            }
            if (!is_member) set_initializer_mode(1);
            target = make_base_initializer((CompilerSymbol *)scope, qualifiers, offset);
            operation = make_return_object_call(0, (CompilerSymbol *)scope,
                target, NULL, 0, is_member, 1);
            operation = check_statement_expression(operation, 0);
            if (operation && !(operation->kind == 0x3d && operation->value.operands.first == target))
                emit_statement_expression(operation);
            set_initializer_mode(0);
            continue;
        }
        type = member->type;
        shape = type->shape;
        missing = 0;
        if (shape) {
            if ((member->definition_flags & 0x80) && (type->attributes & 0x100)) {
                compiler_warning(0x1b7, member);
                continue;
            }
            if (((shape->attributes & 0x100) && !(shape->flags & 3)) || (shape->flags & 0x40))
                missing = 1;
        } else if (type->attributes & 0x100) missing = 1;
        if (missing) compiler_warning(0x2f6, member);
    }
    finish_member_initializers();
    for (operation = initializers; operation; operation = operation->value.operands.second) {
        CompilerNameEntry *name = ((CompilerNamedSymbol *)operation->value.operands.first->
            value.operands.first->value.symbol)->link.name;
        binding = ((CompilerKind4Symbol *)input)->class_binding;
        member = NULL;
        for (link = binding ? binding->members : NULL; link; link = link->next) {
            if (link->name == name) {
                member = (CompilerGlobalSymbolView *)((BYTE *)link - 8);
                break;
            }
        }
        if (!member) compiler_warning(0x266, input, name);
        else if (member->kind == 17 || member->kind == 8 || member->kind == 4)
            compiler_warning(0x1b4, name);
        else if (is_class_scope_symbol((CompilerSymbol *)member)) compiler_warning(0x1b6, name);
        else compiler_warning(0x1b5, name);
    }
    if (!(owner->class_detail->declaration_flags & 0x80)) {
        CompilerRuntimeFunction *function = (CompilerRuntimeFunction *)frontend_bss.current_function;
        mode = 1;
        if ((function->builtin_flags & 0x20) && (owner->class_detail->inherited_member_flags & 4)) mode = 0;
        else if ((function->builtin_flags & 0x10) && (owner->class_detail->inherited_member_flags & 2)) mode = 0;
        emit_constructor_cleanup(input, mode);
    }
    return 1;
}

/* 22add..22af6: the constructor's return pointer carries this qualifiers. */
extern "C" unsigned __cdecl constructor_this_qualifiers(CompilerSymbol *function)
{
    CompilerType *type = ((CompilerIrSymbol *)function)->type;
    return type->shape->payload.function->type->shape->attributes & 3;
}
/* 22af6..22b3f: save the prior work kind for this initializer level. */
extern "C" void __cdecl begin_member_initializers(void)
{
    unsigned depth = frontend_data.member_initializer_depth + 1;
    frontend_data.member_initializer_state = 0;
    frontend_data.member_initializer_depth = depth;
    prepend_work_link(&frontend_bss.initializer_context.head, 4);
    frontend_bss.initializer_context.head->value.kind = frontend_data.parameter_work_kind;
    frontend_data.parameter_work_kind = 2;
    frontend_bss.initializer_context.constructor_state_index = frontend_data.parameter_work_index;
}
extern "C" void __cdecl prepare_constructor_layout(CompilerSymbol *);
/* 22554..225ab, 65bc7..65be0. Queue classes whose member phase is
 * deferred, then require the active debug module's object-model support. */
extern "C" void __cdecl prepare_virtual_base_initializers(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *owner = (CompilerGlobalSymbolView *)input;
    if (owner->class_detail->member_phase_flags & 0xc0) {
        if (!frontend_data.completion_declarations)
            frontend_data.completion_declarations = new_pointer_array(1);
        frontend_data.completion_declarations =
            apply_type_aliases(frontend_data.completion_declarations, input);
    }
    prepare_constructor_layout(input);
    if (!frontend_bss.debug_module->Slot68())
        report_compiler_diagnostic(0x49, ".\\objmodel.c", 0xb87);
}
extern "C" void __cdecl ensure_special_member_declaration(CompilerSymbol *, CompilerNameEntry *);
/* 225ab..225cc, 65be0..65bf5: deleting destructors required by the class. */
extern "C" void __cdecl prepare_constructor_layout(CompilerSymbol *input)
{
    ensure_special_member_declaration(input, frontend_data.builtin_names.deleting_destructor);
    if (((CompilerGlobalSymbolView *)input)->member_flags & 4)
        ensure_special_member_declaration(input, frontend_data.builtin_names.vector_deleting_destructor);
}
extern "C" CompilerSymbol *__cdecl find_class_member_by_name(CompilerSymbol *, CompilerNameEntry *);
extern "C" void __cdecl emit_implicit_function(CompilerSymbol *);
/* 225cc..22659, 65bf5..65c4c. A single function and an overload group
 * share the named link; alias entries borrow the real function symbol. */
extern "C" void __cdecl ensure_special_member_declaration(CompilerSymbol *input,
    CompilerNameEntry *name)
{
    CompilerSymbol *found = find_class_member_by_name(input, name);
    CompilerSymbolLink *current;
    int single;
    if (!found) return;
    single = found->kind == 7 || found->kind == 8 ||
        found->kind == 6 || found->kind == 24;
    current = single ? &((CompilerNamedSymbol *)found)->link :
        ((CompilerSmallSymbol *)found)->variants;
    while (current) {
        CompilerSymbolLink *entry = current;
        current = single ? NULL : current->next;
        CompilerSymbol *raw = (CompilerSymbol *)((BYTE *)entry - 8);
        CompilerSymbol *symbol = raw->kind == 24 ?
            ((CompilerAliasSymbol *)raw)->target->symbol : raw;
        if (((CompilerGlobalSymbolView *)symbol)->symbol_flags & 2) {
            emit_implicit_function(symbol);
            if (((CompilerGlobalSymbolView *)input)->member_flags & 4) {
                symbol = raw->kind == 24 ?
                    ((CompilerAliasSymbol *)raw)->target->symbol : raw;
                ((CompilerGlobalSymbolView *)symbol)->symbol_flags |= 0x80;
            }
        }
    }
}
extern "C" void __cdecl pop_block_output(CompilerWorkLink **);
/* 22c99..22d35. Restore the saved work kind and mark only live
 * cleanup rows created after the constructor's saved state index. */
extern "C" void __cdecl finish_member_initializers(void)
{
    CompilerInitializerContext *context = &frontend_bss.initializer_context;
    int saved = (int)context->constructor_state_index;
    int current = (int)frontend_data.parameter_work_index;
    --frontend_data.member_initializer_depth;
    if (current != saved) {
        context->cleanup_pending_count = 0;
        for (int index = current; index > saved; --index) {
            if (index < 0 || !frontend_data.parameter_work_states ||
                (unsigned)index >= frontend_data.parameter_work_capacity) { __asm { int 3 } }
            CompilerExpressionWorkStateRow *row = &frontend_data.parameter_work_states[index];
            if (row->active == 2 && !row->cleanup_mark) {
                row->cleanup_mark = 1;
                ++context->cleanup_pending_count;
                context->last_cleanup_state_index = index;
            }
        }
        frontend_data.parameter_work_index = context->constructor_state_index;
    }
    frontend_data.parameter_work_kind = context->head->value.kind;
    pop_block_output(&context->head);
}
extern "C" void __cdecl emit_member_group_cleanup(CompilerSymbol *, unsigned, unsigned);
extern "C" void __cdecl emit_additional_member_cleanup(CompilerSymbol *, unsigned);
/* 22659..2267b: the primary group precedes the additional member groups. */
extern "C" void __cdecl emit_constructor_cleanup(CompilerSymbol *owner, unsigned mode)
{
    emit_member_group_cleanup(owner, 0, 0);
    emit_additional_member_cleanup(owner, mode);
}
extern "C" CompilerExpression *__cdecl make_member_cleanup_target(ClassMemberFunctionTable *, CompilerSymbol *);
extern "C" void __cdecl emit_member_cleanup_call(CompilerExpression *, CompilerExpression *);
extern "C" int __cdecl member_group_offset(ClassMemberFunctionTable *);
/* 2267b..22746: the same per-class function groups are traversed before
 * their inherited counterparts. Only the two recovered group columns may be read. */
extern "C" void __cdecl emit_member_group_cleanup(CompilerSymbol *input, unsigned group, unsigned mode)
{
    if (group >= 2) { __asm { int 3 } }
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)input)->class_detail;
    CompilerVector *members = detail->member_groups[group];
    CompilerVector *additional = detail->additional_members;
    short member_index = 0, additional_index = 0;
    for (;;) {
        if (members && member_index < members->count) {
            ClassMemberFunctionTable *table = (ClassMemberFunctionTable *)members->items[member_index++];
            CompilerExpression *function = symbol_expression(table->function, group == 0 || group == 2 ? 1 : 0);
            if (!mode) {
                CompilerExpression *target = make_member_cleanup_target(table, input);
                emit_member_cleanup_call(target, function);
            } else {
                CompilerType *type = function->type;
                int offset = member_group_offset(table);
                initialize_member_expression(offset, type, function, NULL);
            }
            continue;
        }
        if (!additional || additional_index >= additional->count) return;
        InheritedMemberGroup *inherited = (InheritedMemberGroup *)additional->items[additional_index++];
        members = inherited->functions[group];
        member_index = 0;
    }
}
extern "C" CompilerExpression *__cdecl make_member_cleanup_target(ClassMemberFunctionTable *, CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl emit_member_cleanup_call(CompilerExpression *, CompilerExpression *) { __asm { int 3 } }
extern "C" int __cdecl member_group_offset(ClassMemberFunctionTable *) { __asm { int 3 } return 0; }
extern "C" CompilerExpression *__cdecl make_additional_member_value(CompilerSymbol *, InheritedMemberGroup *);
extern "C" CompilerExpression *__cdecl make_additional_cleanup_target(CompilerSymbol *, InheritedMemberGroup *);
/* 22746..2277c, 65c4c..65c83. Only groups with their recorded
 * secondary state produce an additional cleanup expression. */
extern "C" void __cdecl emit_additional_member_cleanup(CompilerSymbol *owner, unsigned mode)
{
    CompilerVector *groups = ((CompilerGlobalSymbolView *)owner)->class_detail->additional_members;
    short index = 0;
    while (groups && index < groups->count) {
        InheritedMemberGroup *group = (InheritedMemberGroup *)groups->items[index++];
        if (group->secondary_state) {
            CompilerExpression *value;
            if (mode) value = make_additional_member_value(owner, group);
            else value = (CompilerExpression *)make_integer_constant(0);
            CompilerExpression *target = make_additional_cleanup_target(owner, group);
            emit_member_cleanup_call(target, value);
        }
    }
}
extern "C" CompilerExpression *__cdecl make_additional_member_value(CompilerSymbol *, InheritedMemberGroup *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_additional_cleanup_target(CompilerSymbol *, InheritedMemberGroup *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl replay_initializer_context(void *, unsigned);
/* 22b3f..22c51, 65e08..65ed4. Remove only the matching initializer;
 * an optional BB wrapper supplies its separately recorded context tree. */
extern "C" CompilerExpression *__cdecl take_member_initializer(
    CompilerExpression **cursor, CompilerSymbol *member)
{
    CompilerExpression *result = NULL;
    void *context = NULL;
    while (*cursor) {
        CompilerExpression *expression = (*cursor)->value.operands.first;
        void *current_context = NULL;
        int matched = 0, hidden = 0;
        if (expression && expression->kind == 0xbb) {
            current_context = expression->value.operands.second;
            expression = expression->value.operands.first;
        }
        CompilerSymbol *symbol = expression->value.operands.first->value.symbol;
        if (symbol->kind == 18) {
            if (member->kind == 15) {
                int index = ((CompilerClassMemberSymbol *)member)->owner_index;
                CompilerPointerArray *table = frontend_data.frontend_pointer_table;
                if (index < 0 || index >= table->count) { __asm { int 3 } }
                CompilerGlobalSymbolView *base = (CompilerGlobalSymbolView *)table->items[index];
                if (base) {
                    CompilerSymbol *target = ((CompilerBoundSymbol *)symbol)->binding->owner->symbol;
                    if (!((base->record_index ^
                        ((CompilerGlobalSymbolView *)target)->record_index) & 0xffffff)) matched = 1;
                }
            }
        } else if (((CompilerNamedSymbol *)symbol)->link.name == ((CompilerNamedSymbol *)member)->link.name) {
            matched = 1;
        } else {
            CompilerNameEntry *name = ((CompilerNamedSymbol *)symbol)->link.name;
            if (!(((CompilerGlobalSymbolView *)symbol)->storage_class & 7)) {
                CompilerScopeMatch *match = frontend_data.translation_unit->FindHiddenName(name);
                symbol = match ? match->symbol : NULL;
                hidden = match && match->scope && match->scope->level == 0xfa;
            }
            if (symbol) {
                if (symbol->kind == 18) symbol = ((CompilerBoundSymbol *)symbol)->binding->owner->symbol;
                if ((((CompilerGlobalSymbolView *)symbol)->storage_class & 7) == 7) {
                    CompilerType *type = ((CompilerIrSymbol *)symbol)->type;
                    int index = (short)type->auxiliary;
                    CompilerPointerArray *table = frontend_data.frontend_pointer_table;
                    if (index < 0 || index >= table->count) { __asm { int 3 } }
                    CompilerSymbol *owner = (CompilerSymbol *)table->items[index];
                    if (owner && type->shape && owner->kind == 4 &&
                        ((CompilerNamedSymbol *)owner)->link.name == ((CompilerNamedSymbol *)member)->link.name) {
                        matched = 1;
                        if (!hidden) report_language_diagnostic(4, 0x61, 0x53, symbol, 0x53, owner);
                    }
                }
            }
        }
        if (matched) {
            *cursor = (*cursor)->value.operands.second;
            result = expression->value.operands.second;
            context = current_context;
            break;
        }
        cursor = &(*cursor)->value.operands.second;
    }
    if (!frontend_bss.suppress_parser_codegen && context)
        replay_initializer_context(context, 0);
    return result;
}
extern "C" void __cdecl replay_initializer_context(void *, unsigned) { __asm { int 3 } }
extern "C" int __cdecl query_initializer_mode(void)
{
    return frontend_bss.initializer_access_mode;
}
extern "C" void __cdecl set_initializer_mode(unsigned) { __asm { int 3 } }
extern "C" void __cdecl emit_virtual_base_initializers(CompilerSymbol *, CompilerExpression **, unsigned) { __asm { int 3 } }
extern "C" CompilerExpression *__cdecl make_base_initializer(CompilerSymbol *, unsigned, int) { __asm { int 3 } return NULL; }
extern "C" int __cdecl assign_symbol_expression(CompilerExpression *, CompilerExpression *);
extern "C" CompilerExpression *__cdecl make_this_pointer_expression(CompilerSymbol *);
extern "C" CompilerType *__cdecl make_member_address_type(CompilerType *, unsigned);
extern "C" int __cdecl requires_array_initializer_diagnostic(CompilerExpression *);
/* 3235b..323ea, 6d838..6d89c. Form the member address with the this
 * pointer's qualifiers, then perform the real assignment conversion. */
extern "C" int __cdecl initialize_member_expression(int offset, CompilerType *type,
    CompilerExpression *expression, CompilerSymbol *member)
{
    if (type->shape && (type->shape->flags & 0x40) && requires_array_initializer_diagnostic(expression)) {
        compiler_warning(0x162, member);
        return 0;
    }
    CompilerExpression *object = make_this_pointer_expression(NULL);
    CompilerType *address_type = make_member_address_type(type, object->type->shape->attributes & 3);
    CompilerExpression *displacement = (CompilerExpression *)make_integer_constant(offset);
    object = make_binary_node(0x53, object, displacement, address_type);
    if (member && member->kind == 2) {
        CompilerGlobalSymbolView *field = (CompilerGlobalSymbolView *)member;
        CompilerExpression *second = (CompilerExpression *)make_integer_constant(field->member_selection.second);
        CompilerExpression *first = (CompilerExpression *)make_integer_constant(field->member_selection.first);
        CompilerExpression *selection = make_ir_binary_node(0x36, first, second);
        object = make_ir_binary_node(0x35, object, selection);
    }
    /* The encountered CF298 rows46/96 are list-class60; the explicitly
     * recovered leaf rows below are not. No other row gets a default. */
    if (expression->kind == 0x2e || expression->kind == 0x60) {
        if (!expression->value.operands.second) expression = expression->value.operands.first;
    } else if (expression->kind != 6 && expression->kind != 7 && expression->kind != 61 &&
        expression->kind != 93 && expression->kind != 100 && expression->kind != 166 &&
        expression->kind != 199) { __asm { int 3 } }
    return assign_symbol_expression(object, expression);
}
extern "C" int __cdecl this_pointer_adjustment(CompilerSymbol *);
/* 2194d..219a2, 651ff..65201. Apply a thunk's this displacement only
 * when the current function's existing flags select that representation. */
extern "C" CompilerExpression *__cdecl make_this_pointer_expression(CompilerSymbol *parameter)
{
    CompilerExpression *expression = make_implicit_this_expression(parameter);
    if (!expression) return NULL;
    int offset = 0;
    CompilerRuntimeFunction *function = (CompilerRuntimeFunction *)frontend_bss.current_function;
    if (function->queue_flags & 0x10) offset = this_pointer_adjustment(function);
    if (offset) {
        CompilerExpression *displacement = (CompilerExpression *)make_integer_constant(offset);
        expression = make_ir_binary_node(0x46, expression, displacement);
        expression->flags |= 0x20;
    }
    return expression;
}
extern "C" int __cdecl this_pointer_adjustment(CompilerSymbol *) { __asm { int 3 } return 0; }
/* 1b4ff..1b51b, 5fe8d..5febd. Preserve the original type unless a
 * missing low qualifier requires its existing scratch/interner path. */
extern "C" CompilerType *__cdecl make_member_address_type(CompilerType *type, unsigned qualifiers)
{
    if ((~type->attributes & qualifiers) & 3) {
        CompilerType *scratch = make_parser_type_scratch(type);
        scratch->attributes ^= (WORD)((qualifiers ^ scratch->attributes) & 3);
        type = intern_type(scratch);
        --frontend_bss.type_scratch_index;
    }
    return type;
}
extern "C" int __cdecl requires_array_initializer_diagnostic(CompilerExpression *) { __asm { int 3 } return 0; }
extern "C" void __cdecl emit_copy_member_initializer(CompilerSymbol *, CompilerSymbol *, CompilerExpression *, int, unsigned, unsigned) { __asm { int 3 } }
extern "C" void __cdecl emit_special_member_initializer(CompilerSymbol *, int, CompilerExpression *, unsigned) { __asm { int 3 } }
extern "C" void __cdecl emit_destructor_body(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl emit_deleting_destructor_body(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl emit_vector_deleting_destructor_body(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl emit_default_constructor_closure(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl emit_vftable_constructor_closure(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl emit_copy_constructor_closure(CompilerSymbol *) { __asm { int 3 } }

extern "C" void __cdecl prepare_return_statement(void)
{
    CompilerExpressionWork frame;
    memset(&frame, 0, sizeof(frame));
    frame.kind = 8;
    push_expression_work(&frame, 16);
    frontend_data.parameter_work_kind = 8;
}

extern "C" {
int __cdecl return_expression_needs_warning(CompilerExpression *); /*15b04*/
CompilerExpression *__cdecl prepare_return_cleanup(CompilerExpression *, unsigned); /*16a0d*/
CompilerExpression *__cdecl make_return_object_call(unsigned, CompilerSymbol *, CompilerExpression *, CompilerExpression *, unsigned, unsigned, unsigned); /*20e8f*/
CompilerExpression *__cdecl materialize_return_value(CompilerType *, CompilerExpression *); /*86726*/
void __cdecl ensure_return_scope(void); /*48030*/
void __cdecl emit_ir_expression(CompilerExpression *);
}

extern "C" void __cdecl emit_return_statement(CompilerExpression *expression, unsigned mode)
{
    CompilerGlobalSymbolView *function, *owner;
    CompilerExpression *converted, *operation;
    CompilerType *result_type, *expression_type;
    int void_result;
    if (!frontend_data.function_result_type) return;
    function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    if ((function->scope_flags & 4) || (expression && (expression->flags & 8))) {
        frontend_data.compiler_mode_flags |= 0x800;
        return;
    }
    if (!frontend_bss.declaration_scope_mode) {
        ensure_return_scope();
        function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    }
    if ((function->type_flags & 0x10) && expression) {
        compiler_warning(0x216, frontend_data.declaration_lookup_owner);
        function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    } else if ((function->type_flags & 0x20) && expression) {
        compiler_warning(0x241, frontend_data.declaration_lookup_owner);
        expression = NULL;
        function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    }
    if ((function->member_flags & 1) && mode) {
        compiler_warning(0x1ea, 0x4c, 0x7d);
        return;
    }
    result_type = frontend_data.function_result_type;
    void_result = result_type->flags == 0x80 && !result_type->shape;
    if (void_result) {
        if (expression) {
            if (frontend_data.options.language_extensions && !(function->type_flags & 0x44) &&
                (frontend_data.source_mode_flags & 0x1000000) && (frontend_data.compiler_mode_flags & 2))
                compiler_warning(0x269, function);
            else compiler_warning(0x232, function);
            goto finished;
        }
        goto empty_return;
    }
    if (!expression) {
        if (frontend_data.options.language_extensions && !(function->type_flags & 0x44)) {
            if (frontend_data.source_mode_flags & 0x1000000) goto infer_void;
            if ((function->name == frontend_data.builtin_names.main_entry ||
                 function->name == frontend_data.builtin_names.winmain_entry ||
                 function->name == frontend_data.builtin_names.dllmain_entry) &&
                !function->declaration_qualifiers && (function->storage_class & 7) == 1)
                goto infer_void;
        }
        if (mode != 2 && !(function->member_flags & 1)) {
            if (function->qualifiers & 0x10) report_language_diagnostic(3, 0x23, function);
            else compiler_warning(0x231, function);
        }
        frontend_data.compiler_mode_flags |= 0x800;
        goto finished;
    infer_void:
        if (frontend_data.compiler_mode_flags & 2) compiler_warning(0x269, function);
        else {
            frontend_data.compiler_mode_flags |= 2;
            frontend_data.function_result_type = frontend_data.builtin_types.type_0080;
            void_result = 1;
            report_language_diagnostic(1, 0x1fc, function);
        }
    }
    if (expression && frontend_data.options.language_extensions && !(function->type_flags & 0x40) &&
        (frontend_data.source_mode_flags & 0x1000000) && !(frontend_data.compiler_mode_flags & 2)) {
        frontend_data.compiler_mode_flags |= 2;
        frontend_data.function_result_type = frontend_data.builtin_types.type_0063;
    }
    if (!expression) goto empty_return;
    converted = convert_assignment_operand(frontend_data.function_result_type, expression);
    if (!converted) goto finished;
    function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    if (function->linkage_flags & 1) {
        owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[
            (short)frontend_data.function_result_type->auxiliary];
        if (!(owner->definition_flags & 4)) return;
        if (!(owner->type_flags & 0x10)) {
            compiler_warning(0x229, frontend_data.function_result_type);
            goto finished;
        }
        expression = lookup_name_expression(frontend_data.builtin_names.return_udt);
        owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[
            (short)frontend_data.function_result_type->auxiliary];
        operation = make_return_object_call(0, (CompilerSymbol *)owner, expression,
            append_expression_list(NULL, converted, 2), 0, 1, 1);
        if (!operation) {
            compiler_warning(0x229, frontend_data.function_result_type);
            goto finished;
        }
        if (mode) { operation = prepare_return_cleanup(operation, 0); mode = 0; }
        emit_statement_expression(operation);
    } else {
        if (return_expression_needs_warning(converted)) report_language_diagnostic(1, 0xac);
        expression = convert_typed_expression(frontend_data.function_result_type,
            make_typed_expression(0x7d, frontend_data.function_result_type, converted, 0));
    }
    if (!expression) goto finished;
    expression_type = expression->type;
    if (expression_type->shape && (expression_type->shape->attributes & 0x300))
        expression = materialize_return_value(expression_type, expression);
    if (mode) { expression = prepare_return_cleanup(expression, 1); mode = 0; }
    emit_context_transition(0x24);
    emit_ir_expression(expression);
    expression->type = expression_type;
    write_ir_record(0x41, expression);
    frontend_data.compiler_mode_flags |= 0x800;
    goto return_label;
empty_return:
    if (mode || frontend_bss.declaration_scope_mode == 2) {
        if (!mode) prepare_return_statement();
        expression = prepare_return_cleanup(NULL, 0);
        mode = 0;
        if (expression) emit_statement_expression(expression);
    }
return_label:
    if (mode && !void_result) {
        operation = prepare_return_cleanup(expression, 0);
        mode = 0;
        if (operation) emit_statement_expression(operation);
    }
    write_ir_record(0x3a, frontend_bss.function_entry_label);
finished:
    if (mode) prepare_return_cleanup(NULL, 1);
}
extern "C" CompilerExpression *__cdecl unwrap_return_expression(CompilerExpression *); /*15a1d*/
extern "C" int __cdecl return_expression_needs_warning(CompilerExpression *expression)
{
    unsigned storage;
    expression = unwrap_return_expression(expression);
    if (!expression || (expression->flags & 7) != 4) return 0;
    storage = ((CompilerIrSymbol *)expression->value.symbol)->storage_class & 7;
    return storage >= 4 && storage <= 6;
}
extern "C" CompilerExpression *__cdecl unwrap_return_expression(CompilerExpression *expression)
{
    CompilerExpression *first, *second, *value;
    for (;;) {
        if (expression->kind == 0x64) {
            expression = expression->value.operands.first;
            continue;
        }
        if (expression->kind != 0x5d) return expression;
        second = expression->value.operands.second;
        if (!second) {
            expression = expression->value.operands.first;
            continue;
        }
        first = expression->value.operands.first;
        if ((first->type->shape->attributes & 3) != 3) return expression;
        value = second->value.operands.first;
        /* Encountered CF298 classes:61/166 share class61. The known leaf
         * classes below differ from61. No other row is assigned a default. */
        if (second->kind == 61 || second->kind == 166)
            value = value->value.operands.first;
        else if (second->kind != 6 && second->kind != 7 && second->kind != 93 &&
                 second->kind != 100 && second->kind != 199) { __asm { int 3 } }
        value = value->value.operands.first;
        if (value->type) return expression;
        expression = first;
    }
}
extern "C" {
CompilerExpression *__cdecl build_expression_cleanup(CompilerExpression *); /*16b5e*/
void __cdecl inspect_expression_cleanup(CompilerExpression *); /*17408*/
unsigned __cdecl allocate_temporary_id(void); /*3ed20*/
CompilerExpression *__cdecl make_temporary_expression(unsigned, CompilerType *); /*3ed4d*/
CompilerExpression *__cdecl make_result_binary_node(unsigned, CompilerExpression *, CompilerExpression *); /*213ec*/
CompilerExpression *__cdecl convert_return_temporary(CompilerExpression *); /*161e7*/
}
extern "C" CompilerExpression *__cdecl prepare_return_cleanup(CompilerExpression *expression, unsigned mode)
{
    jmp_buf recovery;
    CompilerExpressionWork frame, *current;
    CompilerExpression *prefix = NULL, *cleanup;
    CompilerExpression *volatile saved_result;
    CompilerType *type;
    unsigned temporary;
    if (frontend_bss.expression_work_recovery_mode && !frontend_data.expression_work_recovery) {
        if (frontend_data.expression_work_reset_pending) return expression;
        saved_result = expression;
        frontend_data.expression_work_recovery = recovery;
        if (_setjmp3(recovery, 0)) {
            frontend_data.expression_work_recovery = NULL;
            return saved_result;
        }
    }
    current = frontend_data.expression_work_nodes;
    if (current && current->kind == 13) {
        prefix = check_statement_expression(NULL, 2);
        current = frontend_data.expression_work_nodes;
    }
    if (current && current->kind == 8) current->kind = 11;
    else {
        memset(&frame, 0, sizeof(frame));
        frame.kind = 9;
        push_expression_work(&frame, 16);
    }
    if (!expression && mode) {
        if (frontend_data.expression_work_recovery == recovery) frontend_data.expression_work_recovery = NULL;
        frontend_data.parameter_work_kind = 1;
        return NULL;
    }
    frontend_data.parameter_work_kind = 1;
    cleanup = build_expression_cleanup(0);
    inspect_expression_cleanup(cleanup);
    if (prefix || cleanup) {
        saved_result = NULL;
        if (mode && expression) {
            type = expression->type;
            if (type->flags != 0x80 || type->shape) {
                temporary = allocate_temporary_id();
                saved_result = make_temporary_expression(temporary, type);
                expression = make_ir_binary_node(0x34, saved_result, expression);
            }
        }
        if (prefix) expression = expression ? make_result_binary_node(0x37, expression, prefix) : prefix;
        if (cleanup) expression = expression ? make_result_binary_node(0x37, expression, cleanup) : cleanup;
        if (saved_result)
            expression = make_binary_node(0x37, expression,
                convert_return_temporary(make_temporary_expression(temporary, type)), type);
    }
    frontend_bss.return_cleanup_state = 0;
    if (frontend_data.expression_work_recovery == recovery) frontend_data.expression_work_recovery = NULL;
    return expression;
}
/* Partial payload views of the same allocated work records. The two-bit
 * allocation class establishes at least32 bytes before any payload read;
 * no wider record contents or allocation extent are inferred. */
struct CompilerCleanupPayload : CompilerExpressionWork {
    union { void *pointer; CompilerExpression *expression; unsigned index; } value;
};
struct CompilerCleanupCapture : CompilerCleanupPayload {
    CompilerExpression *label; /* original+14, emitted before capture linking */
    CompilerCleanupCapture *capture_next;
};
typedef char CompilerCleanupCapture_offsets[(offsetof(CompilerCleanupPayload, value) == 16 &&
    offsetof(CompilerCleanupCapture, capture_next) == 24) ? 1 : -1];
/* OriginalD05DC pairs the encountered closing/opening work kinds15..54.
 * Inputs outside that interval explicitly select criterion0 in the caller. */
static const unsigned paired_cleanup_kind[40] = {
    16, 15, 24, 19, 18, 21, 20, 23, 22, 17,
    30, 27, 26, 29, 28, 25, 36, 33, 32, 35,
    34, 31, 44, 39, 38, 41, 40, 43, 42, 37,
    46, 45, 48, 47, 54, 51, 50, 53, 52, 49
};
#define CLEANUP_HAS_PAYLOAD(p) do { if (((p)->flags >> 6) == 0) { __asm { int 3 } } } while (0)
#define CLEANUP_VALUE(p) (((CompilerCleanupPayload *)(p))->value)
#define CLEANUP_RETURN_BOUNDARY(k) ((k) == 9 || (k) == 11 || ((k) >= 58 && (k) <= 61))
#define CLEANUP_PARTNER(k) ((k) >= 15 && (k) <= 54 ? paired_cleanup_kind[(k) - 15] : 0)
extern "C" {
CompilerExpression *__cdecl append_scope_cleanup(CompilerExpression *, void *, CompilerExpressionWork *, unsigned); /*3f9b4*/
CompilerExpressionWork *__cdecl scan_parameter_cleanup(CompilerExpressionWork *, unsigned, unsigned); /*15b35*/
CompilerExpression *__cdecl append_local_cleanup(CompilerExpression *, void *, CompilerExpressionWork *); /*45699*/
CompilerExpression *__cdecl collect_frame_cleanup(CompilerExpression *, CompilerExpressionWork *, unsigned, unsigned); /*1c02c*/
CompilerExpressionWork *__cdecl select_cleanup_boundary(CompilerExpressionWork *); /*a9f42*/
CompilerCleanupCapture *__cdecl find_cleanup_capture(void *); /*46e31*/
void __cdecl link_cleanup_capture(CompilerExpressionWork *, CompilerExpressionWork *, unsigned); /*46e48*/
}
extern "C" CompilerExpression *__cdecl build_expression_cleanup(CompilerExpression *expression)
{
    jmp_buf recovery;
    CompilerExpression *volatile recovery_result;
    CompilerExpression *result = expression;
    CompilerExpressionWork *current, *scope, *boundary;
    CompilerCleanupCapture *capture;
    CompilerExpressionWork recursive_frame;
    unsigned kind, depth, active, stop_at_capture;
    if (frontend_bss.expression_work_recovery_mode) {
        if (!frontend_data.expression_work_recovery) {
            if (frontend_data.expression_work_reset_pending) return expression;
            recovery_result = expression;
            frontend_data.expression_work_recovery = recovery;
            if (_setjmp3(recovery, 0)) {
                frontend_data.expression_work_recovery = NULL;
                return recovery_result;
            }
        }
        if (frontend_bss.expression_work_recovery_mode) goto return_input;
    }
    if (frontend_bss.error_count > (int)frontend_data.expression_work_error_count) goto return_input;
    current = frontend_data.expression_work_nodes;
    switch (current->kind) {
    case 4:
        scope = current->previous;
        while (scope->kind == 16) scope = scope->previous;
        kind = scope->kind;
        if (kind == 9 || kind == 11 || kind == 58 || kind == 59)
            frontend_data.parameter_cleanup_active = 1;
        scope = find_expression_work(current->previous, 1, 1)->next;
        if (scope->kind != 2) goto finish_parameter_cleanup;
        current = scan_parameter_cleanup(scope, 3, 1);
        while (current->kind != 2) {
            if (current->kind == 5) {
                CLEANUP_HAS_PAYLOAD(current);
                expression = result = append_scope_cleanup(result, CLEANUP_VALUE(current).pointer, current, 1);
            }
            current = current->previous;
        }
finish_parameter_cleanup:
        frontend_data.parameter_cleanup_active = 0;
        break;
    case 10:
        scope = find_expression_work(current->previous, 1, 1);
        pop_expression_work();
        if (scope->next->kind != 2) break;
        current = scan_parameter_cleanup(scope->next, 3, 1);
        while (current->kind != 2) {
            if (current->kind == 5) {
                CLEANUP_HAS_PAYLOAD(current);
                expression = result = append_scope_cleanup(result, CLEANUP_VALUE(current).pointer, current, 0);
            }
            current = current->previous;
        }
        break;
    case 12:
        current = current->previous;
        depth = frontend_data.nested_cleanup_depth;
        while (current && (current->kind == 13 || current->kind == 62)) {
            if (current->kind == 62) {
                if (!depth) break;
                --depth;
            } else if (!(current->flags & 1)) {
                CLEANUP_HAS_PAYLOAD(current);
                if (current->flags & 2)
                    result = result ? make_result_binary_node(0x37, result, CLEANUP_VALUE(current).expression) :
                        CLEANUP_VALUE(current).expression;
                else result = append_local_cleanup(result, CLEANUP_VALUE(current).pointer, current);
                expression = result;
            }
            current = current->previous;
        }
        break;
    case 9: case 11:
        boundary = current;
        if (current->kind == 9) boundary = find_expression_work(current->previous, 8, 1);
        if (CLEANUP_RETURN_BOUNDARY(boundary->previous->kind)) goto return_input;
        current = current->previous;
        for (;;) {
            if (current->kind == 1) break;
            if (current->kind == 3) current = find_expression_work(current->previous, 2, 0);
            else if (current->kind == 16) current = find_expression_work(current->previous, 15, 0);
            else if (current->kind == 5) {
                CLEANUP_HAS_PAYLOAD(current);
                expression = result = append_scope_cleanup(result, CLEANUP_VALUE(current).pointer, current, 0);
            } else if (current->kind == 14) {
                expression = result = collect_frame_cleanup(result, current->previous, 2, 0);
                if (current->flags & 2) {
                    CLEANUP_HAS_PAYLOAD(current);
                    expression = result = append_scope_cleanup(result, CLEANUP_VALUE(current).pointer, current, 0);
                }
            }
            if (!current) break;
            current = current->previous;
        }
        memset(&recursive_frame, 0, sizeof(recursive_frame));
        recursive_frame.kind = 10;
        push_expression_work(&recursive_frame, 16);
        result = build_expression_cleanup(result);
        break;
    case 16:
        active = 1;
        do {
            current = current->previous;
            if (current->kind == 15) {
                CLEANUP_HAS_PAYLOAD(current);
                frontend_data.parameter_work_index = CLEANUP_VALUE(current).index;
                active = 0;
            } else if (current->kind == 16) current = find_expression_work(current->previous, 15, 1);
            else if (current->kind == 5) {
                CLEANUP_HAS_PAYLOAD(current);
                expression = result = append_scope_cleanup(result, CLEANUP_VALUE(current).pointer, current, 1);
            } else if (current->kind == 14) {
                if (!active) break;
                expression = result = collect_frame_cleanup(result, current->previous, 2, 1);
                if (current->flags & 2) {
                    CLEANUP_HAS_PAYLOAD(current);
                    expression = result = append_scope_cleanup(result, CLEANUP_VALUE(current).pointer, current, 1);
                }
            }
        } while (active);
        break;
    case 21: case 23: case 29: case 33: case 43: case 53:
        current = current->previous;
        if (current->kind == 16) goto return_input;
        scope = find_expression_work(current, CLEANUP_PARTNER(current->kind), 1)->next;
        while (scope->kind == 55 || scope->kind == 56 || scope->kind == 57) scope = scope->next;
        current = select_cleanup_boundary(scope);
        if (current->kind != 14) break;
        expression = result = collect_frame_cleanup(expression, current->previous, 2, 1);
        if (current->flags & 2) {
            CLEANUP_HAS_PAYLOAD(current);
            result = append_scope_cleanup(result, CLEANUP_VALUE(current).pointer, current, 1);
        }
        break;
    case 57:
        CLEANUP_HAS_PAYLOAD(current);
        capture = frontend_data.cleanup_capture_head;
        while (capture) {
            CLEANUP_HAS_PAYLOAD(capture);
            if (capture->value.pointer == CLEANUP_VALUE(current).pointer)
                link_cleanup_capture(capture, current, 1);
            capture = capture->capture_next;
        }
        break;
    case 58:
        if (CLEANUP_RETURN_BOUNDARY(current->previous->kind)) goto return_input;
        CLEANUP_HAS_PAYLOAD(current);
        capture = find_cleanup_capture(CLEANUP_VALUE(current).pointer);
        link_cleanup_capture(current, capture, 3);
        break;
    case 60: case 61:
        stop_at_capture = current->kind == 61;
        current = current->previous;
        if (CLEANUP_RETURN_BOUNDARY(current->kind)) goto return_input;
        while (current->kind != 1) {
            kind = current->kind;
            if (kind == 25 || kind == 31 || kind == 37 || (stop_at_capture && kind == 49)) break;
            if (kind == 16 || kind == 30 || kind == 36 || kind == 44 || kind == 54) {
                current = find_expression_work(current->previous, CLEANUP_PARTNER(kind), 1);
            } else if (kind == 5) {
                CLEANUP_HAS_PAYLOAD(current);
                expression = result = append_scope_cleanup(result, CLEANUP_VALUE(current).pointer, current, 0);
            } else if (kind == 14) {
                expression = result = collect_frame_cleanup(result, current->previous, 2, 0);
                if (current->flags & 2) {
                    CLEANUP_HAS_PAYLOAD(current);
                    expression = result = append_scope_cleanup(result, CLEANUP_VALUE(current).pointer, current, 0);
                }
            }
            current = current->previous;
        }
        break;
    default:
        break;
    }
    if (frontend_data.expression_work_recovery == recovery) frontend_data.expression_work_recovery = NULL;
    return result;
return_input:
    if (frontend_data.expression_work_recovery == recovery) frontend_data.expression_work_recovery = NULL;
    return expression;
}
#undef CLEANUP_HAS_PAYLOAD
#undef CLEANUP_VALUE
#undef CLEANUP_RETURN_BOUNDARY
#undef CLEANUP_PARTNER
extern "C" CompilerExpression *__cdecl append_scope_cleanup(CompilerExpression *, void *, CompilerExpressionWork *, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpressionWork *__cdecl scan_parameter_cleanup(CompilerExpressionWork *record,
    unsigned kind, unsigned required)
{
    unsigned partner = (int)kind >= 15 && (int)kind <= 54 ? paired_cleanup_kind[kind - 15] : 0;
    unsigned depth = 1;
    while (record) {
        if (record->kind == kind) {
            if (--depth == 0) return record;
        } else if (record->kind == partner) ++depth;
        record = record->next;
    }
    if (required && frontend_data.expression_work_recovery) {
        reset_expression_work();
        frontend_data.expression_work_reset_pending = 1;
        longjmp(*(jmp_buf *)frontend_data.expression_work_recovery, -1);
    }
    return NULL;
}
extern "C" CompilerExpression *__cdecl append_local_cleanup(CompilerExpression *, void *, CompilerExpressionWork *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl collect_frame_cleanup(CompilerExpression *, CompilerExpressionWork *, unsigned, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpressionWork *__cdecl select_cleanup_boundary(CompilerExpressionWork *) { __asm { int 3 } return NULL; }
extern "C" CompilerCleanupCapture *__cdecl find_cleanup_capture(void *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl link_cleanup_capture(CompilerExpressionWork *, CompilerExpressionWork *, unsigned) { __asm { int 3 } }

/* Borrowed list headers reached only by the187/188 expression variants.
 * These are consumed pointer fields, not allocation or default claims. */
struct CompilerCleanupExpressionItem {
    CompilerCleanupExpressionItem *next;
    CompilerExpression *expression;
};
struct CompilerCleanupExpressionList {
    CompilerCleanupExpressionList *next;
    CompilerCleanupExpressionItem *items;
};
extern "C" void __cdecl inspect_expression_cleanup(CompilerExpression *expression)
{
    CompilerExpression *first, *second;
    CompilerCleanupExpressionList *list;
    CompilerCleanupExpressionItem *item;
    unsigned first_index, second_index, first_state, second_state;
    WORD count;
    while (expression) {
        if ((expression->flags & 7) == 1) {
            expression = expression->value.operands.first;
            continue;
        }
        if ((expression->flags & 7) == 7) {
            if (expression->kind < 0xbb || expression->kind > 0xbc) return;
            for (list = expression->value.cleanup_list.head; list; list = list->next)
                for (item = list->items; item; item = item->next)
                    inspect_expression_cleanup(item->expression);
            expression = expression->value.operands.first;
            continue;
        }
        if ((expression->flags & 7) != 2) return;
        inspect_expression_cleanup(expression->value.operands.first);
        inspect_expression_cleanup(expression->value.operands.second);
        if (expression->kind != 0x37) return;
        first = expression->value.operands.first;
        if (first->kind != 0xc8 && first->kind != 0xc9) return;
        second = expression->value.operands.second;
        if (first->kind != second->kind) return;
        first_index = first->value.counted_control.index;
        second_index = second->value.counted_control.index;
        if (!frontend_data.parameter_work_states || first_index >= frontend_data.parameter_work_capacity ||
            second_index >= frontend_data.parameter_work_capacity) { __asm { int 3 } }
        first_state = frontend_data.parameter_work_states[first_index].active;
        second_state = frontend_data.parameter_work_states[second_index].active;
        if (first_state != second_state || first->value.counted_control.flags != second->value.counted_control.flags)
            return;
        /* Authentic six-DWORD copy retains the expression's opaque bytes.
         * Read the second count afterwards, preserving the original order. */
        memcpy(expression, first, 24);
        count = expression->value.counted_control.count;
        expression->value.counted_control.count = (WORD)((count & 0xf000) |
            ((count + second->value.counted_control.count) & 0xfff));
        return;
    }
}
extern "C" unsigned __cdecl allocate_temporary_id(void) { __asm { int 3 } return 0; }
extern "C" CompilerExpression *__cdecl make_temporary_expression(unsigned, CompilerType *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_result_binary_node(unsigned, CompilerExpression *, CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_immediate_expression(unsigned, CompilerType *, CompilerExpression *);
extern "C" CompilerExpression *__cdecl qualify_reference_expression(CompilerExpression *); /*867db*/
/* 161e7..16258, 5d298..5d2a9: attach the conversion node and recompute
 * its low constant bit before applying reference-shape qualification. */
extern "C" CompilerExpression *__cdecl convert_return_temporary(CompilerExpression *input)
{
    if (!input) return NULL;
    CompilerExpression *result = make_immediate_expression(0x3d, input->type, input);
    CompilerType *type = result->type;
    unsigned constant = !type->shape && (type->flags & 0x2fdf) == 0x40;
    result->constant_flags = (result->constant_flags & ~1) | constant;
    if (type && type->shape && (BYTE(type->shape->flags) & 0x77) == 1 &&
        (BYTE(type->shape->attributes) & 3)) return qualify_reference_expression(result);
    return result;
}
extern "C" CompilerExpression *__cdecl qualify_reference_expression(CompilerExpression *)
{ __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_return_object_call(unsigned, CompilerSymbol *, CompilerExpression *, CompilerExpression *, unsigned, unsigned, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl materialize_return_value(CompilerType *, CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl ensure_return_scope(void) { __asm { int 3 } }

extern "C" {
CompilerExpression *__cdecl find_untyped_assignment(CompilerExpression *, CompilerExpression **); /*4752d*/
int __cdecl can_resolve_assignment_symbol(CompilerSymbol *, unsigned); /*3d402*/
CompilerSymbol *__cdecl resolve_assignment_symbol(CompilerSymbol *, unsigned); /*3d458*/
CompilerSymbol *__cdecl specialize_assignment_symbol(CompilerType *, CompilerSymbol *, unsigned); /*ab7d6*/
void __cdecl prepare_assignment_template(CompilerType *, CompilerSymbol *, void **, unsigned); /*ab969*/
CompilerType *__cdecl make_assignment_function_type(CompilerType *, unsigned, unsigned, unsigned); /*3fe7e*/
CompilerExpression *__cdecl convert_bound_assignment(CompilerType *, CompilerSymbol *); /*8ac12*/
CompilerSymbol *__cdecl resolve_function_declaration(CompilerSymbol *, CompilerSymbol *, CompilerType *, unsigned);
}
extern "C" CompilerExpression *__cdecl convert_assignment_operand(CompilerType *target, CompilerExpression *expression)
{
    CompilerExpression *untyped;
    CompilerSymbol *symbol;
    CompilerType *return_type, *type;
    CompilerTypeShape *shape;
    DeferredAction *action;
    void *selection;
    unsigned kind;
    if (!expression) return NULL;
    if (expression->type) return expression;
    untyped = find_untyped_assignment(expression, &expression);
    if (!untyped) return NULL;
    symbol = untyped->value.symbol;
    if (symbol->type_flags & 0x10) {
        if (symbol->kind == 18) symbol = ((CompilerBoundSymbol *)symbol)->binding->owner->symbol;
        compiler_warning(0x115, symbol);
        return NULL;
    }
    if (symbol->kind == 18) return convert_bound_assignment(target, symbol);
    kind = symbol->kind;
    if (kind == 24) kind = ((CompilerAliasSymbol *)symbol)->target->symbol->kind;
    if (kind != 16 && kind != 7 && kind != 6 && kind != 17 && kind != 8 && kind != 25)
        return untyped;
    if (can_resolve_assignment_symbol(symbol, 0)) {
        symbol = resolve_assignment_symbol(symbol, 0);
        if (((CompilerRuntimeFunction *)symbol)->linkage_flags & 0x20)
            symbol = specialize_assignment_symbol(target, symbol, 0);
    } else {
        if (!target || !(shape = target->shape) ||
            ((shape->flags & 0x77) != 3 && (shape->flags & 0x44) != 0x44)) return NULL;
        return_type = get_function_return_type(target);
        if (((CompilerNamedSymbol *)symbol)->link.name->flags & 0x8000000) {
            selection = NULL;
            prepare_assignment_template(return_type, symbol, &selection, 0);
        }
        symbol = resolve_function_declaration(symbol, NULL, return_type, 1);
    }
    if (!symbol) {
        action = frontend_bss.deferred_action_head;
        if (action) {
            if (frontend_bss.deferred_action_depth <= 0) {
                do { action->Execute(); action = action->next; } while (action);
            }
            frontend_bss.deferred_action_tail = NULL;
            frontend_bss.deferred_action_head = NULL;
        }
        compiler_warning(0x233);
        return NULL;
    }
    if (frontend_bss.deferred_action_head) {
        frontend_bss.deferred_action_tail = NULL;
        frontend_bss.deferred_action_head = NULL;
    }
    if (frontend_bss.browser_filename) frontend_data.backend->Slot1c((unsigned)symbol);
    untyped->value.symbol = symbol;
    type = ((CompilerIrSymbol *)symbol)->type;
    untyped->type = make_assignment_function_type(type, 0xb, type->shape->attributes & 0x73, 0);
    return untyped;
}
extern "C" CompilerExpression *__cdecl find_untyped_assignment(CompilerExpression *, CompilerExpression **) { __asm { int 3 } return NULL; }
extern "C" int __cdecl can_resolve_assignment_symbol(CompilerSymbol *, unsigned) { __asm { int 3 } return 0; }
extern "C" CompilerSymbol *__cdecl resolve_assignment_symbol(CompilerSymbol *, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl specialize_assignment_symbol(CompilerType *, CompilerSymbol *, unsigned) { __asm { int 3 } return NULL; }
extern "C" void __cdecl prepare_assignment_template(CompilerType *, CompilerSymbol *, void **, unsigned) { __asm { int 3 } }
extern "C" CompilerType *__cdecl make_assignment_function_type(CompilerType *, unsigned, unsigned, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl convert_bound_assignment(CompilerType *, CompilerSymbol *) { __asm { int 3 } return NULL; }

/* Original88-byte stack conversion owner. The constructor initializes only
 * the BYTE code, four named DWORD fields and the17-DWORD workspace/flags
 * range. Its three intervening header bytes remain untouched. */
struct CompilerTypeConversion {
    BYTE code;
    BYTE unmodeled_01_03[3];
    unsigned mode;
    CompilerExpression *input;
    CompilerType *source_type, *target_type;
    unsigned source_rank, target_rank, state;
    typedef CompilerExpression *(RECON_THISCALL CompilerTypeConversion::*Builder)(CompilerExpression *);
    Builder build, build_constant;
    BYTE unmodeled_28_2b[4];
    unsigned diagnostic;
    unsigned auxiliary_diagnostic; /* original+30, diagnostic context consumed by Build */
    BYTE unmodeled_34_3b[8];
    unsigned complexity;
    BYTE unmodeled_40_47[8];
    CompilerSymbol *context_symbol; /* original+48, borrowed diagnostic context */
    unsigned context_kind; /* original+4c */
    BYTE unmodeled_50_53[4];
    unsigned flags;
    RECON_THISCALL CompilerTypeConversion(unsigned, unsigned, CompilerExpression *, CompilerType *); /*11fba*/
    RECON_THISCALL CompilerTypeConversion(unsigned, CompilerExpression *, CompilerType *); /*158fa*/
    void RECON_THISCALL Analyze(void); /*1131b*/
    void RECON_THISCALL AnalyzeIntegral(void); /*113b2, pending*/
    void RECON_THISCALL AnalyzeIntegralPointer(void); /*1452b, pending*/
    void RECON_THISCALL AnalyzeReference(void); /*14a32, pending*/
    void RECON_THISCALL AnalyzeRecord(void); /*1545b, pending*/
    void RECON_THISCALL AnalyzeSameIndirection(void); /*15cd0, pending*/
    void RECON_THISCALL AnalyzeRecordConversion(void); /*1a95f, pending*/
    void RECON_THISCALL AnalyzePointerIntegral(void); /*1acc9, pending*/
    void RECON_THISCALL AnalyzeVoid(void); /*2333d, pending*/
    void RECON_THISCALL AnalyzeArray(void); /*2bb38, pending*/
    void RECON_THISCALL AnalyzeArrayPair(void); /*3d303, pending*/
    void RECON_THISCALL AnalyzePrimitive10(void); /*3f01e, pending*/
    void RECON_THISCALL AnalyzeShape03Pair(void); /*3fc8c, pending*/
    void RECON_THISCALL AnalyzeIntegralMember(void); /*403e0, pending*/
    void RECON_THISCALL AnalyzeEnumTarget(void); /*432fa, pending*/
    void RECON_THISCALL AnalyzeSameRecord(void); /*43cd0, pending*/
    void RECON_THISCALL AnalyzeEnumPointer(void); /*4460d, pending*/
    void RECON_THISCALL Reject(void); /*472a2, pending*/
    void RECON_THISCALL AnalyzePointerShape03(void); /*472bf, pending*/
    void RECON_THISCALL AnalyzeUnknownIndirection(void); /*4756f, pending*/
    void RECON_THISCALL AnalyzeUnknown(void); /*4775c, pending*/
    void RECON_THISCALL AnalyzeShape03Pointer(void); /*477a5, pending*/
    void RECON_THISCALL AnalyzeIntegralUnknown(void); /*48989, pending*/
    void RECON_THISCALL AnalyzeIndirectionUnknown(void); /*48a2c, pending*/
    void RECON_THISCALL AnalyzeFloatingIntegral(void); /*4b434, pending*/
    void RECON_THISCALL AnalyzeFloating(void); /*4b478, pending*/
    void RECON_THISCALL AnalyzeToFloating(void); /*4b5e9, pending*/
    void RECON_THISCALL AnalyzeFromReference(void); /*4e6ce, pending*/
    void RECON_THISCALL AnalyzeFloatingUnknown(void); /*502a2, pending*/
    void RECON_THISCALL RejectBoolean(void); /*9f453, pending*/
    void RECON_THISCALL AnalyzeMemberIntegral(void); /*9f61b, pending*/
    void RECON_THISCALL AnalyzeFunction(void); /*9f660, pending*/
    void RECON_THISCALL AnalyzeShape03Reference(void); /*9f7a7, pending*/
    void RECON_THISCALL AnalyzeFromPrimitive10(void); /*9f7ef, pending*/

    int RECON_THISCALL IsValid(void); /*114f8*/
    CompilerExpression *RECON_THISCALL Build(CompilerExpression *); /*11502*/
    CompilerExpression *RECON_THISCALL BuildScalar(CompilerExpression *);
    CompilerExpression *RECON_THISCALL BuildScalarValue(CompilerExpression *); /*16258 RET4*/ /*15b8f*/
    CompilerExpression *RECON_THISCALL BuildScalarConstant(CompilerExpression *); /*11540*/
    CompilerExpression *RECON_THISCALL BuildNullPointer(CompilerExpression *); /*1c208*/
    int RECON_THISCALL ShouldCheckConstant(void); /*1167e*/
    CompilerExpression *RECON_THISCALL WrapResult(CompilerExpression *, CompilerExpression *); /*1ba31*/
    void RECON_THISCALL SetContext(unsigned, CompilerSymbol *); /*1b723*/
    void RECON_THISCALL Report(unsigned); /*9fc19*/
};
typedef char CompilerTypeConversion_prefix[(offsetof(CompilerTypeConversion, mode) == 4 &&
    offsetof(CompilerTypeConversion, input) == 8 && offsetof(CompilerTypeConversion, source_type) == 12 &&
    offsetof(CompilerTypeConversion, target_type) == 16 && offsetof(CompilerTypeConversion, source_rank) == 20 &&
    offsetof(CompilerTypeConversion, target_rank) == 24 && offsetof(CompilerTypeConversion, state) == 28 &&
    offsetof(CompilerTypeConversion, build) == 32) ? 1 : -1];
#ifdef __linux__
typedef char CompilerTypeConversion_layout[(sizeof(CompilerTypeConversion) == 96 &&
    sizeof(CompilerTypeConversion::Builder) == 8 && offsetof(CompilerTypeConversion, build_constant) == 0x28 &&
    offsetof(CompilerTypeConversion, diagnostic) == 0x34 && offsetof(CompilerTypeConversion, auxiliary_diagnostic) == 0x38 && offsetof(CompilerTypeConversion, complexity) == 0x44 &&
    offsetof(CompilerTypeConversion, context_symbol) == 0x50 && offsetof(CompilerTypeConversion, context_kind) == 0x54 &&
    offsetof(CompilerTypeConversion, flags) == 0x5c) ? 1 : -1];
#else
typedef char CompilerTypeConversion_layout[(sizeof(CompilerTypeConversion) == 88 &&
    sizeof(CompilerTypeConversion::Builder) == 4 && offsetof(CompilerTypeConversion, build_constant) == 0x24 &&
    offsetof(CompilerTypeConversion, diagnostic) == 0x2c && offsetof(CompilerTypeConversion, auxiliary_diagnostic) == 0x30 && offsetof(CompilerTypeConversion, complexity) == 0x3c &&
    offsetof(CompilerTypeConversion, context_symbol) == 0x48 && offsetof(CompilerTypeConversion, context_kind) == 0x4c &&
    offsetof(CompilerTypeConversion, flags) == 0x54) ? 1 : -1];
#endif
extern "C" {
unsigned __cdecl conversion_complexity(CompilerType *, CompilerExpression *); /*9dd3b*/
CompilerExpression *__cdecl evaluate_typed_conversion(unsigned, CompilerType *, CompilerExpression *); /*9e0bb*/
void __cdecl reconcile_typed_conversion(unsigned, CompilerExpression *, CompilerExpression *, CompilerType *); /*9fc73*/
}
extern "C" {
CompilerExpression *__cdecl resolve_constant_reference(CompilerExpression *);
CompilerExpression *__cdecl coerce_symbol_initializer(CompilerSymbol *, CompilerExpression *); /*1c07a*/
int __cdecl classify_symbol_initializer(CompilerSymbol *, CompilerExpression *); /*1c1b4*/
int __cdecl can_inline_initializer(CompilerSymbol *); /*1c040*/
CompilerExpression *__cdecl symbol_expression(CompilerSymbol *, unsigned);
int __cdecl assign_symbol_expression(CompilerExpression *, CompilerExpression *); /*1b938*/
void __cdecl finish_initializer_reference(CompilerSymbol *); /*1c103*/
void __cdecl emit_local_initializer(CompilerSymbol *, CompilerExpression *); /*42ee7*/
unsigned __cdecl record_constant_initializer(CompilerSymbol *, CompilerExpression *); /*48b45*/
int __cdecl prepare_static_initializer(CompilerSymbol *, CompilerExpression **); /*2d4bc*/
void __cdecl begin_static_initializer(CompilerSymbol *); /*42fae*/
CompilerExpression *__cdecl unwrap_return_expression(CompilerExpression *);
void __cdecl annotate_initializer(CompilerSymbol *, CompilerExpression *, OutputBuffer *); /*1ce35*/
void __cdecl initialize_runtime_reference(CompilerSymbol *, CompilerExpression *); /*8be55*/
unsigned __cdecl calculate_initializer_size(CompilerSymbol *, CompilerExpression *); /*48abd*/
void __cdecl emit_initializer_reserve(unsigned); /*48bce*/
}

extern "C" void __cdecl initialize_symbol_value(CompilerSymbol *input, CompilerExpression *expression)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    CompilerExpression *value;
    CompilerType *type;
    int classified;
    if (!input || (input->scope_flags & 4)) return;
    if (expression && expression->kind == 0xee)
        expression = expand_initializer_expression(0x74, expression, 0);
    if (check_symbol_definition(input)) {
        emit_local_initializer(input, expression);
        return;
    }
    if (!expression) return;
    expression = resolve_constant_reference(expression);
    if (expression->kind >= PARSER_TOKEN_KIND_COUNT) { __asm { int 3 } }
    if (parser_token_classes[expression->kind] == 0x60) {
        if (expression->value.operands.second) compiler_warning(0x4e);
        expression = expression->value.operands.first;
    }
    expression = coerce_symbol_initializer(input, expression);
    if (!expression) return;
    classified = classify_symbol_initializer(input, expression);
    if (classified)
        expression->value.counted_control.count = (WORD)record_constant_initializer(input, expression);
    if (!(frontend_data.compiler_flags & 0x100000)) {
        if (!prepare_static_initializer(input, &expression)) {
            if (symbol->member_flags & 2) {
                compiler_warning(0x1e2, input);
                return;
            }
            begin_static_initializer(input);
            frontend_bss.initializer_pending_state = 1;
            frontend_bss.initializer_saved_value_state = frontend_bss.initializer_value_state;
        }
    }
    if (frontend_data.compiler_flags & 0x100000) {
        if (can_inline_initializer(input)) goto assign_value;
    }
    type = symbol->type;
    if (type->shape) {
        if ((input->qualifiers & 4) && !classified) {
            if (expression->kind == 7 && !type->shape->base) {
                int permitted;
                if (expression->value.control.flags & 1)
                    permitted = (type->flags & 0x2fdf) == 0x41;
                else permitted = (type->flags & 0xefdf) == 0x4042;
                if (permitted) { compiler_warning(0x35, input); return; }
            }
            compiler_warning(0x4b, input);
            return;
        }
    } else if (type->flags & 0x400) {
        if (frontend_data.compiler_flags & 0x100000) {
            input->attributes &= ~4;
            goto assign_value;
        }
        compiler_warning(0x4a, input, type);
        return;
    }
    value = unwrap_return_expression(expression);
    if (frontend_data.compiler_flags & 0x100000)
        initialize_runtime_reference(input, expression);
    else annotate_initializer(input, value, &frontend_bss.output_buffers[4]); /* C0840 */
    if (!(frontend_data.compiler_flags & 0x100000) &&
        !classify_symbol_initializer(input, value)) {
        CompilerTypeConversion conversion(0x74, expression, symbol->type);
        if (!conversion.IsValid()) conversion.Report(1);
    } else {
        frontend_bss.initializer_zero_bytes = calculate_initializer_size(input, value);
        if (frontend_bss.initializer_zero_bytes) emit_initializer_reserve(0);
    }
    finish_initializer_reference(input);
    return;
assign_value:
    assign_symbol_expression(symbol_expression(input, 0), expression);
    finish_initializer_reference(input);
}
extern "C" CompilerExpression *__cdecl coerce_symbol_initializer(CompilerSymbol *input, CompilerExpression *expression)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    if (expression && (expression->flags & 7) == 3) {
        CompilerType *type = symbol->type;
        CompilerTypeShape *shape = type->shape;
        int keep_value = shape ? (!shape->base && (shape->flags & 1) &&
            (shape->attributes & 0x100)) : ((type->flags & 0x240) && (type->attributes & 0x100));
        if (keep_value) {
            ((CompilerStoredConstantSymbol *)input)->value.integer = expression->value.integer;
            input->qualifiers |= 0x40;
        }
    }
    if (expression && !(expression->flags & 7) && expression->kind == 0x98) {
        compiler_warning(0x113, expression->value.pointer);
        return NULL;
    }
    CompilerExpression *result = convert_assignment_operand(symbol->type, expression);
    return result ? result : diagnose_parser_operand(0x74, expression);
}
extern "C" int __cdecl classify_symbol_initializer(CompilerSymbol *input, CompilerExpression *expression)
{
    if (!expression || expression->kind != 7 || !(input->qualifiers & 4)) return 0;
    CompilerType *type = ((CompilerIrSymbol *)input)->type;
    unsigned literal_kind = expression->value.control.flags & 1;
    if ((type->flags & 0x2fdf) != 0x41 + literal_kind) return 0;
    return type->shape->base == NULL;
}
extern "C" int __cdecl can_inline_initializer(CompilerSymbol *) { __asm { int 3 } return 0; }
extern "C" CompilerExpression *__cdecl make_scalar_comparison(unsigned, CompilerExpression *, CompilerExpression *);
extern "C" int __cdecl report_assignment_conversion(unsigned, unsigned, unsigned, CompilerType *, CompilerType *);
/* 1b938..1ba31, 60304..603ac. Retain the assignment conversion's
 * actual success result; temporary cleanup is conditional on current state. */
extern "C" int __cdecl assign_symbol_expression(CompilerExpression *target,
    CompilerExpression *expression)
{
    if (!target || !expression) return 0;
    CompilerType *target_type = target->type;
    CompilerType *source_type = expression->type;
    CompilerTypeShape *target_shape = target_type ? target_type->shape : NULL;
    CompilerTypeShape *source_shape = source_type ? source_type->shape : NULL;
    if ((target_shape && (target_shape->flags & 0x77) == 1 && (target_shape->attributes & 3)) ||
        (source_shape && (source_shape->flags & 0x77) == 1 && (source_shape->attributes & 3))) {
        unsigned complexity = conversion_complexity(target_type, expression);
        if (complexity >= 0x400) {
            if (report_assignment_conversion(0x74, 2, complexity, target_type, source_type)) return 0;
            if (target_type && source_type) compiler_warning(0x1b8, 0x74, source_type, target_type);
            else compiler_warning(0x250, target_type);
            return 0;
        }
        expression = evaluate_typed_conversion(0x74, target_type, expression);
        expression = convert_typed_expression(target_type, expression);
        if (!expression) return 0;
        target = convert_return_temporary(target);
        expression = make_scalar_comparison(0x74, target, expression);
    } else {
        CompilerTypeConversion conversion((frontend_data.source_mode_flags & 0x80000) ? 2 : 1,
            0x74, expression, target_type);
        if (!conversion.IsValid()) { conversion.Report(1); return 0; }
        if (target->kind == 6) conversion.SetContext(3, target->value.symbol);
        else conversion.SetContext(6, NULL);
        expression = conversion.Build(target);
    }
    if (frontend_data.force_statement_check || frontend_data.expression_target)
        expression = check_statement_expression(expression, 2);
    emit_statement_expression(expression);
    return 1;
}
extern "C" int __cdecl report_assignment_conversion(unsigned, unsigned, unsigned, CompilerType *, CompilerType *) { __asm { int 3 } return 0; }
extern "C" {
void __cdecl finish_initializer_context(void); /*1bc97*/
void __cdecl finish_static_initializer(void); /*2d305*/
void __cdecl record_dynamic_initializer(CompilerSymbol *); /*47c59*/
void __cdecl emit_label(CompilerExpression *);
}
extern "C" void __cdecl finish_initializer_reference(CompilerSymbol *input)
{
    if (frontend_bss.initializer_pending_state) {
        frontend_bss.initializer_zero_bytes = (unsigned)(frontend_bss.initializer_value_state -
            frontend_bss.initializer_saved_value_state);
        if (frontend_bss.initializer_zero_bytes) {
            frontend_data.compiler_flags &= ~0x100000U;
            emit_initializer_reserve(0);
            frontend_data.compiler_flags |= 0x100000;
            frontend_bss.initializer_value_state -= (unsigned)frontend_bss.initializer_zero_bytes;
        } else if (!frontend_bss.initializer_value_state) input->attributes &= ~4;
    }
    if ((frontend_data.compiler_flags & 0x20000) && !(frontend_data.compiler_flags & 0x20) &&
        (((CompilerIrSymbol *)input)->storage_class & 7) == 3 &&
        (frontend_data.compiler_flags & 0x100000)) record_dynamic_initializer(input);
    if (frontend_bss.initializer_end_label) {
        emit_label(frontend_bss.initializer_end_label);
        frontend_bss.initializer_end_label = NULL;
    }
    frontend_data.last_initializer_symbol = NULL;
    if (frontend_data.source_mode_flags & 0x4000) {
        frontend_data.backend->Slot08();
        frontend_data.source_mode_flags &= ~0x4000U;
    }
    if (!(frontend_data.compiler_flags & 0x100000) || (frontend_data.compiler_flags & 0x40000))
        finish_static_initializer();
    finish_initializer_context();
}
extern "C" void __cdecl pop_block_output(CompilerWorkLink **); /*17f95*/
extern "C" void __cdecl finish_initializer_context(void)
{
    --frontend_data.initializer_context_count;
    if (frontend_bss.initializer_context.symbol) {
        frontend_bss.initializer_context.source_state = frontend_bss.initializer_context.state_index;
        frontend_data.parameter_work_kind = frontend_bss.initializer_context.head->value.kind;
        pop_block_output(&frontend_bss.initializer_context.head);
        frontend_bss.initializer_context.symbol = NULL;
    }
}
extern "C" void __cdecl emit_cleanup_expression(CompilerExpression *);
extern "C" void __cdecl finish_symbol_initializer(CompilerSymbol *untyped)
{
    CompilerIrSymbol *input = (CompilerIrSymbol *)untyped;
    CompilerParameterWorkInput frame;
    CompilerGlobalSymbolView *owner;
    CompilerExpression *expression;
    unsigned classification, storage;
    int index;
    memset(&frame, 0, sizeof(frame));
    frame.kind = 14;
    if (frontend_data.expression_work_reset_pending) return;
    if (input) {
        frame.flags = (BYTE)((input->attributes >> 2) & 1);
        classification = get_type_classification(input->type);
        if (classification & 0x82) {
            owner = (CompilerGlobalSymbolView *)DECL_OWNER(input->type);
            if ((input->attributes & 4) || !(owner->class_detail->generated_member_flags & 2))
                frame.flags |= 1;
            if (!(owner->type_flags & 0x20)) {
                compiler_warning(0x106, input);
                frame.flags &= ~2;
            } else if (owner->class_detail->generated_member_flags & 0x10) {
                frame.flags &= ~2;
            } else {
                storage = input->storage_class & 7;
                if (storage < 4 || storage > 6) frame.flags &= ~2;
                else {
                    frame.flags |= 2;
                    if (frontend_bss.initializer_cleanup_owner == (CompilerSymbol *)owner) {
                        expression = make_resolved_symbol_expression(input, 0);
                        expression = prepare_parameter_expression(expression, 4);
                        make_parameter_operation((CompilerSymbol *)owner, expression, 3, 1, 0);
                    } else if (!frontend_bss.error_count) {
                        frame.state_index = frontend_data.parameter_work_index;
                        frame.source_state = frontend_bss.initializer_context.source_state;
                        frame.active = 1;
                        if (classification & 2) {
                            index = (int)frontend_data.parameter_work_index;
                            while (index > (int)frontend_bss.initializer_context.source_state) {
                                if (!frontend_data.parameter_work_states || index < 0 ||
                                    (unsigned)index >= frontend_data.parameter_work_capacity) { __asm { int 3 } }
                                if (frontend_data.parameter_work_states[index].active == 7)
                                    frontend_data.parameter_work_states[index].active = 1;
                                --index;
                            }
                            frame.active = 0;
                        }
                    }
                    frontend_bss.initializer_cleanup_owner = NULL;
                }
            }
        } else frame.flags &= ~2;
    }
    if (frontend_data.expression_work_nodes && frontend_data.expression_work_nodes->kind == 13)
        emit_cleanup_expression(check_statement_expression(NULL, 2));
    if (frame.flags & 3) {
        frame.symbol = input;
        push_expression_work(&frame, 32);
    }
    frontend_data.expression_target = NULL;
    frontend_data.expression_target_active = 0;
    frontend_data.force_statement_check = 0;
}
extern "C" void __cdecl finish_static_initializer(void)
{
    if (frontend_data.source_mode_flags & 0x800) return;
    OutputBuffer *output = &frontend_bss.output_buffers[4]; /* original C0840 */
    int room = output->cursor + 2 <= output->data + output->capacity ? 1 : output->Grow(2);
    output->ok &= room;
    if (output->ok) *output->cursor++ = 7;
}
extern "C" void __cdecl record_dynamic_initializer(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl emit_local_initializer(CompilerSymbol *, CompilerExpression *) { __asm { int 3 } }
extern "C" unsigned __cdecl record_constant_initializer(CompilerSymbol *, CompilerExpression *) { __asm { int 3 } return 0; }
extern "C" int __cdecl decode_static_initializer(CompilerExpression *, CompilerSymbol **, unsigned *);
extern "C" int __cdecl prepare_static_initializer(CompilerSymbol *input, CompilerExpression **expression)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    CompilerExpression *source = unwrap_return_expression(*expression), *destination, *value;
    CompilerSymbol *reference = NULL;
    unsigned offset = 0;
    int special = 0;
    if ((source->flags & 7) == 5) {
        special = 1;
        if (input->kind >= 1 && input->kind <= 14 && input->kind != 5 && symbol->type) {
            int index = (short)symbol->type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            CompilerSymbol *owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
            if (owner && (owner->type_flags & 0x10)) special = 0;
        }
    }
    CompilerTypeConversion conversion(0x74, *expression, symbol->type);
    if (!classify_symbol_initializer(input, source) && conversion.IsValid()) {
        destination = symbol_expression(input, 0);
        if (destination->kind == 6 && ((*expression)->flags & 7) == 3 &&
            destination->type && destination->type->shape && (destination->type->shape->flags & 0x40)) {
            conversion.SetContext(3, destination->value.symbol);
            *expression = conversion.Build(destination);
        } else *expression = conversion.Build(NULL);
    } else if (special) *expression = source;
    value = *expression;
    if (!value) return 0;
    if (value->type != symbol->type) {
        CompilerTypeShape *shape = symbol->type->shape;
        if (shape) {
            if (shape->flags & 0x30) return 0;
            if (shape->flags & 0x40) return 0;
            if ((shape->flags & 1) && (shape->attributes & 3) == 3 &&
                ((CompilerIrSymbol *)shape->payload.symbol)->type->shape && !(value->flags & 0x80)) {
                if ((value->flags & 7) != 3 || (value->type->flags & 0x2fdf) != 0x63) return 0;
            }
        }
    }
    if (special || (value->flags & 7) == 3) return 1;
    if (!decode_static_initializer(value, &reference, &offset)) return 0;
    if (!reference) return 1;
    CompilerIrSymbol *base = (CompilerIrSymbol *)reference;
    if (base->kind == 14 && (base->encoded_index & 0xffffff) == 6) return 0;
    if ((base->reference_flags & 1) && resolve_symbol_reference(reference)->kind != 14) return 0;
    if ((base->storage_class & 7) == 4) {
        if (input->kind == 3 || input->kind == 2 || (symbol->storage_class & 7) == 3) return 0;
    }
    if ((base->member_flags & 2) && base->kind != 7 && base->kind != 8) return 0;
    return !(base->member_flags & 8);
}
extern "C" int __cdecl decode_static_initializer(CompilerExpression *, CompilerSymbol **, unsigned *) { __asm { int 3 } return 0; }
extern "C" void __cdecl begin_static_initializer(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl initialize_runtime_reference(CompilerSymbol *, CompilerExpression *) { __asm { int 3 } }
extern "C" unsigned __cdecl calculate_initializer_size(CompilerSymbol *, CompilerExpression *) { __asm { int 3 } return 0; }
extern "C" void __cdecl emit_initializer_reserve(unsigned) { __asm { int 3 } }
RECON_THISCALL CompilerTypeConversion::CompilerTypeConversion(unsigned operation,
    CompilerExpression *expression, CompilerType *target)
{
    code = (BYTE)operation;
    switch (code) {
    case 0x64: mode = 4; break;
    case 0x74: mode = 1; break;
    case 0xce: mode = 6; break;
    case 0xdc: mode = 7; break;
    case 0xe3: mode = 8; break;
    case 0xe4: mode = 9; break;
    default: mode = 2; break;
    }
    input = expression;
    source_type = expression ? expression->type : NULL;
    target_type = target;
    source_rank = target_rank = state = 0;
    build = build_constant = NULL;
    memset(unmodeled_28_2b, 0, sizeof(unmodeled_28_2b));
    diagnostic = auxiliary_diagnostic = 0;
    memset(unmodeled_34_3b, 0, sizeof(unmodeled_34_3b));
    complexity = 0;
    memset(unmodeled_40_47, 0, sizeof(unmodeled_40_47));
    context_symbol = NULL; context_kind = 0;
    memset(unmodeled_50_53, 0, sizeof(unmodeled_50_53));
    flags = 0;
    Analyze();
}
extern "C" CompilerExpression *__cdecl make_typed_expression(unsigned code, CompilerType *target,
    CompilerExpression *expression, unsigned mode)
{
    CompilerTypeShape *shape;
    CompilerExpression *evaluated = NULL, *temporary, *result = NULL;
    int special = 0;
    if (target && (shape = target->shape) && (shape->flags & 0x77) == 1 && (shape->attributes & 3))
        special = 1;
    else if (expression->type && (shape = expression->type->shape) &&
        (shape->flags & 0x77) == 1 && (shape->attributes & 3)) special = 1;
    if (frontend_data.conversion_evaluation_mode || special) {
        if (special) {
            if (conversion_complexity(target, expression) < 0x400)
                return evaluate_typed_conversion(code, target, expression);
            return expression;
        }
        frontend_data.compiler_mode_flags |= 0x20000;
        expression = instantiate_parser_expression(expression, frontend_data.constant_arena_index);
        temporary = instantiate_parser_expression(expression, 5);
        if (conversion_complexity(target, temporary) < 0x400) {
            ++frontend_data.parser_classification_state;
            evaluated = evaluate_typed_conversion(code, target, temporary);
            --frontend_data.parser_classification_state;
        }
        frontend_data.compiler_mode_flags &= ~0x20000U;
    }
    CompilerTypeConversion conversion(mode ? mode : 2, code, expression, target);
    if (conversion.IsValid()) result = conversion.Build(0);
    else conversion.Report(1);
    if (frontend_data.conversion_evaluation_mode)
        reconcile_typed_conversion(code, result, evaluated, target);
    return result;
}
RECON_THISCALL CompilerTypeConversion::CompilerTypeConversion(unsigned options, unsigned operation,
    CompilerExpression *expression, CompilerType *target)
{
    code = (BYTE)operation;
    mode = options & 0xf;
    input = expression;
    source_type = expression ? expression->type : NULL;
    target_type = target;
    source_rank = target_rank = state = 0;
    build = build_constant = NULL;
    memset(unmodeled_28_2b, 0, sizeof(unmodeled_28_2b));
    diagnostic = 0;
    auxiliary_diagnostic = 0;
    memset(unmodeled_34_3b, 0, sizeof(unmodeled_34_3b));
    complexity = 0;
    memset(unmodeled_40_47, 0, sizeof(unmodeled_40_47));
    context_symbol = NULL; context_kind = 0;
    memset(unmodeled_50_53, 0, sizeof(unmodeled_50_53));
    flags = ((options & 0x200) >> 4) | ((options & 0x120) == 0x120 ? 0x10 : 0) |
        ((options & 0xe0) >> 4);
    Analyze();
}
/* The classifier's established rank domain. Numbered shape categories retain
 * their actual constructor codes where a broader semantic name is unknown. */
enum CompilerConversionRank {
    CONVERSION_UNKNOWN = 0,
    CONVERSION_PRIMITIVE_10 = 1,
    CONVERSION_INTEGRAL = 2,
    CONVERSION_VOID = 3,
    CONVERSION_FLOATING = 4,
    CONVERSION_ENUM = 5,
    CONVERSION_RECORD = 6,
    CONVERSION_BOOL = 7,
    CONVERSION_POINTER = 8,
    CONVERSION_ARRAY = 9,
    CONVERSION_SHAPE_03 = 10,
    CONVERSION_FUNCTION = 11,
    CONVERSION_SHAPE_10 = 12,
    CONVERSION_SHAPE_20 = 13,
    CONVERSION_REFERENCE = 14,
    CONVERSION_SHAPE_44 = 15,
};
typedef void (RECON_THISCALL CompilerTypeConversion::*CompilerConversionAnalysis)(void);
/* Source rank selects the row, target rank selects the column. This logical
 * method matrix omits the original record's unused diagnostic DWORD. Each
 * method is a genuine independent target; no method-body credit follows
 * from recovering its relation. */
static const CompilerConversionAnalysis conversion_analysis[16][16] = {
    { /* CONVERSION_UNKNOWN */
        &CompilerTypeConversion::AnalyzeUnknown, &CompilerTypeConversion::AnalyzeUnknown, &CompilerTypeConversion::AnalyzeUnknown, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeUnknown, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeUnknownIndirection, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeUnknownIndirection, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeUnknownIndirection,
    },
    { /* CONVERSION_PRIMITIVE_10 */
        &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::AnalyzeFromPrimitive10,
        &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::AnalyzeFromPrimitive10, &CompilerTypeConversion::AnalyzeFromPrimitive10,
    },
    { /* CONVERSION_INTEGRAL */
        &CompilerTypeConversion::AnalyzeIntegralUnknown, &CompilerTypeConversion::AnalyzePrimitive10, &CompilerTypeConversion::AnalyzeIntegral, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::AnalyzeToFloating, &CompilerTypeConversion::AnalyzeEnumTarget, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeIntegralPointer, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeIntegralPointer, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::AnalyzeIntegralMember, &CompilerTypeConversion::AnalyzeIntegralMember, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::Reject,
    },
    { /* CONVERSION_VOID */
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject,
    },
    { /* CONVERSION_FLOATING */
        &CompilerTypeConversion::AnalyzeFloatingUnknown, &CompilerTypeConversion::AnalyzePrimitive10, &CompilerTypeConversion::AnalyzeFloatingIntegral, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::AnalyzeFloating, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::Reject,
    },
    { /* CONVERSION_ENUM */
        &CompilerTypeConversion::AnalyzeIntegralUnknown, &CompilerTypeConversion::AnalyzePrimitive10, &CompilerTypeConversion::AnalyzeIntegral, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::AnalyzeToFloating, &CompilerTypeConversion::AnalyzeEnumTarget, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeEnumPointer, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeEnumPointer, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::Reject,
    },
    { /* CONVERSION_RECORD */
        &CompilerTypeConversion::AnalyzeIndirectionUnknown, &CompilerTypeConversion::AnalyzePrimitive10, &CompilerTypeConversion::AnalyzeRecordConversion, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::AnalyzeRecordConversion, &CompilerTypeConversion::AnalyzeRecordConversion, &CompilerTypeConversion::AnalyzeSameRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeRecordConversion, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeRecordConversion, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::AnalyzeRecordConversion, &CompilerTypeConversion::AnalyzeRecordConversion, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::AnalyzeRecordConversion,
    },
    { /* CONVERSION_BOOL */
        &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean, &CompilerTypeConversion::RejectBoolean,
    },
    { /* CONVERSION_POINTER */
        &CompilerTypeConversion::AnalyzeIndirectionUnknown, &CompilerTypeConversion::AnalyzePrimitive10, &CompilerTypeConversion::AnalyzePointerIntegral, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeSameIndirection, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzePointerShape03, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::Reject,
    },
    { /* CONVERSION_ARRAY */
        &CompilerTypeConversion::AnalyzeArray, &CompilerTypeConversion::AnalyzeArray, &CompilerTypeConversion::AnalyzeArray, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeArray, &CompilerTypeConversion::AnalyzeArrayPair, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::Reject,
    },
    { /* CONVERSION_SHAPE_03 */
        &CompilerTypeConversion::AnalyzeIndirectionUnknown, &CompilerTypeConversion::AnalyzePrimitive10, &CompilerTypeConversion::AnalyzePointerIntegral, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeShape03Pointer, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeShape03Pair, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::AnalyzeShape03Reference,
    },
    { /* CONVERSION_FUNCTION */
        &CompilerTypeConversion::AnalyzeFunction, &CompilerTypeConversion::AnalyzeFunction, &CompilerTypeConversion::AnalyzeFunction, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeFunction, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeFunction, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeFunction, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::AnalyzeReference,
    },
    { /* CONVERSION_SHAPE_10 */
        &CompilerTypeConversion::AnalyzeIndirectionUnknown, &CompilerTypeConversion::AnalyzePrimitive10, &CompilerTypeConversion::AnalyzeMemberIntegral, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::AnalyzeSameIndirection, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::Reject,
    },
    { /* CONVERSION_SHAPE_20 */
        &CompilerTypeConversion::AnalyzeIndirectionUnknown, &CompilerTypeConversion::AnalyzePrimitive10, &CompilerTypeConversion::AnalyzeMemberIntegral, &CompilerTypeConversion::AnalyzeVoid,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeRecord, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeSameIndirection, &CompilerTypeConversion::AnalyzeReference, &CompilerTypeConversion::Reject,
    },
    { /* CONVERSION_REFERENCE */
        &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference,
        &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference,
    },
    { /* CONVERSION_SHAPE_44 */
        &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference,
        &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::RejectBoolean,
        &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::Reject, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::Reject,
        &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference, &CompilerTypeConversion::AnalyzeFromReference,
    },
};
void RECON_THISCALL CompilerTypeConversion::Analyze(void)
{
    source_rank = source_type->ConversionRank();
    target_rank = target_type->ConversionRank();
    if (source_rank >= 16 || target_rank >= 16) { __asm { int 3 } }
    (this->*conversion_analysis[source_rank][target_rank])();
    if (state == 0x70 || (state == 0x60 && complexity >= 0x400)) flags &= ~1U;
    else flags |= 1;
}

int RECON_THISCALL CompilerTypeConversion::IsValid(void) { return -(int)(flags & 1); }
extern "C" int __cdecl should_report_diagnostic(unsigned, unsigned);
extern "C" void __cdecl print_diagnostic_auxiliary(unsigned, ...);
CompilerExpression *RECON_THISCALL CompilerTypeConversion::Build(CompilerExpression *destination)
{
    if (auxiliary_diagnostic && should_report_diagnostic(4, 0xef)) {
        report_language_diagnostic(4, 0xef, (unsigned)code, source_type, target_type);
        print_diagnostic_auxiliary(auxiliary_diagnostic);
    }
    if (build_constant != 0 && (input->flags & 7) == 3) return (this->*build_constant)(destination);
    return (this->*build)(destination);
}
void RECON_THISCALL CompilerTypeConversion::Report(unsigned) { __asm { int 3 } }
extern "C" unsigned __cdecl conversion_complexity(CompilerType *, CompilerExpression *) { __asm { int 3 } return 0; }
extern "C" CompilerExpression *__cdecl evaluate_typed_conversion(unsigned, CompilerType *, CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl reconcile_typed_conversion(unsigned, CompilerExpression *, CompilerExpression *, CompilerType *) { __asm { int 3 } }

extern "C" int __cdecl is_null_pointer_constant(CompilerExpression *); /*1c226*/
#define CONVERSION_WIDTH(f) (((f) & 0x340) || !((f) & 0xfff0) ? \
    (int)compiler_primitive_type_bytes[(f) & 15] : (int)((f) & 15))
void RECON_THISCALL CompilerTypeConversion::AnalyzeIntegral(void)
{
    WORD source, target;
    unsigned difference;
    int source_bytes;
    if ((int)mode >= 7 && (int)mode <= 9) {
        state = 0x70;
        diagnostic = 0x1773;
        return;
    }
    source = source_type->flags;
    target = target_type->flags;
    if (!target_type->shape && (target & 0x2fdf) == 0x40 &&
        (source_type->shape || (source & 0x2fdf) != 0x40) &&
        (!input || !is_null_pointer_constant(input))) {
        build = &CompilerTypeConversion::BuildNullPointer;
    } else {
        build = &CompilerTypeConversion::BuildScalar;
        build_constant = &CompilerTypeConversion::BuildScalarConstant;
    }
    difference = source ^ target;
    if (!(difference & 0xefdf)) {
        state = 0;
        diagnostic = 0x17d4;
        return;
    }
    if ((target & 0x4f) != 0x43 || (target & 0x4000)) goto ordinary_conversion;
    if (frontend_data.options.unsigned_char &&
        ((target & 0x2fdf) != 0x41 || !(target & 0x8000))) goto ordinary_conversion;
    source_bytes = CONVERSION_WIDTH(source);
    if (source_bytes >= 4 && !(source & 0x200)) {
        if (CONVERSION_WIDTH(source) != CONVERSION_WIDTH(target) || (difference & 0xc000))
            goto ordinary_conversion;
    }
    state = 0x10;
    diagnostic = 0x17d5;
    return;
ordinary_conversion:
    state = 0x20;
    diagnostic = 0x17d7;
}
#undef CONVERSION_WIDTH
extern "C" int __cdecl is_null_pointer_constant(CompilerExpression *) { __asm { int 3 } return 0; }
/* 15b8f..15c4c, 5d0f6..5d14b: constant values take the existing
 * constant builder; other scalar conversions retain width-loss diagnostics. */
CompilerExpression *RECON_THISCALL CompilerTypeConversion::BuildScalar(CompilerExpression *destination)
{
    CompilerExpression *resolved = resolve_constant_reference(input);
    if ((resolved->flags & 7) == 3) {
        input = resolved;
        return BuildScalarConstant(destination);
    }
    if (ShouldCheckConstant()) {
        WORD source = source_type->flags, target = target_type->flags;
        unsigned target_bytes, source_bytes;
        if ((target & 0x340) || !(target & 0x2fd0))
            target_bytes = (int)compiler_primitive_type_bytes[target & 15];
        else target_bytes = target & 15;
        if ((source & 0x340) || !(source & 0x2fd0))
            source_bytes = (int)compiler_primitive_type_bytes[source & 15];
        else source_bytes = source & 15;
        if (target_bytes < source_bytes) {
            unsigned severity;
            if ((source & 0x6f) == 0x63 || (!source_type->shape && (source & 0x200))) severity = 4;
            else severity = 3;
            report_language_diagnostic(severity, 0xf4, (unsigned)code, source_type, target_type);
        }
    }
    return BuildScalarValue(destination);
}
/* 16258..162d9, 5d2a9..5d2ba. Preserve same-type expressions unless
 * the explicit mode/boolean case requires a conversion node. */
CompilerExpression *RECON_THISCALL CompilerTypeConversion::BuildScalarValue(CompilerExpression *destination)
{
    CompilerExpression *result = input;
    if ((input->flags & 7) == 3) input->type = target_type;
    else if (target_type != source_type || (frontend_data.compiler_mode_flags & 1)) {
        result = make_immediate_expression(0x64, target_type, input);
        result->constant_flags = (result->constant_flags & ~1U) | (input->constant_flags & 1);
    } else if (!target_type->shape && (target_type->flags & 0x2fdf) == 0x40 && code == 0x60) {
        result = make_immediate_expression(0x64, target_type, input);
    }
    return destination ? WrapResult(destination, result) : result;
}
extern "C" {
__int64 __cdecl convert_constant_value(BYTE, CompilerExpression *, unsigned, unsigned, WORD); /*1171e*/
int __cdecl constant_conversion_overflows(WORD, WORD, WORD, __int64); /*11a63*/
}
CompilerExpression *RECON_THISCALL CompilerTypeConversion::BuildScalarConstant(CompilerExpression *destination)
{
    WORD source, target, unsigned_source;
    unsigned width;
    __int64 value;
    if (ShouldCheckConstant()) {
        value = input->value.integer;
        source = source_type->flags;
        target = target_type->flags;
        if ((target & 0x4000) && !(source & 0x4000) &&
            !(frontend_data.options.unsigned_char && (source & 0x2fdf) == 0x41 && !(source & 0x8000)) &&
            value < 0)
            report_language_diagnostic(4, 0xf5, (unsigned)code, source_type, target_type);
        if (target != source) {
            unsigned_source = (source & 0x4000) ? 1 : 0;
            if (!unsigned_source && frontend_data.options.unsigned_char &&
                (source & 0x2fdf) == 0x41 && !(source & 0x8000)) unsigned_source = 1;
            width = target & 15;
            if ((target & 0x340) || !(target & 0xfff0)) width = compiler_primitive_type_bytes[width];
            if (constant_conversion_overflows((WORD)width, target & 0x4000, unsigned_source, value))
                report_language_diagnostic(2, 0x131, (unsigned)code, source_type, target_type);
        }
    }
    input->value.integer = convert_constant_value(code, input, 0, 0, target_type->flags);
    input->type = target_type;
    if (frontend_data.compiler_mode_flags & 1) input->flags &= 0x7f;
    if (destination) return WrapResult(destination, input);
    return input;
}
int RECON_THISCALL CompilerTypeConversion::ShouldCheckConstant(void)
{
    if (frontend_data.options.warning_level < 2) return 0;
    return code == 0x34 || code == 0x3a || code == 0x47 || code == 0x4b ||
        code == 0x52 || code == 0x60 || code == 0x74 || code == 0x7d;
}
/* 1ba31..1ba56, 603ac..60406. Class destinations use the authentic
 * seven-slot object call; scalar destinations use the typed assignment node. */
CompilerExpression *RECON_THISCALL CompilerTypeConversion::WrapResult(
    CompilerExpression *destination, CompilerExpression *value)
{
    if (target_rank != 6) return make_binary_node(0x34, destination, value, target_type);
    unsigned mode = 1;
    if (context_symbol) {
        unsigned storage = ((CompilerIrSymbol *)context_symbol)->storage_class & 7;
        if (storage < 4 || storage > 6) mode = 0;
    }
    int index = (short)target_type->auxiliary;
    CompilerPointerArray *table = frontend_data.frontend_pointer_table;
    if (index < 0 || index >= table->count) { __asm { int 3 } }
    CompilerSymbol *owner = (CompilerSymbol *)table->items[index];
    CompilerExpression *arguments = append_expression_list(NULL, value, 2);
    return make_return_object_call(0, owner, destination, arguments, 0, 1, mode);
}
/* 1b723..1b734: two fields of the existing conversion owner, RET8. */
void RECON_THISCALL CompilerTypeConversion::SetContext(unsigned kind, CompilerSymbol *symbol)
{
    context_symbol = symbol;
    context_kind = kind;
}
/* Width calculation shared with the existing primitive-size consumers. */
#define CONSTANT_WIDTH(f) (((f) & 0x340) || !((f) & 0xfff0) ? \
    (int)compiler_primitive_type_bytes[(f) & 15] : (int)((f) & 15))
extern "C" __int64 __cdecl convert_constant_value(BYTE operation, CompilerExpression *expression,
    unsigned control, unsigned, WORD target)
{
    __int64 value = expression->value.integer, previous, normalized;
    CompilerType *source_type;
    WORD source, source_unsigned, target_unsigned;
    int width = (int)control, source_width;
    BYTE shift;
    /* The fourth supplied DWORD has no reader in this complete owner.
     * Keep its physical cdecl slot separate without interpreting it. */
    if (control & 1) return value;
    if (target & 0x40) width = CONSTANT_WIDTH(target);
    else if (target & 0x200) width = compiler_primitive_type_bytes[3];
    source_type = expression->type;
    source = source_type->flags;
    if (!source_type->shape) {
        if (source & 0x40) source_width = CONSTANT_WIDTH(source);
        else if (source & 0x200) source_width = compiler_primitive_type_bytes[3];
        else source_width = width;
        if (source_width < width) {
            previous = value;
            if (source_width == 2) value = (source & 0x4000) ? (__int64)(WORD)value : (__int64)(short)value;
            else if (source_width == 4) value = (source & 0x4000) ? (__int64)(DWORD)value : (__int64)(LONG)value;
            else if (source_width != 8 && source_width != 16) {
                if ((source & 0x4000) || (frontend_data.options.unsigned_char && !(source & 0x8000)))
                    value = (BYTE)value;
                else value = (signed char)value;
            }
            if (value != previous) {
                if (operation == 0x64) report_language_diagnostic(4, 0x136);
                else report_language_diagnostic(2, 0x135, (unsigned)operation);
            }
        }
    }
    source_width = CONSTANT_WIDTH(source);
    shift = (BYTE)((8 - source_width) * 8);
    /* The original CRT count is CL, and counts>=64 have defined helper
     * results. Keep those domains explicit instead of using undefined C
     * shift counts. Only the diagnostic check consumes this normalized
     * value; the final conversion below retains the original local value. */
    normalized = shift >= 64 ? 0 : (__int64)((unsigned __int64)value << shift);
    if (source & 0x4000) {
        normalized = shift >= 64 ? 0 : (__int64)((unsigned __int64)normalized >> shift);
        source_unsigned = 1;
    } else {
        normalized = shift >= 64 ? (normalized < 0 ? -1 : 0) : normalized >> shift;
        source_unsigned = 0;
        if (!frontend_data.options.language_extensions && frontend_data.options.unsigned_char &&
            (source & 0x2fdf) == 0x41 && !(source & 0x8000)) source_unsigned = 1;
    }
    target_unsigned = target & 0x4000;
    if (constant_conversion_overflows((WORD)CONSTANT_WIDTH(target), target_unsigned, source_unsigned, normalized)) {
        if (operation == 0x64) report_language_diagnostic(4, 0x136);
        else report_language_diagnostic(2, 0x135, (unsigned)operation);
    }
    if (width == 2) return target_unsigned ? (__int64)(WORD)value : (__int64)(short)value;
    if (width == 4) return target_unsigned ? (__int64)(DWORD)value : (__int64)(LONG)value;
    if (width == 8 || width == 16) return value;
    if (target_unsigned || (frontend_data.options.unsigned_char && !(source & 0x8000))) return (BYTE)value;
    return (signed char)value;
}
#undef CONSTANT_WIDTH
extern "C" int __cdecl constant_conversion_overflows(WORD bytes, WORD target_unsigned,
    WORD source_unsigned, __int64 value)
{
    BYTE shift = (BYTE)((unsigned)bytes * 8 - 1);
    unsigned __int64 lower, upper, magnitude;
    lower = shift >= 64 ? 0 : (~(unsigned __int64)0) << shift;
    upper = ~lower;
    if (target_unsigned || source_unsigned) {
        upper |= upper << 1;
        magnitude = (unsigned __int64)value;
        if (target_unsigned && !source_unsigned && value < 0)
            magnitude = 0 - magnitude;
        return magnitude > upper;
    }
    return value > (__int64)upper || value < (__int64)lower;
}
CompilerExpression *RECON_THISCALL CompilerTypeConversion::BuildNullPointer(CompilerExpression *) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerTypeConversion::AnalyzeIntegralPointer(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeReference(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeRecord(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeSameIndirection(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeRecordConversion(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzePointerIntegral(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeVoid(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeArray(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeArrayPair(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzePrimitive10(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeShape03Pair(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeIntegralMember(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeEnumTarget(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeSameRecord(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeEnumPointer(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::Reject(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzePointerShape03(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeUnknownIndirection(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeUnknown(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeShape03Pointer(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeIntegralUnknown(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeIndirectionUnknown(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeFloatingIntegral(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeFloating(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeToFloating(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeFromReference(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeFloatingUnknown(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::RejectBoolean(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeMemberIntegral(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeFunction(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeShape03Reference(void) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeConversion::AnalyzeFromPrimitive10(void) { __asm { int 3 } }
unsigned RECON_THISCALL CompilerType::ConversionRank(void)
{
    unsigned index;
    /* The original thiscall entry explicitly accepts NULL ECX. */
    if (!this) return 0;
    if (shape) {
        index = (BYTE)shape->flags & 0x77;
        if (index >= sizeof(frontend_bss.complex_type_rank)) { __asm { int 3 } }
        return frontend_bss.complex_type_rank[index];
    }
    index = (flags >> 5) & 0x7e;
    if (index >= sizeof(frontend_bss.primitive_type_rank)) { __asm { int 3 } }
    return frontend_bss.primitive_type_rank[index];
}

extern "C" CompilerExpression *__cdecl convert_typed_expression(CompilerType *target, CompilerExpression *expression)
{
    CompilerTypeShape *shape = target->shape;
    if (expression && shape && (shape->flags & 0x40))
        return prepare_parameter_expression(expression, 0xc);
    return expression;
}

extern "C" CompilerExpressionWork *__cdecl find_expression_work(CompilerExpressionWork *record,
    unsigned kind, unsigned required)
{
    unsigned partner = kind >= 15 && kind <= 54 ? paired_cleanup_kind[kind - 15] : 0;
    unsigned depth = 1;
    while (record) {
        if (record->kind == 1) {
            if (kind == 1) return record;
            break;
        }
        if (record->kind == kind) {
            if (--depth == 0) return record;
        } else if (record->kind == partner) ++depth;
        record = record->previous;
    }
    if (required && frontend_data.expression_work_recovery) {
        reset_expression_work();
        frontend_data.expression_work_reset_pending = 1;
        longjmp(*(jmp_buf *)frontend_data.expression_work_recovery, -1);
    }
    return NULL;
}

extern "C" {
void __cdecl finish_nested_block(void); /*5686*/
void __cdecl finish_block_cleanup(void); /*58a8*/
void __cdecl emit_function_epilogue(void); /*57a1*/
void __cdecl emit_special_function_epilogue(void); /*5842*/
}
extern "C" void __cdecl finish_compound_statement(void)
{
    CompilerScopeTable *scope;
    if (!frontend_bss.declaration_scope_mode) return;
    frontend_data.backend->Slot08();
    if (frontend_data.debug_module_kind == 4) {
        if (frontend_bss.declaration_scope_mode == 2)
            frontend_data.declaration_scope_number = 0;
    } else if (frontend_bss.declaration_scope_mode >= 2) {
        finish_nested_block();
        if (frontend_bss.declaration_scope_mode == 2) {
            if (frontend_data.function_result_type &&
                !(frontend_data.compiler_mode_flags & 0x800))
                emit_return_statement(NULL, 0);
            frontend_data.declaration_scope_number = 0;
            if (!(frontend_bss.current_function->type_flags & 0x30))
                emit_function_epilogue();
            else
                emit_special_function_epilogue();
        }
    }
    if (!(frontend_data.compiler_flags & 0x20)) {
        if (frontend_data.debug_module_kind == 4) {
            if (frontend_bss.declaration_scope_mode == 1)
                frontend_data.translation_unit->PopCurrentScope();
        } else {
            scope = frontend_data.translation_unit->current;
            if (frontend_bss.declaration_scope_mode != scope->level)
                report_compiler_error(0x1fa);
            frontend_data.translation_unit->PopCurrentScope();
        }
        finish_block_cleanup();
    }
    if (!frontend_bss.suppress_parser_codegen && frontend_data.debug_module_kind != 4) {
        if (!(frontend_data.compiler_flags & 0x20))
            frontend_data.translation_unit->FinishLocalScope(scope);
        write_ir_record(0x54, make_integer_constant(frontend_bss.declaration_scope_mode));
    }
    --frontend_bss.declaration_scope_mode;
    frontend_bss.global_output_active = frontend_bss.declaration_scope_mode;
}
extern "C" void __cdecl emit_cleanup_expression(CompilerExpression *); /*17643*/
extern "C" void __cdecl finish_nested_block(void)
{
    CompilerExpressionWork frame;
    CompilerExpressionWork *record;
    unsigned kind;
    int parameter_cleanup;
    if (frontend_data.expression_work_reset_pending) return;
    record = frontend_data.expression_work_nodes;
    if (record && record->kind == 15) {
        /* The source is the genuine20-byte block frame. */
        if ((record->flags >> 6) == 0) { __asm { int 3 } }
        frontend_data.parameter_work_index = ((CompilerBlockWorkInput *)record)->state_index;
        pop_expression_work();
        return;
    }
    /* Like the original, the non-block path requires a current work node. */
    kind = record->kind;
    parameter_cleanup = kind == 9 || kind == 11 || (kind >= 58 && kind <= 61);
    memset(&frame, 0, sizeof(frame));
    frame.kind = 16;
    push_expression_work(&frame, 16);
    if (parameter_cleanup) frontend_data.parameter_cleanup_active = 1;
    emit_cleanup_expression(build_expression_cleanup(NULL));
    if (parameter_cleanup) frontend_data.parameter_cleanup_active = 0;
}
extern "C" void __cdecl emit_cleanup_expression(CompilerExpression *expression)
{
    if (!expression) return;
    inspect_expression_cleanup(expression);
    emit_statement_expression(expression);
}
extern "C" void __cdecl finish_block_cleanup(void)
{
    if (frontend_bss.suppress_parser_codegen) return;
    while (frontend_bss.block_output_entries) {
        if (frontend_bss.declaration_scope_mode != (BYTE)frontend_bss.block_output_entries->value.level)
            return;
        pop_block_output(&frontend_bss.block_output_entries);
        write_ir_record(0x2b, symbol_expression(frontend_bss.block_output_entries->value.symbol, 0));
        pop_block_output(&frontend_bss.block_output_entries);
    }
}
/* 17f95..17fa0: the one generic list-pop owner shared by block and
 * constructor work. The removed arena-owned link is not freed. */
extern "C" void __cdecl pop_block_output(CompilerWorkLink **head)
{
    *head = (*head)->next;
}
extern "C" void __cdecl emit_function_epilogue(void)
{
    CompilerSymbol *function = frontend_bss.current_function;
    CompilerExpressionWork *record;
    CompilerExpressionWork frame;
    if (!function) return;
    record = frontend_data.expression_work_nodes;
    if (!record) goto reset_work;
    if ((((CompilerRuntimeFunction *)function)->queue_flags & 2) &&
        !(function->qualifiers & 1)) {
        compiler_warning(0x2c8);
        record = frontend_data.expression_work_nodes;
    }
    if (!frontend_data.expression_work_reset_pending &&
        frontend_bss.error_count != frontend_data.expression_work_error_count)
        goto reset_work;
    if (record && record->kind == 1) {
        pop_expression_work();
    } else {
        memset(&frame, 0, sizeof(frame));
        frame.kind = 4;
        push_expression_work(&frame, 16);
        emit_cleanup_expression(build_expression_cleanup(NULL));
    }
    if (!(frontend_bss.current_function->type_flags & 0x30))
        emit_special_function_epilogue();
reset_work:
    reset_expression_work();
}
extern "C" {
int __cdecl has_undefined_label(void); /*17662*/
int __cdecl validate_function_work(void); /*17694*/
void __cdecl emit_function_work(void); /*17740*/
}
extern "C" void __cdecl emit_special_function_epilogue(void)
{
    jmp_buf recovery;
    CompilerExpressionWork *record;
    if (!frontend_bss.current_function) return;
    if (!frontend_data.expression_work_nodes) {
        reset_expression_work();
        return;
    }
    if (has_undefined_label()) return;
    if (frontend_bss.expression_work_recovery_mode && !frontend_data.expression_work_recovery) {
        if (frontend_data.expression_work_reset_pending) return;
        frontend_data.expression_work_recovery = recovery;
        if (_setjmp3(recovery, 0)) {
            frontend_data.expression_work_recovery = NULL;
            return;
        }
    }
    if (validate_function_work()) {
        if (!frontend_bss.error_count) emit_function_work();
    } else {
        record = find_expression_work(frontend_data.expression_work_nodes, 1, 1);
        if ((record->flags >> 6) == 0) { __asm { int 3 } }
        compiler_warning(0xca, ((CompilerExpressionWorkInput *)record)->symbol);
    }
    if (frontend_data.expression_work_recovery == recovery)
        frontend_data.expression_work_recovery = NULL;
}
extern "C" int __cdecl has_undefined_label(void)
{
    CompilerLabelSymbol *label = (CompilerLabelSymbol *)frontend_data.pending_label_head;
    CompilerSymbolLink *next;
    while (label) {
        if (!(label->definition_flags & 4) && !label->code) return 1;
        next = label->link.next;
        label = next ? (CompilerLabelSymbol *)((BYTE *)next - 8) : NULL;
    }
    return 0;
}
extern "C" int __cdecl find_function_work_end(CompilerExpressionWork **, unsigned); /*44199*/
extern "C" int __cdecl validate_function_work(void)
{
    CompilerExpressionWork *record = frontend_data.expression_work_current;
    CompilerExpressionWork *previous;
    CompilerIrSymbol *function;
    CompilerType *type;
    CompilerTypeShape *shape;
    if (!record || record->kind != 1) return 1;
    if ((record->flags >> 6) == 0) { __asm { int 3 } }
    function = (CompilerIrSymbol *)((CompilerExpressionWorkInput *)record)->symbol;
    type = function->type;
    if (type->flags & 0x80) {
        shape = type->shape;
        if (shape && (shape->flags & 0x77) == 4 && !shape->base) return 1;
    }
    if (frontend_data.options.language_extensions && (get_type_classification(type) & 0x80) &&
        is_complete_declaration_type(type)) return 1;
    if (function->type_flags & 0x30) return 1;
    previous = scan_parameter_cleanup(record, 4, 1)->previous;
    if (previous->kind == 16) {
        do { previous = previous->previous; } while (previous->kind == 16);
        if (previous->kind == 11 || previous->kind == 9) return 1;
    }
    return find_function_work_end(&record, 4) == 0;
}
extern "C" int __cdecl find_function_work_end(CompilerExpressionWork **, unsigned) { __asm { int 3 } return 0; }
extern "C" void __cdecl emit_function_work(void)
{
    CompilerCleanupCapture *capture = frontend_data.cleanup_capture_head;
    CompilerCleanupCapture *match;
    while (capture) {
        if ((capture->flags >> 6) == 0) { __asm { int 3 } }
        match = find_cleanup_capture(capture->value.pointer);
        if (match) {
            emit_label(capture->label);
            frontend_data.compiler_mode_flags |= 0x8000;
            link_cleanup_capture(capture, match, 2);
            write_ir_record(0x3a, symbol_expression((CompilerSymbol *)capture->value.pointer, 0));
            frontend_data.compiler_mode_flags &= ~0x18000U;
        }
        capture = capture->capture_next;
    }
}
void RECON_THISCALL CompilerTranslationUnit::FinishLocalScope(CompilerScopeTable *scope)
{
    CompilerScopeTable *entry, *previous = NULL;
    if (!scope) return;
    entry = deferred_scopes;
    if (entry) {
        while (entry->level <= scope->level) {
            previous = entry;
            entry = entry->next;
            if (!entry) break;
        }
        if (entry) {
            while (entry->outer != scope) {
                previous = entry;
                entry = entry->next;
                if (!entry) break;
            }
        }
    }
    if (previous) previous->next = scope;
    else deferred_scopes = scope;
    scope->next = entry;
}

extern "C" void __cdecl reset_expression_work(void)
{
    frontend_data.expression_work_head = NULL;
    frontend_data.expression_work_current = NULL;
    frontend_data.expression_work_nodes = NULL;
    frontend_data.saved_expression_work_nodes = NULL;
    frontend_data.nested_cleanup_depth = 0;
    frontend_data.expression_work_depth = 0;
    frontend_data.cleanup_capture_depth = 0;
    frontend_data.cleanup_capture_stack = NULL;
    frontend_data.cleanup_capture_head = NULL;
    frontend_data.expression_cleanup_stack = NULL;
    frontend_data.expression_target = NULL;
    frontend_data.expression_target_active = 0;
    frontend_data.assignment_target_active = 0;
    frontend_data.force_statement_check = 0;
    frontend_data.parameter_work_high_index = 0xffffffff;
    frontend_data.parameter_work_index = 0xffffffff;
    frontend_data.parameter_work_states = NULL;
    frontend_data.parameter_work_kind = 1;
    frontend_data.parameter_work_capacity = 128;
}

extern "C" {
void __cdecl finish_function_output(unsigned); /*5adf*/
void __cdecl clear_function_output_state(void); /*6279*/
void __cdecl reset_function_output_state(void); /*6284*/
void __cdecl initialize_function_output_state(void); /*6291*/
unsigned __cdecl debug_function_name(CompilerNameEntry *, CompilerGlobalSymbolView *); /*9b32c*/
void __cdecl complete_debug_function_output(void); /*a18e7*/
}
extern "C" void __cdecl finish_external_initializer(void)
{
    unsigned module_kind = frontend_data.debug_module_kind;
    unsigned reference = 0, debug_result, preserve;
    CompilerGlobalSymbolView *function, *owner;
    int active, index;
    active = module_kind == 4 ||
        ((CompilerRecordedLexer *)frontend_bss.saved_context_stack.top->object)->active;
    function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    if (function) {
        if (active) {
            if (function->kind == 8) {
                index = (short)function->declaration_qualifiers;
                if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
                reference = (unsigned)owner->class_detail->declaration_reference;
            }
            preserve = frontend_bss.debug_module->Slot84();
            if (!frontend_bss.debug_module->Slot18(reference,
                    frontend_bss.saved_context_stack.position, frontend_bss.lexical_skip_mode,
                    &module_kind, &debug_result))
                report_compiler_diagnostic(73, ".\\funcs.c", 0x759);
            if (frontend_data.options.incremental_rebuild && debug_result) {
                function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
                index = (short)function->declaration_qualifiers;
                if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
                if (!frontend_bss.debug_module->Slot94(reference, debug_function_name(function->name, owner)))
                    report_compiler_diagnostic(73, ".\\funcs.c", 0x75e);
            }
        }
        finish_function_output(frontend_bss.current_function->scope_flags & 1);
        if (active) {
            frontend_data.backend->SetDebugMode(module_kind == 5);
            if (preserve) complete_debug_function_output();
        }
    }
    clear_function_output_state();
    reset_expression_work();
    frontend_bss.function_reset_state[0] = 0;
    frontend_bss.function_reset_state[1] = 0;
    frontend_bss.function_reset_state[2] = 0;
    frontend_data.translation_unit->RestoreSavedLookup(0);
    frontend_data.translation_unit->PopSavedLookup();
    frontend_bss.current_parameters = NULL;
    frontend_bss.active_parameters = NULL;
    frontend_bss.current_parameter_kind = 0;
    frontend_bss.active_parameter_kind = 0;
    frontend_data.compiler_flags &= ~0x20100U;
    frontend_bss.current_function = NULL;
    reset_function_output_state();
    initialize_function_output_state();
    frontend_data.debug_module_kind = module_kind;
}
extern "C" {
void __cdecl finish_function_references(CompilerSymbol *); /*5ea0*/
void __cdecl finish_function_locations(void); /*626e*/
void __cdecl complete_constructor(CompilerSymbol *); /*22d35*/
void __cdecl complete_destructor(CompilerSymbol *); /*232df*/
void __cdecl complete_vbase_destructor(CompilerSymbol *); /*4795a*/
CompilerExpression *__cdecl make_sized_integer_constant(__int64, int); /*5df6*/
}
extern "C" void __cdecl finish_function_output(unsigned preserve)
{
    CompilerGlobalSymbolView *function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    if (function) {
        if (frontend_data.debug_module_kind == 4) {
            finish_compound_statement();
        } else {
            if (!frontend_bss.suppress_parser_codegen) {
                if (!preserve) emit_context_transition(0x24);
                emit_label(frontend_bss.function_entry_label);
                function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
                if (function->type_flags & 0x10) {
                    complete_constructor(frontend_data.declaration_lookup_owner);
                    function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
                }
                if (function->type_flags & 0x20)
                    complete_destructor(frontend_data.declaration_lookup_owner);
                else if (function->name == frontend_data.builtin_names.vbase_destructor)
                    complete_vbase_destructor(frontend_data.declaration_lookup_owner);
                if (frontend_data.options.debug_info) emit_context_transition(0x12);
                write_ir_record(0x47, NULL);
            }
            finish_compound_statement();
            frontend_data.translation_unit->FinishFunctionScopes();
            if (!frontend_bss.suppress_parser_codegen)
                write_ir_record(0x54, make_sized_integer_constant(0, 4));
        }
        function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
        if ((function->symbol_flags & 1) && (function->storage_class & 7) != 3) {
            frontend_bss.function_reset_state[0] = 0;
            frontend_bss.function_reset_state[1] = 0;
            frontend_bss.function_reset_state[2] = 0;
        }
    }
    frontend_data.translation_unit->deferred_scopes = NULL;
    if (preserve && !frontend_data.options.incremental_compile)
        frontend_bss.function_source_position = 0;
    finish_function_references(frontend_bss.current_function);
    frontend_data.function_result_type = NULL;
    frontend_bss.block_output_entries = NULL;
    frontend_bss.function_output_state = 0;
    frontend_bss.global_expression_scope_active = 0;
    frontend_data.token_text_cursor = compiler_token_text;
    frontend_bss.global_output_active = 0;
    frontend_bss.declaration_scope_mode = 0;
    frontend_data.declaration_lookup_owner = NULL;
    frontend_data.declaration_scope_number = 0;
    finish_function_locations();
}
extern "C" void __cdecl prepare_completed_scope_output(void); /*5d42*/
void RECON_THISCALL CompilerTranslationUnit::FinishFunctionScopes(void)
{
    CompilerScopeTable *scope;
    const void *last_identity;
    if (frontend_bss.suppress_parser_codegen) return;
    frontend_bss.global_output_active = 1;
    prepare_completed_scope_output();
    scope = deferred_scopes;
    if (scope) {
        do {
            scope->Check();
            scope = scope->next;
        } while (scope);
        last_identity = scope;
    } else {
        frontend_bss.global_output_active = 1;
        write_global_ir_record(13, NULL);
        frontend_bss.global_output_active = 2;
        write_global_ir_record(13, NULL);
        /* Original reloads its entry PUSH ECX local on this branch. Its
         * value is this receiver, used only for identity against root. */
        last_identity = this;
    }
    if (last_identity != root) write_global_ir_record(6, NULL);
}
extern "C" void __cdecl prepare_completed_scope_output(void)
{
    CompilerLabelSymbol *label;
    CompilerSymbolLink *next;
    if (frontend_bss.error_count) {
        frontend_data.pending_label_head = NULL;
        return;
    }
    label = (CompilerLabelSymbol *)frontend_data.pending_label_head;
    while (label) {
        if (label->kind >= 26 || frontend_data.symbol_record_bytes[label->kind] < 36) { __asm { int 3 } }
        if (!(label->definition_flags & 4) && !label->code) {
            compiler_warning(0x5e, label);
        } else {
            if (!(label->emission_state & 3) && label->link.name)
                report_language_diagnostic(3, 0x66, label);
            if (label->definition_flags & 4)
                write_global_ir_record(3, label);
        }
        next = label->link.next;
        label = next ? (CompilerLabelSymbol *)((BYTE *)next - 8) : NULL;
    }
    frontend_data.pending_label_head = NULL;
}
/* The producer allocates4 +20*32 bytes. Each row's eight cells are
 * individually consumed here; unassigned flag bits and the new tail's
 * next pointer retain their allocation bytes, without an initial-value claim. */
struct CompilerFunctionOutputRow {
    CompilerSymbolLink *binding;
    unsigned saved_reference, local_reference;
    DWORD context;
    unsigned records, parameters, source_position, flags;
};
struct CompilerFunctionOutputBlock {
    CompilerFunctionOutputBlock *next;
    CompilerFunctionOutputRow rows[20];
};
typedef char CompilerFunctionOutputRow_layout[(sizeof(CompilerFunctionOutputRow) == 32 &&
    offsetof(CompilerFunctionOutputRow, flags) == 28 &&
    sizeof(CompilerFunctionOutputBlock) == 644) ? 1 : -1];
extern "C" {
unsigned __cdecl external_symbol_index(CompilerGlobalSymbolView *); /*8dd0f*/
int __cdecl is_function_output_complete(CompilerSymbol *); /*2c39e*/
int __cdecl has_pending_function_output(void); /*2be1c*/
}
extern "C" void __cdecl finish_function_references(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *function = (CompilerGlobalSymbolView *)input;
    CompilerGlobalSymbolView *current;
    CompilerFunctionOutputBlock *block;
    CompilerFunctionOutputRow *row;
    CompilerPool *pool;
    OutputBuffer *buffer = &frontend_bss.output_buffers[4];
    unsigned index, record_index, kind;
    int available;
    BYTE *memory;
    if (frontend_bss.error_count || frontend_bss.options.preprocess_stdout ||
        frontend_bss.suppress_parser_codegen || !function) return;
    /* The non-IL branch writes only kind's low BYTE before a genuine
     * external full-DWORD consumer. Its upper bytes remain unresolved;
     * stop in both builds before producing or passing that representation. */
    if (!frontend_data.option_c9il) { __asm { int 3 } }
    if (frontend_data.debug_module_kind != 4 && frontend_data.option_c9il &&
        !(frontend_data.source_mode_flags & 0x800)) {
        available = buffer->Ensure(2);
        buffer->ok &= available;
        if (buffer->ok) *buffer->cursor++ = 9;
        current = (CompilerGlobalSymbolView *)frontend_bss.current_function;
        if (!current) record_index = 0;
        else if (frontend_data.option_c9il) record_index = current->record_index & 0xffffff;
        else if (current->references & ~7U) record_index = current->references >> 3;
        else record_index = external_symbol_index(current);
        buffer->WriteReferenceIndex((int)record_index);
    }
    if (!frontend_bss.options.preprocess_stdout && !frontend_bss.suppress_parser_codegen) {
        if (frontend_data.option_c9il) {
            frontend_bss.output_buffers[1].Flush();
            frontend_bss.output_buffers[3].Flush();
        } else if (frontend_data.debug_module_kind != 4) {
            write_ir_record(0x4d, NULL);
            /* Authentic partial DWORD writer; the entry guard precedes
             * every native or Win32 use of its unknown upper bytes. */
            *(BYTE *)&kind = (function->symbol_flags & 1) ? 2 : 1;
            record_index = frontend_data.option_c9il ? function->record_index & 0xffffff :
                (function->references & ~7U) ? function->references >> 3 : external_symbol_index(function);
            frontend_bss.output_buffers[3].MarkLocation(record_index, kind);
            frontend_bss.output_buffers[1].MarkLocation(function->references >> 3, kind);
            if (!(frontend_data.compiler_flags & 0x20)) {
                if (is_function_output_complete(input)) {
                    buffer->MarkGlobal(function->references >> 3);
                } else {
                    if (!has_pending_function_output())
                        frontend_bss.output_buffers[5].CopyFrom(buffer);
                    if (buffer->Prepare(1)) *buffer->cursor++ = 0xf;
                    buffer->MarkLocation(function->references >> 3, kind);
                }
            }
        }
    }
    frontend_data.source_mode_flags &= ~0x1000U;
    index = (frontend_data.function_output_index + 1) % 20;
    frontend_data.function_output_index = index;
    if (!index) {
        pool = &frontend_bss.pools[9]; /* actual C0810 */
        if (pool->remaining < 644) pool->Refill(644);
        memory = pool->cursor;
        pool->cursor += 644;
        pool->remaining -= 644;
        block = (CompilerFunctionOutputBlock *)memory;
        if (frontend_data.function_output_head)
            frontend_data.function_output_tail->next = block;
        else frontend_data.function_output_head = block;
        frontend_data.function_output_tail = block;
    }
    block = frontend_data.function_output_tail;
    row = &block->rows[index];
    row->binding = &((CompilerNamedSymbol *)function)->link;
    if (frontend_data.saved_pch_mode && !frontend_data.pch_reference_output_mode) {
        row->flags |= 0x10;
        if (frontend_data.options.pch_use) {
            frontend_bss.saved_reference_count += frontend_data.pch_reference_bytes + 0x484;
            frontend_bss.local_reference_count += frontend_data.pch_output_bytes;
        }
        if ((function->member_flags & 4) &&
            ((function->storage_class & 7) != 3 || (function->type_flags & 4)) &&
            (function->symbol_flags & 1) && !(function->symbol_flags & 0x10) &&
            !frontend_data.options.pch_auto && !frontend_data.options.no_expand_inline)
            frontend_data.deferred_function_output = 1;
    } else if (!frontend_data.saved_pch_mode) row->flags &= ~0x10U;
    row->saved_reference = frontend_bss.saved_reference_count;
    row->local_reference = frontend_bss.local_reference_count;
    row->records = frontend_bss.ir_record_count;
    row->source_position = frontend_bss.function_source_position;
    row->flags = (row->flags & ~1U) | (frontend_data.type_debug_mode & 1);
    row->flags = (row->flags & ~2U) | ((frontend_data.force_type_serialization & 1) << 1);
    row->flags = (row->flags & ~4U) | ((frontend_data.disable_inline_records & 1) << 2);
    row->flags &= ~8U;
    row->parameters = frontend_bss.function_parameter_count;
    row->context = frontend_data.function_output_context;
    row->flags = (row->flags & ~0x20U) | ((frontend_data.emit_global_locations & 1) << 5);
    frontend_bss.ir_record_count = 0;
    frontend_data.emit_global_locations = 0;
}
extern "C" int __cdecl is_function_output_complete(CompilerSymbol *) { __asm { int 3 } return 0; }
extern "C" int __cdecl has_pending_function_output(void) { __asm { int 3 } return 0; }
void RECON_THISCALL OutputBuffer::CopyFrom(OutputBuffer *) { __asm { int 3 } }

extern "C" void __cdecl finish_function_locations(void)
{ frontend_data.function_output_location_state = 0; }
extern "C" void __cdecl finish_constructor_work(void);
/* 22d35..22d6a: a successful constructor returns its actual this value. */
extern "C" void __cdecl complete_constructor(CompilerSymbol *owner)
{
    if (!owner) return;
    finish_constructor_work();
    emit_function_epilogue();
    CompilerExpression *value = make_this_pointer_expression(NULL);
    emit_ir_expression(value);
    write_ir_record(0x41, value);
}
extern "C" CompilerExpression *__cdecl make_constructor_cleanup_expression(unsigned, unsigned, unsigned, unsigned);
/* 22d6a..22db4. The c6cf8 clear uses its existing cleanup/initializer
 * union owner; it is not another field in the context projection. */
extern "C" void __cdecl finish_constructor_work(void)
{
    CompilerInitializerContext *context = &frontend_bss.initializer_context;
    if ((int)context->cleanup_pending_count > 0) {
        CompilerExpression *expression = make_constructor_cleanup_expression(0xc8,
            context->cleanup_pending_count, context->last_cleanup_state_index, 0);
        emit_cleanup_expression(expression);
        context->cleanup_pending_count = 0;
    }
    frontend_bss.return_cleanup_state = 0;
    frontend_data.member_initializer_state = 1;
}
extern "C" CompilerExpression *__cdecl make_constructor_cleanup_expression(unsigned, unsigned, unsigned, unsigned) { __asm { int 3 } return NULL; }
extern "C" void __cdecl complete_destructor(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl complete_vbase_destructor(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl clear_function_output_state(void)
{ frontend_data.debug_notification_index = 0; }
extern "C" void __cdecl reset_function_output_state(void)
{
    frontend_data.function_output_reset_state[0] = 0;
    frontend_data.function_output_reset_state[1] = 0;
}
extern "C" void __cdecl finish_function_deferred_work(void); /*62a3*/
extern "C" void __cdecl initialize_function_output_state(void)
{
    finish_function_deferred_work();
    frontend_data.function_scope_output_state[0] = 0;
    frontend_data.function_scope_output_state[1] = 0;
}
typedef void (__cdecl *CompilerVectorCallback)(void *);
extern "C" void __cdecl visit_vector_items(CompilerVector *, CompilerVectorCallback);
extern "C" void __cdecl finish_deferred_record_a(void *);
extern "C" void __cdecl finish_deferred_record_b(void *);
extern "C" void __cdecl finish_deferred_record_c(void *);
extern "C" void __cdecl finish_deferred_record_d(void *);
extern "C" void __cdecl finish_function_deferred_work(void)
{
    CompilerVector *vector;
    visit_vector_items(frontend_data.function_deferred_vectors[0], finish_deferred_record_a);
    visit_vector_items(frontend_data.function_deferred_vectors[1], finish_deferred_record_b);
    vector = frontend_data.function_deferred_vectors[2];
    frontend_data.function_deferred_vectors[1] = NULL;
    visit_vector_items(vector, finish_deferred_record_c);
    vector = frontend_data.function_deferred_vectors[3];
    frontend_data.function_deferred_vectors[2] = NULL;
    visit_vector_items(vector, finish_deferred_record_d);
    frontend_data.function_deferred_vectors[3] = NULL;
}
extern "C" void __cdecl visit_vector_items(CompilerVector *vector, CompilerVectorCallback callback)
{
    short index;
    if (!vector) return;
    index = 0;
    while (index < vector->count) {
        void *item = vector->items[index];
        ++index;
        callback(item);
    }
}
extern "C" void __cdecl finish_deferred_record_a(void *) { __asm { int 3 } }
extern "C" void __cdecl finish_deferred_record_b(void *) { __asm { int 3 } }
extern "C" void __cdecl finish_deferred_record_c(void *) { __asm { int 3 } }
extern "C" void __cdecl finish_deferred_record_d(void *) { __asm { int 3 } }
extern "C" unsigned __cdecl debug_function_name(CompilerNameEntry *, CompilerGlobalSymbolView *) { __asm { int 3 } return 0; }
extern "C" void __cdecl complete_debug_function_output(void) { __asm { int 3 } }

extern "C" void __cdecl emit_label(CompilerExpression *expression)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)expression->value.symbol;
    int depth;
    if (symbol->kind >= 26 || frontend_data.symbol_record_bytes[symbol->kind] <= 0x25) { __asm { int 3 } }
    symbol->definition_flags |= 4;
    symbol->reference_position.scope_depth = frontend_bss.declaration_scope_mode;
    if (symbol->references & 3) {
        depth = (int)((unsigned)frontend_data.symbol_reference_depths[2] +
            (unsigned)frontend_data.symbol_reference_depths[1] +
            (unsigned)frontend_data.symbol_reference_depths[0]);
        if ((int)symbol->reference_position.depth < depth)
            compiler_warning(0x2c1, 0x53, symbol);
    }
    symbol->reference_position.depth = (BYTE)((unsigned)frontend_data.symbol_reference_depths[2] +
        (unsigned)frontend_data.symbol_reference_depths[1] + (unsigned)frontend_data.symbol_reference_depths[0]);
    write_ir_record(0x29, expression);
}

extern "C" void __cdecl write_global_reference(CompilerSymbolLink *);
void RECON_THISCALL CompilerAuxiliaryScope::Check(void)
{
    CompilerSymbolLink *link = parameters ? parameters->tail : NULL;
    if (frontend_bss.error_count) return;
    frontend_bss.global_output_active = level;
    write_global_ir_record(13, NULL);
    while (link) {
        write_global_reference(link);
        link = ((CompilerParameterSymbol *)((BYTE *)link - 8))->previous_parameter;
    }
}

extern "C" {
void __cdecl set_function_output_context(DWORD); /*33dd6*/
void __cdecl write_global_binding_record(unsigned, CompilerSymbolLink *, unsigned);
void __cdecl prepare_debug_type(CompilerType *);
unsigned __cdecl finish_class_debug_type(CompilerSymbol *);
}
extern "C" void __cdecl emit_function_output_blocks(void)
{
    CompilerFunctionOutputBlock *block;
    CompilerFunctionOutputRow *row;
    CompilerGlobalSymbolView *symbol, *owner;
    CompilerSymbolLink *binding;
    CompilerType *type;
    const char *name;
    char qualified_name[1024];
    unsigned index, count, diagnostic;
    int accepted;
    short scope;
    while (frontend_data.function_output_head) {
        block = frontend_data.function_output_head;
        if (block == frontend_data.function_output_tail) {
            if (frontend_data.function_output_index >= 20) { __asm { int 3 } }
            count = frontend_data.function_output_index + 1;
        } else count = 20;
        for (index = 0; index != count; ++index) {
            set_function_output_context(frontend_data.function_output_head->rows[index].context);
            row = &frontend_data.function_output_head->rows[index];
            binding = row->binding;
            symbol = (CompilerGlobalSymbolView *)((BYTE *)binding - 8);
            if (symbol->definition_flags & 8) continue;
            if (symbol->kind >= 26 || frontend_data.symbol_record_bytes[symbol->kind] < 0x2f) { __asm { int 3 } }
            frontend_bss.inline_record_flag = 0;
            frontend_data.include_pch_reference_counts = (row->flags >> 4) & 1;
            if ((binding->name == frontend_data.builtin_names.main_entry ||
                 binding->name == frontend_data.builtin_names.winmain_entry ||
                 binding->name == frontend_data.builtin_names.dllmain_entry) &&
                !symbol->declaration_qualifiers) {
                if ((symbol->storage_class & 7) == 3) {
                    report_language_diagnostic(3, 8, symbol, 9);
                    symbol->storage_class = (symbol->storage_class & ~7U) ^ 1;
                }
                if (symbol->symbol_flags & 1) {
                    report_language_diagnostic(3, 8, symbol, 14);
                    symbol->symbol_flags &= ~1U;
                }
                symbol->qualifiers |= 0x80;
                frontend_bss.inline_record_flag = 1;
                goto select_output;
            }
            if ((symbol->storage_class & 7) != 3 || (symbol->type_flags & 4)) {
                if (symbol->symbol_flags & 1) {
                    symbol->storage_class |= 0x40;
                    if (!(symbol->member_flags & 8)) {
                        if (symbol->symbol_flags & 0x10) frontend_bss.inline_record_flag = 1;
                        else if ((symbol->member_flags & 4) && !frontend_data.options.no_expand_inline) {
                            if (!frontend_data.include_pch_reference_counts || frontend_data.options.pch_use ||
                                frontend_data.saved_pch_mode || frontend_bss.pch_replayed)
                                frontend_bss.inline_record_flag = 1;
                        }
                    }
                } else if (!(symbol->linkage_flags & 0xc)) frontend_bss.inline_record_flag = 1;
            }
select_output:
            if (symbol->symbol_flags & 0x80) frontend_bss.inline_record_flag = 1;
            if (!frontend_bss.inline_record_flag && !(symbol->references & 3)) {
                accepted = 0;
                if (!frontend_data.option_c9il) {
                    name = binding->name->name;
                    if ((symbol->kind == 7 || symbol->kind == 8) && symbol->declaration_qualifiers) {
                        scope = (short)symbol->declaration_qualifiers;
                        if (scope < 0 || (unsigned)scope >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                        owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[scope];
                        _snprintf(qualified_name, 1024, "%s::%s", owner->name->name, name);
                        qualified_name[1023] = 0;
                        name = qualified_name;
                    }
                    accepted = frontend_bss.debug_module->Slot78(name);
                }
                if (!accepted) {
                    diagnostic = (symbol->symbol_flags & 1) ? 0x202 : 0x1f9;
                    report_language_diagnostic(4, diagnostic, symbol);
                    goto marked_output;
                }
            }
            row = &frontend_data.function_output_head->rows[index];
            frontend_bss.saved_reference_count = row->saved_reference;
            frontend_bss.local_reference_count = row->local_reference;
            frontend_bss.ir_record_count = row->records;
            frontend_bss.function_source_position = row->source_position;
            frontend_bss.function_parameter_count = row->parameters;
            frontend_data.force_type_serialization = 1;
            frontend_data.type_debug_mode = row->flags & 1;
            frontend_data.disable_inline_records = (row->flags >> 2) & 1;
            frontend_data.emit_global_locations = (row->flags >> 5) & 1;
            if (frontend_data.options.debug_info) {
                if (symbol->type_flags & 4) {
                    scope = (short)symbol->declaration_qualifiers;
                    if (scope < 0 || (unsigned)scope >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                    owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[scope];
                    owner->type->debug_cache = finish_class_debug_type((CompilerSymbol *)owner);
                } else if (!frontend_bss.suppress_parser_codegen) {
                    type = symbol->type;
                    prepare_debug_type(type);
                    scope = (short)type->auxiliary;
                    if (scope < 0 || (unsigned)scope >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
                    owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[scope];
                    if (owner && owner->kind == 4 && !owner->class_detail->debug_index)
                        finish_class_debug_type((CompilerSymbol *)owner);
                }
            }
            write_global_binding_record(14, binding, frontend_bss.inline_record_flag);
marked_output:
            symbol->definition_flags |= 8;
        }
        frontend_data.function_output_head = frontend_data.function_output_head->next;
    }
}
/* CB000 is module .data, outside the datafe invocation snapshot. Its
 * producer/provider has not been reached in this IL case. These typed views
 * interpret only the actual +4 service and its slot0c two-argument method. */
struct CompilerContextStringProvider {
    virtual void RECON_THISCALL UnrecoveredSlot00(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot04(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot08(void) = 0;
    virtual void RECON_THISCALL GetText(DWORD, const char **) = 0;
};
struct CompilerFunctionOutputOwner {
    BYTE unmodeled_00_03[4];
    CompilerContextStringProvider *strings;
};
CompilerFunctionOutputOwner *function_output_owner = NULL;
extern "C" void __cdecl write_global_ir_record(unsigned, CompilerSymbol *);
extern "C" void __cdecl set_function_output_context(DWORD context)
{
    if (frontend_data.option_c9il || context == frontend_data.last_output_context) return;
    if (!function_output_owner || !function_output_owner->strings) { __asm { int 3 } }
    function_output_owner->strings->GetText(context, &frontend_bss.global_string_record);
    write_global_ir_record(22, NULL);
    frontend_data.last_output_context = context;
}

/* Both allocations come from the selected monotonic compiler arena. The
 * original leaves a fresh header and the node's next pointer untouched;
 * initial committed pages and ResetStorage provide their zeroed storage. */
extern "C" CompilerSymbolBinding *__cdecl make_symbol_binding(
        CompilerSymbolBinding *binding, CompilerSymbolBindingOwner *inner,
        CompilerSymbol *symbol)
{
    CompilerArena *arena;
    CompilerSymbolBindingOwner *entry;
    unsigned index;
    if (!binding) {
        index = frontend_data.binding_arena_index;
        if (index >= 9) { __asm { int 3 } }
        arena = &frontend_bss.arenas[index];
        binding = (CompilerSymbolBinding *)arena->cursor;
        arena->cursor += sizeof(CompilerSymbolBinding);
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    }
    index = frontend_data.binding_arena_index;
    if (index >= 9) { __asm { int 3 } }
    arena = &frontend_bss.arenas[index];
    entry = (CompilerSymbolBindingOwner *)arena->cursor;
    arena->cursor += sizeof(CompilerSymbolBindingOwner);
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    entry->symbol = symbol;
    entry->inner = inner;
    if (!binding->owner)
        binding->owner = entry;
    else
        binding->last->next = entry;
    binding->last = entry;
    return binding;
}

extern "C" void __cdecl prepare_declaration_lookup(void)
{
    if (!frontend_data.parser_classification_state) {
        CompilerExpressionWork frame;
        memset(&frame, 0, sizeof(frame));
        frame.kind = 62;
        push_expression_work(&frame, 16);
    }
}

/* The check is shared by repeated visits to the same expression. The
 * original allocator leaves this pointer untouched; do not invent an
 * initialization or interpret the separate opaque DWORD at+c. */
extern "C" void __cdecl check_initializer_type(CompilerExpression *expression,
        CompilerType *type)
{
    if (!expression->initializer_check)
        expression->initializer_check = make_binary_node(0x60, expression, NULL, type);
}

extern "C" {
CompilerWorkLink *__cdecl collect_pending_expression_work(void);
void __cdecl collect_expression_reference_work(CompilerWorkLink **, CompilerExpression *);
void __cdecl collect_expression_cleanup_work(CompilerWorkLink **, CompilerExpression *, unsigned);
CompilerExpression *__cdecl attach_pending_expression_work(CompilerExpression *, CompilerWorkLink *);
CompilerExpression *__cdecl attach_expression_reference_work(CompilerExpression *, CompilerWorkLink *);
CompilerExpression *__cdecl attach_expression_cleanup_work(CompilerExpression *, CompilerWorkLink *);
}
extern "C" CompilerExpression *__cdecl mark_parser_expression(CompilerExpression *expression)
{
    CompilerWorkLink *references = NULL, *cleanups = NULL, *pending;
    if (!expression) return NULL;
    if (frontend_data.expression_work_reset_pending) return expression;
    pending = collect_pending_expression_work();
    if (pending) expression = attach_pending_expression_work(expression, pending);
    collect_expression_reference_work(&references, expression);
    if (references) expression = attach_expression_reference_work(expression, references);
    collect_expression_cleanup_work(&cleanups, expression, 0);
    if (cleanups) expression = attach_expression_cleanup_work(expression, cleanups);
    return expression;
}

/* Kind13 has a payload at+10 and two signed values encoded as integer
 * expression nodes from+18/+1c. Its untouched DWORD+14 is retained storage. */
struct CompilerPendingExpressionWork : CompilerExpressionWork {
    CompilerExpression *expression;
    BYTE unmodeled_14_17[4];
    int first_value, second_value;
};
typedef char CompilerPendingExpressionWork_layout[(sizeof(CompilerPendingExpressionWork) == 32 &&
    offsetof(CompilerPendingExpressionWork, expression) == 16 &&
    offsetof(CompilerPendingExpressionWork, first_value) == 24 &&
    offsetof(CompilerPendingExpressionWork, second_value) == 28) ? 1 : -1];
extern "C" void *__cdecl make_integer_constant(int);
#define ALLOCATE_PENDING_LINK(result) do { \
    CompilerArena *link_arena = &frontend_bss.arenas[(frontend_data.source_mode_flags & 0x10) ? 0 : 4]; \
    result = (CompilerWorkLink *)link_arena->cursor; \
    link_arena->cursor += 8; \
    if (link_arena->cursor > link_arena->committed_limit) link_arena->CommitExtent(); \
} while (0)
extern "C" CompilerWorkLink *__cdecl collect_pending_expression_work(void)
{
    CompilerWorkLink *head = NULL, *outer, *first, *second, *third;
    CompilerExpressionWork *node;
    CompilerPendingExpressionWork *frame;
    if (frontend_data.expression_work_reset_pending) return NULL;
    node = frontend_data.expression_work_nodes;
    while (node && node->kind == 13) {
        if (node->flags & 3) break;
        /* The established work allocator's class0 has only16 bytes. */
        if ((node->flags >> 6) == 0) { __asm { int 3 } }
        frame = (CompilerPendingExpressionWork *)node;
        ALLOCATE_PENDING_LINK(outer);
        outer->next = head;
        ALLOCATE_PENDING_LINK(first);
        first->value.payload = make_integer_constant(frame->first_value);
        ALLOCATE_PENDING_LINK(second);
        first->next = second;
        second->value.payload = make_integer_constant(frame->second_value);
        ALLOCATE_PENDING_LINK(third);
        second->next = third;
        third->value.payload = frame->expression;
        /* The final next field is untouched by the original. */
        outer->value.payload = first;
        head = outer;
        pop_expression_work();
        node = frontend_data.expression_work_nodes;
    }
    if (node && node->kind == 62) pop_expression_work();
    return head;
}
#undef ALLOCATE_PENDING_LINK

/* Expression class4/opcode9c uses its +4 pointer as a key owner. This
 * borrowed view exposes only the DWORD consumed at+10; it asserts no
 * allocation size, default or interpretation for the preceding bytes. */
struct CompilerExpressionReferenceKey {
    BYTE unmodeled_00_0f[16];
    DWORD key;
};
#define ALLOCATE_REFERENCE_LINK(result) do { \
    CompilerArena *link_arena = &frontend_bss.arenas[(frontend_data.source_mode_flags & 0x10) ? 0 : 4]; \
    result = (CompilerWorkLink *)link_arena->cursor; \
    link_arena->cursor += 8; \
    if (link_arena->cursor > link_arena->committed_limit) link_arena->CommitExtent(); \
} while (0)
extern "C" void __cdecl collect_expression_reference_work(CompilerWorkLink **head,
        CompilerExpression *expression)
{
    CompilerWorkLink *outer, *inner, *item;
    CompilerExpression *previous;
    unsigned category;
    while (expression) {
        category = expression->flags & 7;
        if (category == 2) {
            collect_expression_reference_work(head, expression->value.operands.first);
            expression = expression->value.operands.second;
            continue;
        }
        if (category == 1) {
            expression = expression->value.operands.first;
            continue;
        }
        if (category == 7) {
            if (expression->kind < 0xbb || expression->kind > 0xbc) return;
            for (outer = (CompilerWorkLink *)expression->value.operands.second;
                 outer; outer = outer->next)
                for (inner = (CompilerWorkLink *)outer->value.payload;
                     inner; inner = inner->next)
                    collect_expression_reference_work(head, (CompilerExpression *)inner->value.payload);
            expression = expression->value.operands.first;
            continue;
        }
        if (category != 4 || expression->kind != 0x9c) return;
        outer = *head;
        while (outer) {
            inner = (CompilerWorkLink *)outer->value.payload;
            previous = (CompilerExpression *)inner->value.payload;
            if (((CompilerExpressionReferenceKey *)previous->type)->key == expression->value.words.low)
                break;
            outer = outer->next;
        }
        if (!outer) {
            ALLOCATE_REFERENCE_LINK(outer);
            outer->next = *head;
            *head = outer;
            outer->value.payload = NULL;
        }
        ALLOCATE_REFERENCE_LINK(item);
        item->value.payload = expression;
        item->next = (CompilerWorkLink *)outer->value.payload;
        outer->value.payload = item;
        return;
    }
}
#undef ALLOCATE_REFERENCE_LINK

extern "C" CompilerWorkLink *__cdecl prepend_expression_work_link(CompilerWorkLink **, void *);
extern "C" void __cdecl collect_expression_cleanup_work(CompilerWorkLink **head,
        CompilerExpression *expression, unsigned preserve_state)
{
    CompilerWorkLink *outer, *inner;
    CompilerExpression *first;
    CompilerExpressionWorkStateRow *row;
    unsigned category, index;
    WORD count;
    while (expression) {
        category = expression->flags & 7;
        if (category == 2) {
            if (expression->kind == 0xc7) {
                prepend_expression_work_link(head, expression);
                if (!preserve_state) {
                    first = expression->value.operands.first;
                    index = first->value.counted_control.index;
                    if (!frontend_data.parameter_work_states || index >= frontend_data.parameter_work_capacity) { __asm { int 3 } }
                    row = &frontend_data.parameter_work_states[index];
                    count = first->value.counted_control.count & 0xfff;
                    first->value.counted_control.count = (WORD)(count |
                        (row->active == 1 ? 0x4000 : (WORD)row->active << 12));
                    first = expression->value.operands.first;
                    if ((first->value.counted_control.count & 0xf000) == 0x4000) {
                        index = first->value.counted_control.index;
                        if (!frontend_data.parameter_work_states || index >= frontend_data.parameter_work_capacity) { __asm { int 3 } }
                        frontend_data.parameter_work_states[index].cleanup_mark = 1;
                    }
                }
            } else collect_expression_cleanup_work(head, expression->value.operands.first, preserve_state);
            expression = expression->value.operands.second;
            continue;
        }
        if (category == 1) {
            expression = expression->value.operands.first;
            continue;
        }
        if (category == 7) {
            if (expression->kind < 0xbb || expression->kind > 0xbd) continue;
            if (expression->kind != 0xbd)
                for (outer = (CompilerWorkLink *)expression->value.operands.second;
                     outer; outer = outer->next)
                    for (inner = (CompilerWorkLink *)outer->value.payload;
                         inner; inner = inner->next)
                        collect_expression_cleanup_work(head, (CompilerExpression *)inner->value.payload, preserve_state);
            expression = expression->value.operands.first;
            continue;
        }
        if (category || (expression->kind != 0xc8 && expression->kind != 0xc9)) return;
        prepend_expression_work_link(head, expression);
        if (!preserve_state) {
            index = expression->value.counted_control.index;
            if (!frontend_data.parameter_work_states || index >= frontend_data.parameter_work_capacity) { __asm { int 3 } }
            count = (WORD)frontend_data.parameter_work_states[index].active;
            expression->value.counted_control.count = (WORD)((expression->value.counted_control.count & 0xfff) |
                (count << 12));
        }
        return;
    }
}

extern "C" CompilerExpression *__cdecl expand_initializer_expression(unsigned, CompilerExpression *, CompilerExpression *);
extern "C" CompilerExpression *__cdecl append_expression_list(CompilerExpression *previous,
        CompilerExpression *expression, unsigned mode)
{
    CompilerExpression *result;
    CompilerType *type;
    if (expression && expression->kind == 0xee)
        expression = expand_initializer_expression(0x74, expression, 0);
    expression = resolve_parser_expression(expression);
    type = mode == 1 ? frontend_data.builtin_types.type_0000 :
        expression ? expression->type : NULL;
    result = make_binary_node(0x60, expression, previous, type);
    if (expression && (expression->flags & 8)) result->flags |= 8;
    if (!previous) result->list_next = result;
    else {
        result->list_next = previous->list_next;
        previous->list_next = result;
    }
    if (expression) expression->list_next = result;
    return result;
}

typedef char ParserDeferredListItem_layout[(sizeof(ParserDeferredListItem) == 20 &&
    offsetof(ParserDeferredListItem, flags) == 12 &&
    offsetof(ParserDeferredListItem, payload) == 16) ? 1 : -1];
ParserDeferredListItem *RECON_THISCALL ParserDeferredListItem::Initialize(
        CompilerExpression *expression, unsigned deferred, unsigned preserve)
{
    flags = (flags & 0xf8) | 1 | ((deferred & 1) << 2);
    next = previous = NULL;
    new(this) ParserDeferredListItem;
    payload = preserve ? expression : instantiate_parser_expression(expression, 0);
    return this;
}

typedef char ParserDeferredList_layout[(sizeof(ParserDeferredList) == 12 &&
    offsetof(ParserDeferredList, tail) == 4 && offsetof(ParserDeferredList, flags) == 8) ? 1 : -1];
ParserDeferredList *RECON_THISCALL ParserDeferredList::Initialize(ParserDeferredRecord *record)
{
    head = tail = record;
    flags = (flags & ~1U) | ((record->flags >> 3) & 1);
    return this;
}

void RECON_THISCALL ParserDeferredList::Prepend(ParserDeferredRecord *record)
{
    record->next = head;
    head->previous = record;
    head = record;
    if (record->flags & 8) flags |= 1;
}

CompilerNameEntry *RECON_THISCALL CompilerSymbol::Name(void)
{
    return ((CompilerNamedSymbol *)this)->link.name;
}

/* 11b98..11bf9, 5a9d0..5aa6f: resolve an unqualified expression name,
 * retaining the lookup result's binding owner for dependent names. */
extern "C" CompilerSymbol *__cdecl recover_unresolved_name(CompilerNameEntry *);
extern "C" CompilerExpression *__cdecl make_most_derived_expression(void);
extern "C" CompilerSymbol *__cdecl declare_external_symbol(CompilerNameEntry *, unsigned);
extern "C" CompilerExpression *__cdecl lookup_name_expression(CompilerNameEntry *name)
{
    CompilerScopeMatch *match = frontend_data.translation_unit->LookupName(name, 0x133);
    CompilerSymbol *symbol = match ? match->symbol : NULL;
    if (!symbol) {
        if (name == frontend_data.builtin_names.most_derived_parameter)
            return make_most_derived_expression();
        symbol = recover_unresolved_name(name);
        if (!symbol) symbol = declare_external_symbol(name, 0);
    }
    if (symbol->definition_flags & 0x20) {
        int instantiate = (symbol->definition_flags & 0x10) != 0;
        CompilerSymbol *function = frontend_bss.current_function;
        if (!instantiate && function && (function->type_flags & 4)) {
            int index = (short)function->declaration_qualifiers;
            CompilerPointerArray *table = frontend_data.frontend_pointer_table;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            CompilerNamedSymbol *owner = (CompilerNamedSymbol *)table->items[index];
            instantiate = owner->link.name == name;
        }
        if (instantiate) instantiate_tag_definition(symbol);
    }
    if (symbol->kind == 18) {
        CompilerSymbolBindingOwner *owner = ((CompilerBoundSymbol *)symbol)->binding->owner;
        CompilerSymbol *scope = owner->symbol;
        if (scope->kind == 15) {
            int index = ((CompilerClassMemberSymbol *)scope)->owner_index;
            CompilerPointerArray *table = frontend_data.frontend_pointer_table;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            owner->symbol = (CompilerSymbol *)table->items[index];
        }
    }
    if (symbol->kind == 23) {
        compiler_warning(0x372, symbol);
        return NULL;
    }
    return make_symbol_expression(symbol);
}
extern "C" CompilerSymbol *__cdecl recover_unresolved_name(CompilerNameEntry *)
{ __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_most_derived_expression(void)
{ __asm { int 3 } return NULL; }

/* 11bf9..11c18, 5aa6f..5aab2: notify the browser before constructing
 * the expression. Constant/overload symbols have no browser reference. */
extern "C" CompilerExpression *__cdecl make_symbol_expression(CompilerSymbol *symbol)
{
    if (frontend_bss.browser_filename) {
        int publish = 1;
        if (frontend_bss.external_declaration_mode) {
            /* The browser-local selector has no encountered writer/default. */
            __asm { int 3 }
            publish = publish_browser_local;
        }
        if (publish && symbol->kind != 16 && symbol->kind != 17 && symbol->kind != 25)
            frontend_data.backend->Slot1c((unsigned)symbol);
    }
    return make_resolved_symbol_expression(symbol, 0);
}

extern "C" {
int __cdecl can_reference_symbol(CompilerSymbol *); /*16100*/
CompilerExpression *__cdecl check_symbol_expression(CompilerSymbol *, CompilerExpression *); /*16176*/
CompilerExpression *__cdecl make_dependent_name_expression(CompilerSymbol *, CompilerSymbol *); /*21c7f*/
CompilerExpression *__cdecl make_enumerator_expression(CompilerSymbol *); /*208eb*/
void __cdecl complete_expression_class(CompilerSymbol *, unsigned); /*32406*/
CompilerGlobalSymbolView *__cdecl get_symbol_context(CompilerGlobalSymbolView *, CompilerGlobalSymbolView **);
}

/* 11c18..11ee0, 5aab2..5ab8a. The original name and its selected binding
 * remain distinct: a qualified expression may retain the former even when
 * its ordinary symbol/type tests use the latter. */
extern "C" CompilerExpression *__cdecl make_resolved_symbol_expression(
    CompilerSymbol *original, unsigned mode)
{
    CompilerSymbol *selected, *scope;
    CompilerBoundSymbol *bound;
    CompilerExpression *expression;
    CompilerType *type;
    CompilerPointerArray *table;
    int index;
restart:
    bound = NULL;
    if (original->kind == 18) {
        bound = (CompilerBoundSymbol *)original;
        if (!bound->binding) return NULL;
        selected = bound->binding->owner->symbol;
    } else selected = original;
    if (selected->kind == 24)
        selected = ((CompilerAliasSymbol *)selected)->target->symbol;

    switch (selected->kind) {
    case 23:
        compiler_warning(0x372, selected);
        return NULL;
    case 22:
        expression = ((CompilerGlobalSymbolView *)selected)->reference_expression;
        return expression ? instantiate_parser_expression(expression, 0) : NULL;
    case 5:
        if (frontend_data.translation_unit->opaque_lookup_state)
            return symbol_expression(original, 0);
        check_qualified_declaration(bound);
        if (original->type_flags & 0x80)
            return make_dependent_name_expression(original, selected);
        return make_enumerator_expression(selected);
    case 16: case 7: case 6: case 25:
        return symbol_expression(selected, 0);
    case 17: case 8:
        if (mode) break;
        /* A plain overload name follows the same bound-name resolution
         * as a class/type name. */
    case 2: case 3:
        if (frontend_data.translation_unit->opaque_lookup_state)
            return symbol_expression(original, 0);
        if (!is_class_scope_symbol(selected)) {
            if (!can_reference_symbol(selected)) return (CompilerExpression *)bound;
            if ((original->type_flags & 0x80) && !(selected->type_flags & 0x10))
                return make_dependent_name_expression(original, selected);
            return original->kind == 18 ? symbol_expression(original, 0) : NULL;
        }
        check_qualified_declaration(bound);
        scope = bound->binding->owner->scope->symbol;
        if (scope->kind == 15) {
            index = ((CompilerClassMemberSymbol *)scope)->owner_index;
            table = frontend_data.frontend_pointer_table;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            scope = (CompilerSymbol *)table->items[index];
        }
        if (!(selected->definition_flags & 4)) {
            CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)scope)->class_detail;
            if ((detail->completion_flags & 1) && !(detail->template_flags & 1))
                complete_expression_class(scope, 0);
        }
        original = resolve_generated_symbol(selected);
        mode = 0;
        goto restart;
    }
    if (selected->definition_flags & 0x10) return symbol_expression(original, 0);
    if (selected->kind == 10 && (selected->qualifiers & 0x10)) {
        CompilerGlobalSymbolView *context;
        CompilerGlobalSymbolView *member = get_symbol_context(
            (CompilerGlobalSymbolView *)selected, &context);
        if (!can_reference_symbol(selected)) return (CompilerExpression *)member;
        index = ((CompilerAliasSymbol *)member)->target->owner_index;
        table = frontend_data.frontend_pointer_table;
        if (index < 0 || index >= table->count) { __asm { int 3 } }
        CompilerSymbol *owner = (CompilerSymbol *)table->items[index];
        CompilerSymbol *lookup = frontend_data.translation_unit->LookupScopeName(
            owner, context->name, 2, 0);
        CompilerExpression *left = symbol_expression(lookup, 0);
        CompilerExpression *right = make_resolved_symbol_expression((CompilerSymbol *)member, 0);
        return make_member_access_expression(0x50, right, left);
    }
    if (!can_reference_symbol(selected)) return NULL;
    expression = symbol_expression(selected,
        selected->kind == 7 || selected->kind == 8 || selected->kind == 6);
    if (!expression) return NULL;
    if (selected->kind == 7 || selected->kind == 8 || selected->kind == 6) {
        type = ((CompilerIrSymbol *)selected)->type;
        if (!type || !type->shape) return NULL;
        expression->type = make_assignment_function_type(type, 0xb,
            (BYTE)type->shape->attributes & 0x73, 0);
        return expression;
    }
    if ((selected->qualifiers & 4) && selected->kind != 11 &&
        selected->kind != 12 && selected->kind != 22) return expression;
    if (expression->kind == 0x98) return expression;
    return check_symbol_expression(selected, expression);
}
/* 16100..16176, 5d218..5d28f: restricted-expression visibility. */
extern "C" int __cdecl can_reference_symbol(CompilerSymbol *symbol)
{
    unsigned flags = frontend_data.source_mode_flags;
    if (!(flags & 0x10)) return 1;
    if (frontend_bss.external_declaration_mode == 0xfa &&
        (symbol->qualifiers & 0x40)) return 1;
    unsigned kind = symbol->kind;
    if (kind == 16 || kind == 7 || kind == 6 || kind == 17 || kind == 8 || kind == 25)
        return 1;
    if (kind == 24) {
        unsigned selected = ((CompilerAliasSymbol *)symbol)->target->symbol->kind;
        if (selected == 16 || selected == 7 || selected == 6 ||
            selected == 17 || selected == 8 || selected == 25) return 1;
    }
    if (kind == 2 || kind == 3) {
        compiler_warning(0x288, symbol);
        return 0;
    }
    if (!(flags & 0x100000)) {
        unsigned storage = ((CompilerIrSymbol *)symbol)->storage_class & 7;
        if (storage >= 4 && storage <= 6) {
            compiler_warning(0x24b, symbol);
            return 0;
        }
    }
    return 1;
}
/* 16176..161e7, 5d28f..5d298: retain constant qualification and
 * propagate the symbol's failed-reference state after temporary conversion. */
extern "C" CompilerExpression *__cdecl check_symbol_expression(
    CompilerSymbol *symbol, CompilerExpression *input)
{
    CompilerExpression *expression = convert_return_temporary(input);
    if (symbol->qualifiers & 0x40) {
        CompilerExpression *value = resolve_constant_reference(expression);
        if ((value->flags & 7) == 3 && !value->type->shape &&
            (value->type->flags & 0x240)) expression->constant_flags |= 1;
    }
    if (is_function_specialization(symbol)) expression = make_cast_expression(0x3d, expression);
    if (symbol->scope_flags & 4) expression->flags |= 8;
    return expression;
}
/* 21c7f..21d58, 6568f..6571d. Keep the selected name distinct from
 * its original binding, including the singleton-binding result annotation. */
extern "C" CompilerExpression *__cdecl make_dependent_name_expression(
    CompilerSymbol *original, CompilerSymbol *selected)
{
    if (selected->kind == 5) {
        check_qualified_declaration(original);
        return make_enumerator_expression(selected);
    }
    CompilerRuntimeFunction *function = (CompilerRuntimeFunction *)frontend_bss.current_function;
    if (!function) {
        if (selected->kind == 17 || selected->kind == 8) return symbol_expression(original, 0);
        compiler_warning(0x28e, selected);
        return NULL;
    }
    if (function->builtin_flags & 1) {
        CompilerExpression *object = make_this_pointer_expression(NULL);
        CompilerExpression *member = symbol_expression(original, 0);
        CompilerExpression *result = make_member_access_expression(0x53, object, member);
        if (result->kind == 0x3d && original->kind == 18) {
            CompilerSymbolBindingOwner *next = ((CompilerBoundSymbol *)original)->binding->owner->next;
            if (next && next == next->next) result->value.name_binding.symbol = original;
        } else if (result->kind == 0x53 && original->kind == 18) {
            CompilerSymbolBindingOwner *binding = ((CompilerBoundSymbol *)original)->binding->owner;
            if (binding->symbol->kind == 17 || binding->symbol->kind == 8) {
                CompilerSymbolBindingOwner *next = binding->next;
                if (next && next == next->next)
                    result->value.operands.second->value.name_binding.symbol = original;
            }
        }
        return result;
    }
    if (!(function->type_flags & 4)) {
        if (selected->kind == 17 || selected->kind == 8) return symbol_expression(original, 0);
        compiler_warning(0x146, function, original);
        return recover_new_expression();
    }
    if (selected->kind != 3 || original->kind == 18) return symbol_expression(original, 0);
    compiler_warning(0x255, selected);
    return recover_new_expression();
}
extern "C" CompilerExpression *__cdecl make_enumerator_expression(CompilerSymbol *)
{ __asm { int 3 } return NULL; }
extern "C" void __cdecl complete_expression_class(CompilerSymbol *, unsigned)
{ __asm { int 3 } }

/* 10187..101c8: the one-operand node stores only kind, flags, operand and
 * type. The remaining expression storage is neither cleared nor interpreted. */
extern "C" CompilerExpression *__cdecl make_immediate_expression(unsigned kind,
    CompilerType *type, CompilerExpression *operand)
{
    CompilerArena *arena = &frontend_bss.arenas[frontend_data.constant_arena_index];
    CompilerExpression *expression = (CompilerExpression *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    expression->kind = (BYTE)kind;
    expression->flags = 1;
    expression->value.operands.first = operand;
    expression->type = type;
    return expression;
}

extern "C" CompilerExpression *__cdecl attach_parser_expression_work(CompilerExpression *, CompilerWorkLink *); /*8e66d*/
/* 3220e..32257, 6d7ce..6d7e5: append the constructor initializer to
 * its cyclic expression chain, retaining pending work on that element. */
extern "C" CompilerExpression *__cdecl append_parser_expression(
    CompilerExpression *list, CompilerExpression *element)
{
    if (!element) return list;
    CompilerWorkLink *work = collect_pending_expression_work();
    if (work) element = attach_parser_expression_work(element, work);
    CompilerExpression *link = make_binary_node(0x60, element, list, NULL);
    if (!list) link->list_next = link;
    else {
        link->list_next = list->list_next;
        list->list_next = link;
    }
    element->list_next = link;
    return link;
}
extern "C" CompilerExpression *__cdecl attach_parser_expression_work(CompilerExpression *, CompilerWorkLink *)
{ __asm { int 3 } return NULL; }

extern "C" CompilerExpression *__cdecl resolve_overloaded_operator(unsigned,
    CompilerExpression *, CompilerExpression *, unsigned, unsigned, unsigned *); /*14f0c*/
/* 15738..15805, 5cd86..5cdd8: overloadable unary operator preparation. */
extern "C" CompilerExpression *__cdecl make_unary_expression(unsigned operation,
    CompilerExpression *operand)
{
    if (!operand) return NULL;
    if (operand->kind == 0xee) {
        operand = expand_initializer_expression(operation, operand, 0);
        if (!operand) return NULL;
    }
    if (operand->kind == 0x98) {
        compiler_warning(0x113, operand->value.symbol);
        return NULL;
    }
    int overloadable = 0;
    CompilerType *type = operand->type;
    if (type && ((type->flags & 0x600) ||
        (type->flags & 0x2fdf) == 0x12 || (type->flags & 0x2fdf) == 0x11)) {
        CompilerTypeShape *shape = type->shape;
        overloadable = !shape || ((shape->flags & 0x40) && !shape->base);
    }
    if (overloadable) {
        unsigned restore_mode = frontend_data.compiler_mode_flags & 4;
        if (restore_mode) frontend_data.compiler_mode_flags &= ~4U;
        unsigned failed = 0;
        CompilerExpression *result = resolve_overloaded_operator(operation, operand, NULL, 1, 1, &failed);
        if (restore_mode) frontend_data.compiler_mode_flags |= 4;
        if (result) return result;
        if (failed) return NULL;
    }
    return make_cast_expression(operation, operand);
}
extern "C" CompilerExpression *__cdecl resolve_overloaded_operator(unsigned,
    CompilerExpression *, CompilerExpression *, unsigned, unsigned, unsigned *)
{ __asm { int 3 } return NULL; }

extern "C" {
int __cdecl check_unary_operand(unsigned, unsigned, CompilerExpression *, CompilerExpression *); /*101c8*/
int __cdecl is_identity_unary_conversion(unsigned, WORD); /*12ccd*/
CompilerType *__cdecl dereference_expression_type(CompilerType *); /*12cdb*/
CompilerExpression *__cdecl make_scalar_comparison(unsigned, CompilerExpression *, CompilerExpression *); /*12ff1*/
CompilerType *__cdecl adjust_reference_type(CompilerType *, unsigned); /*3fef0*/
CompilerExpression *__cdecl promote_scalar_expression(CompilerExpression *); /*1ed9f*/
CompilerType *__cdecl replace_function_parameters(CompilerType *, unsigned, WORD, const void *); /*2d6b2*/
int __cdecl subtract_extended_constants(const CompilerExtendedConstant *,
    const CompilerExtendedConstant *, CompilerExtendedConstant *); /*a1a37*/
void *__cdecl copy_extended_constant(const void *, unsigned); /*4b150*/
}

/* 10239..10701, 59b12..59fad. These are the actual unary operation
 * groups. The original default is defective: its ordinary uninitialized
 * integer local flows to common folding without a writer. */
extern "C" CompilerExpression *__cdecl make_cast_expression(unsigned operation,
    CompilerExpression *value)
{
    CompilerExpression *result, *right;
    CompilerSymbol *symbol, *selected;
    CompilerBoundSymbol *bound;
    CompilerType *type;
    CompilerTypeShape *shape;
    WORD shape_flags, flags;
    unsigned width, shift;
    unsigned __int64 bits;
    CompilerExtendedConstant floating;
    if (!value) return NULL;
    if (!value->type) {
        if ((BYTE)operation == 0x76) return value;
        if (value->kind == 0x31) {
            compiler_warning(0xab, (BYTE)operation);
            return value;
        }
        if (value->kind == 0x50 || value->kind == 0x53 ||
            value->kind == 0x92 || value->kind == 0x93) {
            right = value->value.operands.second;
            if ((BYTE)operation == 0x31 && right && right->kind == 6 && right->value.symbol) {
                selected = right->value.symbol;
                if (selected->kind == 18)
                    selected = ((CompilerBoundSymbol *)selected)->binding->owner->symbol;
                if (can_resolve_assignment_symbol(selected, 1)) {
                    selected = resolve_assignment_symbol(selected, 1);
                    if ((((CompilerIrSymbol *)selected)->storage_class & 7) == 3) {
                        if (frontend_data.compiler_mode_flags & 4) return value;
                        right->value.symbol = selected;
                        right->type = ((CompilerIrSymbol *)selected)->type;
                        value->type = right->type;
                        goto typed_value;
                    }
                }
            }
            if (value->kind == 0x92 || value->kind == 0x93)
                compiler_warning(0x12a, (BYTE)operation);
            else compiler_warning(0x114, 0x4c, (BYTE)operation);
            return NULL;
        }
        symbol = value->value.symbol;
        bound = symbol->kind == 18 ? (CompilerBoundSymbol *)symbol : NULL;
        selected = bound ? bound->binding->owner->symbol : symbol;
        if (selected->kind == 3) {
            if ((BYTE)operation == 0x31) return make_member_pointer_expression((CompilerExpression *)symbol);
            compiler_warning(0xab, (BYTE)operation);
            return NULL;
        }
        if (selected->kind == 2) {
            compiler_warning(0x87, selected);
            return NULL;
        }
        if (selected->type_flags & 0x30) {
            compiler_warning(0x115, selected);
            return NULL;
        }
        if (!can_resolve_assignment_symbol(selected, 1)) {
            if ((BYTE)operation == 0x31) return make_immediate_expression(0x31, NULL, value);
            compiler_warning(0xf4, selected);
            return NULL;
        }
        if ((selected->kind == 17 || selected->kind == 8) && (BYTE)operation == 0x31) {
            bound->binding->owner->symbol = resolve_assignment_symbol(selected, 1);
            return make_member_pointer_expression((CompilerExpression *)bound);
        }
        selected = resolve_assignment_symbol(selected, 1);
        value->value.symbol = selected;
        value->type = ((CompilerIrSymbol *)selected)->type;
    }
typed_value:
    type = value->type;
    shape = type->shape;
    shape_flags = shape ? shape->flags : 0;
    result = make_immediate_expression(operation, type, value);
    if (!check_unary_operand(operation, 1, NULL, value)) return NULL;
    switch ((BYTE)operation) {
    case 0x31:
        return prepare_parameter_expression(value, 12);
    case 0x3d: case 0xa6:
        if (is_identity_unary_conversion(operation, shape_flags))
            return result->value.operands.first;
        if ((shape_flags & 0x77) == 3 || (shape_flags & 0x44) == 0x44) {
            if (shape_flags & 8) return value;
            value->type = adjust_reference_type(value->type, 1);
            return value;
        }
        if ((shape_flags & 0x77) == 1 || (shape_flags & 0x40)) {
            result->type = dereference_expression_type(value->type);
            type = result->type;
            shape = type->shape;
            if (!shape) {
                if ((type->flags & 0x2fdf) == 0x80) {
                    compiler_warning(0x64);
                    result = result->value.operands.first;
                }
            } else if ((shape->flags & 0x77) == 2) {
                value->flags |= 0x20;
                WORD qualifiers = (shape->attributes & 0x1f00) |
                    ((BYTE)value->type->shape->attributes & 0x73);
                value->type = replace_function_parameters(type, 2, qualifiers, &shape->payload);
                return value;
            } else if ((shape->flags & 0x77) == 1 && (BYTE(shape->attributes) & 3))
                return qualify_reference_expression(result);
        } else if ((shape_flags & 0x77) == 2)
            return make_scalar_comparison(0x41, value, (CompilerExpression *)make_integer_constant(0));
        else if ((shape_flags & 0x77) == 4) return value;
        else {
            compiler_warning(0x64);
            result = result->value.operands.first;
        }
        if ((value->flags & 7) == 4 && value->kind == 6 &&
            (result->flags & 7) == 1 && result->kind == 0x3d) {
            right = result->value.operands.first;
            if ((right->flags & 7) == 4 && right->kind == 6) right->flags &= ~0x20;
        }
        return result;
    case 0x5a:
        if (shape_flags || !(value->type->flags & 0x340)) {
            compiler_warning(0xab, 0x5a);
            return result;
        }
        if (!value->type->shape && (value->type->flags & 0x2fdf) == 0x40)
            report_language_diagnostic(1, 0x324, value->type);
        value = promote_scalar_expression(value);
        result->type = value->type;
        flags = value->type->flags;
        if ((flags & 0x4000) || (frontend_data.options.unsigned_char &&
            (flags & 0x2fdf) == 0x41 && !(flags & 0x8000)))
            report_language_diagnostic(2, 0x92);
        value = resolve_constant_reference(value);
        if ((value->flags & 7) != 3) return result;
        flags = value->type->flags;
        if (flags & 0x100) {
            if (subtract_extended_constants(&compiler_extended_zero,
                (const CompilerExtendedConstant *)value->value.pointer, &floating))
                report_language_diagnostic(2, 0x38);
            value->value.pointer = copy_extended_constant(&floating, frontend_data.constant_arena_index);
            return value;
        }
        bits = 0 - (unsigned __int64)value->value.integer;
        if (!(flags & 0x4000)) {
            width = ((flags & 0x340) || !(flags & 0xfff0)) ?
                (unsigned)(int)compiler_primitive_type_bytes[flags & 15] : flags & 15;
            if (width > 8) { __asm { int 3 } }
            if (!width) bits = 0;
            else {
                shift = (8 - width) * 8;
                bits = (__int64)(bits << shift) >> shift;
            }
        }
        break;
    case 0x3c:
        if (!(value->type->flags & 0x340) && !(shape_flags & 3)) {
            compiler_warning(0xab, 0x3c);
            return result;
        }
        result->type = frontend_data.builtin_types.type_0060;
        result->constant_flags |= 1;
        value = resolve_constant_reference(value);
        if ((value->flags & 7) != 3) return result;
        if (!(value->type->flags & 0x240) && !(shape_flags & 1)) {
            result = make_scalar_comparison(0x3b, value, (CompilerExpression *)make_integer_constant(0));
            result->constant_flags |= 1;
            return result;
        }
        bits = !value->value.integer;
        break;
    case 0x59:
        if (!(value->type->flags & 0x240) || shape_flags) {
            compiler_warning(0xab, 0x59);
            return result;
        }
        if (!value->type->shape && (value->type->flags & 0x2fdf) == 0x40)
            report_language_diagnostic(1, 0x324, value->type);
        value = promote_scalar_expression(value);
        result->type = value->type;
        value = resolve_constant_reference(value);
        if ((value->flags & 7) != 3) return result;
        bits = ~(unsigned __int64)value->value.integer;
        break;
    case 0x76:
        if (!(result->type->flags & 0x100)) return value;
        value = resolve_constant_reference(value);
        if ((value->flags & 7) == 3 || shape_flags) return value;
        return result;
    case 0x86:
        if (!(shape_flags & 1) && (shape_flags || !(value->type->flags & 0x340)))
            compiler_warning(0xab, 0x86);
        value = promote_scalar_expression(value);
        result->type = value->type;
        value = resolve_constant_reference(value);
        if ((value->flags & 7) != 3) return result->type == value->type ? value : result;
        if (!shape_flags && (value->type->flags & 0x100)) return value;
        bits = value->value.integer;
        break;
    default:
        /* Original defect: bits is uninitialized on this path. The Win32
         * build retains that source behavior. Native private stack/compiler
         * representation is unqualified, so stop before reading it. */
#ifdef __linux__
        recon_unsupported("unary default: unqualified private stack value");
#else
        break;
#endif
    }
    flags = value->type->flags;
    if (!(flags & 0x80) && ((flags & 0x4000) || (__int64)bits > 0)) {
        width = ((flags & 0x340) || !(flags & 0xfff0)) ?
            (unsigned)(int)compiler_primitive_type_bytes[flags & 15] : flags & 15;
        if (width > 8) { __asm { int 3 } }
        if (!width) bits = 0;
        else {
            shift = (8 - width) * 8;
            bits = (bits << shift) >> shift;
        }
    }
    if (frontend_data.source_mode_flags & 8) value = instantiate_parser_expression(value, 5);
    value->value.integer = bits;
    value->type = result->type;
    return value;
}

extern "C" int __cdecl permits_incomplete_operand(unsigned, unsigned, unsigned); /*21db9*/
/* 101c8..10239, 59ae6..59b12: classification is computed for each
 * supplied operand before deciding whether the operator permits it. */
extern "C" int __cdecl check_unary_operand(unsigned operation, unsigned arity,
    CompilerExpression *left, CompilerExpression *right)
{
    unsigned incomplete_left = left ? get_type_classification(left->type) & 0x80 : 0;
    unsigned incomplete_right = right ? get_type_classification(right->type) & 0x80 : 0;
    if (!incomplete_left && !incomplete_right) return 1;
    if (permits_incomplete_operand(operation, arity, incomplete_left)) return 1;
    compiler_warning(0x58, (BYTE)operation, incomplete_left ? left->type : right->type);
    return 0;
}
extern "C" int __cdecl permits_incomplete_operand(unsigned, unsigned, unsigned)
{ __asm { int 3 } return 0; }
/* 12ccd..12cdb, 5b30c..5b329: invalid qualifier forms retain the
 * original operand after reporting the operator byte. */
extern "C" int __cdecl is_identity_unary_conversion(unsigned operation, WORD flags)
{
    if (!(flags & 0x30)) return 0;
    compiler_warning(0xab, (BYTE)operation);
    return 1;
}
/* 12cdb..12d26, 5b329..5b33b: retain the extracted payload pointer,
 * then merge the shape's low qualifiers into the stripped, interned type. */
extern "C" CompilerType *__cdecl dereference_expression_type(CompilerType *type)
{
    WORD qualifiers = type->shape->attributes & 3;
    if (qualifiers == 1) compiler_warning(0xd6);
    frontend_bss.last_extracted_type_payload = type->shape->payload.type;
    CompilerType *result = get_function_return_type(type);
    result->attributes = (result->attributes & ~3) | qualifiers;
    return intern_type(result);
}
extern "C" void __cdecl balance_binary_operands(CompilerExpression **, CompilerExpression **);
extern "C" int __cdecl check_void_binary_operands(CompilerExpression *, CompilerExpression *, unsigned);
extern "C" CompilerExpression *__cdecl convert_binary_operand(CompilerType *, CompilerExpression *);
extern "C" CompilerExpression *__cdecl check_assignment_destination(CompilerExpression *);
extern "C" CompilerExpression *__cdecl prepare_assignment_target(CompilerExpression *, unsigned);
extern "C" CompilerExpression *__cdecl make_boolean_binary_expression(unsigned, CompilerExpression *, CompilerExpression *);
extern "C" CompilerExpression *__cdecl convert_pointer_operand(unsigned, CompilerType *, CompilerExpression *);
extern "C" unsigned __cdecl serialize_type_reference(CompilerType *);
extern "C" CompilerType *__cdecl common_arithmetic_type(unsigned, CompilerType *, CompilerType *);
extern "C" CompilerExpression *__cdecl simplify_binary_expression(CompilerExpression *, CompilerExpression *, CompilerExpression *);
extern "C" int __cdecl invalid_pointer_arithmetic(CompilerExpression *);
extern "C" void __cdecl check_pointer_difference(unsigned, CompilerExpression *, CompilerExpression *);
extern "C" CompilerExpression *__cdecl resolve_class_pointer_difference(CompilerExpression *, CompilerExpression *, CompilerExpression *);
extern "C" void __cdecl diagnose_binary_operand(unsigned, unsigned);
extern "C" CompilerExpression *__cdecl build_member_pointer_operator(CompilerExpression *);

/* The compressed original dispatch columns are expressed as logical operator
 * groups. These macros inspect existing typed owners and introduce no calls. */
#define BINARY_VOID(t) (!(t)->shape && (((t)->flags & 0x2fdf) == 0x40))
#define BINARY_SHAPE(t) ((t)->shape ? (t)->shape->flags : 0)
#define BINARY_BAD_POINTER(f) ((((f) & 0x77) == 3) || (((f) & 0x44) == 0x44))
extern "C" CompilerExpression *__cdecl make_scalar_comparison(unsigned operation,
    CompilerExpression *left, CompilerExpression *right)
{
    CompilerExpression *result, *temporary;
    CompilerType *type, *left_type, *right_type;
    WORD left_shape, right_shape, qualifiers;
    unsigned code = (BYTE)operation, size, shift;
    int special_constant = 0, swapped = 0, preserve_left = 0;
    if (right->kind == 0xee) {
        right = expand_initializer_expression(operation, right, NULL);
        if (!right) return NULL;
    }
    if (left->kind == 0xee) {
        switch (code) {
        case 0x33: case 0x34: case 0x38: case 0x3a:
        case 0x40: case 0x41: case 0x42: case 0x47:
        case 0x49: case 0x4b: case 0x4e: case 0x52:
        case 0x54: case 0x55: case 0x57: case 0x5c:
            return expand_initializer_expression(operation, left, right);
        default:
            left = expand_initializer_expression(operation, left, NULL);
            if (!left) return NULL;
        }
    }
    /* CF298 classes34/61/74 preserve the left operand's binding form. */
    switch (code) {
    case 0x33: case 0x34: case 0x3a: case 0x42: case 0x47:
    case 0x49: case 0x4b: case 0x4e: case 0x52: case 0x57: case 0x5c:
    case 0x61: case 0x74:
        preserve_left = 1;
    }
    if (!left->type) {
        if (preserve_left) {
            left = resolve_parser_expression(left);
            if (!(left->flags & 8)) compiler_warning(0x293, code);
            return NULL;
        }
        temporary = convert_assignment_operand(right->type, left);
        if (!temporary) {
            if (!right->type) diagnose_parser_operand(operation, left);
            return NULL;
        }
        left = temporary;
    }
    if (!preserve_left) left = resolve_constant_reference(left);
    if (!right->type) {
        temporary = convert_assignment_operand(left->type, right);
        if (!temporary) {
            diagnose_parser_operand(operation, right);
            return NULL;
        }
        right = temporary;
    }
    if (BINARY_VOID(left->type) || BINARY_VOID(right->type))
        special_constant = check_void_binary_operands(left, right, operation);

    switch (code) {
    case 0x32: case 0x34: case 0x36: case 0x37: case 0x38: case 0x40:
    case 0x4f: case 0x50: case 0x53: case 0x54: case 0x55: case 0x56:
    case 0x73: case 0x74: case 0x83: case 0x92: case 0x93: case 0x94: case 0xee:
        goto make_node;
    case 0x33: case 0x3a: case 0x42: case 0x47: case 0x49:
    case 0x4b: case 0x4e: case 0x52: case 0x57: case 0x5c: case 0x61:
        switch (code) {
        case 0x33: case 0x49: case 0x4e: case 0x5c:
            if (left->type->flags & 0x100) {
                right = promote_scalar_expression(right);
                goto make_node;
            }
            if (right->type->Matches(left->type, 0) && BINARY_VOID(right->type))
                goto make_node;
        }
        right = convert_binary_operand(left->type, right);
        goto make_node;
    case 0x41:
        right = promote_scalar_expression(right);
        goto make_node;
    }
    switch (code) {
    case 0x31:
        if ((left->constant_flags | right->constant_flags) & 1) special_constant = 1;
        /* fall through: shifts and integral operators promote operands. */
    case 0x48: case 0x4d: case 0x5b:
        if (left->type->flags & 0x100) {
            right = promote_scalar_expression(right);
            goto make_node;
        }
        if (right->type->flags & 0x100) {
            left = promote_scalar_expression(left);
            goto make_node;
        }
        break;
    case 0x56:
        left = promote_scalar_expression(left);
        right = promote_scalar_expression(right);
        goto make_node;
    }
    balance_binary_operands(&left, &right);
make_node:
    result = make_binary_node(operation, left, right, NULL);
    result->constant_flags = (result->constant_flags & ~1) | (special_constant & 1);
    if (!check_unary_operand(operation, 2, left, right)) return NULL;
    switch (code) {
    case 0x38: case 0x40: case 0x54: case 0x55:
        if (BINARY_VOID(left->type))
            return make_boolean_binary_expression(operation, left, NULL);
        result->type = left->type;
        if (!result->type) return NULL;
        if (result->type->shape && is_identity_unary_conversion(operation, result->type->shape->flags))
            return NULL;
        if ((left->type->flags & 0x200) && !left->type->shape) compiler_warning(0x7e, code);
        temporary = prepare_assignment_target(left, 1);
        if (!temporary) {
            compiler_warning(0x69, code);
            return result;
        }
        result->value.operands.first = temporary;
        left_shape = BINARY_SHAPE(result->type);
        if (!left_shape) {
            result->value.operands.second = convert_pointer_operand(operation, result->type, right);
            return result;
        }
        if ((left_shape & 0x77) == 1) {
            size = serialize_type_reference(result->type);
            frontend_bss.initializer_zero_bytes = size;
            __int64 limit = (__int64)~(~(unsigned __int64)0 <<
                (compiler_primitive_type_bytes[3] * 8 - 1));
            type = (__int64)frontend_bss.initializer_zero_bytes <= limit ?
                frontend_data.builtin_types.type_0063 : frontend_data.builtin_types.type_0044;
            result->value.operands.second = make_typed_constant(type,
                &frontend_bss.initializer_zero_bytes, frontend_data.constant_arena_index);
        } else if (BINARY_BAD_POINTER(left_shape)) compiler_warning(0xab, code);
        if (result->type->shape->attributes & 3) return qualify_reference_expression(result);
        return result;

    case 0x51: /* addition, including pointer plus integral offset */
        if (left->type && left->type->shape && (left->type->shape->flags & 0x77) == 2)
            left->type = reference_expression_type(left);
        if (right->type && right->type->shape && (right->type->shape->flags & 0x77) == 2)
            right->type = reference_expression_type(right);
        result->type = common_arithmetic_type(operation, left->type, right->type);
        if (!result->type) return NULL;
        left_shape = BINARY_SHAPE(left->type);
        right_shape = BINARY_SHAPE(right->type);
        if (invalid_pointer_arithmetic(result)) return NULL;
        if (!(left_shape & 3)) {
            if (right_shape & 3) {
                result->value.operands.first = right;
                result->value.operands.second = left;
                temporary = left; left = right; right = temporary;
                result->type = left->type;
                qualifiers = left_shape; left_shape = right_shape; right_shape = qualifiers;
                swapped = 1;
            } else {
                if (((left->type->flags & 0x80) && !left_shape) ||
                    ((right->type->flags & 0x80) && !right_shape)) compiler_warning(0xba, code);
                return simplify_binary_expression(result, left, right);
            }
        }
        if (BINARY_BAD_POINTER(left_shape)) {
            diagnose_binary_operand(operation, swapped ? 2 : 1);
            return NULL;
        }
        if (right_shape & 3) { compiler_warning(0x6e); return NULL; }
        if (!(right->type->flags & 0x240)) { compiler_warning(0x6f); return NULL; }
        frontend_bss.initializer_zero_bytes = serialize_type_reference(left->type);
        temporary = make_typed_constant(frontend_data.builtin_types.type_0044,
            &frontend_bss.initializer_zero_bytes, frontend_data.constant_arena_index);
        result->value.operands.second = make_scalar_comparison(0x4a, right, temporary);
        return result;

    case 0x46: /* subtraction and pointer difference */
        if (left->type && left->type->shape && (left->type->shape->flags & 0x77) == 2)
            left->type = reference_expression_type(left);
        if (right->type && right->type->shape && (right->type->shape->flags & 0x77) == 2)
            right->type = reference_expression_type(right);
        result->type = common_arithmetic_type(operation, left->type, right->type);
        if (!result->type) return NULL;
        if (invalid_pointer_arithmetic(result)) return NULL;
        left_type = left->type; right_type = right->type;
        left_shape = BINARY_SHAPE(left_type); right_shape = BINARY_SHAPE(right_type);
        if (left_shape && (left_shape & 0x77) <= 2) {
            size = serialize_type_reference(left_type);
            if (right_shape && (right_shape & 0x77) <= 2) {
                if ((left->type->flags & 0x400) && (right->type->flags & 0x400)) {
                    result = resolve_class_pointer_difference(result, left, right);
                    if (!result) return NULL;
                    left = result->value.operands.first;
                    right = result->value.operands.second;
                    size = serialize_type_reference(left->type);
                }
                check_pointer_difference(0x46, left, right);
                result->type = frontend_data.builtin_types.type_0063;
                result = simplify_binary_expression(result, left, right);
                if (size == 1) return result;
                if (!(size & (size - 1))) {
                    size >>= 2;
                    shift = 1;
                    while (size) { ++shift; size >>= 1; }
                    return make_scalar_comparison(0x58, result,
                        (CompilerExpression *)make_integer_constant(shift));
                }
                return make_scalar_comparison(0x39, result,
                    (CompilerExpression *)make_integer_constant(size));
            }
            if (BINARY_BAD_POINTER(right_shape)) { diagnose_binary_operand(operation, 2); return NULL; }
            if (!(right_type->flags & 0x240)) { compiler_warning(0x70); return result; }
            frontend_bss.initializer_zero_bytes = (__int64)(int)size;
            temporary = make_typed_constant(frontend_data.builtin_types.type_0044,
                &frontend_bss.initializer_zero_bytes, frontend_data.constant_arena_index);
            result->value.operands.second = make_scalar_comparison(0x4a, right, temporary);
            return result;
        }
        if (BINARY_BAD_POINTER(left_shape)) { diagnose_binary_operand(operation, 1); return NULL; }
        if (right_shape & 3) { compiler_warning(0x71); return NULL; }
        if (((left_type->flags & 0x80) && !left_shape) ||
            ((right_type->flags & 0x80) && !right_shape)) compiler_warning(0xba, code);
        return simplify_binary_expression(result, left, right);

    case 0x43: case 0x58:
        result->value.operands.first = left;
        result->type = left->type;
        goto finish_numeric;
    case 0x31: case 0x39: case 0x48: case 0x4a: case 0x4d: case 0x5b:
        result->type = common_arithmetic_type(operation, left->type, right->type);
finish_numeric:
        if (!result->type) return NULL;
        return simplify_binary_expression(check_assignment_destination(result), left, right);

    case 0x33: case 0x3a: case 0x42: case 0x47: case 0x49:
    case 0x4b: case 0x4e: case 0x52: case 0x57: case 0x5c:
        if (BINARY_VOID(left->type)) return make_boolean_binary_expression(operation, left, right);
        /* fall through: assignment retains the real left type and target. */
    case 0x34: case 0x74:
        result = check_assignment_destination(result);
        if (!result) return NULL;
        result->type = left->type;
        result->value.operands.first = prepare_assignment_target(left, code != 0x74);
        if (!result->value.operands.first) {
            compiler_warning(0x6a, code);
            return result;
        }
        if (result->type->shape && (result->type->shape->attributes & 3)) {
            if (code == 0x74 || code == 0x34)
                result->value.operands.second = materialize_return_value(result->type, result->value.operands.second);
            return qualify_reference_expression(result);
        }
        return result;

    case 0x41: /* indexing */
        if (left->type && left->type->shape && (left->type->shape->flags & 0x77) == 2)
            left->type = reference_expression_type(left);
        if (right->type && right->type->shape && (right->type->shape->flags & 0x77) == 2)
            right->type = reference_expression_type(right);
        result->type = common_arithmetic_type(operation, left->type, NULL);
        if (!result->type) return NULL;
        left_shape = BINARY_SHAPE(left->type); right_shape = BINARY_SHAPE(right->type);
        if (!left_shape || (left_shape & 0x77) > 2) {
            if (!right_shape || (right_shape & 0x77) > 2) { compiler_warning(0x6d); return result; }
            result->value.operands.first = right;
            result->value.operands.second = left;
            temporary = left; left = right; right = temporary;
            result->type = left->type;
            right_shape = left_shape;
        }
        if ((left->type->shape->attributes & 3) == 1) { compiler_warning(0xd6); return result; }
        if (right_shape) { compiler_warning(0x6b); return result; }
        if (!(right->type->flags & 0x240)) { compiler_warning(0x6c); return result; }
        frontend_bss.initializer_zero_bytes = serialize_type_reference(left->type);
        temporary = make_typed_constant(frontend_data.builtin_types.type_0044,
            &frontend_bss.initializer_zero_bytes, frontend_data.constant_arena_index);
        right = make_scalar_comparison(0x4a, right, temporary);
        result->value.operands.second = right;
        right = resolve_constant_reference(right);
        if ((right->flags & 7) == 3 && left->kind == 0x41 &&
            (left->value.operands.second->flags & 7) == 3) {
            result->value.operands.first = left->value.operands.first;
            right->value.integer = (__int64)((unsigned __int64)right->value.integer +
                (unsigned __int64)left->value.operands.second->value.integer);
        }
        qualifiers = left->type->shape->attributes & 0x1f00;
        result->type = intern_type(get_function_return_type(left->type));
        if (right->type->shape && (right->type->shape->flags & 0x77) == 2 && qualifiers) {
            type = result->type;
            if (type->shape) {
                if (qualifiers != (type->shape->attributes & 0x1f00)) {
                    unsigned payload = type->shape->payload.value;
                    result->type = replace_function_parameters(type, type->shape->flags,
                        type->shape->attributes | qualifiers, &payload);
                }
            } else type->attributes |= qualifiers;
        }
        if (!result->type->shape || (result->type->shape->flags & 0x77) != 2)
            return convert_return_temporary(result);
        return result;

    case 0x3b: case 0x3e: case 0x3f: case 0x44: case 0x45: case 0x4c:
        if ((left->flags & 7) == 5 || (right->flags & 7) == 5)
            report_language_diagnostic(4, 0x82, code);
        /* fall through: comparisons and logical operators produce bool. */
    case 0x32: case 0x4f:
        result = check_assignment_destination(result);
        if (!result) return NULL;
        result->type = frontend_data.builtin_types.type_0060;
        result = simplify_binary_expression(result, left, right);
        result->constant_flags |= 1;
        return result;
    case 0x37: case 0x56:
        result->type = right->type;
        if (right->flags & 0x80) result->flags |= 0x80;
        if (result->type && result->type->shape && (result->type->shape->flags & 0x77) == 2)
            result->type = reference_expression_type(result);
        return result;
    case 0x92: case 0x93:
        type = left->type;
        if (!(type->flags & 0x400) || (code == 0x92 && type->shape) ||
            (code == 0x93 && (!type->shape || (type->shape->flags & 0x77) != 1 || type->shape->base))) {
            diagnose_binary_operand(operation, 1);
            return NULL;
        }
        if (right->type->shape && (right->type->shape->flags & 0x30))
            return build_member_pointer_operator(result);
        diagnose_binary_operand(operation, 2);
        return NULL;
    default:
        return result;
    }
}
#undef BINARY_BAD_POINTER
#undef BINARY_SHAPE
#undef BINARY_VOID
extern "C" CompilerType *__cdecl select_binary_conversion_type(CompilerType *, CompilerExpression **);
extern "C" void __cdecl balance_binary_operands(CompilerExpression **left, CompilerExpression **right)
{
    CompilerType *type;
    if (*right) {
        if (!(*right)->type) *right = NULL;
        else if (!(*right)->type->shape) {
            type = select_binary_conversion_type((*left)->type, right);
            if (type) *right = make_typed_expression(0x64, type, *right, 0);
        }
    }
    if (*left) {
        if (!(*left)->type) *left = NULL;
        else if (!(*left)->type->shape) {
            type = select_binary_conversion_type((*right)->type, left);
            if (type) *left = make_typed_expression(0x64, type, *left, 0);
        }
    }
}
/* CF000 supplies the established primitive widths. Encoded nonprimitive
 * widths use their low nibble directly; no additional type-table data is read. */
#define BINARY_TYPE_WIDTH(f) (((f) & 0x340) || !((f) & 0xfff0) ? \
    (int)compiler_primitive_type_bytes[(f) & 15] : (int)((f) & 15))
extern "C" CompilerType *__cdecl select_binary_conversion_type(CompilerType *context,
    CompilerExpression **operand)
{
    CompilerExpression *value = *operand;
    WORD flags = value->type->flags;
    WORD context_flags;
    int width, context_width;
    int int_width = compiler_primitive_type_bytes[3];
    if (flags & 0x240) {
        context_flags = context->flags;
        if ((context_flags & 0x240) || context->shape) {
            context_width = context->shape ? int_width : BINARY_TYPE_WIDTH(context_flags);
            width = BINARY_TYPE_WIDTH(flags);
            if (context_width <= int_width && width < int_width)
                return frontend_data.builtin_types.type_0063;
            if (context_width > width) return context;
            if ((flags & 0x200) || (flags & 0x2fff) == 0x60)
                return frontend_data.builtin_types.type_0063;
            return NULL;
        }
        width = BINARY_TYPE_WIDTH(flags);
        if (context_flags & 0x100) {
            if (width < int_width)
                *operand = make_typed_expression(0x64, frontend_data.builtin_types.type_0063, value, 0);
            return context;
        }
        return width < int_width ? frontend_data.builtin_types.type_0063 : NULL;
    }
    if ((flags & 0x100) && (context->flags & 0x100)) {
        context_width = BINARY_TYPE_WIDTH(context->flags);
        width = BINARY_TYPE_WIDTH(flags);
        if (context_width > width) return context;
    }
    return NULL;
}
#undef BINARY_TYPE_WIDTH

extern "C" int __cdecl check_void_binary_operands(CompilerExpression *, CompilerExpression *, unsigned) { __asm { int 3 } return 0; }
extern "C" CompilerExpression *__cdecl convert_binary_operand(CompilerType *, CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl check_assignment_destination(CompilerExpression *) { __asm { int 3 } return NULL; }

extern "C" CompilerExpression *__cdecl make_boolean_binary_expression(unsigned, CompilerExpression *, CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl convert_pointer_operand(unsigned, CompilerType *, CompilerExpression *) { __asm { int 3 } return NULL; }
#define ARITHMETIC_TYPE_WIDTH(f) (((f) & 0x340) || !((f) & 0xfff0) ? \
    (int)compiler_primitive_type_bytes[(f) & 15] : (int)((f) & 15))
extern "C" CompilerType *__cdecl common_arithmetic_type(unsigned,
    CompilerType *left, CompilerType *right)
{
    CompilerType *selected = left;
    int left_width, right_width;
    if (!left) selected = right;
    else if (right) {
        if (left->shape) {
            if (right->shape && (right->flags & 0x80)) selected = right;
        } else if (right->shape) selected = right;
        else if (left->flags & 0x100) {
            if (right->flags & 0x100) {
                left_width = ARITHMETIC_TYPE_WIDTH(left->flags);
                right_width = ARITHMETIC_TYPE_WIDTH(right->flags);
                if (left_width < right_width) selected = right;
            }
        } else if (right->flags & 0x100) selected = right;
        else if ((left->flags & 0x240) && (right->flags & 0x240)) {
            left_width = ARITHMETIC_TYPE_WIDTH(left->flags);
            right_width = ARITHMETIC_TYPE_WIDTH(right->flags);
            if (left_width == right_width) selected = (left->flags & 0x4000) ? left : right;
            else selected = left_width > right_width ? left : right;
        }
    }
    if (selected->GetCallingConvention() & 0xf00) selected = selected->StripQualifiers();
    return selected;
}
#undef ARITHMETIC_TYPE_WIDTH
extern "C" char *__cdecl format_integer64(const __int64 *, char *, int, int);
extern "C" int __cdecl compare_extended_constants(unsigned, const CompilerExtendedConstant *, const CompilerExtendedConstant *);
extern "C" int __stdcall parse_extended_constant(const char *, CompilerExtendedConstant *);
extern "C" CompilerExpression *__cdecl fold_extended_binary_expression(CompilerExpression *, const CompilerExtendedConstant *, const CompilerExtendedConstant *);
extern "C" int __cdecl integer_difference_out_of_range(__int64, unsigned __int64, WORD, WORD);
extern "C" int __cdecl integer_product_out_of_range(__int64, unsigned __int64, WORD, WORD);
extern "C" int __cdecl validate_member_offset(__int64, __int64, WORD, WORD);

/* Control-flow source seed: C1 fold_binary_expression. This owner retains
 * C1XX's two reference resolutions, type masks and unsigned-char predicate. */
extern "C" CompilerExpression *__cdecl simplify_binary_expression(CompilerExpression *node,
    CompilerExpression *left, CompilerExpression *right)
{
    WORD left_flags = left->type->flags, right_flags = right->type->flags;
    unsigned operation;
    CompilerType *result_type = NULL;
    int left_width, right_width, width, unsigned_values, wide_result = 0;
    unsigned __int64 first, second, value;
    BYTE left_real[12], right_real[12];
    char integer_text[32];
    if (!node) return NULL;
    operation = node->kind;
    left = resolve_constant_reference(left);
    if ((left->flags & 7) != 3) return node;
    right = resolve_constant_reference(right);
    first = ((unsigned __int64)left->value.words.high << 32) | left->value.words.low;
    if ((right->flags & 7) != 3) {
        if (operation != 50 && operation != 79) return node;
        if (left_flags & 0x100) {
            /* Preserve the complete extended value, including its precision. */
            memcpy(left_real, left->value.pointer, 12);
            value = (__int64)compare_extended_constants(6, (const CompilerExtendedConstant *)left_real,
                &compiler_extended_zero);
        } else value = first != 0;
        unsigned_values = (left_flags & 0x4000) ||
            (frontend_data.options.unsigned_char &&
             (left_flags & 0x2fdf) == 0x41 && !(left_flags & 0x8000));
        /* Preserve this dispatch's actual predicate and carried truth value. */
        if (!((operation == 50) ^ (value != 0))) return node;
        frontend_bss.initializer_zero_bytes = value;
        return make_typed_constant(unsigned_values ? frontend_data.builtin_types.type_4063_first :
            frontend_data.builtin_types.type_0063, &frontend_bss.initializer_zero_bytes,
            frontend_data.constant_arena_index);
    }
    second = ((unsigned __int64)right->value.words.high << 32) | right->value.words.low;
    if ((left_flags | right_flags) & 0x100) {
        /* Real pending format/parse/fold children bound the floating domain.
         * Copy the existing extended object without interpreting its tail. */
        if (left_flags & 0x100) memcpy(left_real, left->value.pointer, 12);
        else if (parse_extended_constant(format_integer64((const __int64 *)&first, integer_text, 10,
            left_flags & 0x4000), (CompilerExtendedConstant *)left_real))
            report_language_diagnostic(2, 56);
        if (right_flags & 0x100) memcpy(right_real, right->value.pointer, 12);
        else if (parse_extended_constant(format_integer64((const __int64 *)&second, integer_text, 10,
            right_flags & 0x4000), (CompilerExtendedConstant *)right_real))
            report_language_diagnostic(2, 56);
        return fold_extended_binary_expression(node,
            (const CompilerExtendedConstant *)left_real, (const CompilerExtendedConstant *)right_real);
    }
    left_width = ((left_flags & 0x340) || !(left_flags & 0xfff0)) ?
        compiler_primitive_type_bytes[left_flags & 15] : left_flags & 15;
    right_width = ((right_flags & 0x340) || !(right_flags & 0xfff0)) ?
        compiler_primitive_type_bytes[right_flags & 15] : right_flags & 15;
    if (left->type->shape || right->type->shape) {
        unsigned_values = 1;
        WORD flags = node->type->flags;
        width = ((flags & 0x340) || !(flags & 0xfff0)) ?
            compiler_primitive_type_bytes[flags & 15] : flags & 15;
    } else if (left_width == right_width) {
        unsigned_values = left_width == 1 ? 0 : ((left_flags | right_flags) & 0x4000) != 0;
        width = left_width < compiler_primitive_type_bytes[3] ? compiler_primitive_type_bytes[3] : left_width;
    } else if (left_width > right_width) {
        unsigned_values = left_flags & 0x4000;
        width = left_width;
    } else {
        unsigned_values = right_flags & 0x4000;
        width = right_width;
    }
    if (width == compiler_primitive_type_bytes[4] &&
        ((left_width && (left_flags & 0x6f) != 0x63) ||
         (right_width && (right_flags & 0x6f) != 0x63))) wide_result = 1;
    if (operation == 50 || operation == 67 || operation == 79 || operation == 88) {
        if (left_width <= compiler_primitive_type_bytes[3]) {
            left->value.words.high = unsigned_values ? 0 : ((int)left->value.words.low < 0 ? ~0U : 0);
            first = ((unsigned __int64)left->value.words.high << 32) | left->value.words.low;
        }
    } else if (unsigned_values && !left->type->shape && !right->type->shape &&
        (((left_flags & 0x4000) == 0 && (__int64)first < 0 &&
          (!frontend_data.options.unsigned_char ||
           ((left_flags & 0x2fdf) == 0x41 && (left_flags & 0x8000)))) ||
         ((right_flags & 0x4000) == 0 && (__int64)second < 0 &&
          (!frontend_data.options.unsigned_char ||
           ((right_flags & 0x2fdf) == 0x41 && (right_flags & 0x8000))))) &&
        operation != 49 && operation != 77 && operation != 91)
        report_language_diagnostic(2, 308);
    if (left_width <= compiler_primitive_type_bytes[3]) {
        left->value.words.high = unsigned_values ? 0 : ((int)left->value.words.low < 0 ? ~0U : 0);
        first = ((unsigned __int64)left->value.words.high << 32) | left->value.words.low;
    }
    if (right_width <= compiler_primitive_type_bytes[3]) {
        right->value.words.high = unsigned_values ? 0 : ((int)right->value.words.low < 0 ? ~0U : 0);
        second = ((unsigned __int64)right->value.words.high << 32) | right->value.words.low;
    }
    switch (operation) {
    case 49: value = first & second; break;
    case 50: value = first && second; goto boolean_result;
    case 57:
        if (!second) { right->flags = 0x40; return right; }
        value = unsigned_values ? first / second : (__int64)first / (__int64)second;
        break;
    case 59: value = first == second; goto boolean_result;
    case 62:
        value = unsigned_values ? first > second : (__int64)first > (__int64)second;
        goto boolean_result;
    case 63:
        value = unsigned_values ? first >= second : (__int64)first >= (__int64)second;
        goto boolean_result;
    case 67: {
        unsigned count = (BYTE)right->value.words.low; /* original helper consumes CL */
        value = count >= 64 ? 0 : first << count;
        result_type = node->type;
        break;
    }
    case 68:
        value = unsigned_values ? first < second : (__int64)first < (__int64)second;
        goto boolean_result;
    case 69:
        value = unsigned_values ? first <= second : (__int64)first <= (__int64)second;
        goto boolean_result;
    case 70:
        value = first - second;
        if (integer_difference_out_of_range((__int64)first, second,
            (WORD)unsigned_values, (WORD)width))
            report_language_diagnostic(2, 307, operation);
        break;
    case 72:
        if (!second) { right->flags = 0x40; return right; }
        value = unsigned_values ? first % second : (__int64)first % (__int64)second;
        break;
    case 74:
        value = first * second;
        if (integer_product_out_of_range((__int64)first, second,
            (WORD)unsigned_values, (WORD)width))
            report_language_diagnostic(2, 307, operation);
        break;
    case 76: value = first != second; goto boolean_result;
    case 77: value = first | second; break;
    case 79: value = first || second; goto boolean_result;
    case 81: case 83:
        value = first + second;
        if (validate_member_offset((__int64)first, second,
            (WORD)unsigned_values, (WORD)width))
            report_language_diagnostic(2, 307, operation);
        break;
    case 88: {
        unsigned count = (BYTE)right->value.words.low; /* original helper consumes CL */
        if (unsigned_values) value = count >= 64 ? 0 : first >> count;
        else value = count >= 64 ? ((__int64)first < 0 ? (unsigned __int64)-1 : 0) :
            (__int64)first >> count;
        result_type = node->type;
        break;
    }
    case 91: value = first ^ second; break;
    default: return node;
    }
    goto finish_integer;
boolean_result:
    result_type = frontend_data.builtin_types.type_0063;
    unsigned_values = 0;
finish_integer:
    if ((operation == 81 || operation == 70) && node->type->shape &&
        (node->type->shape->flags & 1)) {
        left->value.words.low = (unsigned)value;
        left->value.words.high = (unsigned)(value >> 32);
        return left;
    }
    if (result_type) {
        WORD flags = result_type->flags;
        width = ((flags & 0x340) || !(flags & 0xfff0)) ?
            compiler_primitive_type_bytes[flags & 15] : flags & 15;
    }
    if (width == 8) {
        frontend_bss.initializer_zero_bytes = value;
        return make_typed_constant(unsigned_values ? frontend_data.builtin_types.type_404c :
            frontend_data.builtin_types.type_004c, &frontend_bss.initializer_zero_bytes,
            frontend_data.constant_arena_index);
    }
    value = unsigned_values ? (unsigned)value : (__int64)(int)value;
    if (result_type)
        wide_result = width == compiler_primitive_type_bytes[4] && (result_type->flags & 0x6f) != 0x63;
    if (compiler_primitive_type_bytes[3] == compiler_primitive_type_bytes[2] && !wide_result)
        value = unsigned_values ? (WORD)value : (__int64)(short)value;
    frontend_bss.initializer_zero_bytes = value;
    CompilerType *type = wide_result ?
        (unsigned_values ? frontend_data.builtin_types.type_4044 : frontend_data.builtin_types.type_0044) :
        (unsigned_values ? frontend_data.builtin_types.type_4063_first : frontend_data.builtin_types.type_0063);
    return make_typed_constant(type, &frontend_bss.initializer_zero_bytes, frontend_data.constant_arena_index);
}
extern "C" int __stdcall parse_extended_constant(const char *, CompilerExtendedConstant *) { __asm { int 3 } return 0; }
extern "C" CompilerExpression *__cdecl fold_extended_binary_expression(CompilerExpression *, const CompilerExtendedConstant *, const CompilerExtendedConstant *) { __asm { int 3 } return NULL; }
extern "C" int __cdecl integer_difference_out_of_range(__int64, unsigned __int64, WORD, WORD) { __asm { int 3 } return 0; }
extern "C" int __cdecl integer_product_out_of_range(__int64, unsigned __int64, WORD, WORD) { __asm { int 3 } return 0; }

extern "C" int __cdecl invalid_pointer_arithmetic(CompilerExpression *expression)
{
    int invalid = 0;
    CompilerTypeShape *shape = expression->value.operands.first->type->shape;
    if (shape && (shape->flags & 0x30)) {
        diagnose_binary_operand(expression->kind, 1);
        invalid = 1;
    }
    shape = expression->value.operands.second->type->shape;
    if (shape && (shape->flags & 0x30)) {
        diagnose_binary_operand(expression->kind, 2);
        invalid = 1;
    }
    return invalid;
}
extern "C" void __cdecl check_pointer_difference(unsigned, CompilerExpression *, CompilerExpression *) { __asm { int 3 } }
extern "C" CompilerExpression *__cdecl resolve_class_pointer_difference(CompilerExpression *, CompilerExpression *, CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl diagnose_binary_operand(unsigned, unsigned) { __asm { int 3 } }
extern "C" CompilerExpression *__cdecl build_member_pointer_operator(CompilerExpression *) { __asm { int 3 } return NULL; }

extern "C" CompilerType *__cdecl adjust_reference_type(CompilerType *, unsigned)
{ __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl promote_scalar_expression(CompilerExpression *)
{ __asm { int 3 } return NULL; }
extern "C" CompilerType *__cdecl replace_function_parameters(CompilerType *, unsigned, WORD, const void *)
{ __asm { int 3 } return NULL; }
extern "C" int __cdecl subtract_extended_constants(const CompilerExtendedConstant *,
    const CompilerExtendedConstant *, CompilerExtendedConstant *)
{ __asm { int 3 } return 0; }
extern "C" void *__cdecl copy_extended_constant(const void *, unsigned)
{ __asm { int 3 } return NULL; }

/* 3252d..3255b: lookup without producing the ordinary missing-name error. */
CompilerScopeMatch *RECON_THISCALL CompilerTranslationUnit::FindHiddenName(CompilerNameEntry *name)
{
    int saved = suppress_lookup_errors;
    suppress_lookup_errors = 1;
    CompilerScopeMatch *result = LookupName(name, 0x132);
    suppress_lookup_errors = saved;
    return result;
}

/* 10e63..10e78. One saved selector, not a nested arena stack. */
extern "C" void __cdecl select_temporary_symbol_arena(unsigned index)
{
    frontend_data.saved_binding_arena_index = frontend_data.binding_arena_index;
    frontend_data.binding_arena_index = index;
}

/* 10e78..10e8d: consume the one saved selector and restore its sentinel. */
extern "C" void __cdecl restore_temporary_symbol_arena(void)
{
    unsigned saved = frontend_data.saved_binding_arena_index;
    frontend_data.saved_binding_arena_index = 0xffffffffUL;
    frontend_data.binding_arena_index = saved;
}

/* Kind13's captured payload is transferred as20 bytes. Only its leading
 * pointer is tested here; the following16 bytes retain their representation. */
struct CompilerStatementWorkInput : CompilerExpressionWork {
    void *payload;
    BYTE retained_tail[16];
};
typedef char CompilerStatementWorkInput_layout[(
    sizeof(CompilerStatementWorkInput) == 36 &&
    offsetof(CompilerStatementWorkInput, payload) == 16) ? 1 : -1];
extern "C" int __cdecl statement_needs_temporary(unsigned);
/* 181d2..183d9, 5de46..5df7a. Resolve statement temporaries and unwind
 * the expression work markers, retaining the final captured cleanup payload. */
extern "C" CompilerExpression *__cdecl check_statement_expression(
    CompilerExpression *expression, unsigned mode)
{
    jmp_buf recovery;
    CompilerExpression *volatile saved_result;
    CompilerExpressionWork *current;
    CompilerExpressionWork temporary_frame;
    CompilerStatementWorkInput captured;
    CompilerExpression *converted, *one, *value;
    CompilerType *type;
    unsigned temporary;
    if (frontend_bss.expression_work_recovery_mode && !frontend_data.expression_work_recovery) {
        if (frontend_data.expression_work_reset_pending) return expression;
        saved_result = expression;
        frontend_data.expression_work_recovery = recovery;
        if (_setjmp3(recovery, 0)) {
            frontend_data.expression_work_recovery = NULL;
            return saved_result;
        }
    }
    if (frontend_data.expression_work_reset_pending || frontend_data.parser_classification_state)
        goto return_expression;
    current = frontend_data.expression_work_nodes;
    if (!current || (current->kind != 13 && current->kind != 62)) goto return_expression;
    memset(&captured, 0, sizeof(captured));
    captured.kind = 13;
    captured.flags = 3;
    captured.payload = NULL;
    if (expression && statement_needs_temporary(mode) && !expression->type) {
        converted = convert_assignment_operand(frontend_data.builtin_types.tagged_0080, expression);
        if (!converted) converted = diagnose_parser_operand(expression->kind, expression);
        expression = converted;
        memset(&temporary_frame, 0, sizeof(temporary_frame));
        temporary_frame.kind = 12;
        push_expression_work(&temporary_frame, 16);
        one = (CompilerExpression *)make_integer_constant(1);
        converted = build_expression_cleanup(expression);
        converted = make_binary_expression(0x37, converted, one);
        if (converted) expression = converted;
    } else if (expression && statement_needs_temporary(mode) &&
        !(expression->type->flags == 0x80 && expression->type->shape)) {
        type = expression->type;
        temporary = allocate_temporary_id();
        value = make_temporary_expression(temporary, type);
        expression = make_ir_binary_node(0x34, value, expression);
        memset(&temporary_frame, 0, sizeof(temporary_frame));
        temporary_frame.kind = 12;
        push_expression_work(&temporary_frame, 16);
        expression = build_expression_cleanup(expression);
        value = make_temporary_expression(temporary, type);
        value = convert_return_temporary(value);
        expression = make_binary_node(0x37, expression, value, type);
    } else {
        memset(&temporary_frame, 0, sizeof(temporary_frame));
        temporary_frame.kind = 12;
        push_expression_work(&temporary_frame, 16);
        expression = build_expression_cleanup(expression);
    }
    pop_expression_work();
    if (++frontend_data.nested_cleanup_depth) {
        do {
            current = frontend_data.expression_work_nodes;
            while (current && current->kind == 13) {
                if (current->flags & 1) {
                    if ((current->flags >> 6) < 2) { __asm { int 3 } }
                    memcpy(&captured.payload, (BYTE *)current + 16, 20);
                }
                pop_expression_work();
                current = frontend_data.expression_work_nodes;
            }
            --frontend_data.nested_cleanup_depth;
            if (!(mode & 2) || !current || current->kind != 62) break;
            pop_expression_work();
        } while (frontend_data.nested_cleanup_depth);
    }
    if (captured.payload) push_expression_work(&captured, 36);
    inspect_expression_cleanup(expression);
return_expression:
    if (frontend_data.expression_work_recovery == recovery)
        frontend_data.expression_work_recovery = NULL;
    return expression;
}
extern "C" int __cdecl statement_needs_temporary(unsigned) { __asm { int 3 } return 0; }

extern "C" CompilerExpression *__cdecl unwrap_statement_operand(CompilerExpression *);
/* 183d9..18419, 5df7a..5e01b. Typed statements emit their context,
 * expression and terminator; unresolved function names use the real resolver. */
extern "C" void __cdecl emit_statement_expression(CompilerExpression *expression)
{
    if (frontend_bss.error_count) return;
    if (!expression || expression->type) {
        emit_context_transition(0x24);
        emit_ir_expression(expression);
        write_ir_record(0x4b, NULL);
        return;
    }
    CompilerExpression *operand = unwrap_statement_operand(expression);
    CompilerSymbol *symbol = NULL;
    if (operand->kind == 0x53) {
        CompilerExpression *name = operand->value.operands.second->value.operands.first;
        CompilerSymbolBinding *binding = (CompilerSymbolBinding *)name->value.pointer;
        symbol = binding->owner->symbol;
    } else if (operand->kind == 6) symbol = operand->value.symbol;
    if (!symbol) return;
    unsigned kind = symbol->kind;
    if (kind == 24) kind = ((CompilerAliasSymbol *)symbol)->target->symbol->kind;
    if (kind != 16 && kind != 7 && kind != 6 && kind != 17 && kind != 8 && kind != 25) return;
    if (!can_resolve_assignment_symbol(symbol, 1))
        compiler_warning(0x238, (unsigned)operand->kind, symbol);
}
extern "C" CompilerExpression *__cdecl unwrap_statement_operand(CompilerExpression *) { __asm { int 3 } return NULL; }

extern "C" {
WORD __cdecl member_object_qualifiers(CompilerType *);
CompilerExpression *__cdecl resolve_member_expression(unsigned, CompilerExpression *, CompilerSymbol *, unsigned, CompilerType *);
CompilerExpression *__cdecl prepare_member_object(CompilerExpression *);
int __cdecl can_discard_member_object(CompilerExpression *);
}
/* 1ad7f..1b040, 5f9f1..5fbda. Resolve the named member against a
 * pointer/object expression, retaining aliases and the original name binding. */
extern "C" CompilerExpression *__cdecl make_member_access_expression(unsigned operation,
    CompilerExpression *object, CompilerExpression *member)
{
    CompilerTranslationUnit *unit = frontend_data.translation_unit;
    CompilerExpression *initial_object = object, *result, *selection;
    CompilerSymbol *reference, *selected, *field;
    CompilerGlobalSymbolView *field_view;
    CompilerType *object_type, *result_type;
    CompilerTypeShape *shape;
    unsigned preserve = 0, reference_mode = 0;
    WORD qualifiers;
    unit->suppress_lookup_errors = 0;
    unit->retired_scopes = NULL;
    unit->lookup_context = 1;
    unit->opaque_lookup_state = 0;
    if (!member || !object) return NULL;
    object_type = object->type;
    if (member->kind == 0x3d) {
        CompilerSymbol *base = member->value.operands.first->value.symbol;
        if (base->kind == 15) {
            int index = (short)object_type->auxiliary;
            CompilerPointerArray *table = frontend_data.frontend_pointer_table;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            compiler_warning(0x27, 0x53, base, table->items[index]);
        }
        return NULL;
    }
    if (member->kind != 6) {
        compiler_warning((BYTE)operation == 0x53 ? 0x111 : 0x112, (unsigned)member->kind);
        return NULL;
    }
    reference = member->value.symbol;
    selected = reference;
    if (selected->kind == 18) selected = ((CompilerBoundSymbol *)selected)->binding->owner->symbol;
    field = selected->kind == 24 ? ((CompilerAliasSymbol *)selected)->target->symbol : selected;
    field_view = (CompilerGlobalSymbolView *)field;
    if (is_class_scope_symbol(field)) {
        check_qualified_declaration(reference);
        result = make_resolved_symbol_expression(resolve_generated_symbol(field), 0);
        if (!can_discard_member_object(object)) result = make_result_binary_node(0x37, object, result);
        return result;
    }
    if (field->kind == 5) {
        check_qualified_declaration(reference);
        return make_enumerator_expression(field);
    }
    shape = object_type->shape;
    if ((BYTE)operation == 0x53) {
        if (!shape) { compiler_warning(0xe8, 0x53, reference, object_type); return NULL; }
        if (shape->base || !(shape->flags & 0x43)) {
            compiler_warning(0xe3, ((CompilerNamedSymbol *)selected)->link.name);
            return NULL;
        }
    } else if (shape) {
        if (shape->flags & 0x40) reference_mode = 1;
        else if ((shape->flags & 3) && !shape->base)
            compiler_warning(0xe7, 0x53, reference, object_type);
        else compiler_warning(0xe4, ((CompilerNamedSymbol *)selected)->link.name);
    } else {
        preserve = object->flags & 0x20;
        object = prepare_member_object(object);
        if (object->kind == 0x34 && object->type && !object->type->shape) preserve = 1;
    }
    if (!(object->flags & 7) && object->kind == 0x98) {
        compiler_warning(0x113, object->value.pointer);
        return NULL;
    }
    if (field->kind == 17 || field->kind == 8) {
        unsigned arena_index = frontend_data.constant_arena_index; /* actual CD88C */
        if (arena_index >= 9) { __asm { int 3 } }
        CompilerArena *arena = &frontend_bss.arenas[arena_index];
        result = (CompilerExpression *)arena->cursor;
        arena->cursor += 24;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        result->flags = 2;
        if ((BYTE)operation == 0x50) {
            operation = 0x53;
            if (!reference_mode) object = prepare_parameter_expression(initial_object, 2);
        }
        result->kind = (BYTE)operation;
        result->value.operands.first = object;
        member->value.symbol = reference;
        result->value.operands.second = member;
        return result;
    }
    qualifiers = member_object_qualifiers(object_type);
    if (field_view->reference_flags & 4) qualifiers &= 0xfeff;
    result = resolve_member_expression(1, object, reference, 1, object->type);
    /* EE retains the carried object type for the following qualifier path. */
    result_type = object_type;
    if (result->kind != 0xee) {
        unsigned inherited = (BYTE)operation == 0x53 ?
            object_type->shape->attributes & 3 : object_type->attributes & 3;
        result_type = make_member_address_type(((CompilerIrSymbol *)field)->type, inherited);
        result->type = result_type;
    }
    if (qualifiers) result->type = result_type->ApplyTypeQualifiers(qualifiers);
    if (field->qualifiers & 4) return result;
    if (field->kind == 2) {
        CompilerExpression *second = (CompilerExpression *)make_integer_constant(field_view->member_selection.second);
        CompilerExpression *first = (CompilerExpression *)make_integer_constant(field_view->member_selection.first);
        selection = make_ir_binary_node(0x36, first, second);
        result = make_ir_binary_node(0x35, result, selection);
        if (qualifiers) {
            CompilerType *scratch = make_parser_type_scratch(result->type);
            scratch->attributes |= qualifiers;
            result->type = intern_type(scratch);
            --frontend_bss.type_scratch_index;
        }
    }
    if (result->kind != 0xee) result = check_symbol_expression(field, result);
    if (preserve) result->flags |= 0x20;
    return result;
}
extern "C" CompilerTypeShape *__cdecl find_function_type_shape(CompilerType *);
/* 1b4d7..1b4ff, 5fe82..5fe8d. A retained base shape supplies the
 * qualifier word; otherwise it comes from the type itself. */
extern "C" WORD __cdecl member_object_qualifiers(CompilerType *type)
{
    CompilerTypeShape *shape = find_function_type_shape(type);
    return (WORD)((shape && shape->base ? shape->base->attributes : type->attributes) & 0x1f00);
}
extern "C" int __cdecl can_assign_bound_member(CompilerSymbol *);
extern "C" CompilerExpression *__cdecl build_bound_member_expression(unsigned, CompilerExpression *, CompilerSymbolBindingOwner *, unsigned, CompilerType *);
/* 1b040..1b0a4, 5fbda..5fc1a. CF298 rows1/100/206 establish the
 * supported operation classes; other rows are not assigned a default. */
extern "C" CompilerExpression *__cdecl resolve_member_expression(unsigned operation,
    CompilerExpression *object, CompilerSymbol *name, unsigned mode, CompilerType *type)
{
    unsigned kind = (BYTE)operation;
    if ((kind != 1 && kind != 100 && kind != 206) || !name || name->kind != 18) { __asm { int 3 } }
    CompilerSymbolBindingOwner *binding = ((CompilerBoundSymbol *)name)->binding->owner;
    if (kind == 1) check_qualified_declaration(name);
    else if (!can_assign_bound_member(name)) {
        if (kind == 206 || !(frontend_data.compiler_mode_flags & 1))
            compiler_warning(0xf3, kind, object->type, type);
        else report_language_diagnostic(3, 0xf3, kind, object->type, type);
    }
    return build_bound_member_expression(operation, object, binding, mode, type);
}
extern "C" void __cdecl reset_access_diagnostic(void);
extern "C" void __cdecl prepare_access_diagnostic(BYTE, CompilerSymbol *, CompilerSymbol *);
extern "C" int __cdecl is_friend_of_class(CompilerSymbol *, CompilerSymbol *);
extern "C" BYTE __cdecl combine_member_access(BYTE, BYTE);
extern "C" CompilerSymbolBindingOwner *__cdecl find_virtual_binding_owner(CompilerSymbolBindingOwner *);
extern "C" BYTE __cdecl virtual_binding_access(CompilerSymbol *, CompilerSymbol *);
extern "C" void __cdecl set_access_diagnostic(CompilerSymbol *, CompilerSymbol *, BYTE, BYTE, CompilerSymbol *);
extern "C" CompilerSymbol *__cdecl current_declarator_context(void);
extern "C" int __cdecl can_assign_bound_member(CompilerSymbol *input)
{
    CompilerTranslationUnit *unit = frontend_data.translation_unit;
    CompilerSymbol *scope_class = NULL, *current_class, *target, *symbol, *owner;
    CompilerSymbol *first_class = NULL, *previous_symbol = NULL;
    CompilerSymbolBindingOwner *binding;
    int depth = 1, first_restricted_depth = 0;
    int unrestricted = 0, friend_path = 0, first_restricted_friend = 0;
    int friend_context = 0, last_virtual_self = 0;
    BYTE access, required, inherited_access, previous_access = 0;
    if (input->scope_flags & 1) return 1;
    if (unit->current->level >= 0xfb) {
        if (unit->current->level < 0xfb)
            report_compiler_diagnostic(1, "symtable.inl", 0xba7);
        scope_class = ((CompilerClassScope *)unit->current)->symbol;
    }
    if (frontend_data.declaration_context_index >= 0)
        current_class = current_declarator_context();
    else if (frontend_bss.current_function && frontend_bss.current_function->kind == 8) {
        int index = (short)frontend_bss.current_function->declaration_qualifiers;
        CompilerPointerArray *table = frontend_data.frontend_pointer_table;
        if (index < 0 || index >= table->count) { __asm { int 3 } }
        current_class = (CompilerSymbol *)table->items[index];
    } else if (unit->current->level == 1) {
        CompilerScopeTable *outer = unit->EnclosingNonParameterScope();
        current_class = outer->level < 0xfb ? NULL :
            ((CompilerClassScope *)unit->EnclosingNonParameterScope())->symbol;
    } else current_class = NULL;

    binding = ((CompilerBoundSymbol *)input)->binding->owner;
    reset_access_diagnostic();
    target = binding->symbol;
    if (scope_class) {
        unsigned index = frontend_bss.parser_context_depth;
        if (index >= 60) { __asm { int 3 } }
        if (input == frontend_bss.parser_qualification_scopes[index] && target == scope_class)
            return 1;
    }
    if (input->type_flags & 0x80) {
        CompilerGlobalSymbolView *value = (CompilerGlobalSymbolView *)target;
        unrestricted = (value->storage_class & 7) == 3 || target->kind == 5 ||
            (target->definition_flags & 0x10);
        if (unrestricted && target->kind == 4 && target == scope_class) return 1;
        required = 1;
    } else required = 0;
    if (!required) required = query_initializer_mode() ? 2 : 3;
    access = target->kind == 15 ? 3 : target->definition_flags & 3;
    binding = binding->inner;
    if (!binding) return 1;

    for (;;) {
        symbol = binding->symbol;
        if (symbol->kind == 10 && ((((CompilerIrSymbol *)symbol)->storage_class & 7) == 7)) {
            CompilerType *type = ((CompilerIrSymbol *)symbol)->type;
            if (!(get_type_classification(type) & 0x80)) {
                compiler_warning(0x28d, 0x53, symbol);
                return 0;
            }
            int index = (short)type->auxiliary;
            CompilerPointerArray *table = frontend_data.frontend_pointer_table;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            symbol = (CompilerSymbol *)table->items[index];
        }
        if (symbol->kind == 22) return 1;
        if (symbol->kind == 4 || symbol->kind == 23) owner = symbol;
        else if (symbol->kind == 15) {
            CompilerClassMemberSymbol *base = (CompilerClassMemberSymbol *)symbol;
            if (base->flags & 2) return 1;
            int index = base->owner_index;
            CompilerPointerArray *table = frontend_data.frontend_pointer_table;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            owner = (CompilerSymbol *)table->items[index];
        } else {
            /* Other record kinds have no recovered class-owner interpretation. */
            __asm { int 3 }
            return 0;
        }
        if (owner->kind == 23) return 1;
        if (!first_class) {
            first_class = owner;
            prepare_access_diagnostic(access, target, owner);
        }
        if (unrestricted && binding == binding->next) {
            if (owner == first_class) return 1;
            return access > 1;
        }
        if (owner == scope_class) required = 1;
        if (((CompilerGlobalSymbolView *)owner)->class_detail->context_flags & 0x40) {
            friend_context = 0;
            if (scope_class) friend_context = is_friend_of_class(scope_class, owner);
            if (!friend_context && current_class)
                friend_context = is_friend_of_class(current_class, owner);
            if (!friend_context && frontend_bss.current_function)
                friend_context = is_friend_of_class(frontend_bss.current_function, owner);
            if (friend_context) {
                if (owner == first_class) friend_path = 1;
                required = 1;
            }
        }
        if (symbol == current_class) required = 1;
        if (!binding->inner) {
            if (friend_path && first_restricted_depth &&
                first_restricted_depth >= (int)(depth - 1) && first_restricted_friend == 1)
                return 1;
            if (last_virtual_self) return previous_access >= 2;
            return access >= required;
        }
        if (symbol->kind == 15) {
            CompilerClassMemberSymbol *base = (CompilerClassMemberSymbol *)symbol;
            last_virtual_self = owner == scope_class;
            if (base->flags & 1) {
                binding = find_virtual_binding_owner(binding);
                int index = base->owner_index;
                CompilerPointerArray *table = frontend_data.frontend_pointer_table;
                if (index < 0 || index >= table->count) { __asm { int 3 } }
                inherited_access = virtual_binding_access((CompilerSymbol *)table->items[index],
                    binding->inner->symbol);
                frontend_bss.access_diagnostic_ambiguous = 1;
            } else inherited_access = symbol->definition_flags & 3;
        } else {
            last_virtual_self = 0;
            inherited_access = symbol->definition_flags & 3;
            if (!inherited_access) inherited_access = 3;
        }
        if ((inherited_access == 1 || inherited_access == 2) && !first_restricted_depth) {
            first_restricted_depth = depth;
            first_restricted_friend = friend_context;
        }
        previous_access = access;
        access = combine_member_access(access, inherited_access);
        if (previous_access > access && access < required) {
            BYTE previous_code = previous_symbol ? previous_symbol->definition_flags & 3 : 0;
            set_access_diagnostic(target, symbol, previous_code, inherited_access, previous_symbol);
        }
        previous_symbol = symbol;
        binding = binding->inner;
        ++depth;
        if (!binding) return 1;
    }
}
extern "C" void __cdecl reset_access_diagnostic(void)
{
    frontend_bss.access_diagnostic_owner = NULL;
    frontend_bss.access_diagnostic_ambiguous = 0;
}
extern "C" void __cdecl prepare_access_diagnostic(BYTE access, CompilerSymbol *symbol, CompilerSymbol *owner)
{
    frontend_bss.access_diagnostic_owner = owner;
    frontend_bss.access_diagnostic_symbol = symbol;
    frontend_bss.access_diagnostic_code = access;
}
extern "C" int __cdecl is_friend_of_class(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } return 0; }
extern "C" BYTE __cdecl combine_member_access(BYTE, BYTE) { __asm { int 3 } return 0; }
extern "C" CompilerSymbolBindingOwner *__cdecl find_virtual_binding_owner(CompilerSymbolBindingOwner *) { __asm { int 3 } return NULL; }
extern "C" BYTE __cdecl virtual_binding_access(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } return 0; }
extern "C" void __cdecl set_access_diagnostic(CompilerSymbol *, CompilerSymbol *, BYTE, BYTE, CompilerSymbol *) { __asm { int 3 } }
CompilerScopeTable *RECON_THISCALL CompilerTranslationUnit::EnclosingNonParameterScope(void) { __asm { int 3 } return NULL; }

extern "C" CompilerSymbol *__cdecl prepare_qualified_symbol(CompilerSymbol *, unsigned);
extern "C" CompilerSymbolBindingOwner *__cdecl join_member_bindings(CompilerSymbolBindingOwner *, CompilerSymbolBindingOwner *);
extern "C" CompilerExpression *__cdecl offset_member_expression(CompilerExpression *, int, CompilerType *);
extern "C" CompilerExpression *__cdecl offset_member_pointer(CompilerExpression *, int, CompilerType *);
extern "C" CompilerExpression *__cdecl build_virtual_member_expression(unsigned, CompilerExpression *, CompilerSymbolBindingOwner *, CompilerType *, int);
extern "C" CompilerExpression *__cdecl convert_member_symbol(unsigned, CompilerSymbol *);
extern "C" CompilerExpression *__cdecl build_bound_member_expression(unsigned operation,
    CompilerExpression *object, CompilerSymbolBindingOwner *binding, unsigned mode, CompilerType *type)
{
    CompilerSymbol *symbol;
    int offset = 0;
    if (binding->symbol->kind == 24) {
        CompilerSymbol *qualified = ((CompilerAliasSymbol *)binding->symbol)->target->qualified_symbol;
        CompilerSymbolBindingOwner *alias = ((CompilerBoundSymbol *)qualified)->binding->owner;
        CompilerSymbolBindingOwner *alias_last = alias, *last = binding;
        CompilerSymbolBindingOwner *inner = binding->inner;
        while (alias_last->inner) alias_last = alias_last->inner;
        while (last->inner) last = last->inner;
        if (alias_last->symbol == last->symbol) binding = alias;
        else {
            CompilerSymbol *prepared = prepare_qualified_symbol(qualified, 5);
            binding = join_member_bindings(((CompilerBoundSymbol *)prepared)->binding->owner, inner);
        }
    }
    symbol = binding->symbol;
    if (symbol->kind == 3 && (((CompilerIrSymbol *)symbol)->reference_flags & 0x80))
        return make_ir_binary_node(0xee, object, convert_member_symbol(6, symbol));
    if (symbol->kind == 3 || symbol->kind == 2) {
        offset = ((CompilerGlobalSymbolView *)symbol)->member_offset;
        binding = binding->inner;
    } else if (symbol->kind == 17 || symbol->kind == 8 || symbol->kind == 4 || symbol->kind == 7)
        binding = binding->inner;
    while (binding && binding->symbol->kind == 15) {
        CompilerClassMemberSymbol *base = (CompilerClassMemberSymbol *)binding->symbol;
        if (base->flags & 1)
            return build_virtual_member_expression(operation, object, binding, type, offset);
        offset += base->base_offset;
        binding = binding->inner;
    }
    /* CF298's encountered operation classes:1 is ordinary object access;
     *100 and206 share the member-pointer class100. Other rows stay unknown. */
    unsigned code = (BYTE)operation;
    if (code == 100 || code == 206) {
        if (!mode) offset = -offset;
        return offset_member_pointer(object, offset, type);
    }
    if (code != 1) { __asm { int 3 } }
    return offset_member_expression(object, offset, type);
}
extern "C" CompilerSymbolBindingOwner *__cdecl join_member_bindings(CompilerSymbolBindingOwner *, CompilerSymbolBindingOwner *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl offset_member_expression(CompilerExpression *object,
    int offset, CompilerType *type)
{
    CompilerExpression *value = (CompilerExpression *)make_integer_constant(offset);
    return make_binary_node(0x53, object, value, type);
}
extern "C" CompilerExpression *__cdecl offset_member_pointer(CompilerExpression *, int, CompilerType *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl build_virtual_member_expression(unsigned, CompilerExpression *, CompilerSymbolBindingOwner *, CompilerType *, int) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl convert_member_symbol(unsigned, CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl prepare_member_object(CompilerExpression *expression)
{
    while (expression->kind == 0x64 || expression->kind == 0x76)
        expression = expression->value.operands.first;
    if (expression->kind == 0x37) {
        expression->value.operands.second = prepare_member_object(expression->value.operands.second);
        expression->type = expression->value.operands.second->type;
        return expression;
    }
    return prepare_parameter_expression(expression, 2);
}
extern "C" int __cdecl can_discard_member_object(CompilerExpression *) { __asm { int 3 } return 0; }

extern "C" int __cdecl has_class_assignment(CompilerType *);
extern "C" CompilerExpression *__cdecl make_binary_expression(unsigned operation,
    CompilerExpression *left, CompilerExpression *right)
{
    if (left && left->kind == 0xee) {
        switch ((BYTE)operation) {
        case 0x33: case 0x34: case 0x38: case 0x3a:
        case 0x40: case 0x41: case 0x42: case 0x47:
        case 0x49: case 0x4b: case 0x4e: case 0x52:
        case 0x54: case 0x55: case 0x57: case 0x5c:
            return expand_initializer_expression(operation, left, right);
        default:
            left = expand_initializer_expression(operation, left, NULL);
            if (!left) return NULL;
        }
    }
    if (right && right->kind == 0xee) {
        right = expand_initializer_expression(operation, right, NULL);
        if (!right) return NULL;
    }
    if (!left || !right) {
        if ((BYTE)operation == 0x32 || (BYTE)operation == 0x4f)
            finish_deferred_declaration();
        return NULL;
    }
    if (left->kind == 0x98) {
        compiler_warning(0x113, left->value.symbol);
        return NULL;
    }
    if (right->kind == 0x98) {
        compiler_warning(0x113, right->value.symbol);
        return NULL;
    }
    int left_class = 0, right_class = 0;
    CompilerType *type = left->type;
    if (type && ((type->flags & 0x600) || (type->flags & 0x2fdf) == 0x12 ||
        (type->flags & 0x2fdf) == 0x11))
        left_class = !type->shape || ((type->shape->flags & 0x40) && !type->shape->base);
    right = resolve_constant_reference(right);
    if (!left_class && right && right->type) {
        type = right->type;
        if ((type->flags & 0x600) || (type->flags & 0x2fdf) == 0x12 ||
            (type->flags & 0x2fdf) == 0x11)
            right_class = !type->shape || ((type->shape->flags & 0x40) && !type->shape->base);
    }
    if (left_class || right_class) {
        unsigned failed = 0;
        CompilerExpression *result = resolve_overloaded_operator(operation, left, right,
            left_class, 1, &failed);
        if (result) {
            if (((BYTE)operation == 0x32 || (BYTE)operation == 0x4f) &&
                result->kind != 0x32 && result->kind != 0x4f)
                finish_deferred_declaration();
            /* CF390's complete boolean membership for classes27/28 is
             * precisely expression kinds32/4f; no other column is needed. */
            return result;
        }
        if ((BYTE)operation == 0x34 && left_class && !(left->type->flags & 0x200)) {
            int index = (short)left->type->auxiliary;
            CompilerPointerArray *table = frontend_data.frontend_pointer_table;
            if (index < 0 || index >= table->count) { __asm { int 3 } }
            CompilerSymbol *owner = (CompilerSymbol *)table->items[index];
            if (!has_class_assignment(left->type)) compiler_warning(0x246, owner);
            return NULL;
        }
        if (failed) return NULL;
    }
    if ((BYTE)operation == 0x32 || (BYTE)operation == 0x4f)
        right = check_statement_expression(right, 3);
    return make_scalar_comparison(operation, left, right);
}
extern "C" int __cdecl has_class_assignment(CompilerType *) { __asm { int 3 } return 0; }

/* 1b0cf..1b18e, 5fc1a..5fdb9: prepare the qualified member-lookup
 * context, retaining the original expression when no overload replaces it. */
CompilerExpression *RECON_THISCALL CompilerTranslationUnit::PrepareAllocation(
    unsigned operation, CompilerExpression *expression, unsigned mode)
{
    CompilerType *type;
    CompilerSymbol *owner, *instantiated;
    CompilerGlobalClassDetail *detail;
    CompilerExpression *replacement;
    ParserTokenView *first, *second, *third, *fourth;
    ParserFrontendContextView *context =
        (ParserFrontendContextView *)&frontend_bss.saved_context_stack;
    unsigned rewritten;
    int index;
    if (expression && expression->kind == 0xee)
        expression = expand_initializer_expression(1, expression, NULL);
    opaque_lookup_state = 1;
    lookup_context = (BYTE)operation;
    suppress_lookup_errors = 0;
    if (!expression) return NULL;
    type = expression->type;
    if (!type) return NULL;
    if (!(type->flags & 0x400)) {
        first = context->ReadToken();
        if (!first) return NULL;
        if (first->kind == 6) {
            second = context->ReadToken();
            if (second) {
                if (second->kind == 0x95) {
                    third = context->ReadToken();
                    if (third) {
                        if (third->kind == 0x59) {
                            fourth = context->ReadToken();
                            if (fourth) {
                                if (fourth->kind == 6) {
                                    opaque_lookup_state = 0;
                                    lookup_context = 1;
                                }
                                ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)fourth);
                            }
                        }
                        ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)third);
                    }
                }
                ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)second);
            }
        }
        ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)first);
        return NULL;
    }
    index = (short)type->auxiliary;
    if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
    owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
    detail = ((CompilerGlobalSymbolView *)owner)->class_detail;
    if ((detail->completion_flags & 1) && !(detail->template_flags & 1) &&
        !(owner->definition_flags & 4) && !(detail->completion_flags & 4)) {
        index = frontend_data.declaration_context_index;
        if (index >= 16) { __asm { int 3 } }
        if ((index < 0 || frontend_bss.class_scope_rows[index].symbol->name !=
                ((CompilerNamedSymbol *)owner)->link.name) &&
            !(owner->scope_flags & 4) && !(frontend_data.source_mode_flags & 0x200020)) {
            instantiated = instantiate_declaration_class(owner);
            if (((CompilerGlobalSymbolView *)instantiated)->class_detail->completion_flags & 8)
                merge_declaration_classes(instantiated, owner);
        }
    }
    if (!(owner->definition_flags & 4)) {
        if (!frontend_bss.current_parser_expression)
            compiler_warning(0x1b, owner);
        return NULL;
    }
    if (!mode && operation == 0x53 && (get_type_classification(expression->type) & 0x80)) {
        rewritten = 0;
        replacement = resolve_overloaded_operator(0x53, expression, NULL, 1, 1, &rewritten);
        if (replacement) {
            if (rewritten) return replacement;
            replacement = PrepareAllocation(0x53, replacement, 0);
            if (replacement) return replacement;
        }
    }
    retired_scopes = owner;
    return expression;
}

extern "C" int __cdecl requires_member_address(CompilerExpression *);
extern "C" CompilerExpression *__cdecl make_member_pointer_address(CompilerExpression *);
extern "C" CompilerSymbol *__cdecl find_address_symbol(CompilerExpression *);
extern "C" CompilerExpression *__cdecl find_address_reference(CompilerExpression *);
/* The C1 convert_unary_operand body is a readability seed. This C++ owner
 * retains its extra member-address helpers, function modifier and flags. */
extern "C" CompilerExpression *__cdecl prepare_parameter_expression(
    CompilerExpression *expression, unsigned mode)
{
    CompilerType *type;
    CompilerTypeShape *shape;
    CompilerExpression *result;
    CompilerSymbol *symbol;
    WORD shape_flags;
    if ((expression->flags & 7) == 3) {
        compiler_warning(0x65, 0x31);
        return expression;
    }
    type = expression->type;
    if (!type) return make_cast_expression(0x31, expression);
    shape = type->shape;
    if (shape && (shape->flags & 0x77) == 4) {
        if (mode & 0x10) type = type->RemoveFunctionQualifier();
        type = type->RemoveFunction();
    }
    shape = type->shape;
    if (shape && (shape->flags & 0x77) == 3 && (shape->flags & 8)) {
        expression->type = adjust_reference_type(type, 2);
        return expression;
    }
    if (requires_member_address(expression))
        return make_member_pointer_address(expression);
    shape = expression->type->shape;
    shape_flags = shape ? shape->flags : 0;
    result = prepare_assignment_target(expression, mode);
    if (!result) {
        unsigned kind = shape_flags & 0x77;
        if (kind == 2) {
            if ((expression->flags & 7) == 5) return expression;
            result = expression;
        } else if (kind == 3 || kind == 0x44) {
            if (shape_flags & 0x400) compiler_warning(0x66, 0x31);
            return expression;
        } else {
            if (mode & 2) return expression;
            compiler_warning(0x66, 0x31);
            return NULL;
        }
    }
    if ((result->flags & 7) == 4 && result->kind != 0x9c) {
        symbol = result->value.symbol;
        if (mode & 4) symbol->qualifiers |= 8;
        unsigned storage = ((CompilerIrSymbol *)symbol)->storage_class & 7;
        if (storage >= 5 && storage <= 6)
            ((CompilerIrSymbol *)symbol)->storage_class =
                (((CompilerIrSymbol *)symbol)->storage_class & ~7U) | 4;
    } else {
        if (result->kind == 0x35 && (mode & 4)) compiler_warning(0x68, 0x31);
        if (result->kind == 0x53 && (result->value.operands.first->flags & 7) == 3) {
            CompilerExpression *constant = result->value.operands.first;
            result = result->value.operands.second;
            result->value.integer = (unsigned __int64)result->value.integer +
                (unsigned __int64)constant->value.integer;
        } else {
            symbol = find_address_symbol(result);
            if (symbol) symbol->qualifiers |= 8;
            else if (!find_address_reference(result) && (frontend_data.compiler_flags & 4)) {
                shape = result->type ? result->type->shape : NULL;
                if (result->kind != 0x70 || !shape || !(shape->flags & 0x41)) {
                    compiler_warning(0x66, 0x31);
                    return NULL;
                }
            }
        }
    }
    result->type = type->AddressType(0);
    if (frontend_data.compiler_flags & 4) result->flags |= 0x20;
    return result;
}

extern "C" int __cdecl requires_member_address(CompilerExpression *expression)
{
    if (expression->kind != 0x46) return 0;
    expression = expression->value.operands.first;
    if (expression->kind != 0x3d) return 0;
    expression = expression->value.operands.first;
    if (expression->kind != 6) return 0;
    CompilerSymbol *symbol = expression->value.symbol;
    if (((CompilerNamedSymbol *)symbol)->link.name != frontend_data.builtin_names.this_parameter)
        return 0;
    CompilerGlobalSymbolView *function = (CompilerGlobalSymbolView *)frontend_bss.current_function;
    return function && (function->kind == 7 || function->kind == 8) &&
        (function->builtin_flags & 1) && (function->queue_flags & 0x10) ? 1 : 0;
}
extern "C" CompilerExpression *__cdecl make_member_pointer_address(CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl find_address_symbol(CompilerExpression *expression)
{
    for (;;) {
        switch (expression->kind) {
        case 6: return expression->value.symbol;
        case 54:
            expression = expression->type == expression->value.operands.second->type ?
                expression->value.operands.second : expression->value.operands.first;
            break;
        case 65: case 80: case 93:
            expression = expression->value.operands.first;
            break;
        default: return NULL;
        }
    }
}
extern "C" CompilerExpression *__cdecl find_address_reference(CompilerExpression *expression)
{
    for (;;) {
        switch (expression->kind) {
        case 6: case 100: return expression;
        case 54:
            expression = expression->type == expression->value.operands.second->type ?
                expression->value.operands.second : expression->value.operands.first;
            break;
        case 65: case 80: case 118:
            expression = expression->value.operands.first;
            break;
        case 83: return expression->value.operands.first;
        case 93:
            expression = expression->value.operands.first;
            if (expression->kind == 61) expression = expression->value.operands.first;
            break;
        default: return NULL;
        }
    }
}
CompilerType *RECON_THISCALL CompilerType::RemoveFunctionQualifier(void) { __asm { int 3 } return NULL; }
CompilerType *RECON_THISCALL CompilerType::AddressType(unsigned attributes)
{
    CompilerTypeShape address;
    CompilerType description;
    memset(&address, 0, sizeof(address));
    if (shape && (shape->flags & 0x77) == 4 && !(shape->flags & 0x2000)) {
        address.flags = 3;
        address.attributes = (WORD)((shape->attributes & 0x70) | attributes);
    } else {
        address.flags = 1;
        address.attributes = (WORD)attributes;
    }
    address.base = shape;
    /* Preserve the complete16-byte source type, including its opaque cache. */
    memcpy(&description, this, 16);
    description.shape = intern_type_shape(&address);
    description.attributes &= ~0x70U;
    return intern_type(&description);
}

extern "C" CompilerExpression *__cdecl prepare_assignment_value(CompilerExpression *);
extern "C" CompilerExpression *__cdecl prepare_conditional_lvalue(CompilerExpression *, unsigned);
extern "C" CompilerExpression *__cdecl prepare_control_lvalue(CompilerExpression *, unsigned);
extern "C" CompilerExpression *__cdecl prepare_comma_lvalue(CompilerExpression *, unsigned);
extern "C" CompilerExpression *__cdecl find_address_reference(CompilerExpression *);
/* The encountered classification61 column contains precisely the two
 * indirection kinds. This is its semantic predicate, not an image table. */
#define INDIRECT_LVALUE(e) ((e)->kind == 61 || (e)->kind == 166)
extern "C" CompilerExpression *__cdecl prepare_assignment_target(
    CompilerExpression *expression, unsigned mode)
{
    CompilerExpression *reference, *inner;
    CompilerType *type;
    CompilerSymbol *owner;
    int preserve = 0;
    unsigned constant;
    if (!expression) return NULL;
    if (expression->flags & 0x20) {
        switch (mode & 10) {
        case 0: return NULL;
        case 2: break;
        case 8:
            type = expression->type;
            if (type->shape || !(type->flags & 0x400) ||
                (short)type->auxiliary < 0 || !frontend_data.options.language_extensions)
                return NULL;
            report_language_diagnostic(4, 0xee);
            break;
        default:
            report_compiler_diagnostic(1, ".\\trees.c", 0x18e1);
            break;
        }
    }
    while (expression->kind == 0x76) expression = expression->value.operands.first;
    if (expression->kind == 0x64) {
        if (!frontend_data.options.language_extensions) return NULL;
        for (;;) {
            inner = expression->value.operands.first;
            if (expression->type->StripQualifiers() != inner->type->StripQualifiers())
                return NULL;
            inner->type = expression->type;
            expression = inner;
            if (expression->kind != 0x64) break;
        }
        if (mode & 1) report_language_diagnostic(4, 0xd5);
        if ((expression->flags & 7) == 4)
            expression->value.symbol->qualifiers |= 8;
        else {
            reference = find_address_reference(INDIRECT_LVALUE(expression) ?
                expression->value.operands.first : expression);
            if (reference && (reference->flags & 7) == 4)
                reference->value.symbol->qualifiers |= 8;
        }
    }
    if (expression->kind == 0x5d) expression = expression->value.operands.first;
    if (expression->kind == 0x5e) return NULL;
    if (expression->kind >= PARSER_TOKEN_KIND_COUNT) { __asm { int 3 } }
    if (INDIRECT_LVALUE(expression)) goto type_properties;
    type = expression->type;
    if (!type) return NULL;
    switch (expression->kind) {
    case 51: case 52: case 58: case 66: case 71: case 73: case 75:
    case 78: case 82: case 84: case 85: case 87: case 92: case 116:
        expression = prepare_assignment_value(expression);
        break;
    case 55:
        expression = prepare_comma_lvalue(expression, mode & 2);
        break;
    case 86:
        expression = prepare_conditional_lvalue(expression, mode & 2);
        break;
    case 199:
        expression = prepare_control_lvalue(expression, mode & 2);
        break;
    default:
        if (!type->shape || !(type->shape->flags & 0x40)) return NULL;
        break;
    }
    if (!expression) return NULL;
    preserve = 1;
type_properties:
    type = expression->type;
    constant = (get_function_calling_convention(type) >> 8) & 1;
    if ((type->flags & 0x400) && !type->shape) {
        int index = (short)type->auxiliary;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
        constant |= (((CompilerGlobalSymbolView *)owner)->class_detail->declaration_flags >> 4) & 1;
    }
    if (!preserve) expression = expression->value.operands.first;
    if (expression->kind == 0x50) {
        inner = expression->value.operands.first;
        if (inner->kind == 0x70 && !inner->type->shape) return NULL;
    }
    if ((mode & 1) && constant) compiler_warning(0xa6);
    return expression;
}
#undef INDIRECT_LVALUE

extern "C" CompilerExpression *__cdecl prepare_assignment_value(CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl prepare_conditional_lvalue(CompilerExpression *, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl prepare_control_lvalue(CompilerExpression *, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl prepare_comma_lvalue(CompilerExpression *, unsigned) { __asm { int 3 } return NULL; }

/* The resolver's returned view exposes the consumed symbol alternatives,
 * candidate link and forwarded context. Allocation and gaps remain unmodeled. */
struct CompilerCallSelection {
    CompilerSymbol *ordinary, *selected;
    unsigned mode; /* +8 */
    CompilerType **parameter_start, **parameter_cursor; /* +c/+10 */
    BYTE unmodeled_14_17[4];
    CompilerType *member_type; /* +18, conditional writer only */
    CompilerCallSelection *next, *previous; /* +1c/+20 */
    CompilerTypeConversion *conversions; /* +24 */
    BYTE unmodeled_28_37[16];
    unsigned completion_context; /* +38, forwarded without interpretation */
    BYTE flags; /* +3c, constructor preserves bits and sets bit1 */
    BYTE unmodeled_3d_3f[3];
    RECON_THISCALL CompilerCallSelection(CompilerSymbol *, unsigned, CompilerType *,
        CompilerTypeConversion *, CompilerCallSelection *);
};
typedef char CompilerCallSelection_layout[(sizeof(CompilerCallSelection) == 64 &&
    offsetof(CompilerCallSelection, mode) == 8 &&
    offsetof(CompilerCallSelection, parameter_start) == 12 &&
    offsetof(CompilerCallSelection, parameter_cursor) == 16 &&
    offsetof(CompilerCallSelection, member_type) == 24 &&
    offsetof(CompilerCallSelection, next) == 28 &&
    offsetof(CompilerCallSelection, previous) == 32 &&
    offsetof(CompilerCallSelection, conversions) == 36 &&
    offsetof(CompilerCallSelection, flags) == 60 &&
    offsetof(CompilerCallSelection, completion_context) == 56) ? 1 : -1];
extern "C" {
int __cdecl count_call_arguments(CompilerExpression *);
int __cdecl member_call_qualification(CompilerExpression *);
CompilerCallSelection *__cdecl select_call_overload(CompilerSymbol *, CompilerExpression *,
    CompilerExpression *, int, unsigned, unsigned);
void __cdecl bind_selected_call(CompilerSymbol *, CompilerSymbol *);
CompilerExpression *__cdecl make_selected_call(CompilerSymbol *, CompilerExpression *,
    CompilerExpression *, int, CompilerSymbol *);
int __cdecl is_class_call_candidate(CompilerExpression *);
CompilerExpression *__cdecl prepare_call_prefix(CompilerExpression *, CompilerExpression **);
unsigned __cdecl check_call_signature(CompilerType *, CompilerTypeShape *, CompilerExpression *, int);
CompilerExpression *__cdecl convert_call_arguments(unsigned, const void *, CompilerExpression *, int);
CompilerExpression *__cdecl finish_call_expression(CompilerExpression *, unsigned, unsigned);
CompilerExpression *__cdecl make_call_result_temporary(CompilerType *, CompilerExpression *);
void __cdecl finish_call_result_temporary(CompilerExpression *, CompilerType *);
}
/* 16611..169e5, 5d415..5d6e1. Resolve named/member calls, overloaded
 * operator(), constructor conversions and calls through function pointers. */
extern "C" CompilerExpression *__cdecl resolve_function_call(
    CompilerExpression *expression, CompilerExpression *arguments)
{
    CompilerExpression *object = NULL, *prefix = NULL, *result, *single = NULL;
    CompilerSymbol *bound = NULL, *function, *initial_function, *callee;
    CompilerType *type, *original_type, *return_type;
    CompilerTypeShape *shape;
    CompilerCallSelection *selection;
    unsigned qualification = 0;
    int count, preserve_binding;
    WORD shape_flags;
    if (!expression) return NULL;
    if (expression->kind == 0x98) {
        type = expression->type;
        if ((type->flags & 0x2fdf) == 0x12) return NULL;
        if (!arguments) {
            if (type->shape || !(type->flags & 0x400) || (short)type->auxiliary < 0)
                single = (CompilerExpression *)make_integer_constant(0);
        } else switch (arguments->flags & 7) {
        case 0: single = arguments; break;
        case 1: single = arguments->value.operands.first; break;
        case 2:
            if (!arguments->value.operands.second) single = arguments->value.operands.first;
            break;
        }
        if (!(get_type_classification(type) & 0x80)) {
            compiler_warning(0x234, type);
            return NULL;
        }
        if (single) {
            frontend_data.compiler_flags |= 1;
            single->list_next = NULL;
            CompilerTypeConversion conversion(5, 0x64, single, type);
            result = NULL;
            if (conversion.IsValid()) result = conversion.Build(result);
            else conversion.Report(1);
            frontend_data.compiler_flags &= ~1U;
            return result;
        }
        if (!is_class_call_candidate(expression)) return NULL;
    } else if (get_type_classification(expression->type) & 0x80) {
        int index = (short)expression->type->auxiliary;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        CompilerSymbol *owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
        bound = frontend_data.translation_unit->LookupScopeName(owner, frontend_data.call_identifier, 2, 0);
        if (bound) {
            count = count_call_arguments(arguments);
            if (count < 0) return NULL;
            object = prepare_parameter_expression(expression, 6);
            callee = ((CompilerBoundSymbol *)bound)->binding->owner->symbol;
            selection = select_call_overload(callee, object, arguments, count, 0, 1);
            if (selection) {
                function = selection->selected ? selection->selected : selection->ordinary;
                bind_selected_call(bound, function);
                return make_selected_call(function, object, arguments, count, NULL);
            }
        }
    }
count_arguments:
    count = count_call_arguments(arguments);
    if (count < 0) return NULL;
classify_designator:
    if (expression->kind == 6) {
        qualification = (unsigned)expression->value.operands.second;
        initial_function = expression->value.symbol;
        function = initial_function;
        if (function->kind == 18) {
            bound = function;
            function = ((CompilerBoundSymbol *)function)->binding->owner->symbol;
        }
        switch (function->kind) {
        case 6: case 7: case 8: case 16: case 17: case 25:
            break;
        case 24: {
            CompilerSymbol *head = ((CompilerAliasSymbol *)function)->target->symbol;
            if (head->kind != 6 && head->kind != 7 && head->kind != 8 &&
                head->kind != 16 && head->kind != 17 && head->kind != 25) {
                /* The original repeats this same alias-head predicate. */
                compiler_warning(0x40);
                return NULL;
            }
            break;
        }
        default:
            compiler_warning(0x40);
            return NULL;
        }
        if (((CompilerNamedSymbol *)function)->link.name == frontend_data.builtin_names.setjmp_name ||
            ((CompilerNamedSymbol *)function)->link.name == frontend_data.builtin_names.catch_name) {
            if (((CompilerGlobalSymbolView *)frontend_bss.current_function)->member_flags & 1)
                compiler_warning(0x1ea, 0x53, function);
            else report_language_diagnostic(4, 0x263, function);
            frontend_bss.current_function->qualifiers |= 2;
        }
    } else if (expression->kind == 0x53) {
        qualification = member_call_qualification(expression);
        object = expression->value.operands.first;
        expression = expression->value.operands.second;
        if (expression->kind != 6) {
            compiler_warning(0x40);
            return NULL;
        }
        initial_function = expression->value.symbol;
        bound = initial_function;
        function = ((CompilerBoundSymbol *)initial_function)->binding->owner->symbol;
    } else {
        prefix = NULL;
        if (expression->kind == 0x93) expression = prepare_call_prefix(expression, &prefix);
        type = original_type = expression->type;
        if (!type) {
            expression = expression->value.operands.first;
            count = count_call_arguments(arguments);
            if (count < 0) return NULL;
            goto classify_designator;
        }
        shape = type->shape;
        shape_flags = shape ? shape->flags : 0;
        if (shape && ((shape_flags & 0x77) == 3 || (shape_flags & 0x44) == 0x44 ||
            ((shape_flags & 0x20) && (expression->kind == 0x99 || expression->kind == 0x9a)))) {
            type = get_function_return_type(type);
            shape = type->shape;
            shape_flags = shape->flags;
        }
        if ((shape_flags & 0x77) != 4) {
            compiler_warning(0x40);
            return NULL;
        }
        if (check_call_signature(original_type, shape, arguments, count) & 0x2000)
            return NULL;
        return_type = get_function_return_type(type);
        result = convert_call_arguments(0, shape->payload.function, arguments, count);
        result = make_binary_node(0x70, expression, result, return_type);
        if (prefix) result = make_result_binary_node(0x37, prefix, result);
        return finish_call_expression(result, 0, shape_flags);
    }
    preserve_binding = (initial_function->type_flags & 8) != 0;
    initial_function->type_flags &= ~8U;
    selection = select_call_overload(function, object, arguments, count, qualification, 1);
    if (!selection) return NULL;
    function = selection->selected ? selection->selected : selection->ordinary;
    if (bound && function) bind_selected_call(bound, function);
    if ((function->type_flags & 4) && !object && (function->type_flags & 0x10)) {
        CompilerType *function_result = ((CompilerIrSymbol *)function)->type->shape->payload.function->type;
        return_type = get_function_return_type(function_result);
        result = make_temporary_expression(allocate_temporary_id(), return_type);
        result = make_call_result_temporary(function_result, result);
        CompilerExpression *call = make_return_object_call((unsigned)function, NULL,
            result, arguments, 0, 1, 1);
        finish_call_result_temporary(result, return_type);
        call->flags |= 0x20;
        return call;
    }
    return make_selected_call(function, object, arguments, count,
        preserve_binding ? initial_function : NULL);
}

extern "C" int __cdecl count_call_arguments(CompilerExpression *arguments)
{
    unsigned count = 0;
    while (arguments) {
        ++count;
        if (arguments->flags & 8) return -1;
        if ((arguments->flags & 7) != 2) break;
        arguments = arguments->value.operands.second;
    }
    return (int)count;
}
extern "C" int __cdecl member_call_qualification(CompilerExpression *) { __asm { int 3 } return 0; }

extern "C" void __cdecl bind_selected_call(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" CompilerExpression *__cdecl make_selected_call(CompilerSymbol *, CompilerExpression *, CompilerExpression *, int, CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl find_class_constructor(CompilerSymbol *);
extern "C" CompilerSymbol *__cdecl make_class_constructor_reference(CompilerSymbol *, CompilerSymbol *);
extern "C" int __cdecl is_class_call_candidate(CompilerExpression *expression)
{
    CompilerSymbol *owner = expression->value.symbol, *instantiated, *constructor;
    CompilerBoundSymbol *bound = NULL;
    if (owner->kind == 18) {
        bound = (CompilerBoundSymbol *)owner;
        owner = bound->binding->owner->symbol;
    }
    if ((((CompilerIrSymbol *)owner)->storage_class & 7) == 7 &&
        (get_type_classification(((CompilerIrSymbol *)owner)->type) & 0x80)) {
        int index = (short)((CompilerIrSymbol *)owner)->type->auxiliary;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
    }
    CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)owner)->class_detail;
    if ((detail->completion_flags & 1) && !(detail->template_flags & 1) &&
        !(owner->definition_flags & 4) && !(detail->completion_flags & 4)) {
        int index = frontend_data.declaration_context_index;
        if (index >= 16) { __asm { int 3 } }
        if ((index < 0 || frontend_bss.class_scope_rows[index].symbol->name !=
                ((CompilerNamedSymbol *)owner)->link.name) &&
            !(owner->scope_flags & 4) && !(frontend_data.source_mode_flags & 0x200020)) {
            instantiated = instantiate_declaration_class(owner);
            if (((CompilerGlobalSymbolView *)instantiated)->class_detail->completion_flags & 8)
                merge_declaration_classes(instantiated, owner);
        }
    }
    if (!(owner->type_flags & 0x10)) {
        compiler_warning(0x202, owner);
        return 0;
    }
    if (((CompilerGlobalSymbolView *)owner)->class_detail->context_flags & 0x80) {
        diagnose_member_declaration(owner);
        return 0;
    }
    expression->kind = 6;
    constructor = find_class_constructor(owner);
    if (bound) {
        bound->binding = make_symbol_binding(NULL, bound->binding->owner, constructor);
        return 1;
    }
    expression->value.symbol = make_class_constructor_reference(owner, constructor);
    return 1;
}
extern "C" CompilerExpression *__cdecl prepare_call_prefix(CompilerExpression *, CompilerExpression **) { __asm { int 3 } return NULL; }
extern "C" unsigned __cdecl check_call_signature(CompilerType *, CompilerTypeShape *, CompilerExpression *, int) { __asm { int 3 } return 0; }
extern "C" CompilerExpression *__cdecl convert_call_arguments(unsigned, const void *, CompilerExpression *, int) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl finish_call_expression(CompilerExpression *, unsigned, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_call_result_temporary(CompilerType *, CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl finish_call_result_temporary(CompilerExpression *, CompilerType *) { __asm { int 3 } }

extern "C" CompilerSymbol *__cdecl find_class_constructor(CompilerSymbol *input)
{
    CompilerClassBinding *binding = ((CompilerKind4Symbol *)input)->class_binding;
    CompilerClassMemberIterator members;
    members.InitializeHead(binding ? binding->members : NULL);
    members.reset_value = 0;
    members.current = members.valid ? members.chain : NULL;
    members.owner = &((CompilerNamedSymbol *)input)->link;
    members.mode = 2;
    /* This owner contains the original inlined mode2 iteration. */
    while (members.current) {
        members.entry = members.current;
        if (members.chain) members.chain = members.chain->next;
        members.current = members.chain;
        CompilerSymbol *member = (CompilerSymbol *)((BYTE *)members.entry - 8);
        if ((member->kind == 17 || member->kind == 8) && (member->type_flags & 0x10))
            return member;
    }
    return NULL;
}
extern "C" CompilerSymbol *__cdecl make_class_constructor_reference(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } return NULL; }

/* The wrapper owns76 bytes of stack storage. Constructor-consumed fields
 * are typed below; unencountered portions remain uninterpreted. */
struct CompilerOverloadResolution {
    CompilerSymbol *function; /* +0 */
    void *template_arguments; /* +4, borrowed cursor domain */
    unsigned candidate_count, remaining_candidates; /* +8/+c */
    unsigned argument_count; /* +10, includes the implicit object */
    CompilerCallSelection *candidates, *current_candidate; /* +14/+18 */
    CompilerCallSelection *last_candidate; /* +1c */
    CompilerTypeConversion *conversions; /* +20, storage follows candidate rows */
    unsigned constructed_candidates, conversion_count; /* +24/+28 */
    unsigned phase_argument, phase_candidates; /* +2c/+30, prepared diagnostic phase */
    BYTE flags; /* +34 */
    BYTE unmodeled_35_37[3];
    unsigned resolution_limit; /* +38, initialized10; wider role pending */
    unsigned failed; /* +3c */
    BYTE lookup_mode; /* +40, only the low incoming BYTE is stored */
    BYTE unmodeled_41_43[3];
    CompilerExpression *arguments; /* +44, borrowed argument list */
    unsigned qualification; /* +48 */
    typedef CompilerCallSelection *(RECON_THISCALL CompilerOverloadResolution::*FailureHandler)(void);
    RECON_THISCALL CompilerOverloadResolution(CompilerSymbol *, CompilerExpression *, int, unsigned, unsigned);
    CompilerOverloadResolution *RECON_THISCALL Initialize(CompilerSymbol *, int, unsigned, unsigned);
    void RECON_THISCALL SetDiagnosticSymbol(CompilerSymbol *);
    void RECON_THISCALL InitializeCandidates(void);
    void RECON_THISCALL CollectFunctions(CompilerSymbol *, unsigned);
    void RECON_THISCALL CollectMemberFunctions(CompilerSymbol *, unsigned);
    void RECON_THISCALL AddCandidate(CompilerSymbol *, unsigned);
    void RECON_THISCALL FinishTemplates(void);
    void RECON_THISCALL PrepareDiagnostics(unsigned, unsigned);
    int RECON_THISCALL RejectCandidate(CompilerCallSelection *, CompilerExpression *);
    void RECON_THISCALL ReportRejected(CompilerCallSelection *, CompilerExpression *);
    int RECON_THISCALL CollectArguments(CompilerExpression *);
    int RECON_THISCALL BindObject(CompilerExpression *);
    CompilerCallSelection *RECON_THISCALL Select(void);
    CompilerCallSelection *RECON_THISCALL Diagnose(unsigned, FailureHandler);
    CompilerCallSelection *RECON_THISCALL NoMatch(void);
};
typedef char CompilerOverloadResolution_layout[(sizeof(CompilerOverloadResolution) == 76 &&
    offsetof(CompilerOverloadResolution, function) == 0 &&
    offsetof(CompilerOverloadResolution, template_arguments) == 4 &&
    offsetof(CompilerOverloadResolution, candidate_count) == 8 &&
    offsetof(CompilerOverloadResolution, remaining_candidates) == 12 &&
    offsetof(CompilerOverloadResolution, argument_count) == 16 &&
    offsetof(CompilerOverloadResolution, candidates) == 20 &&
    offsetof(CompilerOverloadResolution, current_candidate) == 24 &&
    offsetof(CompilerOverloadResolution, last_candidate) == 28 &&
    offsetof(CompilerOverloadResolution, conversions) == 32 &&
    offsetof(CompilerOverloadResolution, constructed_candidates) == 36 &&
    offsetof(CompilerOverloadResolution, conversion_count) == 40 &&
    offsetof(CompilerOverloadResolution, flags) == 52 &&
    offsetof(CompilerOverloadResolution, failed) == 60 &&
    offsetof(CompilerOverloadResolution, arguments) == 68 &&
    offsetof(CompilerOverloadResolution, qualification) == 72) ? 1 : -1];
extern "C" CompilerCallSelection *__cdecl select_call_overload(CompilerSymbol *input,
    CompilerExpression *object, CompilerExpression *arguments, int count,
    unsigned qualification, unsigned mode)
{
    CompilerCallSelection *selection;
    CompilerGlobalSymbolView *function;
    if (count < 0) return NULL;
    CompilerOverloadResolution resolution(input, arguments, count, qualification, mode);
    if (resolution.failed) return NULL;
    if (!resolution.CollectArguments(arguments)) return NULL;
    if (!resolution.BindObject(object)) return NULL;
    selection = resolution.Select();
    if (!selection) selection = resolution.Diagnose(0, &CompilerOverloadResolution::NoMatch);
    if (!selection) return NULL;
    function = (CompilerGlobalSymbolView *)selection->ordinary;
    if ((function->linkage_flags & 0x40) && !(function->definition_flags & 4) && !(mode & 8)) {
        if (!frontend_bss.debug_module->Slot6c())
            report_compiler_diagnostic(73, "..\\Cxx\\dymto.c", 0x19e1);
        complete_expression_class((CompilerSymbol *)function, selection->completion_context);
    } else if (selection->selected)
        function = (CompilerGlobalSymbolView *)selection->selected;
    if ((mode & 1) && frontend_bss.browser_filename)
        frontend_data.backend->Slot1c((unsigned)function);
    return selection;
}

extern "C" int __cdecl check_function_argument_count(CompilerTypeShape *, unsigned);
int RECON_THISCALL CompilerOverloadResolution::CollectArguments(CompilerExpression *arguments)
{
    PrepareDiagnostics(1, 0);
    CompilerCallSelection *candidate = current_candidate;
    while (candidate) {
        if (candidate->flags & 4) break;
        unsigned count = argument_count;
        if (candidate->flags & 2) --count;
        int match = check_function_argument_count(
            ((CompilerIrSymbol *)candidate->ordinary)->type->shape, count);
        if (match >= 0 && match <= 1) {
            if (RejectCandidate(candidate, arguments)) return 0;
        } else if (match == 3) candidate->flags |= 0x10;
        candidate = candidate->next;
    }
    return 1;
}
extern "C" int __cdecl check_function_argument_count(CompilerTypeShape *shape, unsigned count)
{
    if (shape->flags & 0x4000) return count ? 0 : 2;
    CompilerFunctionType *function = shape->payload.function;
    unsigned expected = function->parameters->count;
    int variadic = (shape->flags & 0x8000) != 0;
    if (variadic) --expected;
    if (count == expected) return 2;
    if (count > expected) return variadic ? 3 : 0;
    return count + function->symbol_count < expected ? 1 : 2;
}
int RECON_THISCALL CompilerOverloadResolution::BindObject(CompilerExpression *) { __asm { int 3 } return 0; }
CompilerCallSelection *RECON_THISCALL CompilerOverloadResolution::Select(void) { __asm { int 3 } return NULL; }
CompilerCallSelection *RECON_THISCALL CompilerOverloadResolution::Diagnose(unsigned, FailureHandler) { __asm { int 3 } return NULL; }
CompilerCallSelection *RECON_THISCALL CompilerOverloadResolution::NoMatch(void) { __asm { int 3 } return NULL; }

extern "C" {
int __cdecl is_member_function_group(CompilerSymbol *);
unsigned __cdecl count_function_candidates(CompilerSymbol *);
void __cdecl initialize_pending_declaration_cursor(PendingDeclarationCursor *, void *);
int __cdecl next_pending_declaration(PendingDeclarationCursor *);
void __cdecl finish_overload_templates(void *);
}
RECON_THISCALL CompilerOverloadResolution::CompilerOverloadResolution(
    CompilerSymbol *input, CompilerExpression *arguments, int count,
    unsigned qualification, unsigned mode)
{
    Initialize(input, count, 1, mode);
    this->arguments = arguments;
    this->qualification = qualification;
    if (function->kind == 8 || function->kind == 17 ||
        (function->kind == 24 && is_member_function_group(((CompilerAliasSymbol *)function)->target->symbol)))
        ++argument_count;
    if ((((CompilerNamedSymbol *)function)->link.name->flags >> 27) & 1)
        SetDiagnosticSymbol(function);
    candidate_count = count_function_candidates(function);
    if (template_arguments) {
        PendingDeclarationCursor cursor;
        initialize_pending_declaration_cursor(&cursor, template_arguments);
        while (next_pending_declaration(&cursor)) ++candidate_count;
    }
    if (!candidate_count) {
        failed = 1;
        DeferredAction *action = frontend_bss.deferred_action_head;
        if (action && (flags & 1)) {
            if (frontend_bss.deferred_action_depth <= 0) {
                do { action->Execute(); action = action->next; } while (action);
            }
            frontend_bss.deferred_action_tail = NULL;
            frontend_bss.deferred_action_head = NULL;
        }
        return;
    }
    InitializeCandidates();
    CollectFunctions(function, 0);
    if (template_arguments) {
        FinishTemplates();
        finish_overload_templates(template_arguments);
    }
    remaining_candidates = candidate_count;
    current_candidate = candidates;
    if (flags & 4) {
        PrepareDiagnostics(0, 0);
        CompilerCallSelection *candidate = current_candidate;
        while (candidate) {
            if (((CompilerIrSymbol *)candidate->ordinary)->reference_flags & 8) {
                if (RejectCandidate(candidate, 0)) {
                    failed = 1;
                    return;
                }
            }
            candidate = candidate->next;
        }
    }
    frontend_bss.deferred_action_tail = NULL;
    frontend_bss.deferred_action_head = NULL;
}

extern "C" int __cdecl is_member_function_group(CompilerSymbol *) { __asm { int 3 } return 0; }
extern "C" unsigned __cdecl count_function_candidates(CompilerSymbol *input)
{
    if (input->kind == 7 || input->kind == 8 || input->kind == 6)
        return (((CompilerGlobalSymbolView *)input)->linkage_flags & 0xe0) == 0;
    if (input->kind == 24) {
        CompilerAliasTarget *alias = ((CompilerAliasSymbol *)input)->target;
        if (alias->selected) return 0;
        return (((CompilerGlobalSymbolView *)alias->symbol)->linkage_flags & 0xe0) == 0;
    }
    unsigned count = 0;
    if (input->kind == 25) {
        CompilerOverloadLink *link = ((CompilerOverloadSymbol *)input)->group->head;
        do {
            count += count_function_candidates(link->symbol);
            link = link->next;
        } while (link);
        return count;
    }
    /* Other symbol variants do not establish this member-chain payload. */
    if (input->kind != 16 && input->kind != 17) { __asm { int 3 } }
    CompilerSymbolLink *cursor = &((CompilerNamedSymbol *)input)->link;
    if (!input->IsFunction() && input->kind != 24)
        cursor = ((CompilerMemberGroupSymbol *)input)->variants;
    while (cursor) {
        CompilerSymbolLink *link = cursor;
        if (input->IsFunction() || input->kind == 24) cursor = NULL;
        else cursor = cursor->next;
        CompilerSymbol *member = (CompilerSymbol *)((BYTE *)link - 8);
        if (member->kind == 7 || member->kind == 8 || member->kind == 6) {
            if (!(((CompilerGlobalSymbolView *)member)->linkage_flags & 0xe0)) ++count;
        } else if (member->kind == 24) {
            CompilerAliasTarget *alias = ((CompilerAliasSymbol *)member)->target;
            if (!alias->selected &&
                !(((CompilerGlobalSymbolView *)alias->symbol)->linkage_flags & 0xe0)) ++count;
        }
    }
    return count;
}
extern "C" void __cdecl finish_overload_templates(void *) { __asm { int 3 } }
CompilerOverloadResolution *RECON_THISCALL CompilerOverloadResolution::Initialize(
    CompilerSymbol *input, int count, unsigned lookup_mode, unsigned flags)
{
    function = input;
    template_arguments = NULL;
    candidate_count = 0;
    argument_count = count;
    last_candidate = NULL;
    constructed_candidates = 0;
    conversion_count = 0;
    failed = 0;
    this->flags = (BYTE)flags;
    resolution_limit = 10;
    this->lookup_mode = (BYTE)lookup_mode;
    return this;
}
void RECON_THISCALL CompilerOverloadResolution::SetDiagnosticSymbol(CompilerSymbol *) { __asm { int 3 } }
void RECON_THISCALL CompilerOverloadResolution::InitializeCandidates(void)
{
    CompilerArena *arena = &frontend_bss.arenas[5];
    BYTE *storage = arena->cursor;
    /* Original rows are64 bytes, followed by88-byte conversion records.
     * Native uses the same real types with its natural member pointers. */
    unsigned bytes = candidate_count *
        (sizeof(CompilerCallSelection) + argument_count * sizeof(CompilerTypeConversion));
    arena->cursor += (bytes + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    candidates = (CompilerCallSelection *)storage;
    conversions = (CompilerTypeConversion *)(storage + candidate_count * sizeof(CompilerCallSelection));
}
extern "C" int __cdecl is_free_function_group(CompilerSymbol *);
#define DIRECT_CALLABLE(node) ((node)->kind == 7 || (node)->kind == 6 || \
    ((node)->kind == 24 && is_free_function_group(((CompilerAliasSymbol *)(node))->target->symbol)) || \
    (node)->kind == 8 || \
    ((node)->kind == 24 && is_member_function_group(((CompilerAliasSymbol *)(node))->target->symbol)))
void RECON_THISCALL CompilerOverloadResolution::CollectFunctions(CompilerSymbol *input, unsigned mode)
{
    if (!function) function = input;
    if (DIRECT_CALLABLE(input)) {
        CompilerSymbol *actual = input->kind == 24 ? ((CompilerAliasSymbol *)input)->target->symbol : input;
        if (!(((CompilerGlobalSymbolView *)actual)->linkage_flags & 0xe0))
            AddCandidate(input, mode);
        return;
    }
    if (input->kind != 25) {
        CollectMemberFunctions(input, mode);
        return;
    }
    CompilerOverloadLink *link = ((CompilerOverloadSymbol *)input)->group->head;
    do {
        CompilerSymbol *member = link->symbol;
        if (DIRECT_CALLABLE(member)) {
            if (member->kind == 24 && ((CompilerAliasSymbol *)member)->target->selected) {
                link = link->next;
                continue;
            }
            /* In this list arm the original tests the alias record itself. */
            if (!(((CompilerGlobalSymbolView *)member)->linkage_flags & 0xe0))
                AddCandidate(member, mode);
        } else CollectMemberFunctions(member, mode);
        link = link->next;
    } while (link);
}
#undef DIRECT_CALLABLE
void RECON_THISCALL CompilerOverloadResolution::FinishTemplates(void) { __asm { int 3 } }
void RECON_THISCALL CompilerOverloadResolution::PrepareDiagnostics(unsigned phase, unsigned argument)
{
    resolution_limit = phase;
    switch (phase) {
    case 0: case 3: break;
    case 4: case 5:
        if (phase_candidates) break;
        /* Fall through to refresh the remaining-candidate count. */
    default:
        phase_candidates = remaining_candidates;
        break;
    }
    if ((int)phase >= 3 && (int)phase <= 5) phase_argument = argument;
}
int RECON_THISCALL CompilerOverloadResolution::RejectCandidate(CompilerCallSelection *candidate,
    CompilerExpression *argument)
{
    candidate->flags |= 0x40;
    if (current_candidate == candidate) current_candidate = candidate->next;
    if (candidate->next) candidate->next->previous = candidate->previous;
    if (candidate->previous) candidate->previous->next = candidate->next;
    if (!(candidate->flags & 0x80)) --remaining_candidates;
    if (remaining_candidates) return 0;
    ReportRejected(candidate, argument);
    return 1;
}
void RECON_THISCALL CompilerOverloadResolution::ReportRejected(CompilerCallSelection *, CompilerExpression *) { __asm { int 3 } }

extern "C" int __cdecl is_free_function_group(CompilerSymbol *) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerOverloadResolution::CollectMemberFunctions(
    CompilerSymbol *input, unsigned mode)
{
    CompilerSymbolLink *root = &((CompilerNamedSymbol *)input)->link;
    CompilerSymbolLink *cursor = root;
    if (input->kind != 7 && input->kind != 8 && input->kind != 6 && input->kind != 24) {
        if (input->kind != 16 && input->kind != 17) { __asm { int 3 } }
        cursor = ((CompilerMemberGroupSymbol *)input)->variants;
    }
    while (root && cursor) {
        CompilerSymbolLink *link = cursor;
        if (input->kind == 7 || input->kind == 8 || input->kind == 6 || input->kind == 24)
            cursor = NULL;
        else cursor = cursor->next;
        CompilerSymbol *member = (CompilerSymbol *)((BYTE *)link - 8);
        CompilerSymbol *actual = member;
        if (member->kind == 24) {
            CompilerAliasTarget *alias = ((CompilerAliasSymbol *)member)->target;
            if (alias->selected) continue;
            actual = alias->symbol;
        } else if (member->kind != 6 && member->kind != 7 && member->kind != 8) {
            /* Other member records do not establish the extended flag byte. */
            __asm { int 3 }
        }
        if (!(((CompilerGlobalSymbolView *)actual)->linkage_flags & 0xe0))
            AddCandidate(member, mode);
    }
}
void RECON_THISCALL CompilerOverloadResolution::AddCandidate(CompilerSymbol *input, unsigned mode)
{
    CompilerType *member_type = NULL;
    if (input->kind == 24 && is_member_function_group(((CompilerAliasSymbol *)input)->target->symbol)) {
        CompilerAliasTarget *alias = ((CompilerAliasSymbol *)input)->target;
        int index = alias->owner_index;
        if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        CompilerSymbol *owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
        member_type = ((CompilerIrSymbol *)alias->symbol)->type->BindMemberScope(owner);
    }
    if (constructed_candidates >= candidate_count) { __asm { int 3 } }
    CompilerCallSelection *candidate = candidates + constructed_candidates++;
    if (candidate)
        last_candidate = new(candidate) CompilerCallSelection(input, mode, member_type,
            conversions + conversion_count, last_candidate);
    else last_candidate = NULL;
    conversion_count += argument_count;
}

RECON_THISCALL CompilerCallSelection::CompilerCallSelection(CompilerSymbol *input, unsigned mode,
    CompilerType *member_type, CompilerTypeConversion *conversions, CompilerCallSelection *previous)
{
    ordinary = input->kind == 24 ? ((CompilerAliasSymbol *)input)->target->symbol : input;
    selected = input->kind == 24 ? input : NULL;
    CompilerParameterTypes *parameters =
        ((CompilerIrSymbol *)ordinary)->type->shape->payload.function->parameters;
    this->mode = mode;
    parameter_start = parameter_cursor = parameters->types;
    this->conversions = conversions;
    if (ordinary->type_flags & 4) {
        flags |= 2;
        if ((((CompilerIrSymbol *)ordinary)->storage_class & 7) != 3)
            this->member_type = member_type ? member_type :
                ((CompilerIrSymbol *)ordinary)->type->BindMemberScope(NULL);
    }
    if (previous) {
        this->previous = previous;
        previous->next = this;
    }
}
CompilerType *RECON_THISCALL CompilerType::BindMemberScope(CompilerSymbol *owner)
{
    CompilerTypeShape member_scope;
    CompilerType description;
    memset(&member_scope, 0, sizeof(member_scope));
    member_scope.flags = 0x40;
    /* The function shape borrows the complete underlying type record. */
    memcpy(&description, shape->payload.type, 16);
    if (owner)
        description.auxiliary = ((CompilerGlobalSymbolView *)owner)->class_detail->scope_number;
    description.shape = intern_type_shape(&member_scope);
    return intern_type(&description);
}
