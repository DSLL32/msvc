#include <new.h>
#include <string.h>
#include <mbctype.h>
#include <ctype.h>
#include <io.h>
#include "frontend.h"
#include "input.h"
#include "keywords.h"
#include "global-record.h"
#include "pragma.h"
#pragma intrinsic(memcpy, strlen, strcmp, strcpy)
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
    int skipped, saved_preprocess;
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
    /* The later else-state lookup and recorded #ident payload consumers
     * remain unsupported. Elif enters the genuine conditional skip owner. */
    if (directive == PP_ELSE || directive == PP_IDENT) {
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
        saved_preprocess = c1_bss.options.preprocess_stdout;
        c1_bss.options.preprocess_stdout = 0;
        value = read_condition_value();
        c1_data.lexer_flags &= ~0x4000U;
        c1_bss.options.preprocess_stdout = saved_preprocess;
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
            if (frame->second) {
#ifdef __linux__
                recon_unsupported("line directive return callback is not yet qualified");
#else
                __asm { int 3 }
#endif
                ((C1Initializer)frame->second)();
            }
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
    if (index > 1) {
#ifdef __linux__
        recon_unsupported("conditional stack slot outside encountered states0/1");
#else
        __asm { int 3 }
#endif
    }
    directive_conditional_states[index] = value ? 3 : 1;
    goto done;
push_expression:
    if (!c1_bss.debug_module->Slot28()) compiler_error(73, ".\\p0prepro.c", 679);
    index = push_conditional_state();
    if (index > 1) {
#ifdef __linux__
        recon_unsupported("conditional stack slot outside encountered states0/1");
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

extern "C" int __cdecl splice_input_line(void);
extern "C" unsigned __cdecl parse_directive_character(unsigned input)
{
    BYTE character = (BYTE)input, next;
    int consumed;
    for (;;) {
        if (character == '/') {
            consumed = 0;
            next = *c1_bss.source_cursor++;
            if (c1_data.scanner.classes[next] == SC_SPECIAL)
                next = (BYTE)resolve_special_character(next);
            if ((c1_data.options.language_extensions || c1_data.options.preprocess_only) &&
                next == '/') {
                c1_data.lexer_flags |= 0x800;
                skip_line_comment();
                c1_data.lexer_flags &= ~0x800U;
                consumed = 1;
            } else {
                /* In the strict-language path a second slash is diagnosed
                 * before consuming the remainder as a line comment. */
                if (!c1_data.options.language_extensions && !c1_data.options.preprocess_only &&
                    next == '/') {
                    next = *c1_bss.source_cursor++;
                    if (c1_data.scanner.classes[next] == SC_SPECIAL)
                        next = (BYTE)resolve_special_character(next);
                    if (next != '*') {
                        --c1_bss.source_cursor;
                        report_language_diagnostic(4, 1);
                        c1_data.lexer_flags |= 0x800;
                        skip_line_comment();
                        c1_data.lexer_flags &= ~0x800U;
                        consumed = 1;
                    } else {
                        report_language_diagnostic(4, 179);
                        c1_data.lexer_flags |= 0x800;
                        if (c1_bss.options.preserve_comments && !(c1_data.lexer_flags & 0x8000))
                            skip_preserved_macro_comment();
                        else skip_macro_block_comment();
                        c1_data.lexer_flags &= ~0x800U;
                        return '/';
                    }
                } else {
                    /* Rereading the peek is intentional: special-character
                     * resolution may have moved the cursor. */
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
                        consumed = 1;
                    } else --c1_bss.source_cursor;
                }
            }
            if (!consumed) return '/';
        } else if (character == '\\') {
            if (!splice_input_line()) return '\\';
        } else if (!character || character == 0x1a) {
            process_special_character();
        } else return character;

        do { character = *c1_bss.source_cursor++; }
        while (c1_data.scanner.classes[character] == SC_SPACE);
        if (c1_data.scanner.flags[character] & 0x20) return character;
    }
}

extern "C" BYTE __cdecl read_skipped_directive(void);
extern "C" int __cdecl skip_conditional_group(int mode)
{
    unsigned depth = 0;
    BYTE directive;
    for (;;) {
        directive = read_skipped_directive();
        if (directive == PP_IF || directive == PP_IFDEF || directive == PP_IFNDEF) {
            ++depth;
        } else if (directive == PP_ENDIF) {
            if (depth) --depth;
            else { finish_directive_line("#endif"); return 4; }
        } else if (directive == PP_ELIF || directive == PP_ELSE) {
            if (depth) continue;
            if (mode == 6) { compiler_fatal(22); continue; }
            if (directive == PP_ELIF) return 2;
            if (c1_data.options.language_extensions) discard_input_character();
            else finish_directive_line("#else");
            return 3;
        }
    }
}
extern "C" void __cdecl skip_directive_literal(BYTE);
extern "C" BYTE __cdecl read_skipped_directive(void)
{
    unsigned character;
    c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
scan:
    do { character = *c1_bss.source_cursor++; }
    while (!(c1_data.scanner.flags[character] & 0x80));
    if (!(c1_data.scanner.flags[character] & 0x20)) {
        while (c1_data.scanner.classes[character] == SC_SPACE)
            character = *c1_bss.source_cursor++;
        if (!(c1_data.scanner.flags[character] & 0x20))
            character = parse_directive_character(character) & 0xff;
    }
    if (c1_data.scanner.classes[character] == SC_NEWLINE) {
        ++c1_bss.current_context.value;
        if (c1_bss.options.preprocess_stdout) fputc('\n', c1_bss.preprocessor_output);
        character = *c1_bss.source_cursor++;
        while (c1_data.scanner.classes[character] == SC_SPACE)
            character = *c1_bss.source_cursor++;
        if (!(c1_data.scanner.flags[character] & 0x20))
            character = parse_directive_character(character) & 0xff;
        if (character != '#') { --c1_bss.source_cursor; goto scan; }
        character = *c1_bss.source_cursor++;
        while (c1_data.scanner.classes[character] == SC_SPACE)
            character = *c1_bss.source_cursor++;
        if (!(c1_data.scanner.flags[character] & 0x20))
            character = parse_directive_character(character) & 0xff;
        if (!(c1_data.scanner.flags[character] & 1)) {
            --c1_bss.source_cursor;
            goto scan;
        }
        scan_identifier(character);
        c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
        return classify_preprocessor_directive(c1_data.current_identifier->name);
    }
    if (c1_data.scanner.classes[character] == SC_SQUOTE ||
        c1_data.scanner.classes[character] == SC_DQUOTE)
        skip_directive_literal((BYTE)character);
    else if (c1_data.scanner.classes[character] == SC_LEAD)
        read_multibyte_character();
    goto scan;
}
extern "C" void __cdecl skip_directive_literal(BYTE quote)
{
    BYTE character;
    for (;;) {
        character = *c1_bss.source_cursor++;
        switch (c1_data.scanner.classes[character]) {
        case SC_NEWLINE:
            --c1_bss.source_cursor;
            return;
        case SC_SQUOTE: case SC_DQUOTE:
            if (character == quote) return;
            break;
        case SC_SPECIAL:
            if (process_special_character() & 0x10) ++c1_bss.source_cursor;
            break;
        case SC_LEAD:
            read_multibyte_character();
            break;
        }
    }
}

/* The conditional evaluator's arena5 values are distinct from ordinary
 * 24-byte expression nodes. Only their QWORD payload is consumed here. */
extern "C" CompilerConditionValue *__cdecl parse_condition_expression(void);
extern "C" __int64 __cdecl read_condition_value(void)
{
    c1_data.condition_parentheses = 0;
    c1_data.condition_token = 136;
    CompilerConditionValue *expression = parse_condition_expression();
    if (c1_data.condition_token == 126) {
        if (!c1_data.condition_parentheses--) compiler_error(12, 40);
    } else if (c1_data.condition_token != 45)
        report_language_diagnostic(1, 67, "#if/#elif");
    if (c1_data.condition_parentheses > 0) compiler_error(12, 41);
    __int64 value = expression->value;
    c1_bss.arenas[5].ClearUsed();
    return value;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_logical_or(void);
extern "C" int __cdecl match_condition_token(unsigned);
extern "C" CompilerConditionValue *__cdecl parse_condition_expression(void)
{
    CompilerConditionValue *condition = parse_condition_logical_or();
    if (!match_condition_token(86)) return condition;
    CompilerConditionValue *first = parse_condition_logical_or();
    CompilerConditionValue *second;
    if (match_condition_token(54)) second = parse_condition_logical_or();
    else {
        /* The malformed-colon path carries an unwritten incoming stack word
         * in the original. It has no supported semantic result here. */
#ifdef __linux__
        recon_unsupported("conditional expression without a colon has no recovered result");
#else
        __asm { int 3 }
#endif
    }
    return condition->value ? first : second;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_logical_and(void);
extern "C" CompilerConditionValue *__cdecl fold_condition_binary(unsigned,
    CompilerConditionValue *, CompilerConditionValue *);
extern "C" CompilerConditionValue *__cdecl parse_condition_logical_or(void)
{
    unsigned saved_skip = c1_bss.literal.condition.skipped;
    CompilerConditionValue *value = parse_condition_logical_and();
    while (match_condition_token(79)) {
        if (value->value) ++c1_bss.literal.condition.skipped;
        value = fold_condition_binary(79, value, parse_condition_logical_and());
    }
    c1_bss.literal.condition.skipped = saved_skip;
    return value;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_bitwise_or(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_logical_and(void)
{
    unsigned saved_skip = c1_bss.literal.condition.skipped;
    CompilerConditionValue *value = parse_condition_bitwise_or();
    while (match_condition_token(50)) {
        if (!value->value) ++c1_bss.literal.condition.skipped;
        value = fold_condition_binary(50, value, parse_condition_bitwise_or());
    }
    c1_bss.literal.condition.skipped = saved_skip;
    return value;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_bitwise_xor(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_bitwise_or(void)
{
    CompilerConditionValue *value = parse_condition_bitwise_xor();
    while (match_condition_token(77))
        value = fold_condition_binary(77, parse_condition_bitwise_xor(), value);
    return value;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_bitwise_and(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_bitwise_xor(void)
{
    CompilerConditionValue *value = parse_condition_bitwise_and();
    while (match_condition_token(91))
        value = fold_condition_binary(91, parse_condition_bitwise_and(), value);
    return value;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_equality(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_bitwise_and(void)
{
    CompilerConditionValue *value = parse_condition_equality();
    while (match_condition_token(49))
        value = fold_condition_binary(49, parse_condition_equality(), value);
    return value;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_relational(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_equality(void)
{
    CompilerConditionValue *value = parse_condition_relational();
    if (match_condition_token(59))
        return fold_condition_binary(59, parse_condition_relational(), value);
    if (match_condition_token(76))
        return fold_condition_binary(76, parse_condition_relational(), value);
    return value;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_shift(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_relational(void)
{
    CompilerConditionValue *value = parse_condition_shift();
    if (match_condition_token(68))
        return fold_condition_binary(68, value, parse_condition_shift());
    if (match_condition_token(62))
        return fold_condition_binary(62, value, parse_condition_shift());
    if (match_condition_token(69))
        return fold_condition_binary(69, value, parse_condition_shift());
    if (match_condition_token(63))
        return fold_condition_binary(63, value, parse_condition_shift());
    return value;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_additive(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_shift(void)
{
    CompilerConditionValue *value = parse_condition_additive();
    if (match_condition_token(88))
        return fold_condition_binary(88, value, parse_condition_additive());
    if (match_condition_token(67))
        return fold_condition_binary(67, value, parse_condition_additive());
    return value;
}
extern "C" CompilerConditionValue *__cdecl parse_condition_multiplicative(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_additive(void)
{
    CompilerConditionValue *value = parse_condition_multiplicative();
    for (;;) {
        if (match_condition_token(81))
            value = fold_condition_binary(81, value, parse_condition_multiplicative());
        else if (match_condition_token(70))
            value = fold_condition_binary(70, value, parse_condition_multiplicative());
        /* The original retries plus at this join before returning. */
        else if (match_condition_token(81))
            value = fold_condition_binary(81, value, parse_condition_multiplicative());
        else return value;
    }
}
extern "C" CompilerConditionValue *__cdecl parse_condition_unary(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_multiplicative(void)
{
    CompilerConditionValue *value = parse_condition_unary();
    for (;;) {
        if (match_condition_token(74))
            value = fold_condition_binary(74, value, parse_condition_unary());
        else if (match_condition_token(57))
            value = fold_condition_binary(57, value, parse_condition_unary());
        else if (match_condition_token(72))
            value = fold_condition_binary(72, value, parse_condition_unary());
        else return value;
    }
}
extern "C" CompilerConditionValue *__cdecl parse_condition_primary(void);
extern "C" CompilerConditionValue *__cdecl parse_condition_unary(void)
{
    if (match_condition_token(60))
        return fold_condition_binary(60, parse_condition_unary(), NULL);
    if (match_condition_token(89))
        return fold_condition_binary(89, parse_condition_unary(), NULL);
    if (match_condition_token(81))
        return fold_condition_binary(134, parse_condition_unary(), NULL);
    if (match_condition_token(70))
        return fold_condition_binary(90, parse_condition_unary(), NULL);
    return parse_condition_primary();
}
extern "C" CompilerConditionValue *__cdecl parse_condition_primary(void)
{
    if (match_condition_token(118)) {
        ++c1_data.condition_parentheses;
        CompilerConditionValue *value = parse_condition_expression();
        if (!match_condition_token(126)) compiler_error(12, 41);
        --c1_data.condition_parentheses;
        return value;
    }
    if (!match_condition_token(101)) compiler_fatal(17);
    CompilerArena *arena = &c1_bss.arenas[5];
    CompilerConditionValue *value = (CompilerConditionValue *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    /* Preserve the actual16-byte transfer, including the unmodeled six-byte
     * gap. Only flags and the QWORD value have semantic consumers here. */
    memcpy(value, &c1_bss.literal, 16);
    return value;
}
extern "C" const signed char c1_primitive_widths[16];
#define CONDITION_WIDTH(f) (((f) & 0x340) || !((f) & 0xfff0) ? \
    (int)c1_primitive_widths[(f) & 15] : (int)((f) & 15))
extern "C" CompilerConditionValue *__cdecl fold_condition_binary(unsigned operation,
    CompilerConditionValue *first, CompilerConditionValue *second)
{
    unsigned first_unsigned = first->flags & 0x4000;
    int unsigned_values = first_unsigned || (second && (second->flags & 0x4000));
    int width = CONDITION_WIDTH(first->flags);
    if (second && width < CONDITION_WIDTH(second->flags))
        width = CONDITION_WIDTH(second->flags);
    if (width == c1_primitive_widths[1]) unsigned_values = 0;
    unsigned count;
    switch ((BYTE)operation) {
    case 79:
        first->value = first->value || second->value;
        unsigned_values = 0;
        break;
    case 50:
        first->value = first->value && second->value;
        unsigned_values = 0;
        break;
    case 49: first->value &= second->value; break;
    case 77: first->value |= second->value; break;
    case 91: first->value ^= second->value; break;
    case 59:
        first->value = first->value == second->value;
        unsigned_values = 0;
        break;
    case 76:
        first->value = first->value != second->value;
        unsigned_values = 0;
        break;
    case 68:
        first->value = unsigned_values ? (unsigned __int64)first->value < (unsigned __int64)second->value :
            first->value < second->value;
        unsigned_values = 0;
        break;
    case 62:
        first->value = unsigned_values ? (unsigned __int64)first->value > (unsigned __int64)second->value :
            first->value > second->value;
        unsigned_values = 0;
        break;
    case 69:
        first->value = unsigned_values ? (unsigned __int64)first->value <= (unsigned __int64)second->value :
            first->value <= second->value;
        unsigned_values = 0;
        break;
    case 63:
        first->value = unsigned_values ? (unsigned __int64)first->value >= (unsigned __int64)second->value :
            first->value >= second->value;
        unsigned_values = 0;
        break;
    case 67:
        unsigned_values = first_unsigned || (!c1_data.options.language_extensions &&
            c1_data.options.unsigned_char && (first->flags & 0x2fcf) == 0x41 &&
            !(first->flags & 0x8000));
        count = (BYTE)second->value;
        first->value = count >= 64 ? 0 : (unsigned __int64)first->value << count;
        break;
    case 88:
        unsigned_values = first_unsigned || (!c1_data.options.language_extensions &&
            c1_data.options.unsigned_char && (first->flags & 0x2fcf) == 0x41 &&
            !(first->flags & 0x8000));
        count = (BYTE)second->value;
        if (unsigned_values)
            first->value = count >= 64 ? 0 : (unsigned __int64)first->value >> count;
        else first->value = count >= 64 ? (first->value < 0 ? -1 : 0) : first->value >> count;
        break;
    case 81: first->value = (unsigned __int64)first->value + (unsigned __int64)second->value; break;
    case 70: first->value = (unsigned __int64)first->value - (unsigned __int64)second->value; break;
    case 74: first->value = (unsigned __int64)first->value * (unsigned __int64)second->value; break;
    case 57:
    case 72:
        if (!second->value) {
            if (!c1_bss.literal.condition.skipped) compiler_warning(124);
            break;
        }
        if (!unsigned_values && first->value == (-(__int64)0x7fffffffffffffff - 1) &&
            second->value == -1) {
            /* This signed-overflow domain has not been qualified. */
#ifdef __linux__
            recon_unsupported("conditional signed division overflow is not yet qualified");
#else
            __asm { int 3 }
#endif
        }
        if ((BYTE)operation == 57) {
            if (unsigned_values) first->value = (unsigned __int64)first->value / (unsigned __int64)second->value;
            else first->value /= second->value;
        } else {
            if (unsigned_values) first->value = (unsigned __int64)first->value % (unsigned __int64)second->value;
            else first->value %= second->value;
        }
        break;
    case 60:
        first->value = !first->value;
        unsigned_values = 0;
        break;
    case 89: first->value = ~first->value; break;
    case 90: first->value = -(unsigned __int64)first->value; break;
    }
    first->flags = unsigned_values ? 0x4044 : 0x44;
    return first;
}
#undef CONDITION_WIDTH
extern "C" int __cdecl expand_defined_operator(CompilerNameEntry *);
extern "C" int __cdecl match_condition_token(unsigned expected)
{
    BYTE kind, category;
    CompilerParserToken *token;
    if (c1_data.condition_token == 45) return 0;
    if (c1_data.condition_token != 136) {
        if (c1_data.condition_token != (BYTE)expected) return 0;
        c1_data.condition_token = 136;
        return 1;
    }
    token = c1_bss.current_context.ReadToken();
    kind = token->kind;
    if (kind != 6 && kind != 45 && kind != 50 && kind != 59 && kind != 60 && kind != 67 &&
        kind != 68 && kind != 79 && kind != 101 && kind != 118 && kind != 126) {
#ifdef __linux__
        recon_unsupported("conditional token classification cell is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    category = c1_expression_classes.indexed[kind];
    if (category == 6) {
        c1_bss.literal.condition.value = expand_defined_operator(token->value.name);
        c1_bss.literal.condition.flags = 0x63;
    } else if (category == 101) {
        c1_bss.literal.condition.flags = token->value.typed_constant.type->flags;
        if (!(c1_bss.literal.condition.flags & 0x40)) compiler_fatal(17);
        c1_bss.literal.condition.value = token->value.typed_constant.value;
    } else {
        if ((BYTE)expected == kind) return 1;
        c1_data.condition_token = kind;
        return 0;
    }
    if ((BYTE)expected == 101) return 1;
    c1_data.condition_token = 101;
    return 0;
}

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
extern "C" void __cdecl skip_macro_block_comment(void)
{
    unsigned character;
    for (;;) {
        do { character = *c1_bss.source_cursor++; }
        while (c1_data.scanner.flags[character] & 0x40);
        if (!character) { process_special_character(); continue; }
        if (character == '\n') {
            ++c1_bss.current_context.value;
            if (c1_bss.options.preprocess_stdout) fputc('\n', c1_bss.preprocessor_output);
            continue;
        }
        if (character != '*') {
            if (c1_data.scanner.classes[character] == SC_LEAD) read_multibyte_character();
            continue;
        }
    after_star:
        do { character = *c1_bss.source_cursor++; } while (character == '*');
        if (character == '/') return;
        if (!character) { process_special_character(); goto after_star; }
        if (character == '\\') {
            if (process_special_character() & 0x10) continue;
            goto after_star;
        }
        if (c1_data.scanner.classes[character] == SC_LEAD) read_multibyte_character();
        --c1_bss.source_cursor;
    }
}
extern "C" void __cdecl skip_preserved_macro_comment(void) { __asm { int 3 } }

/* The same spelling column serves diagnostics and preprocessing. Its gaps
 * remain unmodeled; consumers guard before every unknown indexed read. */
extern "C" {
const CompilerTokenSpellings c1_token_spellings = { {
    {0}, "=", {0}, ",", {0}, "*", {0}, "*", {0}, "+", {0}, "{", "(",
    {0}, "}", {0}, ")", ";"
} };
}
extern "C" const char *__cdecl parse_angle_include(int);
extern "C" const char *__cdecl parse_include_name(int diagnose, unsigned *kind)
{
    int saved_preprocess = c1_bss.options.preprocess_stdout;
    CompilerParserToken *token;
    const char *name;
    c1_data.lexer_flags |= 0x2000;
    c1_bss.options.preprocess_stdout = 0;
    token = c1_bss.current_context.ReadToken();
    c1_data.lexer_flags &= ~0x2000U;
    c1_bss.options.preprocess_stdout = saved_preprocess;
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
        compiler_warning(6, c1_token_spellings.indexed[token->kind]);
    }
    discard_input_character();
    if (diagnose) c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
    return NULL;
}
extern "C" char *__cdecl trim_include_name(char *, char *);
extern "C" const char *__cdecl parse_angle_include(int diagnose)
{
    char *out = (char *)c1_bss.quoted_workspace;
    BYTE character = *c1_bss.source_cursor++;
    if (!(c1_data.scanner.flags[character] & 0x20)) {
        while (c1_data.scanner.classes[character] == SC_SPACE)
            character = *c1_bss.source_cursor++;
        if (!(c1_data.scanner.flags[character] & 0x20))
            character = (BYTE)parse_directive_character(character);
    }
    if (character == '\n') {
        --c1_bss.source_cursor;
        if (diagnose) compiler_warning(12);
        return NULL;
    }
    while (character != '>') {
        if (character == '\n') {
            --c1_bss.source_cursor;
            if (diagnose) compiler_warning(13);
            return NULL;
        }
        *out++ = character;
        if (_mbctype[character + 1] & 4)
            *out++ = read_multibyte_character();
        character = *c1_bss.source_cursor++;
        if (c1_data.scanner.classes[character] == SC_SPECIAL)
            character = (BYTE)resolve_special_character(character);
    }
    if (out != (char *)c1_bss.quoted_workspace)
        out = trim_include_name((char *)c1_bss.quoted_workspace, out);
    *out = 0;
    return (const char *)c1_bss.quoted_workspace;
}
extern "C" char *__cdecl trim_include_name(char *begin, char *end)
{
    char *trailing = NULL;
    if (!end) end = strchr(begin, 0);
    while (begin < end) {
        if ((isspace)((BYTE)*begin)) {
            if (!trailing) trailing = begin;
        } else trailing = NULL;
        begin += (_mbctype[(BYTE)*begin + 1] & 4) ? 2 : 1;
    }
    return trailing ? trailing : end;
}

extern "C" CompilerParserToken *__cdecl open_include_file(const char *name, unsigned kind)
{
    if (open_forced_include(name, kind) > -1)
        return make_source_state_token(c1_bss.current_context.object);
    return (CompilerParserToken *)c1_data.predefined_tokens[1];
}

extern "C" CompilerParserToken *__cdecl read_line_directive(void)
{
    int saved_preprocess = c1_bss.options.preprocess_stdout;
    CompilerParserToken *number, *filename, *result, *token;
    CompilerRecordedLexer *recording;
    CompilerContextFrame *frame;
    CompilerPool *pool;
    CompilerArena *arena;
    void *memory;
    c1_bss.options.preprocess_stdout = 0;
    number = c1_bss.current_context.ReadToken();
    if (number->kind != 101 || !(number->value.typed_constant.type->flags & 0x40)) {
        /* The diagnostic spelling for arbitrary token kinds remains unknown. */
#ifdef __linux__
        recon_unsupported("line directive diagnostic token spelling is not modeled");
#else
        __asm { int 3 }
#endif
        compiler_warning(5, c1_token_spellings.indexed[number->kind]);
        c1_bss.options.preprocess_stdout = saved_preprocess;
        goto invalid_line;
    }
    if (number->value.typed_constant.value <= 0 || number->value.typed_constant.value > 32767)
        report_language_diagnostic(c1_data.options.language_extensions ? 4 : 1, 112);
    c1_bss.current_context.value = (unsigned)number->value.typed_constant.value - 1;
    filename = c1_bss.current_context.ReadToken();
    c1_bss.options.preprocess_stdout = saved_preprocess;
    switch (filename->kind) {
    case 7:
        if (filename->value.string_attributes.flags & 1) {
            compiler_warning(2);
            filename = NULL;
        } else if (strcmp(filename->value.string_attributes.text,
                          c1_bss.current_context.FileName()) == 0) filename = NULL;
        else c1_bss.current_context.object = c1_data.translation_unit->FindSourceFile(
            filename->value.string_attributes.text, 0);
        break;
    case 45:
        filename = NULL;
        break;
    case 1:
        break;
    default:
        compiler_warning(130, (unsigned)filename->kind);
    invalid_line:
        discard_input_character();
        c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
        return (CompilerParserToken *)c1_data.predefined_tokens[1];
    }
    /* Preserve the authentic24-byte literal storage. The marker consumes only
     * its kind; the following number/filename tokens own their typed values. */
    if (c1_bss.options.preprocess_stdout) {
        begin_line_output();
        result = (CompilerParserToken *)c1_data.predefined_tokens[1];
    } else {
        recording = (CompilerRecordedLexer *)c1_bss.arenas[0].AllocateBytes(64);
        if (recording) recording = recording->Initialize(0, 0);
        c1_bss.context.Push((CompilerLexer *)recording, 0, 0, 0, &c1_bss.current_context);
        if (c1_bss.symbol_arena_kind)
            memory = c1_bss.arenas[c1_bss.symbol_arena_kind].AllocateBytes(32);
        else memory = c1_bss.pools[6].AllocateBytes(32);
        token = (CompilerParserToken *)memory;
        if (token) { token->kind = 43; memcpy(&token->value, &c1_bss.literal, 24); }
        c1_bss.context.saved_frame->stream->Trace((unsigned)token);
        c1_bss.context.saved_frame->stream->Trace((unsigned)number);
        if (filename) c1_bss.context.saved_frame->stream->Trace((unsigned)filename);
        c1_bss.context.saved_frame->stream->Trace((unsigned)c1_data.predefined_tokens[45]);
        frame = c1_bss.context.saved_frame;
        if (frame) {
            recording = (CompilerRecordedLexer *)frame->stream;
            c1_bss.context.saved_frame = frame->next;
            c1_bss.context.object = frame->object;
            c1_bss.context.value = frame->value;
            if (frame->second) {
#ifdef __linux__
                recon_unsupported("line directive return callback is not yet qualified");
#else
                __asm { int 3 }
#endif
                ((C1Initializer)frame->second)();
            }
        } else recording = NULL;
        c1_bss.literal.recorded_stream = recording;
        if (c1_bss.symbol_arena_kind) {
            arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
            memory = arena->cursor; arena->cursor += 32;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        } else {
            pool = &c1_bss.pools[6];
            if (pool->remaining < 32) pool->Refill(32);
            memory = pool->cursor; pool->cursor += 32; pool->remaining -= 32;
        }
        result = (CompilerParserToken *)memory;
        if (result) { result->kind = 242; memcpy(&result->value, &c1_bss.literal, 24); }
    }
    finish_directive_line("#line");
    c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
    return result;
}

extern "C" CompilerParserToken *__cdecl read_preprocessed_pragma(void);
extern "C" void __cdecl mark_source_once(void);
extern "C" void __cdecl mark_source_once(void)
{
    CompilerSourceFile *file = (CompilerSourceFile *)c1_bss.current_context.object;
    file->record_id = (file->record_id & 0x00ffffff) | 0x01000000;
}
extern "C" int __cdecl classify_pragma_name(const char *);
extern "C" void __cdecl read_include_alias_pragma(void);
extern "C" int __cdecl finish_recorded_source_tokens(void);
extern "C" CompilerParserToken *__cdecl expect_directive_token(BYTE);
extern "C" CompilerParserToken *__cdecl read_pragma_directive(void)
{
    CompilerParserToken *token;
    CompilerRecordedLexer *recording;
    CompilerContextFrame *frame;
    CompilerArena *arena;
    CompilerPool *pool;
    BYTE *memory;
    int kind;
    if (c1_bss.options.preprocess_stdout) return read_preprocessed_pragma();
    if (c1_bss.function_output_mode) {
        discard_input_character();
        return (CompilerParserToken *)c1_data.predefined_tokens[1];
    }
    token = c1_bss.current_context.ReadToken();
    if (token->kind == 45) return (CompilerParserToken *)c1_data.predefined_tokens[1];
    c1_bss.current_context.saved_frame->stream->Unread((unsigned)token);
    token = expect_directive_token(6);
    if (!token) goto discard_line;
    kind = classify_pragma_name(token->value.name->name);
    if (kind == PRAGMA_INCLUDE_ALIAS) {
        read_include_alias_pragma();
discard_line:
        finish_recorded_source_tokens();
        return (CompilerParserToken *)c1_data.predefined_tokens[1];
    }
    recording = (CompilerRecordedLexer *)c1_bss.arenas[1].AllocateBytes(64);
    if (recording) recording = recording->Initialize(1, 0);
    c1_bss.context.Push((CompilerLexer *)recording, 1, 0, 0, &c1_bss.current_context);
    arena = &c1_bss.arenas[1];
    memory = arena->cursor;
    arena->cursor += 32;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    token = (CompilerParserToken *)memory;
    if (token) { token->kind = (BYTE)kind; token->value.operation = (BYTE)kind; }
    c1_bss.context.saved_frame->stream->Trace((unsigned)token);
    c1_data.lexer_flags = (c1_data.lexer_flags & ~0x2000U) |
        (kind == PRAGMA_HDRSTOP ? 0x2000 : 0);
    do {
        token = c1_bss.current_context.ReadToken();
        c1_bss.context.saved_frame->stream->Trace((unsigned)token);
    } while (token->kind != 45);
    c1_data.lexer_flags &= ~0x2000U;
    frame = c1_bss.context.saved_frame;
    if (frame) {
        recording = (CompilerRecordedLexer *)frame->stream;
        c1_bss.context.saved_frame = frame->next;
        c1_bss.context.object = frame->object;
        c1_bss.context.value = frame->value;
        if (frame->second) {
#ifdef __linux__
            recon_unsupported("pragma recorded-context callback is not yet qualified");
#else
            __asm { int 3 }
#endif
            ((C1Initializer)frame->second)();
        }
    } else recording = NULL;
    c1_bss.literal.recorded_stream = recording;
    if (c1_bss.symbol_arena_kind) {
        arena = &c1_bss.arenas[c1_bss.symbol_arena_kind];
        memory = arena->cursor;
        arena->cursor += 32;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &c1_bss.pools[6];
        if (pool->remaining < 32) pool->Refill(32);
        memory = pool->cursor;
        pool->cursor += 32;
        pool->remaining -= 32;
    }
    token = (CompilerParserToken *)memory;
    if (token) { token->kind = 0xf2; memcpy(&token->value, &c1_bss.literal, 24); }
    return token;
}
extern "C" CompilerParserToken *__cdecl read_preprocessed_pragma(void)
{
    CompilerParserToken *token;
    const char *text;
    int kind;
    fwrite("#pragma ", 8, 1, c1_bss.preprocessor_output);
    c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
    c1_bss.options.preprocess_stdout = 0;
    token = c1_bss.current_context.ReadToken();
    c1_bss.options.preprocess_stdout = 1;
    if (token->kind == 6) {
        text = token->value.name->name;
        fwrite(text, strlen(text), 1, c1_bss.preprocessor_output);
        kind = classify_pragma_name(text);
        if (kind == PRAGMA_INCLUDE_ALIAS) read_include_alias_pragma();
        else if (kind == PRAGMA_ONCE) mark_source_once();
    } else {
        if (!C1_HAS_TOKEN_SPELLING(token->kind)) {
#ifdef __linux__
        recon_unsupported("unmodeled initial preprocessed pragma token spelling");
#else
        __asm { int 3 }
#endif
        }
        text = c1_token_spellings.indexed[token->kind];
        fwrite(text, strlen(text), 1, c1_bss.preprocessor_output);
    }
    for (;;) {
        token = c1_bss.current_context.ReadToken();
        switch (token->kind) {
        case 1: case 136:
            continue;
        case 45:
            c1_data.type_arena_kind = c1_data.expression_arena_kind = 3;
            return (CompilerParserToken *)c1_data.predefined_tokens[1];
        case 101:
            fwrite(token->value.string.text, (WORD)token->value.string.length - 1,
                1, c1_bss.preprocessor_output);
            break;
        default:
            if (!C1_HAS_TOKEN_SPELLING(token->kind)) {
#ifdef __linux__
            recon_unsupported("unmodeled preprocessed pragma token spelling");
#else
            __asm { int 3 }
#endif
            }
            text = c1_token_spellings.indexed[token->kind];
            fwrite(text, strlen(text), 1, c1_bss.preprocessor_output);
            break;
        }
    }
}
/* Encountered 'p' bucket, original relation indexes25..34. Other buckets
 * remain unsupported; no default entries are implied for them. */
static const struct CompilerPragmaName {
    const char *suffix;
    CompilerPragma selector;
} pragma_p_names[] = {
    { "ush", PRAGMA_PUSH },
    { "rocname", PRAGMA_PROCNAME },
    { "op", PRAGMA_POP },
    { "ointers_to_members", PRAGMA_POINTERS_TO_MEMBERS },
    { "lmn", PRAGMA_PLMN },
    { "lmf", PRAGMA_PLMF },
    { "arameter", PRAGMA_PARAMETER },
    { "agesize", PRAGMA_PAGESIZE },
    { "age", PRAGMA_PAGE },
    { "ack", PRAGMA_PACK }
};
/* Encountered prefix3..7 of the 'c' bucket, in original search order.
 * The following check_pointer row remains unsupported. */
static const CompilerPragmaName pragma_c_prefix[] = {
    { "onst_seg", PRAGMA_CONST_SEG },
    { "omponent", PRAGMA_COMPONENT },
    { "omment", PRAGMA_COMMENT },
    { "ode_seg", PRAGMA_CODE_SEG },
    { "heck_stack", PRAGMA_CHECK_STACK }
};
extern "C" int __cdecl classify_pragma_name(const char *name)
{
    unsigned i;
    signed char first = (signed char)*name;
    if (first < '_') return PRAGMA_UNKNOWN;
    /* Complete encountered 'm' bucket, relation20. */
    if (first == 'm') return strcmp(name + 1, "essage") == 0 ? PRAGMA_MESSAGE : PRAGMA_UNKNOWN;
    /* Complete encountered 'w' bucket, relation48. */
    if (first == 'w') return strcmp(name + 1, "arning") == 0 ? PRAGMA_WARNING : PRAGMA_UNKNOWN;
    /* Encountered prefix12..16 of the 'i' bucket, in search order. */
    if (first == 'i') {
        if (!strcmp(name + 1, "ntrinsic")) return PRAGMA_INTRINSIC;
        if (!strcmp(name + 1, "nline_recursion")) return PRAGMA_INLINE_RECURSION;
        if (!strcmp(name + 1, "nline_depth")) return PRAGMA_INLINE_DEPTH;
        if (!strcmp(name + 1, "nit_seg")) return PRAGMA_INIT_SEG;
        if (!strcmp(name + 1, "nclude_alias")) return PRAGMA_INCLUDE_ALIAS;
        __asm { int 3 }
    }
    /* Complete two-row 'a' bucket, relations0..1. */
    if (first == 'a') {
        if (!strcmp(name + 1, "uto_inline")) return PRAGMA_AUTO_INLINE;
        if (!strcmp(name + 1, "lloc_text")) return PRAGMA_ALLOC_TEXT;
        return PRAGMA_UNKNOWN;
    }
    /* Encountered relation24 in the 'o' bucket: suffix nce -> ONCE37.
     * Other 'o' spellings remain unsupported rather than missing keywords. */
    if (first == 'o') {
        if (!strcmp(name + 1, "ptions")) return PRAGMA_OPTIONS;
        if (!strcmp(name + 1, "ptimize")) return PRAGMA_OPTIMIZE;
        if (!strcmp(name + 1, "nce")) return PRAGMA_ONCE;
        return PRAGMA_UNKNOWN;
    }
    if (first == 'f') return !strcmp(name + 1, "unction") ? PRAGMA_FUNCTION : PRAGMA_UNKNOWN;
    /* Complete encountered 'b' bucket, relation2. */
    if (first == 'b') return !strcmp(name + 1, "ss_seg") ? PRAGMA_BSS_SEG : PRAGMA_UNKNOWN;
    /* The complete encountered 'd' bucket is relation9: ata_seg ->22. */
    if (first == 'd') return strcmp(name + 1, "ata_seg") == 0 ? PRAGMA_DATA_SEG : PRAGMA_UNKNOWN;
    if (first == 'c') {
        for (i = 0; i != sizeof(pragma_c_prefix) / sizeof(pragma_c_prefix[0]); ++i)
            if (strcmp(pragma_c_prefix[i].suffix, name + 1) == 0)
                return pragma_c_prefix[i].selector;
        __asm { int 3 }
    }
    if (first != 'p') { __asm { int 3 } }
    ++name;
    for (i = 0; i != sizeof(pragma_p_names) / sizeof(pragma_p_names[0]); ++i)
        if (strcmp(pragma_p_names[i].suffix, name) == 0)
            return pragma_p_names[i].selector;
    return PRAGMA_UNKNOWN;
}
extern "C" char *__cdecl duplicate_argument(const char *, int);
extern "C" void __cdecl skip_to_directive_newline(CompilerParserToken *);
typedef char IncludeAlias_extent[sizeof(CompilerIncludeAlias) == 20 ? 1 : -1];
typedef char IncludeAlias_kind_offset[offsetof(CompilerIncludeAlias, kind) == 12 ? 1 : -1];
extern "C" void __cdecl read_include_alias_pragma(void)
{
    CompilerArena *arena;
    CompilerIncludeAlias *alias;
    const char *delimiter;
    if (!c1_bss.debug_module->Slot24(36)) compiler_error(73, ".\\p0pragma.c", 0xa6e);
    if (!expect_directive_token(118)) return;
    if (c1_bss.options.preprocess_stdout) fwrite("(", 1, 1, c1_bss.preprocessor_output);
    arena = &c1_bss.arenas[0];
    alias = (CompilerIncludeAlias *)arena->cursor;
    arena->cursor += 20;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    alias->name = parse_include_name(0, (unsigned *)&alias->kind);
    if (!alias->name) {
        report_language_diagnostic(1, 612);
        skip_to_directive_newline(c1_bss.current_context.ReadToken());
        return;
    }
    if (c1_bss.options.preprocess_stdout) {
        delimiter = alias->kind ? "<" : "\"";
        fwrite(delimiter, 1, 1, c1_bss.preprocessor_output);
        fwrite(alias->name, strlen(alias->name), 1, c1_bss.preprocessor_output);
        delimiter = alias->kind ? ">" : "\"";
        fwrite(delimiter, 1, 1, c1_bss.preprocessor_output);
    }
    if (!expect_directive_token(55)) return;
    if (c1_bss.options.preprocess_stdout) fwrite(",", 1, 1, c1_bss.preprocessor_output);
    alias->name = duplicate_argument(alias->name, 0);
    alias->replacement = parse_include_name(0, (unsigned *)&alias->replacement_kind);
    if (!alias->replacement) {
        report_language_diagnostic(1, 612);
        skip_to_directive_newline(c1_bss.current_context.ReadToken());
        return;
    }
    if (c1_bss.options.preprocess_stdout) {
        delimiter = alias->replacement_kind ? "<" : "\"";
        fwrite(delimiter, 1, 1, c1_bss.preprocessor_output);
        fwrite(alias->replacement, strlen(alias->replacement), 1, c1_bss.preprocessor_output);
        delimiter = alias->replacement_kind ? ">" : "\"";
        fwrite(delimiter, 1, 1, c1_bss.preprocessor_output);
    }
    if (!expect_directive_token(126)) return;
    if (c1_bss.options.preprocess_stdout) fwrite(")", 1, 1, c1_bss.preprocessor_output);
    if (alias->kind != alias->replacement_kind) {
        report_language_diagnostic(1, 612);
        skip_to_directive_newline(c1_bss.current_context.ReadToken());
        return;
    }
    alias->replacement = duplicate_argument(alias->replacement, 0);
    c1_data.options.include_aliases->Insert(alias);
}
void RECON_THISCALL CompilerIncludeAliases::Insert(CompilerIncludeAlias *alias)
{
    alias->next = head;
    head = alias;
}

extern "C" int __cdecl process_forced_macro_directive(unsigned, const char *, const char *);
extern "C" int __cdecl process_forced_macro_directive(unsigned, const char *, const char *)
{ __asm { int 3 } return 0; }
extern "C" void __cdecl clear_macro_definition(void)
{
    CompilerNameEntry *identifier;
    CompilerMacro *macro;
    int action;
    c1_bss.debug_module->Slot1c();
    if (!c1_data.source_include_depth && c1_bss.forced_source_index) {
        action = process_forced_macro_directive(10, NULL, c1_data.current_identifier->name);
        if (action == 1 || (action == 2 && c1_bss.forced_source_index == 2)) return;
    }
    identifier = c1_data.current_identifier;
    if (identifier == c1_data.builtin_names.defined_operator) {
        report_language_diagnostic(1, 117, identifier, "#undef");
        return;
    }
    macro = identifier->macro;
    if (!macro) return;
    if (macro->body->kind < -3)
        report_language_diagnostic(1, 117, identifier, "#undef");
    c1_data.current_identifier->macro = NULL;
    if (c1_bss.browser_filename) c1_data.backend->Slot24(macro);
}

extern "C" unsigned char __cdecl read_multibyte_character(void)
{ __asm { int 3 } return 0; }

extern "C" {
int __cdecl compare_pch_paths(const char *, const char *);
unsigned __cdecl classify_pch_include(const char *);
int __cdecl search_relative_include(const char *, int, char *);
int __cdecl search_include_directories(const char *, int, char *);
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
extern "C" int __cdecl search_include_directories(const char *name, int open_file,
    char *resolved)
{
    char path[512];
    CompilerVector *directories = c1_bss.options.include_directories;
    const BYTE *directory, *last_character;
    char *out;
    unsigned hash, character;
    short index = 0;
    int attempted = 0, descriptor;
    while (directories && index < directories->count) {
        directory = (const BYTE *)directories->items[index++];
        attempted = 1;
        out = path;
        hash = 0;
        if (directory && strcmp((const char *)directory, "./") != 0) {
            /* An empty entry reads an unwritten/carried stack pointer in
             * the original. That domain is unsupported in both builds. */
            if (!*directory) {
#ifdef __linux__
                recon_unsupported("empty include-directory last-character state");
#else
                __asm { int 3 }
#endif
            }
            while (*directory && out < path + sizeof(path)) {
                last_character = directory;
                character = *directory;
                *out++ = character;
                hash = (hash >> 4) + hash * 4 + character;
                if (_mbctype[character + 1] & 4) {
                    character = *++directory;
                    *out++ = character;
                    hash = (hash >> 4) + hash * 4 + character;
                }
                ++directory;
            }
            if (!strchr(c1_data.options.path_separators, *last_character)) {
                *out++ = '\\';
                hash = (hash >> 4) + hash * 4 + '\\';
            }
        }
        if (strlen(name) + (out - path) + 1 > sizeof(path)) {
            *out = 0;
            compiler_error(81, path, 512);
        }
        const BYTE *source = (const BYTE *)name;
        while ((*out = *source++) != 0)
            hash = (hash >> 4) + hash * 4 + (BYTE)*out++;
        if (open_file) {
            descriptor = open_source_file(path, hash, 0, 1, 1);
            if (descriptor == -1) continue;
            if (descriptor > 0 &&
                (c1_bss.forced_source_index == 1 || c1_bss.pch_file_failed))
                report_source_file_error(path, descriptor);
            if (resolved) strcpy(resolved, path);
            return descriptor;
        }
        if (_access(path, 0) == 0) {
            if (resolved) strcpy(resolved, path);
            return 0;
        }
    }
    if (resolved) *resolved = 0;
    if (!attempted) compiler_error(34, name);
    else if (open_file) report_response_error(83, 343, name);
    return -1;
}
