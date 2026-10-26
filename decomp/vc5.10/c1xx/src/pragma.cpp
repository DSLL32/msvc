/* Pragma dispatch and source-control markers. The wider branches retain
 * source-only state; native currently reaches the real recorded-reader child. */
#include <locale.h>
#include <string.h>
#include "parser-state.h"
#include "lexer.h"
#include "ir.h"
#include "pragma.h"
#include <recon.h>
#pragma intrinsic(strcmp)

/* These are source-only cells, not native initialized state. Existing owners
 * below must remain aliases; notably the three declaration segment defaults. */
struct PragmaSourceState {
    CompilerSymbol *allocated_text_segment; /* cc4a0 */
    unsigned init_segment_blocked;      /* cc6a0, predicate-only use */
    unsigned init_segment_kind;         /* ccb54, actual initial1 */
    CompilerNameEntry *init_segment_name;      /* cc604 */
    void *pch_stop_owner;               /* cd4cc, helper output only */
    unsigned pch_stop_seen, pch_stop_failed; /* cd4d4/d8 */
    WORD pch_header_status;             /* cd858, actual initialffff */
    const char *optimization_text;      /* c06d4 */
    int optimization_enabled;          /* bf488 */
    unsigned optimization_restore_mask, optimization_disable_mask; /* cdb6c/70 */
};
#ifndef RECON_NATIVE_TEST
static PragmaSourceState pragma_state = {
    NULL, 0, 1, NULL, NULL, 0, 0, 0xffff,
    NULL, 0, 0x03fff800, 0x037eb800
};
#else
#define pragma_state (*(PragmaSourceState *)(recon_unsupported("pragma: source-only wider state"),0))
#endif
#define PRAGMA_CONTEXT (*(ParserFrontendContextView *)&frontend_bss.saved_context_stack)
#define PRAGMA_STREAM ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)
#define PRAGMA_ASSERT(test,line) do { if (!(test)) \
    report_compiler_diagnostic(73, ".\\p0pragma.c", line); } while (0)
#define PRAGMA_UNREAD(t) PRAGMA_STREAM->Unread((unsigned)(t))
#define PRAGMA_SCOPE_WARNING() do { if (frontend_bss.declaration_scope_mode) compiler_warning(156); } while (0)

extern "C" {
CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
ParserTokenView *__cdecl expect_directive_token(BYTE);
void __cdecl skip_to_directive_newline(ParserTokenView *);
void __cdecl set_warning_state(unsigned, unsigned); /*424ef*/
ParserTokenView *__cdecl read_optional_pragma_value(unsigned); /*80ef9*/
ParserTokenView *__cdecl read_required_pragma_value(unsigned); /*80eab*/
ParserTokenView *__cdecl read_pragma_string(void); /*465ce*/
ParserTokenView *__cdecl read_pragma_switch(void); /*45bde*/
CompilerSymbol *__cdecl find_or_create_segment(CompilerNameEntry *, CompilerNameEntry *, unsigned, unsigned); /*4858d*/
void __cdecl select_code_segment(unsigned); /*47efa*/
void __cdecl prepare_code_segment(unsigned); /*47db5*/
void __cdecl enable_minimal_rebuild_component(void); /*45cf7*/
void __cdecl enable_minimum_type_info_component(void); /*45bad*/
void __cdecl enable_browser_component(void); /*49549*/
void __cdecl set_member_pointer_generality(unsigned); /*aff8e*/
void __cdecl set_member_pointer_inheritance(unsigned); /*affc7*/
void __cdecl set_init_segment_function(CompilerNameEntry *); /*89866*/
void __cdecl mark_precompiled_header_stop(void **); /*3ced6*/
unsigned __cdecl pack_option_flags(void);
void __cdecl apply_option_flags(unsigned); /*3c7c7*/
void __cdecl print_pragma_message(const char *); /*809f9*/
void __cdecl read_pragma_identifier_list(void (__cdecl *)(CompilerNameEntry *)); /*49616*/
void __cdecl mark_function_nonintrinsic(CompilerNameEntry *); /*4af69*/
void __cdecl mark_function_intrinsic(CompilerNameEntry *); /*8b8a4*/
void __cdecl assign_allocated_text_segment(CompilerNameEntry *); /*8d06f*/
void __cdecl process_pack_pragma(void); /*40f01*/
void __cdecl prepare_expression_input(const char *);
void __cdecl disable_multibyte_identifiers(void);
void __cdecl initialize_multibyte_categories(void);
}

