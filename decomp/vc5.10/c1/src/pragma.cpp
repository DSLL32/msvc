/* C1 pragma directives and compiler-generated source-control records.
 * The wider directive state is source-only. Its entry guard traps in both
 * builds before unmodeled values are consumed. */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include "frontend.h"
#include "keywords.h"
#include "global-record.h"
#include "pragma.h"
#include "packing.h"
#pragma intrinsic(strcmp, memcpy, memset, strlen)
#include <recon.h>

/* Contextual cells on currently unsupported branches. No initialized value,
 * snapshot bytes or equality claim is assigned to these source-only owners. */
struct PragmaSourceState {
    CompilerIrSymbol *allocated_text_segment; /* RVA5e470 */
    WORD pch_header_status;                  /* RVA5f5a8 */
    const char *optimization_text;           /* RVA54c84 */
    int optimization_enabled;               /* RVA53ff0 */
};
static PragmaSourceState pragma_state;
void *c1_pch_stop_owner; /* Shared source-only PCH stop/file owner, RVA5f31c */
/* Respectively RVA5e588(BSS), 5e58c(CONST), 5e590(DATA). */
extern "C" int function_pch_replayed; /* Same owner as check_finished_function */
extern const char *current_forced_include; /* Same owner as forced-source input */
#define PRAGMA_CONTEXT c1_bss.current_context
#define PRAGMA_STREAM c1_bss.current_context.saved_frame->stream
#define PRAGMA_ASSERT(test,line) do { if (!(test)) \
    compiler_error(73, ".\\p0pragma.c", line); } while (0)
#define PRAGMA_UNREAD(t) PRAGMA_STREAM->Unread((unsigned)(t))
#define PRAGMA_SCOPE_WARNING() do { if (c1_bss.parameter_nesting) compiler_warning(156); } while (0)
#ifdef __linux__
#define PRAGMA_UNSUPPORTED(reason) recon_unsupported(reason)
#else
#define PRAGMA_UNSUPPORTED(reason) __asm { int 3 }
#endif

extern "C" {
CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
CompilerParserToken *__cdecl expect_directive_token(BYTE);
void __cdecl skip_to_directive_newline(CompilerParserToken *);
void __cdecl set_warning_state(unsigned, unsigned);
CompilerParserToken *__cdecl read_optional_pragma_value(unsigned);
CompilerParserToken *__cdecl read_required_pragma_value(unsigned);
CompilerParserToken *__cdecl read_pragma_string(void);
CompilerParserToken *__cdecl read_pragma_switch(void);
CompilerIrSymbol *__cdecl find_or_create_segment(CompilerNameEntry *, CompilerNameEntry *, unsigned, unsigned);
void __cdecl enable_minimal_rebuild_component(void);
void __cdecl enable_minimum_type_info_component(void);
void __cdecl enable_browser_component(void);
void __cdecl mark_precompiled_header_stop(void **);
unsigned __cdecl pack_option_flags(void);
void __cdecl apply_option_flags(unsigned);
void __cdecl print_pragma_message(const char *);
void __cdecl read_pragma_identifier_list(void (__cdecl *)(CompilerNameEntry *));
void __cdecl mark_function_nonintrinsic(CompilerNameEntry *);
void __cdecl mark_function_intrinsic(CompilerNameEntry *);
void __cdecl assign_allocated_text_segment(CompilerNameEntry *);
void __cdecl process_pack_pragma(void);
void __cdecl prepare_pch_expression_input(const char *);
void __cdecl disable_multibyte_identifiers(void);
void __cdecl emit_control_record(int);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
void __cdecl compiler_warning(int, ...);
}

/* The nine real option rows at RVA60fb8, expressed by letter, selected value
 * and existing destination. The terminator is not an optimization option. */
struct PragmaOptimizationOption { char letter; int enabled; int *destination; };
static const PragmaOptimizationOption optimization_options[] = {
    {'a',1,&c1_data.options.assume_no_alias},
    {'g',1,&c1_data.options.global_optimize},
    {'p',1,&c1_data.options.precise_float},
    {'s',0,&c1_data.options.optimize_size},
    {'t',1,&c1_data.options.optimize_size},
    {'w',1,&c1_data.options.assume_no_global_alias},
    {'q',1,&c1_data.normalized_oq},
    {'v',1,&c1_data.options.option_ov},
    {'y',1,&c1_data.options.omit_frame_pointer},
    {0,0,NULL}
};

