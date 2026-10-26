#include <new.h>
#include <string.h>
#include <mbctype.h>
#include <io.h>
#include "frontend.h"
#include "input.h"
#include "keywords.h"
#include "global-record.h"
#pragma intrinsic(memcpy, strlen)
#include <recon.h>

/* C1 reads imported _mbctype data at IAT5d094 directly, as scanner.c does;
 * the period /MD accessor macro would introduce a spurious call edge. */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(_DLL)
#undef _mbctype
extern "C" { _CRTIMP extern unsigned char _mbctype[]; }
#endif

/* Only the first conditional state is encountered. The genuine push bound
 * is32, but no other slot has a semantic/default contract. Both builds guard
 * before unsupported reads/writes, independently of that original bound. */
#define directive_conditional_states c1_bss.conditional_states
extern "C" {
unsigned char __cdecl classify_preprocessor_directive(const char *);
void __cdecl finish_directive_line(const char *);
int __cdecl scan_identifier(unsigned);
unsigned __cdecl process_special_character(void);
unsigned __cdecl resolve_special_character(unsigned);
unsigned __cdecl parse_directive_character(unsigned);
unsigned __cdecl read_unspliced_character(void);
int __cdecl skip_language_comment(void);
int __cdecl skip_block_comment(void);
int __cdecl skip_line_comment(void);
void __cdecl discard_input_character(void);
int __cdecl skip_conditional_group(int);
__int64 __cdecl read_condition_value(void);
unsigned __cdecl push_conditional_state(void);
void __cdecl begin_line_output(void);
void __cdecl prepare_forced_directive(void);
int __cdecl query_forced_directive(void);
int __cdecl transform_forced_condition(short);
void __cdecl read_macro_definition(int);
unsigned __cdecl read_macro_replacement(void);
void __cdecl parse_macro_parameters(void);
void __cdecl skip_macro_block_comment(void);
void __cdecl skip_preserved_macro_comment(void);
void __cdecl define_preprocessor_macro(const char *, unsigned, int);
const char *__cdecl parse_include_name(int, unsigned *);
CompilerParserToken *__cdecl open_include_file(const char *, unsigned);
int __cdecl open_forced_include(const char *, int);
CompilerParserToken *__cdecl make_source_state_token(void *);
CompilerParserToken *__cdecl read_line_directive(void);
CompilerParserToken *__cdecl read_pragma_directive(void);
void __cdecl clear_macro_definition(void);
void __cdecl compiler_warning(int, ...);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
void __cdecl compiler_fatal(int);
unsigned char __cdecl read_multibyte_character(void);
extern char c1_numeric_token_text[128];
}

/* These two diagnostics are compared by literal identity at4c9a/2878b,
 * as well as passed from the corresponding directive branches. */
static const char directive_else[] = "#else";
static const char directive_endif[] = "#endif";

enum PreprocessorDirective {
    PP_IF, PP_ELIF, PP_ELSE, PP_ENDIF, PP_IFDEF, PP_IFNDEF,
    PP_DEFINE, PP_INCLUDE, PP_PRAGMA, PP_UNDEF, PP_LINE, PP_UNKNOWN,
    PP_ERROR, PP_IDENT, PP_IMPORT
};

