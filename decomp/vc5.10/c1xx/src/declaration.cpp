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
extern "C" CompilerExpression *__cdecl make_ir_binary_node(unsigned, CompilerExpression *, CompilerExpression *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl emit_ir_expression(CompilerExpression *) { __asm { int 3 } }
extern "C" void __cdecl emit_parameter_declaration(CompilerSymbol *) { __asm { int 3 } }
extern "C" void __cdecl emit_function_parameters(CompilerSymbol *, CompilerParameterSet *) { __asm { int 3 } }
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
extern "C" WORD __cdecl get_type_classification(CompilerType *) { __asm { int 3 } return 0; }
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