/* Semantic option letters/values/destinations from the nine real12-byte rows.
 * This is not a copy of compressed parser data or neighboring image bytes. */
struct PragmaOptimizationOption { char letter; int enabled; int *destination; };
static const PragmaOptimizationOption optimization_options[] = {
    {'a',1,&frontend_data.options.assume_no_alias},
    {'g',1,&frontend_data.options.global_optimize},
    {'p',1,&frontend_data.options.precise_float},
    {'s',0,&frontend_data.options.optimize_speed},
    {'t',1,&frontend_data.options.optimize_speed},
    {'w',1,&frontend_data.options.assume_no_global_alias},
    {'q',1,&frontend_data.normalized_oq},
    {'v',1,&frontend_data.options.option_ov},
    {'y',1,&frontend_data.options.omit_frame_pointer},
    {0,0,NULL}
};

extern "C" ParserTokenView *__cdecl dispatch_recorded_pragma(void)
{
    ParserTokenView *token, *value;
    CompilerNameEntry *name, *class_name;
    CompilerSymbol **segment;
    const char *text;
    unsigned code, segment_kind, number, state;
    int signed_position, position;
    PRAGMA_ASSERT(frontend_bss.debug_module->Slota4(), 0x410);
    token = PRAGMA_CONTEXT.ReadToken();
    code = token->kind;
    if (code != PRAGMA_PACK && code != PRAGMA_WARNING) {
#ifdef __linux__
        recon_unsupported("pragma dispatcher: only encountered packing/warning directives");
#else
        __asm { int 3 }
#endif
    }
    switch (code) {
    case PRAGMA_PACK:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(PRAGMA_PACK), 0x4fa);
        process_pack_pragma();
        break;
    case PRAGMA_ONCE: {
        CompilerSourceFile *file = (CompilerSourceFile *)frontend_bss.saved_context_stack.origin;
        file->record_index = (file->record_index & 0x00ffffff) | 0x01000000;
        break;
    }
    case 43: /* Recorded source-line/file marker. */
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot24(43), 0x433);
        token = PRAGMA_CONTEXT.ReadToken();
        frontend_bss.saved_context_stack.position = (unsigned)token->value.literal.value;
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 7) {
            PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(43), 0x440);
            frontend_bss.saved_context_stack.origin = frontend_data.translation_unit->FindSourceFile(
                (const char *)token->value.pointer, 0);
            if (frontend_bss.declaration_scope_mode) emit_context_transition(2);
            if (frontend_bss.browser_filename) {
                frontend_data.backend->Slot10();
                frontend_data.backend->Slot18();
                emit_context_transition(1);
                break;
            }
        } else PRAGMA_UNREAD(token);
        emit_context_transition(1);
        break;
    case 44: /* Recorded source owner, followed by the initial line. */
        text = (const char *)token->value.pointer;
        frontend_bss.saved_context_stack.origin = text ? (void *)text :
            frontend_data.translation_unit->FindSourceFile(frontend_bss.default_source_name, 0);
        frontend_bss.saved_context_stack.position = 1;
        emit_context_transition(2);
        emit_context_transition(1);
        if (frontend_bss.browser_filename) frontend_data.backend->Slot18();
        break;
    case 45:
        signed_position = (int)token->value.pointer;
        position = signed_position < 0 ? -signed_position : signed_position;
        if ((frontend_data.compiler_flags & 0x20) && position <= 0) select_code_segment(1);
        if (signed_position < 0)
            report_language_diagnostic(1, 0x67, frontend_bss.saved_context_stack.FileName());
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 0xf0) {
            frontend_bss.saved_context_stack.position = ((FrontendSourcePosition *)&token->value)->position;
            frontend_bss.saved_context_stack.origin = ((FrontendSourcePosition *)&token->value)->origin;
            if ((frontend_data.compiler_flags & 0x20000) && !(frontend_data.compiler_flags & 0x20))
                emit_context_transition(0x24);
            token = PRAGMA_CONTEXT.ReadToken();
        }
        PRAGMA_UNREAD(token);
        emit_context_transition(2); emit_context_transition(1);
        if (position > 0 && frontend_bss.browser_filename) frontend_data.backend->Slot14(position);
        if (position == 1 && pragma_state.pch_stop_seen && !pragma_state.pch_stop_failed) {
            prepare_expression_input(frontend_data.options.pch_create_file);
            return (ParserTokenView *)frontend_data.canonical_token_records[3];
        }
        if (!frontend_data.source_include_depth && frontend_bss.forced_source_index == 1 &&
            !pragma_state.pch_header_status)
            return (ParserTokenView *)frontend_data.canonical_token_records[3];
        break;
    case PRAGMA_LOOP_OPT: case PRAGMA_CHECK_POINTER: case PRAGMA_SWITCH_CHECK:
    case PRAGMA_PLMN: case PRAGMA_PLMF: case PRAGMA_PAGE: case PRAGMA_SKIP:
    case PRAGMA_SUBTITLE: case PRAGMA_TITLE: case PRAGMA_PAGESIZE:
    case PRAGMA_LINESIZE: case PRAGMA_NATIVE_CALLER:
        report_language_diagnostic(4, 0x44);
        skip_to_directive_newline(token);
        break;
    case PRAGMA_STRING_POOLING:
        if (frontend_bss.declaration_scope_mode) { compiler_warning(156); break; }
        value = read_optional_pragma_value(1);
        frontend_data.normalized_pool_strings = value ? (int)value->value.pointer : frontend_data.options.pool_strings;
        /* Actual nonNULL selection clears this cell; NULL preserves its input. */
        frontend_data.normalized_readonly_strings = value ? 0 : frontend_data.options.readonly_strings;
        break;
    case PRAGMA_INTRINSIC: case PRAGMA_FUNCTION:
        PRAGMA_SCOPE_WARNING();
        if (!expect_directive_token(118)) break;
        read_pragma_identifier_list(code == PRAGMA_INTRINSIC ? mark_function_intrinsic : mark_function_nonintrinsic);
        expect_directive_token(126);
        break;
    case PRAGMA_ALLOC_TEXT:
        PRAGMA_SCOPE_WARNING();
        if (!expect_directive_token(118)) break;
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 7) {
            PRAGMA_UNREAD(token); value = read_pragma_string(); token = value;
            if (!*(const char *)value->value.pointer) goto empty_segment_name;
            name = find_or_create_identifier((const char *)value->value.pointer);
        } else if (token->kind == 6) name = (CompilerNameEntry *)token->value.pointer;
        else {
            report_language_diagnostic(1, 0x50, token->kind);
            skip_to_directive_newline(token); break;
        }
        pragma_state.allocated_text_segment = find_or_create_segment(name, frontend_data.builtin_names.unnamed, 0, 2);
        if (!expect_directive_token(55)) break;
        frontend_data.compiler_flags |= 0x10000;
        read_pragma_identifier_list(assign_allocated_text_segment);
        expect_directive_token(126);
        break;
    case PRAGMA_SAME_SEG:
        report_language_diagnostic(1, 0x8f);
        skip_to_directive_newline(PRAGMA_CONTEXT.ReadToken());
        break;
    case PRAGMA_CHECK_STACK:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(0),0x567);
        token = PRAGMA_CONTEXT.ReadToken();
        switch (token->kind) {
        case 0x51: frontend_data.normalized_stack_checks = 1; break;
        case 0x46: frontend_data.normalized_stack_checks = 0; break;
        case 45:
            PRAGMA_UNREAD(token);
            frontend_data.normalized_stack_checks = frontend_data.options.stack_checks;
            break;
        case 118:
            PRAGMA_UNREAD(token); value = read_optional_pragma_value(1);
            frontend_data.normalized_stack_checks = value ? (int)value->value.pointer : frontend_data.options.stack_checks;
            break;
        default: report_language_diagnostic(1, 0x4d); skip_to_directive_newline(token); break;
        }
        break;
    case PRAGMA_SETLOCALE:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(32),0x585);
        value = read_required_pragma_value(7);
        if (!value) break;
        if (!setlocale(LC_ALL, (const char *)value->value.pointer)) compiler_warning(175, value->value.pointer);
        disable_multibyte_identifiers(); initialize_multibyte_categories();
        break;
    case PRAGMA_MESSAGE:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot24(18),0x595);
        if (!expect_directive_token(118)) break;
        value = read_pragma_string();
        if (value && expect_directive_token(126)) {
            print_pragma_message((const char *)value->value.pointer);
            print_pragma_message("\n");
        }
        break;
    case PRAGMA_COMMENT:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot24(19),0x5b4);
        if (!expect_directive_token(118)) break;
        token = expect_directive_token(6);
        if (!token) break;
        text = ((CompilerNameEntry *)token->value.pointer)->name;
        frontend_data.control_option_text = NULL;
        if (!strcmp(text,"compiler")) {
            frontend_data.control_option_kind = 1;
            frontend_data.control_option_text = compiler_empty_string;
        } else if (!strcmp(text,"lib")) frontend_data.control_option_kind = 4;
        else if (!strcmp(text,"exestr")) frontend_data.control_option_kind = 3;
        else if (!strcmp(text,"user")) frontend_data.control_option_kind = 6;
        else if (!strcmp(text,"nolib")) frontend_data.control_option_kind = 9;
        else if (!strcmp(text,"linker")) frontend_data.control_option_kind = 10;
        else goto unknown_pragma_name;
        value = PRAGMA_CONTEXT.ReadToken();
        if (!frontend_data.control_option_text && value->kind == 55) {
            value = read_pragma_string();
            if (!value) break;
            frontend_data.control_option_text = (const char *)value->value.pointer;
            value = PRAGMA_CONTEXT.ReadToken();
        }
        PRAGMA_UNREAD(value);
        if (expect_directive_token(126) && frontend_data.control_option_text)
            emit_context_transition(11);
        break;
    case 47: /* Prepared linker-library comment, without a keyword prefix. */
        frontend_data.control_option_kind = 3;
        value = read_pragma_string();
        if (value) {
            frontend_data.control_option_text = (const char *)value->value.pointer;
            if (frontend_data.control_option_text) emit_context_transition(11);
        }
        break;
    case PRAGMA_INIT_SEG:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(30),0x5f3);
        if (pragma_state.init_segment_blocked || pragma_state.init_segment_kind != 1)
            compiler_warning(356);
        if (!expect_directive_token(118)) break;
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 7) {
            report_language_diagnostic(1,75);
            PRAGMA_UNREAD(token);
            token = read_pragma_string();
            if (!*(const char *)token->value.pointer) goto empty_segment_name;
            pragma_state.init_segment_kind = 5;
            pragma_state.init_segment_name = find_or_create_identifier((const char *)token->value.pointer);
            value = PRAGMA_CONTEXT.ReadToken();
            if (value->kind == 55) {
                value = expect_directive_token(6);
                if (value) {
                    set_init_segment_function((CompilerNameEntry *)value->value.pointer);
                    expect_directive_token(126);
                }
            } else {
                PRAGMA_UNREAD(value);
                expect_directive_token(value->kind == 126 ? 126 : 55);
            }
        } else if (token->kind == 6) {
            text = ((CompilerNameEntry *)token->value.pointer)->name;
            if (!strcmp(text,"compiler")) {
                report_language_diagnostic(1,74); pragma_state.init_segment_kind = 3;
            } else if (!strcmp(text,"lib")) {
                report_language_diagnostic(3,73); pragma_state.init_segment_kind = 2;
            } else if (!strcmp(text,"user")) pragma_state.init_segment_kind = 1;
            else goto unknown_pragma_name;
            expect_directive_token(126);
        } else { PRAGMA_UNREAD(token); expect_directive_token(6); }
        break;
    case PRAGMA_DATA_SEG: case PRAGMA_BSS_SEG: case PRAGMA_CONST_SEG: case PRAGMA_CODE_SEG:
        if (code == PRAGMA_CODE_SEG) { segment = &frontend_data.default_code_segment; segment_kind = 0; }
        else {
            if (frontend_bss.declaration_scope_mode) {
                const char *directive = code == PRAGMA_DATA_SEG ? "data_seg" :
                    code == PRAGMA_BSS_SEG ? "bss_seg" : "const_seg";
                if (frontend_data.options.incremental_compile) compiler_warning(307,directive);
                else report_language_diagnostic(1,177,directive);
            }
            if (code == PRAGMA_DATA_SEG) { segment = &declaration_reference_defaults.definition; segment_kind = 1; }
            else if (code == PRAGMA_BSS_SEG) { segment = &declaration_reference_defaults.fallback; segment_kind = 3; }
            else { segment = &declaration_reference_defaults.type_context; segment_kind = 4; }
        }
        frontend_bss.debug_module->Slot20(code);
        if (!expect_directive_token(118)) break;
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 126) { *segment = NULL; break; }
        if (token->kind != 7) {
            PRAGMA_UNREAD(token); read_pragma_string(); break;
        }
        PRAGMA_UNREAD(token); token = read_pragma_string();
        if (!*(const char *)token->value.pointer) goto empty_segment_name;
        name = find_or_create_identifier((const char *)token->value.pointer);
        value = PRAGMA_CONTEXT.ReadToken();
        if (value->kind == 55) {
            token = read_pragma_string();
            if (!token) break;
            if (!*(const char *)token->value.pointer) goto empty_segment_name;
            class_name = find_or_create_identifier((const char *)token->value.pointer);
        } else if (value->kind == 126) {
            class_name = frontend_data.builtin_names.unnamed;
            PRAGMA_UNREAD(value);
        } else {
            PRAGMA_UNREAD(value); expect_directive_token(55); break;
        }
        *segment = find_or_create_segment(name,class_name,segment_kind,2);
        if (!segment_kind && (frontend_data.compiler_flags & 0x20)) {
            select_code_segment(1); prepare_code_segment(0);
        }
        expect_directive_token(126);
        break;
    case PRAGMA_SEGMENT:
        frontend_bss.debug_module->Slot20(20);
        if (!expect_directive_token(118)) break;
        token = expect_directive_token(6);
        if (!token) break;
        name = (CompilerNameEntry *)token->value.pointer;
        if (!strcmp(name->name,"_TEXT")) { segment=&frontend_data.default_code_segment; segment_kind=0; }
        else if (!strcmp(name->name,"_DATA")) { segment=&declaration_reference_defaults.definition; segment_kind=1; }
        else if (!strcmp(name->name,"_BSS")) { segment=&declaration_reference_defaults.fallback; segment_kind=3; }
        else if (!strcmp(name->name,"CONST")) { segment=&declaration_reference_defaults.type_context; segment_kind=4; }
        else goto unknown_pragma_name;
        value = PRAGMA_CONTEXT.ReadToken();
        if (value->kind == 126) { *segment=NULL; break; }
        if (value->kind != 55) { PRAGMA_UNREAD(value); expect_directive_token(55); break; }
        value = expect_directive_token(6);
        if (!value) break;
        *segment = find_or_create_segment((CompilerNameEntry *)value->value.pointer,
            frontend_data.builtin_names.unnamed,segment_kind,2);
        expect_directive_token(126);
        break;
    case PRAGMA_OPTIMIZE:
        if (frontend_bss.declaration_scope_mode) { compiler_warning(156); break; }
        if (!expect_directive_token(118)) break;
        value = read_pragma_string();
        if (!value) break;
        pragma_state.optimization_text = (const char *)value->value.pointer;
        if (!expect_directive_token(55)) break;
        value = read_pragma_switch();
        if (!value) break;
        pragma_state.optimization_enabled = (int)value->value.pointer;
        if (!expect_directive_token(126)) break;
        if (*pragma_state.optimization_text) {
            while (*pragma_state.optimization_text) {
                const PragmaOptimizationOption *option;
                for (option=optimization_options;option->letter;++option) {
                    if (*pragma_state.optimization_text == option->letter)
                        *option->destination = pragma_state.optimization_enabled ? option->enabled : !option->enabled;
                }
                ++pragma_state.optimization_text;
            }
        } else {
            unsigned flags = pack_option_flags();
            if (pragma_state.optimization_enabled)
                flags = (flags & ~pragma_state.optimization_restore_mask) |
                    (frontend_bss.initial_option_flags & pragma_state.optimization_restore_mask);
            else flags &= ~pragma_state.optimization_disable_mask;
            apply_option_flags(flags);
        }
        break;
    case PRAGMA_AUTO_INLINE:
        if (frontend_bss.declaration_scope_mode) { compiler_warning(156); break; }
        value = read_optional_pragma_value(1);
        frontend_data.type_debug_mode = frontend_data.auto_inline = value ? (int)value->value.pointer : 1;
        break;
    case PRAGMA_VTORDISP:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(31),0x77c);
        value = read_optional_pragma_value(1);
        frontend_data.current_vtordisp = value ? (int)value->value.pointer : frontend_data.options.vtordisp;
        break;
    case PRAGMA_POINTERS_TO_MEMBERS:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(33),0x78c);
        if (!expect_directive_token(118)) break;
        do {
            token = expect_directive_token(6);
            if (!token) return NULL;
            text = ((CompilerNameEntry *)token->value.pointer)->name;
            if (!strcmp(text,"best_case")) set_member_pointer_generality(0);
            else if (!strcmp(text,"full_generality")) set_member_pointer_generality(1);
            else if (!strcmp(text,"single_inheritance")) set_member_pointer_inheritance(1);
            else if (!strcmp(text,"multiple_inheritance")) set_member_pointer_inheritance(2);
            else if (!strcmp(text,"virtual_inheritance")) set_member_pointer_inheritance(3);
            else goto unknown_pragma_name;
            value = PRAGMA_CONTEXT.ReadToken();
        } while (value->kind == 55);
        PRAGMA_UNREAD(value); expect_directive_token(126);
        break;
    case PRAGMA_WARNING:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(28),0x7bd);
        if (!expect_directive_token(118)) break;