extern "C" CompilerParserToken *__cdecl dispatch_recorded_pragma(void)
{
    CompilerParserToken *token, *value;
    CompilerNameEntry *name, *class_name;
    CompilerIrSymbol **segment;
    const char *text;
    unsigned code, segment_kind, number, state;
    int signed_position, position;
    PRAGMA_ASSERT(c1_bss.debug_module->Slota4(), 0x410);
    token = PRAGMA_CONTEXT.ReadToken();
    code = token->kind;
    if (code != PRAGMA_PACK && code != PRAGMA_MESSAGE && code != PRAGMA_WARNING && code != PRAGMA_CHECK_STACK &&
        code != PRAGMA_INLINE_DEPTH && code != PRAGMA_INLINE_RECURSION && code != PRAGMA_AUTO_INLINE &&
        code != PRAGMA_OPTIMIZE && code != PRAGMA_INTRINSIC && code != PRAGMA_FUNCTION &&
        code != PRAGMA_COMMENT && code != PRAGMA_ALLOC_TEXT && code != PRAGMA_CONST_SEG && code != PRAGMA_BSS_SEG &&
        code != PRAGMA_DATA_SEG && code != PRAGMA_CODE_SEG && code != 45 &&
        (code != 44 || !token->value.context.object)) {
#ifdef __linux__
        recon_unsupported("pragma dispatcher: unsupported source marker or missing owner");
#else
        __asm { int 3 }
#endif
    }
    switch (code) {
    case PRAGMA_INIT_SEG: case PRAGMA_VTORDISP: case PRAGMA_POINTERS_TO_MEMBERS:
        report_language_diagnostic(1, 157); skip_to_directive_newline(token); break;
    case PRAGMA_PACK:
        PRAGMA_ASSERT(c1_bss.debug_module->Slot20(PRAGMA_PACK), 0x4fa);
        process_pack_pragma();
        break;
    case PRAGMA_ONCE: {
        CompilerSourceFile *file = (CompilerSourceFile *)c1_bss.current_context.object;
        file->record_id = (file->record_id & 0x00ffffff) | 0x01000000;
        break;
    }
    case 43: /* Recorded source-line/file marker. */
        PRAGMA_ASSERT(c1_bss.debug_module->Slot24(43), 0x433);
        token = PRAGMA_CONTEXT.ReadToken();
        c1_bss.current_context.value = (unsigned)token->value.typed_constant.value;
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 7) {
            PRAGMA_ASSERT(c1_bss.debug_module->Slot20(43), 0x440);
            c1_bss.current_context.object = c1_data.translation_unit->FindSourceFile(
                (const char *)token->value.pointer, 0);
            if (c1_bss.parameter_nesting) emit_control_record(2);
            if (c1_bss.browser_filename) {
                c1_data.backend->Slot10();
                c1_data.backend->Slot18();
                emit_control_record(1);
                break;
            }
        } else PRAGMA_UNREAD(token);
        emit_control_record(1);
        break;
    case 44: { /* Recorded source owner, followed by the initial line. */
        CompilerSourceFile *file = (CompilerSourceFile *)token->value.context.object;
        c1_bss.current_context.object = file ? file :
            c1_data.translation_unit->FindSourceFile(current_forced_include, 0);
        c1_bss.current_context.value = 1;
        emit_control_record(2);
        emit_control_record(1);
        if (c1_bss.browser_filename) c1_data.backend->Slot18();
        break;
    }
    case 45:
        signed_position = (int)token->value.pointer;
        position = signed_position < 0 ? -signed_position : signed_position;
        if (signed_position < 0)
            report_language_diagnostic(1, 0x67, c1_bss.current_context.FileName());
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 0xf0) {
            c1_bss.current_context.value = token->value.context.value;
            c1_bss.current_context.object = token->value.context.object;
            if ((c1_data.lexer_flags & 0x20000) && !(c1_data.lexer_flags & 0x20))
                emit_control_record(0x24);
            token = PRAGMA_CONTEXT.ReadToken();
        }
        PRAGMA_UNREAD(token);
        emit_control_record(2); emit_control_record(1);
        if (position > 0 && c1_bss.browser_filename) c1_data.backend->Slot14(position);
        if (position == 1 && c1_data.pch_creation_name_seen)
            PRAGMA_UNSUPPORTED("pragma: PCH source-return state");
        if (!c1_data.source_include_depth && c1_bss.forced_source_index == 1)
            PRAGMA_UNSUPPORTED("pragma: PCH header-status state");
        if (position == 1 && c1_data.pch_creation_name_seen && !function_pch_replayed) {
            prepare_pch_expression_input(c1_data.options.pch_create_file);
            return (CompilerParserToken *)c1_data.predefined_tokens[3];
        }
        if (!c1_data.source_include_depth && c1_bss.forced_source_index == 1 &&
            !pragma_state.pch_header_status)
            return (CompilerParserToken *)c1_data.predefined_tokens[3];
        break;
    case PRAGMA_LOOP_OPT: case PRAGMA_CHECK_POINTER: case PRAGMA_SWITCH_CHECK:
    case PRAGMA_PLMN: case PRAGMA_PLMF: case PRAGMA_PAGE: case PRAGMA_SKIP:
    case PRAGMA_SUBTITLE: case PRAGMA_TITLE: case PRAGMA_PAGESIZE:
    case PRAGMA_LINESIZE: case PRAGMA_NATIVE_CALLER:
        report_language_diagnostic(4, 0x44);
        skip_to_directive_newline(token);
        break;
    case PRAGMA_STRING_POOLING:
        if (c1_bss.parameter_nesting) { compiler_warning(156); break; }
        value = read_optional_pragma_value(1);
        c1_data.normalized_pool_strings = value ? (int)value->value.pointer : c1_data.options.pool_strings;
        /* Actual nonNULL selection clears this cell; NULL preserves its input. */
        c1_data.normalized_readonly_strings = value ? 0 : c1_data.options.readonly_strings;
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
        pragma_state.allocated_text_segment = find_or_create_segment(name, c1_data.builtin_names.unnamed, 0, 2);
        if (!expect_directive_token(55)) break;
        c1_data.lexer_flags |= 0x10000;
        read_pragma_identifier_list(assign_allocated_text_segment);
        expect_directive_token(126);
        break;
    case PRAGMA_SAME_SEG:
        report_language_diagnostic(1, 0x8f);
        skip_to_directive_newline(PRAGMA_CONTEXT.ReadToken());
        break;
    case PRAGMA_CHECK_STACK:
        PRAGMA_ASSERT(c1_bss.debug_module->Slot20(0),0x567);
        token = PRAGMA_CONTEXT.ReadToken();
        switch (token->kind) {
        case 0x51: c1_data.normalized_stack_checks = 1; break;
        case 0x46: c1_data.normalized_stack_checks = 0; break;
        case 45:
            PRAGMA_UNREAD(token);
            c1_data.normalized_stack_checks = c1_data.options.stack_checks;
            break;
        case 118:
            PRAGMA_UNREAD(token); value = read_optional_pragma_value(1);
            c1_data.normalized_stack_checks = value ? (int)value->value.pointer : c1_data.options.stack_checks;
            break;
        default: report_language_diagnostic(1, 0x4d); skip_to_directive_newline(token); break;
        }
        break;
    case PRAGMA_SETLOCALE:
        PRAGMA_ASSERT(c1_bss.debug_module->Slot20(32),0x585);
        value = read_required_pragma_value(7);
        if (!value) break;
        if (!setlocale(LC_ALL, (const char *)value->value.pointer)) compiler_warning(175, value->value.pointer);
        disable_multibyte_identifiers(); initialize_multibyte_scanner();
        break;
    case PRAGMA_MESSAGE:
        PRAGMA_ASSERT(c1_bss.debug_module->Slot24(18),0x595);
        if (!expect_directive_token(118)) break;
        value = read_pragma_string();
        if (value && expect_directive_token(126)) {
            print_pragma_message((const char *)value->value.pointer);
            print_pragma_message("\n");
        }
        break;
    case PRAGMA_COMMENT:
        PRAGMA_ASSERT(c1_bss.debug_module->Slot24(19),0x5b4);
        if (!expect_directive_token(118)) break;
        token = expect_directive_token(6);
        if (!token) break;
        text = ((CompilerNameEntry *)token->value.pointer)->name;
        control_option_value = NULL;
        if (!strcmp(text,"compiler")) {
            control_option_kind = 1;
            control_option_value = (const BYTE *)"";
        } else if (!strcmp(text,"lib")) control_option_kind = 4;
        else if (!strcmp(text,"exestr")) control_option_kind = 3;
        else if (!strcmp(text,"user")) control_option_kind = 6;
        else if (!strcmp(text,"nolib")) control_option_kind = 9;
        else if (!strcmp(text,"linker")) control_option_kind = 10;
        else goto unknown_pragma_name;
        value = PRAGMA_CONTEXT.ReadToken();
        if (!control_option_value && value->kind == 55) {
            value = read_pragma_string();
            if (!value) break;
            control_option_value = (const BYTE *)value->value.pointer;
            value = PRAGMA_CONTEXT.ReadToken();
        }
        PRAGMA_UNREAD(value);
        if (expect_directive_token(126) && control_option_value)
            emit_control_record(11);
        break;
    case 47: /* Prepared linker-library comment, without a keyword prefix. */
        control_option_kind = 3;
        value = read_pragma_string();
        if (value) {
            control_option_value = (const BYTE *)value->value.pointer;
            if (control_option_value) emit_control_record(11);
        }
        break;
    case PRAGMA_DATA_SEG: case PRAGMA_BSS_SEG: case PRAGMA_CONST_SEG: case PRAGMA_CODE_SEG:
        if (code == PRAGMA_CODE_SEG) { segment = &(*(CompilerIrSymbol **)&c1_data.function_context); segment_kind = 0; }
        else {
            if (c1_bss.parameter_nesting) {
                const char *directive = code == PRAGMA_DATA_SEG ? "data_seg" :
                    code == PRAGMA_BSS_SEG ? "bss_seg" : "const_seg";
                if (c1_data.options.incremental_compile) compiler_warning(307,directive);
                else report_language_diagnostic(1,177,directive);
            }
            if (code == PRAGMA_DATA_SEG) { segment = &declaration_reference_defaults.definition; segment_kind = 1; }
            else if (code == PRAGMA_BSS_SEG) { segment = &declaration_reference_defaults.fallback; segment_kind = 3; }
            else { segment = &declaration_reference_defaults.type_context; segment_kind = 4; }
        }
        c1_bss.debug_module->Slot20(code);
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
            class_name = c1_data.builtin_names.unnamed;
            PRAGMA_UNREAD(value);
        } else {
            PRAGMA_UNREAD(value); expect_directive_token(55); break;
        }
        *segment = find_or_create_segment(name,class_name,segment_kind,2);
        expect_directive_token(126);
        break;
    case PRAGMA_SEGMENT:
        c1_bss.debug_module->Slot20(20);
        if (!expect_directive_token(118)) break;
        token = expect_directive_token(6);
        if (!token) break;
        name = (CompilerNameEntry *)token->value.pointer;
        if (!strcmp(name->name,"_TEXT")) { segment=&(*(CompilerIrSymbol **)&c1_data.function_context); segment_kind=0; }
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
            c1_data.builtin_names.unnamed,segment_kind,2);
        expect_directive_token(126);
        break;
    case PRAGMA_OPTIMIZE:
        if (c1_bss.parameter_nesting) { compiler_warning(156); break; }
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
                flags = (flags & ~c1_data.pragma_restore_options) |
                    (c1_bss.initial_option_flags & c1_data.pragma_restore_options);
            else flags &= ~c1_data.pragma_disable_options;
            apply_option_flags(flags);
        }
        break;
    case PRAGMA_AUTO_INLINE:
        if (c1_bss.parameter_nesting) { compiler_warning(156); break; }
        value = read_optional_pragma_value(1);
        c1_data.ir_symbol_state_ready = c1_data.function_reference_epoch = value ? (int)value->value.pointer : 1;
        break;
    case PRAGMA_WARNING:
        PRAGMA_ASSERT(c1_bss.debug_module->Slot20(28),0x7bd);
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
            state=(unsigned)token->value.typed_constant.value;
            if (state<1 || state>4) {
                report_language_diagnostic(1,0x268,state,1,4);
                skip_to_directive_newline(token); break;
            }
        } else if (token->kind == 103) state=0;
        else goto invalid_warning_option;
        if (!expect_directive_token(54)) break;
        value=PRAGMA_CONTEXT.ReadToken();
        while (value->kind == 101) {
            number=(unsigned)value->value.typed_constant.value;
            set_warning_state(number,state);
            value=PRAGMA_CONTEXT.ReadToken();
        }
        if (value->kind == 127) goto next_warning_option;
        if (value->kind != 126) { PRAGMA_UNREAD(value); expect_directive_token(126); }
        break;
    case PRAGMA_HDRSTOP:
        c1_data.lexer_flags |= 0x8000;
        mark_precompiled_header_stop(&c1_pch_stop_owner);
        c1_data.lexer_flags &= ~0x8000U;
        return (CompilerParserToken *)c1_data.predefined_tokens[1];
    case PRAGMA_INLINE_RECURSION:
        PRAGMA_ASSERT(c1_bss.debug_module->Slot20(26),0x81f);
        value=read_optional_pragma_value(1);
        control_state_second=value ? (WORD)(unsigned)value->value.pointer : 0;
        if ((unsigned)c1_data.options.inline_level>0) emit_control_record(32);
        break;
    case PRAGMA_INLINE_DEPTH:
        PRAGMA_ASSERT(c1_bss.debug_module->Slot20(25),0x82c);
        value=read_optional_pragma_value(101);
        control_state_first=value ? (WORD)value->value.typed_constant.value : 8;
        if ((unsigned)c1_data.options.inline_level>0) emit_control_record(32);
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