extern "C" CompilerParserToken *__cdecl read_directive_token(void)
{
    const char *name = c1_data.current_identifier->name;
    unsigned character = *c1_bss.source_cursor++;
    unsigned directive, category, index, kind, saved_skip, arena_kind;
    int skipped;
    __int64 value;
    CompilerParserToken *token;
    CompilerRecordedLexer *recording;
    CompilerContextFrame *frame;
    CompilerArena *arena;
    CompilerPool *pool;
    BYTE *memory;
    char *text;
read_character:
    if (c1_data.scanner.flags[character] & 1) {
        scan_identifier(character);
        name = c1_data.current_identifier->name;
classify:
        c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
        directive = classify_preprocessor_directive(name);
        goto dispatch;
    }
    category = c1_data.scanner.classes[character];
    switch (category) {
    case SC_SPECIAL:
        if (!(process_special_character() & ~0x50U)) goto classify;
        break;
    case SC_SPACE:
        break;
    case SC_SLASH:
        if (c1_data.options.language_extensions || c1_data.options.preprocess_only) {
            if (read_unspliced_character() == '/') {
                c1_data.lexer_flags |= 0x800;
                skip_line_comment();
                goto comment_finished;
            }
            --c1_bss.source_cursor;
            if (read_unspliced_character() == '*') {
                c1_data.lexer_flags |= 0x800;
                skip_block_comment();
comment_finished:
                c1_data.lexer_flags &= ~0x800U;
            } else {
                --c1_bss.source_cursor;
                goto classify;
            }
        } else if (!skip_language_comment()) goto classify;
        break;
    case SC_NEWLINE:
        --c1_bss.source_cursor;
        return (CompilerParserToken *)c1_data.predefined_tokens[1];
    default:
        compiler_warning(19, character);
        discard_input_character();
        return (CompilerParserToken *)c1_data.predefined_tokens[1];
    }
    character = *c1_bss.source_cursor++;
    goto read_character;

dispatch:
    /* Their full control flow remains below, but conditional-expression state and
     * recorded #ident payload consumers have not yet been established. */
    if (directive < PP_ENDIF || directive == PP_IDENT) {
#ifdef __linux__
        recon_unsupported("conditional and recorded-ident directive state is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    switch (directive) {
    case PP_IFDEF:
    case PP_IFNDEF:
        if (!c1_data.source_include_depth && c1_bss.forced_source_index) {
            prepare_forced_directive();
            if (query_forced_directive() == 1) goto done;
            if (c1_bss.forced_source_index == 2) goto transform_defined;
        }
        do { character = *c1_bss.source_cursor++; }
        while (c1_data.scanner.classes[character] == SC_SPACE);
        if (!(c1_data.scanner.flags[character] & 0x20))
            character = parse_directive_character(character) & 0xff;
        if (!(c1_data.scanner.flags[character] & 1)) compiler_fatal(16);
        scan_identifier(character);
        value = c1_data.current_identifier->macro != NULL;
        finish_directive_line(directive == PP_IFDEF ? "#ifdef" : "#ifndef");
        if (directive == PP_IFNDEF) value = !value;
transform_defined:
        if (!c1_data.source_include_depth && c1_bss.forced_source_index)
            value = transform_forced_condition((short)value);
        if (value) goto push_defined;
        skipped = skip_conditional_group(5);
        if (skipped == 3) goto push_defined;
        if (skipped != 2) goto done;
reclassify_if:
        name = "if";
        c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
        directive = classify_preprocessor_directive("if");
        goto dispatch;
    case PP_IF:
        if (!c1_data.source_include_depth && c1_bss.forced_source_index) {
            prepare_forced_directive();
            if (query_forced_directive() == 1) goto done;
            if (c1_bss.forced_source_index == 2) goto transform_expression;
        }
        c1_data.lexer_flags |= 0x4000;
        saved_skip = c1_bss.lexical_skip_mode;
        c1_bss.lexical_skip_mode = 0;
        value = read_condition_value();
        c1_data.lexer_flags &= ~0x4000U;
        c1_bss.lexical_skip_mode = saved_skip;
        finish_directive_line("#if/#elif");
transform_expression:
        if (!c1_data.source_include_depth && c1_bss.forced_source_index)
            value = transform_forced_condition((short)value);
        if (value) goto push_expression;
        skipped = skip_conditional_group(5);
        if (skipped == 3) goto push_expression;
        if (!c1_data.options.preprocess_no_lines) begin_line_output();
        if (skipped == 2) goto reclassify_if;
        goto done;
    case PP_DEFINE:
        read_macro_definition(1);
        goto done;
    case PP_INCLUDE:
        name = parse_include_name(1, &kind);
        if (!name) goto done;
        return open_include_file(name, kind);
    case PP_ELIF:
        if (c1_data.conditional_depth-- < 0) compiler_fatal(18);
        if (!c1_bss.debug_module->Slot2c()) compiler_error(73, ".\\p0prepro.c", 727);
        while (skip_conditional_group(5) != 4) {}
        if (!c1_data.options.preprocess_no_lines) begin_line_output();
        goto done;
    case PP_ELSE:
        if (c1_data.conditional_depth < 0 ||
            directive_conditional_states[c1_data.conditional_depth--] != 1)
            compiler_fatal(19);
        if (!c1_bss.debug_module->Slot2c()) compiler_error(73, ".\\p0prepro.c", 746);
        finish_directive_line(directive_else);
        skip_conditional_group(6);
        if (!c1_data.options.preprocess_no_lines) begin_line_output();
        goto done;
    case PP_ENDIF:
        if (!c1_bss.debug_module->Slot2c()) compiler_error(73, ".\\p0prepro.c", 754);
        if (c1_data.conditional_depth-- < 0) compiler_fatal(20);
        if (!c1_data.options.preprocess_no_lines) begin_line_output();
        if (!c1_data.options.language_extensions) {
            finish_directive_line(directive_endif);
            goto done;
        }
        discard_input_character();
        goto done;
    case PP_PRAGMA:
        if (!c1_data.source_include_depth && c1_bss.forced_source_index) {
            prepare_forced_directive();
            if (query_forced_directive() == 1) goto done;
        }
        return read_pragma_directive();
    case PP_UNDEF:
        do { character = *c1_bss.source_cursor++; }
        while (c1_data.scanner.classes[character] == SC_SPACE);
        if (!(c1_data.scanner.flags[character] & 0x20))
            character = parse_directive_character(character) & 0xff;
        if (!(c1_data.scanner.flags[character] & 1)) {
            report_language_diagnostic(1, 6);
            finish_directive_line("#undef");
            goto done;
        }
        scan_identifier(character);
        clear_macro_definition();
        finish_directive_line("#undef");
        goto done;
    case PP_LINE:
        if (!c1_data.source_include_depth && c1_bss.forced_source_index)
            return (CompilerParserToken *)c1_data.predefined_tokens[3];
        return read_line_directive();
    case PP_ERROR:
        text = c1_numeric_token_text;
        for (;;) {
            character = *c1_bss.source_cursor++;
            category = c1_data.scanner.classes[character];
            if (category == SC_NEWLINE) break;
            if (category == SC_SPECIAL) {
                if (process_special_character() & ~0x50U) continue;
            } else if (category == SC_LEAD) {
                *text++ = (char)character;
                read_multibyte_character();
            }
            *text++ = (char)character;
        }
        --c1_bss.source_cursor;
        *text = 0;
        compiler_error(189, c1_numeric_token_text);
        finish_directive_line("#error");
        goto done;
    case PP_IDENT:
        /* A temporary recorded stream consumes through the explicit newline
         * token. The original transfer retains all24 payload storage bytes. */
        arena_kind = c1_bss.symbol_arena_kind;
        saved_skip = c1_bss.lexical_skip_mode;
        c1_bss.lexical_skip_mode = 0;
        memory = c1_bss.arenas[arena_kind].AllocateBytes(64);
        recording = memory ? new(memory) CompilerRecordedLexer(
            c1_bss.current_context.object, c1_bss.current_context.value, arena_kind) : NULL;
        c1_bss.context.Push((CompilerLexer *)recording, arena_kind, 0, 0, &c1_bss.current_context);
        if (!arena_kind) {
            pool = &c1_bss.pools[6];
            if (pool->remaining < 32) pool->Refill(32);
            memory = pool->cursor; pool->cursor += 32; pool->remaining -= 32;
        } else {
            arena = &c1_bss.arenas[arena_kind];
            memory = arena->cursor; arena->cursor += 32;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        }
        token = (CompilerParserToken *)memory;
        if (token) { token->kind = 47; memcpy(&token->value, &c1_bss.literal, 24); }
        c1_bss.context.saved_frame->stream->Trace((unsigned)token);
        do {
            token = c1_bss.current_context.ReadToken();
            c1_bss.context.saved_frame->stream->Trace((unsigned)token);
        } while (token->kind != 45);
        frame = c1_bss.context.saved_frame;
        if (frame) {
            recording = (CompilerRecordedLexer *)frame->stream;
            c1_bss.context.saved_frame = frame->next;
            c1_bss.context.object = frame->object;
            c1_bss.context.value = frame->value;
            if (frame->second) ((C1Initializer)frame->second)();
        } else recording = NULL;
        c1_bss.literal.recorded_stream = recording;
        c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
        c1_bss.lexical_skip_mode = saved_skip;
        if (saved_skip) goto done;
        if (!arena_kind) {
            pool = &c1_bss.pools[6];
            if (pool->remaining < 32) pool->Refill(32);
            memory = pool->cursor; pool->cursor += 32; pool->remaining -= 32;
        } else {
            arena = &c1_bss.arenas[arena_kind];
            memory = arena->cursor; arena->cursor += 32;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        }
        token = (CompilerParserToken *)memory;
        if (token) { token->kind = 0xf2; memcpy(&token->value, &c1_bss.literal, 24); }
        return token;
    case PP_IMPORT:
        compiler_warning(773);
        discard_input_character();
        goto done;
    case PP_UNKNOWN:
        compiler_error(21, name);
        goto done;
    default:
        goto done;
    }
push_defined:
    if (!c1_bss.debug_module->Slot28()) compiler_error(73, ".\\p0prepro.c", 595);
    index = push_conditional_state();
    if (index != 0) {
#ifdef __linux__
        recon_unsupported("conditional stack slot outside the encountered first state");
#else
        __asm { int 3 }
#endif
    }
    directive_conditional_states[index] = value ? 3 : 1;
    goto done;
push_expression:
    if (!c1_bss.debug_module->Slot28()) compiler_error(73, ".\\p0prepro.c", 679);
    index = push_conditional_state();
    if (index != 0) {
#ifdef __linux__
        recon_unsupported("conditional stack slot outside the encountered first state");
#else
        __asm { int 3 }
#endif
    }
    directive_conditional_states[index] = value ? 3 : 1;
    if (!value && !c1_data.options.preprocess_no_lines) begin_line_output();
done:
    c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
    return (CompilerParserToken *)c1_data.predefined_tokens[1];
}

/* Original148-byte search plus the two seven-byte unknown exits. Entries
 * retain the original order within each initial-letter group. No string
 * address identity escapes this classifier. */
extern "C" unsigned char __cdecl classify_preprocessor_directive(const char *word)
{
    static const struct DirectiveName {
        const char *name;
        unsigned char directive;
    } names[] = {
        { "define", PP_DEFINE },
        { "error", PP_ERROR }, { "endif", PP_ENDIF },
        { "else", PP_ELSE }, { "elif", PP_ELIF },
        { "include", PP_INCLUDE }, { "import", PP_IMPORT },
        { "ifndef", PP_IFNDEF }, { "ifdef", PP_IFDEF },
        { "if", PP_IF }, { "ident", PP_IDENT },
        { "line", PP_LINE }, { "pragma", PP_PRAGMA }, { "undef", PP_UNDEF }
    };
    unsigned index;
    const char *left, *right;
    signed char initial = (signed char)*word;
    if (initial < '_') return PP_UNKNOWN;
    /* The original unbounded first-character selector can enter adjacent
     * data above'z'. That nonidentifier domain has not been reconstructed. */
    if (initial > 'z') {
#ifdef __linux__
        recon_unsupported("directive classifier first character outside modeled identifiers");
#else
        __asm { int 3 }
#endif
    }
    for (index = 0; index < sizeof(names) / sizeof(names[0]); ++index) {
        if (names[index].name[0] != *word) continue;
        left = word + 1;
        right = names[index].name + 1;
        while (*left == *right) {
            if (!*left) return names[index].directive;
            ++left; ++right;
        }
    }
    return PP_UNKNOWN;
}

extern "C" void __cdecl finish_directive_line(const char *directive)
{
    unsigned character = *c1_bss.source_cursor++;
    if (!(c1_data.scanner.flags[character] & 0x20)) {
        while (c1_data.scanner.classes[character] == SC_SPACE)
            character = *c1_bss.source_cursor++;
        if (!(c1_data.scanner.flags[character] & 0x20))
            character = (BYTE)parse_directive_character(character);
    }
    if ((BYTE)character == '\n') {
        --c1_bss.source_cursor;
        return;
    }
    if (!c1_data.options.language_extensions ||
        (directive != directive_else && directive != directive_endif))
        report_language_diagnostic(1, 67, directive);
    discard_input_character();
}

extern "C" unsigned __cdecl parse_directive_character(unsigned)
{ __asm { int 3 } return 0; }

extern "C" int __cdecl skip_conditional_group(int)
{ __asm { int 3 } return 0; }

extern "C" __int64 __cdecl read_condition_value(void)
{ __asm { int 3 } return 0; }

extern "C" unsigned __cdecl push_conditional_state(void)
{
    c1_data.conditional_depth = (int)((unsigned)c1_data.conditional_depth + 1);
    if ((unsigned)c1_data.conditional_depth >= 32) compiler_fatal(52);
    return c1_data.conditional_depth;
}

extern "C" void __cdecl prepare_forced_directive(void)
{ __asm { int 3 } }

extern "C" int __cdecl query_forced_directive(void)
{ __asm { int 3 } return 0; }

extern "C" int __cdecl transform_forced_condition(short)
{ __asm { int 3 } return 0; }

/* Object-like and function-like definitions share the owned macro text
 * buffer. The parameter parser supplies the signed argument-count selector. */
extern "C" void __cdecl read_macro_definition(int notify_debug)
{
    unsigned character, next, category, length;
    int comment;
    if (notify_debug) c1_bss.debug_module->Slot1c();
    character = *c1_bss.source_cursor++;
    if (!(c1_data.scanner.flags[character] & 0x20)) {
        while (c1_data.scanner.classes[character] == SC_SPACE)
            character = *c1_bss.source_cursor++;
        if (!(c1_data.scanner.flags[character] & 0x20))
            character = (BYTE)parse_directive_character(character);
    }
    if (!(c1_data.scanner.flags[character] & 1)) {
        compiler_warning(7);
        discard_input_character();
        return;
    }
    scan_identifier(character);
    c1_bss.current_macro_text = (const char *)c1_bss.work_buffers[1].data;
    c1_bss.macro_parameter_count = 0;
read_character:
    character = *c1_bss.source_cursor++;
    category = c1_data.scanner.classes[character];
    switch (category) {
    case SC_SPECIAL:
        if (process_special_character() & ~0x50U) goto read_character;
        break;
    case SC_SPACE:
        if (character == '\r') goto read_character;
        goto replacement;
    case SC_SLASH:
        next = *c1_bss.source_cursor++;
        if (c1_data.scanner.classes[next] == SC_SPECIAL)
            next = (BYTE)resolve_special_character(next);
        comment = 0;
        if (next == '/') {
            if (c1_data.options.language_extensions || c1_data.options.preprocess_only) {
                c1_data.lexer_flags |= 0x800;
                skip_line_comment();
                c1_data.lexer_flags &= ~0x800U;
                comment = 1;
            } else {
                next = *c1_bss.source_cursor++;
                if (c1_data.scanner.classes[next] == SC_SPECIAL)
                    next = (BYTE)resolve_special_character(next);
                if (next == '*') {
                    report_language_diagnostic(4, 179);
                    c1_data.lexer_flags |= 0x800;
                    if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
                        skip_preserved_macro_comment();
                    else skip_macro_block_comment();
                    c1_data.lexer_flags &= ~0x800U;
                } else {
                    --c1_bss.source_cursor;
                    report_language_diagnostic(4, 1);
                    c1_data.lexer_flags |= 0x800;
                    skip_line_comment();
                    c1_data.lexer_flags &= ~0x800U;
                    comment = 1;
                }
            }
        } else {
            --c1_bss.source_cursor;
            next = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[next] == SC_SPECIAL)
                next = (BYTE)resolve_special_character(next);
            if (next == '*') {
                c1_data.lexer_flags |= 0x800;
                if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
                    skip_preserved_macro_comment();
                else skip_macro_block_comment();
                c1_data.lexer_flags &= ~0x800U;
                comment = 1;
            } else --c1_bss.source_cursor;
        }
        if (comment) goto replacement;
        character = '/';
        break;
    case SC_LPAREN:
        parse_macro_parameters();
        if (!c1_bss.macro_parameter_count) c1_bss.macro_parameter_count = -1;
        goto replacement;
    case SC_NEWLINE:
        --c1_bss.source_cursor;
        define_preprocessor_macro(NULL, 0, 0);
        return;
    }
    compiler_warning(8, character);
replacement:
    length = read_macro_replacement();
    if (!c1_data.options.language_extensions) {
        length += (unsigned)(c1_bss.current_macro_text - (const char *)c1_bss.work_buffers[1].data);
        c1_bss.current_macro_text = (const char *)c1_bss.work_buffers[1].data;
    }
    define_preprocessor_macro(c1_bss.current_macro_text, length, c1_bss.macro_parameter_count);
}
extern "C" void __cdecl skip_macro_block_comment(void) { __asm { int 3 } }
extern "C" void __cdecl skip_preserved_macro_comment(void) { __asm { int 3 } }