next_warning_option:
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 6) {
            text = ((CompilerNameEntry *)token->value.pointer)->name;
            if (!strcmp(text,"disable")) state=13;
            else if (!strcmp(text,"error")) state=14;
            else if (!strcmp(text,"once")) state=5;
            else goto invalid_warning_option;
        } else if (token->kind == 101) {
            state=(unsigned)token->value.literal.value;
            if (state<1 || state>4) {
                report_language_diagnostic(1,0x268,state,1,4);
                skip_to_directive_newline(token); break;
            }
        } else if (token->kind == 103) state=0;
        else goto invalid_warning_option;
        if (!expect_directive_token(54)) break;
        value=PRAGMA_CONTEXT.ReadToken();
        while (value->kind == 101) {
            number=(unsigned)value->value.literal.value;
            set_warning_state(number,state);
            value=PRAGMA_CONTEXT.ReadToken();
        }
        if (value->kind == 127) goto next_warning_option;
        if (value->kind != 126) { PRAGMA_UNREAD(value); expect_directive_token(126); }
        break;
    case PRAGMA_HDRSTOP:
        frontend_data.compiler_flags |= 0x8000;
        mark_precompiled_header_stop(&pragma_state.pch_stop_owner);
        frontend_data.compiler_flags &= ~0x8000U;
        return (ParserTokenView *)frontend_data.canonical_token_records[1];
    case PRAGMA_INLINE_RECURSION:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(26),0x81f);
        value=read_optional_pragma_value(1);
        frontend_data.inline_recursion=value ? (WORD)(unsigned)value->value.pointer : 0;
        if ((unsigned)frontend_data.options.inline_level>0) emit_context_transition(32);
        break;
    case PRAGMA_INLINE_DEPTH:
        PRAGMA_ASSERT(frontend_bss.debug_module->Slot20(25),0x82c);
        value=read_optional_pragma_value(101);
        frontend_data.inline_depth=value ? (WORD)value->value.literal.value : 8;
        if ((unsigned)frontend_data.options.inline_level>0) emit_context_transition(32);
        break;
    case PRAGMA_COMPONENT:
        token=PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 45) { PRAGMA_UNREAD(token); break; }
        if (token->kind != 118) { PRAGMA_UNREAD(token); expect_directive_token(118); break; }
        token=PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 6) {
            text=((CompilerNameEntry *)token->value.pointer)->name;
            if (!strcmp(text,"browser")) enable_browser_component();
            else if (!strcmp(text,"minrebuild")) enable_minimal_rebuild_component();
            else if (!strcmp(text,"mintypeinfo")) enable_minimum_type_info_component();
            else {
                report_language_diagnostic(1,174,text);
                skip_to_directive_newline(token);
            }
        } else if (token->kind != 126) { PRAGMA_UNREAD(token); expect_directive_token(6); }
        break;
    case PRAGMA_STOP_MAP_REGION:
        if (!(frontend_data.source_mode_flags & 0x800000)) report_language_diagnostic(1,195);
        frontend_data.source_mode_flags &= ~0x800000U;
        break;
    default:
        report_language_diagnostic(1,68);
        skip_to_directive_newline(token);
        break;

    }
    return NULL;
unknown_pragma_name:
    report_language_diagnostic(1,109,token->value.pointer);
    skip_to_directive_newline(token);
    return NULL;
invalid_warning_option:
    report_language_diagnostic(1,0x267);
    skip_to_directive_newline(token);
    return NULL;
empty_segment_name:
    report_language_diagnostic(1, 0x26a);
    skip_to_directive_newline(token);
    return NULL;
}