/* Genuine pending directive children, one body per original identity. */
extern "C" CompilerParserToken *__cdecl expect_directive_token(BYTE kind)
{
    CompilerParserToken *token = c1_bss.current_context.ReadToken();
    if (token->kind == kind) return token;
    if (token->kind == 6)
        report_language_diagnostic(1, 83, kind, token->value.name);
    else report_language_diagnostic(1, 81, kind, token->kind);
    skip_to_directive_newline(token);
    return NULL;
}
extern "C" void __cdecl skip_to_directive_newline(CompilerParserToken *) { __asm { int 3 } }
extern "C" void __cdecl set_warning_state(unsigned number, unsigned state)
{
    unsigned index = number > 4000 ? number - 4000 : number;
    if (index < 1 || index > 999) {
        report_language_diagnostic(1, 616, number, 4001, 4999);
        return;
    }
    c1_data.options.warning_levels[index] = (BYTE)state;
    if (index >= 700) {
        if (c1_data.options.lowest_warning >= index)
            c1_data.options.lowest_warning = index;
        if (c1_data.options.highest_warning <= index)
            c1_data.options.highest_warning = index;
    }
}
extern "C" CompilerParserToken * __cdecl read_optional_pragma_value(unsigned kind)
{
    CompilerParserToken *token, *value;
    if (!expect_directive_token(118)) return NULL;
    token = PRAGMA_CONTEXT.ReadToken();
    if (token->kind == 126) return NULL;
    PRAGMA_UNREAD(token);
    if ((BYTE)kind == 1) value = read_pragma_switch();
    else if ((BYTE)kind == 7) value = read_pragma_string();
    else value = expect_directive_token((BYTE)kind);
    if (!value) return NULL;
    return expect_directive_token(126) ? value : NULL;
}
extern "C" CompilerParserToken * __cdecl read_required_pragma_value(unsigned) { __asm { int 3 } return NULL; }
extern "C" void __cdecl append_literal_text(CompilerStringValue *, const CompilerStringValue *);
extern "C" CompilerParserToken * __cdecl read_pragma_string(void)
{
    CompilerParserToken *token = expect_directive_token(7), *result;
    CompilerStringValue text;
    if (!token) return NULL;
    text = token->value.string_attributes;
    token = PRAGMA_CONTEXT.ReadToken();
    while (token->kind == 7) {
        append_literal_text(&text, &token->value.string_attributes);
        token = PRAGMA_CONTEXT.ReadToken();
    }
    PRAGMA_UNREAD(token);
    c1_bss.literal.string_attributes = text;
    if (c1_bss.symbol_arena_kind) {
        if (c1_bss.symbol_arena_kind >= 6)
            PRAGMA_UNSUPPORTED("pragma string token arena is outside the recovered owners");
        CompilerArena *arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
        result = (CompilerParserToken *)arena->cursor;
        arena->cursor += 32;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        CompilerPool *pool = &c1_bss.pools[6];
        if (pool->remaining < 32) pool->Refill(32);
        result = (CompilerParserToken *)pool->cursor;
        pool->cursor += 32;
        pool->remaining -= 32;
    }
    if (result) {
        result->kind = 7;
        /* Both representations retain this existing24-byte owner. Only its
         * string prefix is interpreted here; the copied tail stays opaque. */
        memcpy(&result->value, &c1_bss.literal, 24);
    }
    return result;
}
extern "C" CompilerParserToken * __cdecl read_pragma_switch(void)
{
    CompilerParserToken *token = expect_directive_token(6), *result;
    const char *name;
    if (!token) return NULL;
    name = token->value.name->name;
    if (!strcmp(name, "on")) {
        c1_bss.literal.integer = 1;
        if (c1_bss.symbol_arena_kind) {
            if (c1_bss.symbol_arena_kind >= 6)
                PRAGMA_UNSUPPORTED("pragma switch token arena outside recovered owners");
            result = (CompilerParserToken *)c1_bss.arenas[c1_bss.symbol_arena_kind].AllocateBytes(32);
        } else {
            CompilerPool *pool = &c1_bss.pools[6];
            if (pool->remaining < 32) pool->Refill(32);
            result = (CompilerParserToken *)pool->cursor;
            pool->cursor += 32;
            pool->remaining -= 32;
        }
    } else if (!strcmp(name, "off")) {
        c1_bss.literal.integer = 0;
        if (c1_bss.symbol_arena_kind) {
            if (c1_bss.symbol_arena_kind >= 6)
                PRAGMA_UNSUPPORTED("pragma switch token arena outside recovered owners");
            CompilerArena *arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
            result = (CompilerParserToken *)arena->cursor;
            arena->cursor += 32;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        } else {
            CompilerPool *pool = &c1_bss.pools[6];
            if (pool->remaining < 32) pool->Refill(32);
            result = (CompilerParserToken *)pool->cursor;
            pool->cursor += 32;
            pool->remaining -= 32;
        }
    } else {
        report_language_diagnostic(1, 85);
        return NULL;
    }
    if (result) {
        result->kind = 101;
        /* Retain the authentic opaque tail; only the first QWORD is written. */
        memcpy(&result->value, &c1_bss.literal, 24);
    }
    return result;
}
extern "C" CompilerIrSymbol *__cdecl create_segment_symbol(CompilerNameEntry *, CompilerNameEntry *, unsigned, unsigned);
extern "C" int __cdecl segment_has_suffix(const char *, const char *);
extern "C" CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
extern "C" void __cdecl compiler_fatal(int);
typedef char SegmentClassOffset[offsetof(CompilerIrSymbol, segment_class) == 32 ? 1 : -1];
typedef char SegmentKindOffset[offsetof(CompilerIrSymbol, segment_kind) == 36 ? 1 : -1];
extern "C" CompilerIrSymbol * __cdecl find_or_create_segment(CompilerNameEntry *name,
    CompilerNameEntry *class_name, unsigned requested_kind, unsigned)
{
    CompilerIrSymbol *segment = c1_data.translation_unit->FindAllocatedSegment(name);
    BYTE kind = (BYTE)requested_kind;
    if (segment) {
        BYTE previous = segment->segment_kind;
        if (kind != 8 && kind != previous &&
            ((kind == 0 || kind == 2) != (previous == 0 || previous == 2))) {
            if ((previous == 0 || previous == 2) && (segment->record_id & 0xffffff) != 5)
                compiler_warning(194, segment);
            else if (kind != 1) compiler_warning(195, segment);
        }
        if (class_name && class_name != c1_data.builtin_names.unnamed) {
            if (!segment->segment_class || segment->segment_class == c1_data.builtin_names.unnamed)
                segment->segment_class = class_name;
            if (class_name != segment->segment_class)
                report_language_diagnostic(1, 613, segment);
        }
        if ((segment->record_id & 0xffffff) == 5 && (c1_data.lexer_flags & 0x20000) &&
            (c1_bss.current_scope_symbol->function_flags & 1))
            segment = lookup_ir_symbol(c1_bss.current_scope_symbol);
        if (c1_data.function_context && (segment->record_id & 0xffffff) == 5)
            segment = (CompilerIrSymbol *)c1_data.function_context;
        if (declaration_reference_defaults.definition && (segment->record_id & 0xffffff) == 2)
            segment = declaration_reference_defaults.definition;
        if (segment->segment_kind == 2 && !kind) segment->segment_kind = 0;
        return segment;
    }
    if (kind == 8 && segment_has_suffix(name->name, "_TEXT"))
        requested_kind = (requested_kind & ~0xffU) | 2;
    if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
    return create_segment_symbol(name, class_name, c1_data.next_symbol_record_id++, requested_kind);
}
/* Genuine children: their bodies remain unavailable at this frontier. */
extern "C" CompilerIrSymbol *__cdecl create_segment_symbol(CompilerNameEntry *name,
    CompilerNameEntry *class_name, unsigned identifier, unsigned kind)
{
    if (c1_data.segment_symbol_cache_index != 9 ||
        c1_data.record_sizes.known.segment_symbol_bytes != 52)
        PRAGMA_UNSUPPORTED("segment symbol size or cache selector is outside the recovered cells");
    CompilerSymbolBinding *binding = c1_data.abstract_symbol_free;
    CompilerIrSymbol *symbol = NULL;
    if (binding) {
        c1_data.abstract_symbol_free = binding->next;
        symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
        binding->name = NULL;
        binding->next = NULL;
        memset(symbol, 0, c1_data.record_sizes.known.segment_symbol_bytes);
        binding->name = name;
        symbol->kind = 14;
    }
    if (!symbol) {
        BYTE *memory = c1_bss.arenas[0].AllocateBytes(
            (c1_data.record_sizes.known.segment_symbol_bytes + 3) & ~3U);
        symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(14, name, 0) : NULL;
    }
    symbol->record_id = (symbol->record_id & 0xff000000U) | (identifier & 0xffffffU);
    if (identifier < 16) symbol->reference_flags = (symbol->reference_flags & 7) | (identifier << 3);
    symbol->segment_kind = (BYTE)kind;
    symbol->value_type = c1_data.builtin_types.type_0802;
    symbol->segment_class = class_name;
    if (name && name != c1_data.builtin_names.unnamed) {
        CompilerSymbolBinding *head = c1_data.translation_unit->symbols;
        if (head) symbol->binding.next = head;
        c1_data.translation_unit->symbols = &symbol->binding;
    }
    return symbol;
}
extern "C" int __cdecl segment_has_suffix(const char *, const char *)
{ __asm { int 3 } return 0; }
extern "C" void __cdecl enable_minimal_rebuild_component(void) { __asm { int 3 } }
extern "C" void __cdecl enable_minimum_type_info_component(void) { __asm { int 3 } }
extern "C" void __cdecl enable_browser_component(void) { __asm { int 3 } }
extern "C" void __cdecl mark_precompiled_header_stop(void **) { __asm { int 3 } }
extern "C" void __cdecl apply_option_flags(unsigned flags)
{
    unsigned index;
    for (index = 0; index != 21; ++index, flags <<= 1)
        if (c1_option_flag_destinations[index])
            *c1_option_flag_destinations[index] = (int)flags < 0;
}
extern "C" void __cdecl print_pragma_message(const char *text)
{
    fwrite(text, strlen(text), 1, stderr);
}
extern "C" void __cdecl read_pragma_identifier_list(void (__cdecl *apply)(CompilerNameEntry *))
{
    CompilerParserToken *token;
    CompilerNameEntry *name;
    for (;;) {
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 6) name = token->value.name;
        else {
            PRAGMA_UNREAD(token);
            if (token->kind != 7) return;
            token = read_pragma_string();
            name = find_or_create_identifier((const char *)token->value.pointer);
        }
        if (apply) apply(name);
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind != 55) {
            PRAGMA_UNREAD(token);
            return;
        }
    }
}
extern "C" int __cdecl find_intrinsic_number(CompilerNameEntry *);
extern "C" void __cdecl reset_function_intrinsic(CompilerIrSymbol *);
extern "C" int __cdecl find_intrinsic_number(CompilerNameEntry *identifier)
{
    /* Encountered 'a' bucket: the actual bounds select relations30..34.
     * Suffixes retain the original lookup order and intrinsic numbers. */
    static const char *suffixes[] = { "tan2", "tan", "sin", "cos", "bs" };
    const char *name = identifier->name;
    unsigned i;
    if ((signed char)*name < '_') return -1;
    if (*name != 'a') PRAGMA_UNSUPPORTED("intrinsic classifier bucket not recovered");
    for (i = 0; i != sizeof(suffixes) / sizeof(suffixes[0]); ++i)
        if (!strcmp(name + 1, suffixes[i])) return 30 + i;
    return -1;
}
extern "C" void __cdecl reset_function_intrinsic(CompilerIrSymbol *symbol)
{
    symbol->kind = 7;
    symbol->intrinsic_flags = 0x21;
}
extern "C" void __cdecl mark_function_nonintrinsic(CompilerNameEntry *name)
{
    CompilerIrSymbol **found, *symbol;
    if (find_intrinsic_number(name) == -1) {
        report_language_diagnostic(1, 163, name);
        return;
    }
    found = c1_data.translation_unit->FindName(name);
    if (!found) {
        report_language_diagnostic(1, 164, name, name);
        return;
    }
    symbol = *found;
    if (symbol->kind != 7 && symbol->kind != 8 && symbol->kind != 6) {
        compiler_warning(170, name);
        return;
    }
    if (symbol->intrinsic_traits & 0x80) {
        report_language_diagnostic(1, 167, name, name);
        return;
    }
    reset_function_intrinsic(symbol);
}
extern "C" void __cdecl initialize_intrinsic_symbol(CompilerIrSymbol *, unsigned);
extern "C" void __cdecl mark_function_intrinsic(CompilerNameEntry *name)
{
    int number = find_intrinsic_number(name);
    CompilerIrSymbol **found, *symbol;
    if (number == -1) {
        report_language_diagnostic(1, 163, name);
        return;
    }
    found = c1_data.translation_unit->FindName(name);
    symbol = found ? *found : NULL;
    if (!symbol) {
        report_language_diagnostic(1, 164, name, name);
        return;
    }
    if (symbol->kind != 7 && symbol->kind != 8 && symbol->kind != 6) {
        compiler_warning(170, name);
        return;
    }
    initialize_intrinsic_symbol(symbol, number);
}
extern "C" void __cdecl apply_function_context(CompilerIrSymbol *, CompilerIrSymbol *);
extern "C" void __cdecl assign_allocated_text_segment(CompilerNameEntry *name)
{
    CompilerIrSymbol **found = (CompilerIrSymbol **)c1_data.translation_unit->root->Find(name);
    CompilerIrSymbol *symbol = found ? *found : NULL;
    if (!symbol) { compiler_warning(157, name); return; }
    if (symbol->function_flags & 1) {
        if (lookup_ir_symbol(symbol) != pragma_state.allocated_text_segment)
            compiler_warning(193, symbol);
        return;
    }
    if (c1_data.lexer_flags & 0x10000) {
        if (symbol->kind != 7 && symbol->kind != 8) {
            compiler_warning(63, symbol);
            goto apply;
        }
        if (!(symbol->flags & 1)) goto apply;
        compiler_warning(200, symbol);
        apply_function_context(symbol, pragma_state.allocated_text_segment);
        return;
    }
    if (symbol->kind != 10) {
        symbol->scope_bits |= 1;
        compiler_warning(158, symbol);
        return;
    }
    if ((symbol->storage_class & 7) == 1) goto apply;
    symbol->scope_bits |= 1;
    compiler_warning(178, symbol);
    return;
apply:
    apply_function_context(symbol, pragma_state.allocated_text_segment);
}
extern "C" unsigned __cdecl normalize_packing_alignment(unsigned);
extern "C" void __cdecl pop_packing(unsigned, const void *);
extern "C" void __cdecl process_pack_pragma(void)
{
    CompilerParserToken *token;
    const char *name;
    const void *tag = NULL;
    unsigned alignment = 0;
    int operation = 0;
    if (!expect_directive_token(118)) return;
    token = PRAGMA_CONTEXT.ReadToken();
    if (token->kind == 6) {
        name = token->value.name->name;
        if (strcmp(name, "push") == 0) operation = 1;
        else if (strcmp(name, "pop") == 0) operation = 2;
        else { report_language_diagnostic(1, 79, 6); return; }
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 126) goto apply;
        PRAGMA_UNREAD(token);
        if (!expect_directive_token(55)) return;
        token = PRAGMA_CONTEXT.ReadToken();
        if (token->kind == 6) {
            tag = token->value.name;
            token = PRAGMA_CONTEXT.ReadToken();
            if (token->kind == 126) goto apply;
            PRAGMA_UNREAD(token);
            if (!expect_directive_token(55)) return;
            token = expect_directive_token(101);
            if (!token) return;
        }
    } else if (token->kind == 126) {
        alignment = c1_data.options.packing_alignment;
        if (alignment == (unsigned)-1) alignment = 8;
        goto apply;
    } else if (token->kind != 101) {
        PRAGMA_UNREAD(token);
        expect_directive_token(126);
        return;
    }
    if (token->kind == 101) {
        alignment = normalize_packing_alignment((unsigned)token->value.typed_constant.value);
        if (!expect_directive_token(126)) return;
    }
