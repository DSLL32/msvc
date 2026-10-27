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
void __cdecl append_implicit_parameter(unsigned, CompilerParameterSet **, CompilerNameEntry *, CompilerType *); /*21787*/
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

void RECON_THISCALL CompilerTranslationUnit::MarkDefinitionScopes(void) { __asm { int 3 } }
extern "C" void __cdecl append_implicit_parameter(unsigned, CompilerParameterSet **, CompilerNameEntry *, CompilerType *) { __asm { int 3 } }
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
extern "C" ParserTokenView *__cdecl current_declaration_lookahead(void) { __asm { int 3 } return NULL; }
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
CompilerExpression *__cdecl make_implicit_this_expression(unsigned); /*21862*/
CompilerExpression *__cdecl make_ir_binary_node(unsigned, CompilerExpression *, CompilerExpression *); /*48df*/
void __cdecl emit_ir_expression(CompilerExpression *); /*48fb*/
void __cdecl emit_parameter_declaration(CompilerSymbol *); /*218fd*/
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
extern "C" CompilerExpression *__cdecl make_implicit_this_expression(unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_ir_binary_node(unsigned code,
    CompilerExpression *first, CompilerExpression *second)
{
    return make_binary_node(code, first, second, first->type);
}
extern "C" void __cdecl emit_parameter_declaration(CompilerSymbol *) { __asm { int 3 } }
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
extern "C" void __cdecl check_qualified_declaration(CompilerSymbol *) { __asm { int 3 } }
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
          (input->kind == 11 && shape && ((shape->flags & 0x77) == 0x40 ||
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
        (symbol != (CompilerGlobalSymbolView *)input || !(symbol->kind == 7 ||
        (symbol->kind == 10 && (symbol->storage_class & 7) == 7 &&
         (symbol->definition_flags & 0x10)))))
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
extern "C" int __cdecl is_function_specialization(CompilerSymbol *) { __asm { int 3 } return 0; }
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
    if (operation != 0x34 && operation != 0x8c && operation != 0x8d)
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

#ifndef RECON_NATIVE_TEST
DeclarationReferenceDefaults declaration_reference_defaults;
#endif
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
#ifdef RECON_NATIVE_TEST
    recon_unsupported("symbol emission: unmodeled non-typedef domain");
#endif
    classification = symbol->storage_class & 7;
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
extern "C" void __cdecl apply_declaration_reference(CompilerSymbol *, CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl check_global_declaration(CompilerSymbol *) { __asm { int 3 } }
extern "C" int __cdecl check_type_declaration_attribute(CompilerType *) { __asm { int 3 } return 0; }
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
    if (!((kind == 7 || kind == 8) || (!previous && kind == 11) ||
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
extern "C" CompilerTypeAlias *__cdecl initialize_type_aliases(void);
extern "C" CompilerTypeAlias *__cdecl apply_type_aliases(CompilerTypeAlias *, CompilerSymbol *);
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
extern "C" CompilerTypeAlias *__cdecl initialize_type_aliases(void) { __asm { int 3 } return NULL; }
extern "C" CompilerTypeAlias *__cdecl apply_type_aliases(CompilerTypeAlias *, CompilerSymbol *) { __asm { int 3 } return NULL; }
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
extern "C" CompilerSymbolBinding *__cdecl make_symbol_binding(void *, void *, CompilerSymbol *);
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
/* The search context is a genuine five-DWORD local object. Its two zero
 * stores are retained without inferring the later collector's meanings. */
struct CompilerClassBindingSearch {
    CompilerNameEntry *name;
    unsigned mode, reset_08, reset_0c;
    CompilerVector *matches;
};
typedef char CompilerClassBindingSearch_layout[(sizeof(CompilerClassBindingSearch) == 20 &&
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
        search.reset_08 = search.reset_0c = 0;
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
extern "C" CompilerSymbolBinding *__cdecl collect_class_bindings(CompilerSymbolBinding *, CompilerClassBindingSearch *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl finish_class_bindings(CompilerSymbolBinding *, CompilerVector *) { __asm { int 3 } }
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
#ifdef RECON_NATIVE_TEST
    /* Actual current calls contain no named nodes. A later nonempty list
     * must establish its value writer/owner before this predicate is used. */
    if (count) recon_unsupported("parameter binding: unmodeled nonempty value list");
#endif
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
void RECON_THISCALL CompilerMemberVariantIterator::SetLink(CompilerSymbolLink *) { __asm { int 3 } }
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

int RECON_THISCALL CompilerMemberVariantIterator::IsPrimary(void) { __asm { int 3 } return 0; }
CompilerSymbol *RECON_THISCALL CompilerMemberVariantIterator::GetSymbol(void) { __asm { int 3 } return NULL; }

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
    frame.source_state = frontend_bss.parameter_source_state;
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
extern "C" CompilerExpression *__cdecl prepare_parameter_expression(CompilerExpression *, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_parameter_operation(CompilerSymbol *, CompilerExpression *, unsigned, unsigned, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_parameter_statement(unsigned, CompilerExpression *, CompilerSymbol *) { __asm { int 3 } return NULL; }

extern "C" {
void __cdecl begin_nested_block(void); /*51db*/
void __cdecl emit_member_initializers(CompilerSymbol *, CompilerExpression *); /*2277c*/
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
extern "C" void __cdecl emit_member_initializers(CompilerSymbol *, CompilerExpression *) { __asm { int 3 } }
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
CompilerExpression *__cdecl make_return_object_call(unsigned, CompilerSymbol *, CompilerExpression *, CompilerExpression *, unsigned, unsigned); /*20e8f*/
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
            append_expression_list(NULL, converted, 2), 1, 1);
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
extern "C" CompilerExpression *__cdecl convert_return_temporary(CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" CompilerExpression *__cdecl make_return_object_call(unsigned, CompilerSymbol *, CompilerExpression *, CompilerExpression *, unsigned, unsigned) { __asm { int 3 } return NULL; }
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
    typedef CompilerExpression *(RECON_THISCALL CompilerTypeConversion::*Builder)(unsigned);
    Builder build, build_constant;
    BYTE unmodeled_28_2b[4];
    unsigned diagnostic;
    unsigned auxiliary_diagnostic; /* original+30, diagnostic context consumed by Build */
    BYTE unmodeled_34_3b[8];
    unsigned complexity;
    BYTE unmodeled_40_53[20];
    unsigned flags;
    RECON_THISCALL CompilerTypeConversion(unsigned, unsigned, CompilerExpression *, CompilerType *); /*11fba*/
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
    CompilerExpression *RECON_THISCALL Build(unsigned); /*11502*/
    CompilerExpression *RECON_THISCALL BuildScalar(unsigned); /*15b8f*/
    CompilerExpression *RECON_THISCALL BuildScalarConstant(unsigned); /*11540*/
    CompilerExpression *RECON_THISCALL BuildNullPointer(unsigned); /*1c208*/
    int RECON_THISCALL ShouldCheckConstant(void); /*1167e*/
    CompilerExpression *RECON_THISCALL WrapResult(unsigned, CompilerExpression *); /*1ba31*/
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
    offsetof(CompilerTypeConversion, flags) == 0x5c) ? 1 : -1];
#else
typedef char CompilerTypeConversion_layout[(sizeof(CompilerTypeConversion) == 88 &&
    sizeof(CompilerTypeConversion::Builder) == 4 && offsetof(CompilerTypeConversion, build_constant) == 0x24 &&
    offsetof(CompilerTypeConversion, diagnostic) == 0x2c && offsetof(CompilerTypeConversion, auxiliary_diagnostic) == 0x30 && offsetof(CompilerTypeConversion, complexity) == 0x3c &&
    offsetof(CompilerTypeConversion, flags) == 0x54) ? 1 : -1];
#endif
extern "C" {
unsigned __cdecl conversion_complexity(CompilerType *, CompilerExpression *); /*9dd3b*/
CompilerExpression *__cdecl evaluate_typed_conversion(unsigned, CompilerType *, CompilerExpression *); /*9e0bb*/
void __cdecl reconcile_typed_conversion(unsigned, CompilerExpression *, CompilerExpression *, CompilerType *); /*9fc73*/
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
    memset(unmodeled_40_53, 0, sizeof(unmodeled_40_53));
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
CompilerExpression *RECON_THISCALL CompilerTypeConversion::Build(unsigned mode)
{
    if (auxiliary_diagnostic && should_report_diagnostic(4, 0xef)) {
        report_language_diagnostic(4, 0xef, (unsigned)code, source_type, target_type);
        print_diagnostic_auxiliary(auxiliary_diagnostic);
    }
    if (build_constant != 0 && (input->flags & 7) == 3) return (this->*build_constant)(mode);
    return (this->*build)(mode);
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
CompilerExpression *RECON_THISCALL CompilerTypeConversion::BuildScalar(unsigned) { __asm { int 3 } return NULL; }
extern "C" {
__int64 __cdecl convert_constant_value(BYTE, CompilerExpression *, unsigned, unsigned, WORD); /*1171e*/
int __cdecl constant_conversion_overflows(WORD, WORD, WORD, __int64); /*11a63*/
}
CompilerExpression *RECON_THISCALL CompilerTypeConversion::BuildScalarConstant(unsigned result_mode)
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
    if (result_mode) return WrapResult(result_mode, input);
    return input;
}
int RECON_THISCALL CompilerTypeConversion::ShouldCheckConstant(void)
{
    if (frontend_data.options.warning_level < 2) return 0;
    return code == 0x34 || code == 0x3a || code == 0x47 || code == 0x4b ||
        code == 0x52 || code == 0x60 || code == 0x74 || code == 0x7d;
}
CompilerExpression *RECON_THISCALL CompilerTypeConversion::WrapResult(unsigned, CompilerExpression *) { __asm { int 3 } return NULL; }
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
CompilerExpression *RECON_THISCALL CompilerTypeConversion::BuildNullPointer(unsigned) { __asm { int 3 } return NULL; }
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
/* A view of the reached stack nodes, not an allocation-size assertion.
 * Level and symbol occupy the same payload cell on alternating nodes. */
struct CompilerBlockOutputEntry {
    CompilerBlockOutputEntry *next;
    union { unsigned level; CompilerSymbol *symbol; } value;
};
extern "C" void __cdecl pop_block_output(CompilerBlockOutputEntry **); /*17f95*/
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
extern "C" void __cdecl pop_block_output(CompilerBlockOutputEntry **) { __asm { int 3 } }
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
    if (frontend_bss.error_count || frontend_bss.quoted_skip_mode ||
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
    if (!frontend_bss.quoted_skip_mode && !frontend_bss.suppress_parser_codegen) {
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
extern "C" void __cdecl complete_constructor(CompilerSymbol *) { __asm { int 3 } }
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