/* The diagnostic spelling column is representation-only until a malformed
 * token actually requires it. No default or original256-cell extent follows
 * from the BYTE index; the guard precedes every unknown column read. */
extern "C" { const char *c1_unmodeled_token_spellings[256]; } /* Original column RVA638a0. */
extern "C" const char *__cdecl parse_angle_include(int);
extern "C" const char *__cdecl parse_include_name(int diagnose, unsigned *kind)
{
    unsigned saved_skip = c1_bss.lexical_skip_mode;
    CompilerParserToken *token;
    const char *name;
    c1_data.lexer_flags |= 0x2000;
    c1_bss.lexical_skip_mode = 0;
    token = c1_bss.current_context.ReadToken();
    c1_data.lexer_flags &= ~0x2000U;
    c1_bss.lexical_skip_mode = saved_skip;
    if (token->kind == 7) {
        if ((token->value.string_attributes.flags & 1) && diagnose)
            compiler_warning(2);
        if (kind) *kind = 0;
        return token->value.string_attributes.text;
    }
    if (token->kind == 68) {
        name = parse_angle_include(diagnose);
        if (name) {
            if (kind) *kind = 1;
            return name;
        }
    } else if (diagnose) {
#ifdef __linux__
        recon_unsupported("include diagnostic token spelling is not yet modeled");
#else
        __asm { int 3 }
#endif
        compiler_warning(6, c1_unmodeled_token_spellings[token->kind]);
    }
    discard_input_character();
    if (diagnose) c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
    return NULL;
}
extern "C" const char *__cdecl parse_angle_include(int) { __asm { int 3 } return NULL; }