apply:
    if (operation == 1) push_packing(alignment, tag);
    else if (operation == 2) pop_packing(alignment, tag);
    else {
        CompilerPackingFrame *frame = c1_data.options.packing_stack->active;
        frame->alignment = normalize_packing_alignment(alignment);
    }
}
extern "C" unsigned __cdecl normalize_packing_alignment(unsigned alignment)
{
    if (alignment == 1 || alignment == 2 || alignment == 4 ||
        alignment == 8 || alignment == 16) return alignment;
    report_language_diagnostic(1, 86);
    return c1_data.options.packing_stack->active->alignment;
}
extern "C" void __cdecl pop_packing(unsigned alignment, const void *tag)
{
    CompilerPacking *stack = c1_data.options.packing_stack;
    CompilerPackingFrame *frame = stack->active;
    if (!frame->previous || !frame->previous->previous) {
        report_language_diagnostic(3, 161);
        if (!alignment) {
            alignment = c1_data.options.packing_alignment;
            if (alignment == (unsigned)-1) alignment = 8;
        }
        goto apply;
    }
    stack->Pop();
    while (frame->tag != tag) {
        if (!tag) {
            report_language_diagnostic(3, 159, frame->tag);
            break;
        }
        if (!frame->previous) {
            report_language_diagnostic(3, 160, tag);
            if (!alignment) {
                alignment = c1_data.options.packing_alignment;
                if (alignment == (unsigned)-1) alignment = 8;
            }
            break;
        }
        frame = stack->active;
        stack->Pop();
    }
apply:
    if (alignment) frame->alignment = alignment;
}