extern "C" CompilerParserToken *__cdecl open_include_file(const char *name, unsigned kind)
{
    if (open_forced_include(name, kind) > -1)
        return make_source_state_token(c1_bss.current_context.object);
    return (CompilerParserToken *)c1_data.predefined_tokens[1];
}

extern "C" CompilerParserToken * __cdecl read_line_directive(void)
{ __asm { int 3 } return 0; }

extern "C" CompilerParserToken * __cdecl read_pragma_directive(void)
{ __asm { int 3 } return 0; }

extern "C" void __cdecl clear_macro_definition(void)
{ __asm { int 3 } }

extern "C" unsigned char __cdecl read_multibyte_character(void)
{ __asm { int 3 } return 0; }

extern "C" {
int __cdecl compare_pch_paths(const char *, const char *);
unsigned __cdecl classify_pch_include(const char *);
int __cdecl search_relative_include(const char *, int, char *);
int __cdecl search_include_directories(const char *, int, int);
int __cdecl open_source_file(const char *, unsigned, int, int, int);
void __cdecl report_source_file_error(const char *, int);
void __cdecl report_response_error(int, ...);
}

extern "C" int __cdecl open_forced_include(const char *name, int kind)
{
    const char *alias;
    const char *separators;
    int descriptor;
    unsigned pch_kind;
    if (!c1_data.source_include_depth) {
        if (c1_data.options.pch_create_file &&
            compare_pch_paths(c1_data.options.pch_create_file, name)) {
#ifdef __linux__
            recon_unsupported("include PCH-create marker is not yet modeled");
#else
            __asm { int 3 }
#endif
            c1_data.pch_creation_name_seen = 1;
        }
        if (c1_bss.forced_source_index) {
            pch_kind = classify_pch_include(name);
            if (pch_kind == 1) return -4;
            if (pch_kind == 2 && c1_bss.forced_source_index != 1) {
                finish_directive_line("#include");
                return -4;
            }
        }
    }
    finish_directive_line("#include");
    alias = c1_data.options.include_aliases->Find(name, &kind);
    if (alias) name = alias;
    separators = c1_data.options.path_separators;
    if (strchr(separators, (unsigned char)name[0]) ||
        (strchr(separators, ':') && !(_mbctype[(unsigned char)name[0] + 1] & _M1) &&
         name[1] == ':')) {
        descriptor = open_source_file(name, 0, 0, 1, 1);
        if (descriptor == -1) {
            report_response_error(83, 343, name);
            return descriptor;
        }
        if (descriptor > -1 && (c1_bss.forced_source_index == 1 || c1_bss.pch_file_failed))
            report_source_file_error(name, descriptor);
        return descriptor;
    }
    if (!kind) {
        descriptor = search_relative_include(name, 1, NULL);
        if (descriptor != -1) return descriptor;
    }
    return search_include_directories(name, 1, 0);
}
const char *RECON_THISCALL CompilerIncludeAliases::Find(const char *name, int *kind)
{
    CompilerIncludeAlias *alias;
    for (alias = head; alias; alias = alias->next) {
        if (_stricmp(alias->name, name) == 0 && alias->kind == *kind) {
            *kind = alias->replacement_kind;
            return alias->replacement;
        }
    }
    return NULL;
}
extern "C" unsigned __cdecl classify_pch_include(const char *)
{ __asm { int 3 } return 0; }
/* Search each active source directory, starting with the immediate includer.
 * The bound deliberately includes its entire filename, as in the original. */
extern "C" int __cdecl search_relative_include(const char *name, int open_file, char *resolved)
{
    char path[512];
    CompilerSourceFrame *frame = c1_data.source_frames;
    CompilerSourceFrame *next;
    const char *source, *cursor, *separator;
    char *out;
    unsigned hash;
    int descriptor;
    do {
        source = frame->file->name->name;
        next = frame->next;
        separator = NULL;
        for (cursor = source; *cursor; ++cursor) {
            if (_mbctype[(BYTE)*cursor + 1] & _M1) ++cursor;
            else if (strchr(c1_data.options.path_separators, (BYTE)*cursor))
                separator = cursor;
        }
        if (strlen(name) + strlen(source) + 2 >= sizeof(path)) compiler_fatal(64);
        hash = 0;
        out = path;
        while (separator && source <= separator) {
            *out = *source++;
            hash = (hash >> 4) + hash * 4 + (BYTE)*out++;
        }
        source = name;
        while ((*out = *source++) != 0) {
            hash = (hash >> 4) + hash * 4 + (BYTE)*out++;
        }
        if (open_file) {
            descriptor = open_source_file(path, hash, 0, 1, 1);
            if (descriptor != -1) {
                if (descriptor > -1 &&
                    (c1_bss.forced_source_index == 1 || c1_bss.pch_file_failed))
                    report_source_file_error(path, descriptor);
                if (resolved) memcpy(resolved, path, strlen(path) + 1);
                return descriptor;
            }
        } else if (_access(path, 0) == 0) {
            if (resolved) memcpy(resolved, path, strlen(path) + 1);
            return 0;
        }
        frame = next;
    } while (frame);
    if (resolved) *resolved = 0;
    return -1;
}
extern "C" int __cdecl search_include_directories(const char *, int, int)
{ __asm { int 3 } return 0; }
