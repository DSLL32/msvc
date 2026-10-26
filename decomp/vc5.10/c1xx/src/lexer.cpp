/* VC5 C++ character lexer. Each punctuation return and the three dispatch
 * domains are recovered from this frontend; token storage remains typed.
 * Wider token/semantic domains stop before their unsupported consumers. */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <new.h>
#include "parser.h"
#include "parser-state.h"
#include "keywords.h"
#include "lexer.h"
#include <recon.h>
#pragma intrinsic(memcpy, memcmp, strcmp)

#ifdef RECON_NATIVE_TEST
/* Only the actual saved-pointer writer is enabled here; wider arms below
 * retain the parser-state guard. Both writes share BA8C8's storage. */
#define LEXER_SAVED_POINTER frontend_bss.saved_token_value.pointer
#define LEXER_SAVED_STORAGE frontend_bss.saved_token_value
#else
#define LEXER_SAVED_POINTER saved_parser_value.pointer
#define LEXER_SAVED_STORAGE saved_parser_value
#endif

#define source_exhausted frontend_data.source_exhausted
#define lexical_skip_mode frontend_bss.lexical_skip_mode
/* Shared bounded spelling scratch at C0988; floating consumers remain pending. */
#define floating_token_text frontend_bss.numeric_literal_text
#define LEXER_FLAGS frontend_data.compiler_flags
extern "C" {
int __cdecl read_source_chunk(void); /*1d5f8 existing*/
unsigned __cdecl read_unspliced_character(void); /*23401*/
int __cdecl skip_line_comment(void); /*e289*/
int __cdecl skip_block_comment(void); /*412e3*/
unsigned __cdecl process_special_character(void); /*ef91*/
unsigned __cdecl process_trigraph(void); /*2c445*/
void __cdecl discard_input_character(void); /*245d8*/
int __cdecl splice_input_line(void); /*41155*/
unsigned __cdecl resolve_special_character(unsigned); /*48a42; consumer masks to low8; upper argument scope held*/
ParserTokenView *__cdecl process_source_newline(void); /*e523*/
int __cdecl at_source_buffer_start(void); /*492c1*/
int __cdecl begin_first_line_forced_include(void); /*492da*/
ParserTokenView *__cdecl read_directive_token(void); /*1f55f*/
int __cdecl skip_newline_comment(void); /*e839*/
void __cdecl begin_line_output(void); /*807bc*/
void __cdecl scan_quoted_literal(int, unsigned); /*fe4c*/
void __cdecl copy_quoted_literal(unsigned, int); /*82047*/
ParserTokenView *__cdecl make_character_constant(unsigned, const void *, unsigned); /*2cfa4*/
int __cdecl scan_identifier(unsigned); /*e339; consumes only the low byte*/
int __cdecl expand_macro(struct CompilerMacro *); /*e90b*/
ParserTokenView *__cdecl scan_numeric_token(unsigned, int); /*f4bd; first byte consumed, wider domain held*/
unsigned __cdecl collect_floating_token(char *); /*82384*/
ParserTokenView *__cdecl scan_floating_token(char *); /*500ad*/
int __cdecl finish_forced_include(void); /*90ddb*/
unsigned __cdecl make_arena_token(unsigned); /*207f3*/
}

/* Inline source operations retain the original allocation and lookahead
 * sites, without introducing helper vertices absent from the target. */
#define LEXER_PROTOTYPE(kind) ((ParserTokenView *)frontend_data.canonical_token_records[(kind) - 1])
#define LEXER_ALLOCATE_TOKEN() do { \
    if (frontend_bss.symbol_arena_kind) { \
        arena = &frontend_bss.arenas[frontend_bss.symbol_arena_kind]; \
        token = (ParserTokenView *)arena->cursor; \
        arena->cursor += 48; \
        if (arena->cursor > arena->committed_limit) arena->CommitExtent(); \
    } else { \
        pool = &frontend_bss.pools[7]; \
        if (pool->remaining < 48) pool->Refill(48); \
        token = (ParserTokenView *)pool->cursor; \
        pool->cursor += 48; pool->remaining -= 48; \
    } \
} while (0)
#define LEXER_PEEK_CLASS() do { \
    for (;;) { \
        next = *frontend_bss.source_cursor++; \
        next_class = frontend_data.scanner.classes[next]; \
        if (next_class != SC_SPECIAL) break; \
        special = process_special_character(); \
        if (special & ~0x70U) continue; \
        if (special & 0x20) ++frontend_bss.source_cursor; \
        next_class = SC_UNUSED2; \
        break; \
    } \
} while (0)

unsigned RECON_THISCALL CompilerLexer::Read(void)
{
    BYTE character, next;
    unsigned dispatch, next_class, special, arena_kind;
    int directive, wide;
    ParserTokenView *token;
    CompilerArena *arena;
    CompilerPool *pool;
    FrontendContextFrame *frame;
    CompilerTokenStream *recorded;
    CompilerRecordedLexer *new_stream;
    CompilerNameEntry *name;
    const char *text;
    unsigned length;
    if (unread_count) {
#ifdef RECON_NATIVE_TEST
        if (unread_count != 1)
            recon_unsupported("lexer: wider unread stack pending");
#endif
        return unread_tokens[--unread_count];
    }
    for (;;) {
        character = *frontend_bss.source_cursor++;
        dispatch = frontend_data.scanner.classes[character];
#ifdef RECON_NATIVE_TEST
        /* Encountered lexical domains, including the NUL refill boundary.
         * Wider special characters stop before their pending consumers. */
        if (dispatch != SC_NEWLINE && dispatch != SC_IDENTIFIER && dispatch != SC_SPACE &&
            dispatch != SC_LPAREN && dispatch != SC_RPAREN && dispatch != SC_COMMA && dispatch != SC_COLON &&
            dispatch != SC_LBRACKET && dispatch != SC_RBRACKET &&
            dispatch != SC_DIGIT && dispatch != SC_DOT && dispatch != SC_AMP && dispatch != SC_DQUOTE && dispatch != SC_STAR && dispatch != SC_LBRACE && dispatch != SC_RBRACE && dispatch != SC_SEMICOLON &&
            !(dispatch == SC_SPECIAL && character == 0))
            recon_unsupported("lexer: character/payload domain pending");
#endif
        switch (dispatch) {
        case SC_NEWLINE:
            if (LEXER_FLAGS & 0x8000) {
                --frontend_bss.source_cursor;
                return (unsigned)LEXER_PROTOTYPE(45);
            }
            token = process_source_newline();
            if (token) return (unsigned)token;
            continue;
        case SC_SPACE:
            do {
                if (frontend_bss.options.preprocess_stdout && character != '\r')
                    fputc(character, frontend_bss.preprocessor_output);
                character = *frontend_bss.source_cursor++;
            } while (frontend_data.scanner.classes[character] == SC_SPACE);
            --frontend_bss.source_cursor;
            continue;
        case SC_SLASH:
            if (read_unspliced_character() == '/') {
                LEXER_FLAGS |= 0x800;
                skip_line_comment();
                LEXER_FLAGS &= ~0x800U;
                continue;
            }
            --frontend_bss.source_cursor;
            if (read_unspliced_character() == '*') {
                LEXER_FLAGS |= 0x800;
                skip_block_comment();
                LEXER_FLAGS &= ~0x800U;
                continue;
            }
            --frontend_bss.source_cursor;
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(58);
            --frontend_bss.source_cursor;
            return (unsigned)LEXER_PROTOTYPE(57);
        case SC_HASH:
            if (frontend_bss.options.preprocess_stdout) {
                fwrite("#", 1, 1, frontend_bss.preprocessor_output);
                continue;
            }
            compiler_warning(lexical_skip_mode ? 121 : 14);
            if (!lexical_skip_mode) discard_input_character();
            continue;
        case SC_SPECIAL:
            if (character == '\\') {
                if (!frontend_bss.options.preprocess_stdout) {
                    if (!splice_input_line()) compiler_warning(17);
                } else {
                    fputc('\\', frontend_bss.preprocessor_output);
                    next = *frontend_bss.source_cursor++;
                    if (frontend_data.scanner.classes[next] == SC_SPECIAL)
                        next = (BYTE)resolve_special_character(next);
                    fputc(next, frontend_bss.preprocessor_output);
                }
                continue;
            }
            if (character == '?') {
                if (process_trigraph() & 0x40)
                    return (unsigned)LEXER_PROTOTYPE(86);
                continue;
            }
            if (source_exhausted) {
                --frontend_bss.source_cursor;
                return (unsigned)LEXER_PROTOTYPE(109);
            }
            if (lexical_skip_mode) { process_special_character(); continue; }
            if (!read_source_chunk()) continue;
            if (frontend_bss.options.preprocess_stdout) {
                if (restore_source_input()) continue;
                return (unsigned)LEXER_PROTOTYPE(109);
            }
            if (!frontend_data.source_include_depth && frontend_bss.forced_source_index == 2 &&
                finish_forced_include() == 1) continue;
            directive = restore_source_input();
            if (!directive) {
                if (LEXER_FLAGS & 0x8000) return (unsigned)LEXER_PROTOTYPE(45);
                source_exhausted = 1;
                frontend_bss.source_cursor = (BYTE *)"";
                frontend_bss.input->Mark();
            }
            if (directive == 1000) continue;
            arena_kind = frontend_bss.symbol_arena_kind;
            new_stream = (CompilerRecordedLexer *)frontend_bss.arenas[arena_kind].Allocate(64);
            if (new_stream) new(new_stream) CompilerRecordedLexer(
                frontend_bss.saved_context_stack.origin, frontend_bss.saved_context_stack.position, arena_kind);
            /* Push reads only the common vptr/object/value prefix. This cast
             * preserves the original shared layout, not an88-byte allocation. */
            frontend_bss.context_stack.Push((CompilerLexer *)new_stream, arena_kind, 0, 0,
                                &frontend_bss.saved_context_stack);
            saved_parser_value.integer = directive;
            LEXER_ALLOCATE_TOKEN();
            if (token) { token->kind = 45; memcpy(&token->value, &LEXER_SAVED_STORAGE, 40); }
            ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)token);
            token = (ParserTokenView *)make_arena_token(frontend_bss.symbol_arena_kind);
            ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)token);
            ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)LEXER_PROTOTYPE(45));
            ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)LEXER_PROTOTYPE(244));
            frame = frontend_bss.context_stack.top;
            if (frame) {
                recorded = ((CompilerTokenStream *)frame->object);
                frontend_bss.context_stack.top = frame->previous;
                frontend_bss.context_stack.origin = frame->saved_origin;
                frontend_bss.context_stack.position = frame->saved_position;
                if (frame->callback) frame->callback();
            } else recorded = NULL;
            LEXER_SAVED_POINTER = recorded;
            LEXER_ALLOCATE_TOKEN();
            if (token) { token->kind = 0xf2; memcpy(&token->value, &LEXER_SAVED_STORAGE, 40); }
            if (frontend_data.compiler_mode_flags & 0x04000000) {
                process_context_token(token);
                return make_arena_token(0);
            }
            return (unsigned)token;
        case SC_LETTER_L:
            LEXER_PEEK_CLASS();
            if (next_class == SC_SQUOTE) { wide = 1; goto character_literal; }
            if (next_class == SC_DQUOTE) { wide = 1; goto string_literal; }
            --frontend_bss.source_cursor;
            frontend_bss.source_cursor[-1] = character;
            /* FALL THROUGH */
        case SC_IDENTIFIER:
            if (scan_identifier(character)) {
                name = frontend_data.current_identifier;
            } else {
                name = frontend_data.current_identifier;
                if (name->macro && expand_macro(name->macro)) continue;
            }
            if (frontend_bss.options.preprocess_stdout) {
                for (text = name->name; *text; ++text)
                    fputc(*text, frontend_bss.preprocessor_output);
                return (unsigned)LEXER_PROTOTYPE(136);
            }
            if (name->flags & 0x10000)
                return (unsigned)LEXER_PROTOTYPE(((CompilerIdentifier *)name)->token);
            LEXER_ALLOCATE_TOKEN();
            if (token) { token->kind = 6; token->value.pointer = frontend_data.current_identifier; }
            return (unsigned)token;
        case SC_DQUOTE:
            wide = 0;
        string_literal:
            if (frontend_bss.options.preprocess_stdout) { copy_quoted_literal('"', wide); continue; }
            scan_quoted_literal(wide, '"');
            LEXER_ALLOCATE_TOKEN();
            if (token) { token->kind = 7; memcpy(&token->value, &LEXER_SAVED_STORAGE, 40); }
            return (unsigned)token;
        case SC_SQUOTE:
            wide = 0;
        character_literal:
            if (frontend_bss.options.preprocess_stdout) { copy_quoted_literal('\'', wide); continue; }
            scan_quoted_literal(wide, '\'');
            return (unsigned)make_character_constant(wide,
                saved_parser_value.pair.first, (unsigned)saved_parser_value.pair.second - 1);
        case SC_DOT:
            LEXER_PEEK_CLASS();
            if (next_class == SC_DIGIT) {
                floating_token_text[0] = '.';
                floating_token_text[1] = frontend_bss.source_cursor[-1];
                if (!(LEXER_FLAGS & 0x4000) &&
                    (frontend_bss.options.preprocess_stdout || (LEXER_FLAGS & 0x800000))) {
                    length = collect_floating_token(floating_token_text + 2) + 1;
                    if (frontend_bss.options.preprocess_stdout) {
                        fwrite(floating_token_text, length, 1, frontend_bss.preprocessor_output);
                        return (unsigned)LEXER_PROTOTYPE(136);
                    }
                    return (unsigned)LEXER_PROTOTYPE(136);
                }
                return (unsigned)scan_floating_token(floating_token_text + 2);
            }
            if (next_class == SC_STAR) return (unsigned)LEXER_PROTOTYPE(146);
            if (next_class == SC_DOT) {
                next = *frontend_bss.source_cursor++;
                if (frontend_data.scanner.classes[next] == SC_SPECIAL)
                    next = (BYTE)resolve_special_character(next);
                if (next == '.') {
                    LEXER_ALLOCATE_TOKEN();
                    if (token) { token->kind = 106; memcpy(&token->value, &LEXER_SAVED_STORAGE, 40); }
                    return (unsigned)token;
                }
                --frontend_bss.source_cursor;
                frontend_bss.source_cursor[-1] = '.';
            }
            --frontend_bss.source_cursor;
            return (unsigned)LEXER_PROTOTYPE(80);
        case SC_DIGIT:
            return (unsigned)scan_numeric_token(character,
                (LEXER_FLAGS & 0x200000) && !(LEXER_FLAGS & 0x8000));
        case SC_STAR:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(75);
            if (next_class == SC_SLASH && !frontend_bss.options.preprocess_stdout)
                report_language_diagnostic(1, 138);
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(74);
        case SC_LESS:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(69);
            if (next_class == SC_LESS) {
                LEXER_PEEK_CLASS();
                if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(66);
                --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(67);
            }
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(68);
        case SC_GREATER:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(63);
            if (next_class == SC_GREATER) {
                LEXER_PEEK_CLASS();
                if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(87);
                --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(88);
            }
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(62);
        case SC_MINUS:
            LEXER_PEEK_CLASS();
            switch (next_class) {
            case SC_EQUAL: return (unsigned)LEXER_PROTOTYPE(71);
            case SC_MINUS: return (unsigned)LEXER_PROTOTYPE(56);
            case SC_GREATER:
                LEXER_PEEK_CLASS();
                if (next_class == SC_STAR) return (unsigned)LEXER_PROTOTYPE(147);
                --frontend_bss.source_cursor;
                return (unsigned)LEXER_PROTOTYPE(83);
            default: --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(70);
            }
        case SC_PLUS:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(82);
            if (next_class == SC_PLUS) return (unsigned)LEXER_PROTOTYPE(64);
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(81);
        case SC_AMP:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(51);
            if (next_class == SC_AMP) return (unsigned)LEXER_PROTOTYPE(50);
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(49);
        case SC_BAR:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(78);
            if (next_class == SC_BAR) return (unsigned)LEXER_PROTOTYPE(79);
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(77);
        case SC_COLON:
            LEXER_PEEK_CLASS();
            if (next_class == SC_GREATER) return (unsigned)LEXER_PROTOTYPE(94);
            if (next_class == SC_COLON) return (unsigned)LEXER_PROTOTYPE(149);
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(54);
        case SC_CARET:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(92);
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(91);
        case SC_PERCENT:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(73);
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(72);
        case SC_EQUAL:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(59);
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(52);
        case SC_BANG:
            LEXER_PEEK_CLASS();
            if (next_class == SC_EQUAL) return (unsigned)LEXER_PROTOTYPE(76);
            --frontend_bss.source_cursor; return (unsigned)LEXER_PROTOTYPE(60);
        case SC_LPAREN: return (unsigned)LEXER_PROTOTYPE(118);
        case SC_RPAREN: return (unsigned)LEXER_PROTOTYPE(126);
        case SC_LBRACE: return (unsigned)LEXER_PROTOTYPE(117);
        case SC_RBRACE: return (unsigned)LEXER_PROTOTYPE(123);
        case SC_LBRACKET: return (unsigned)LEXER_PROTOTYPE(65);
        case SC_RBRACKET: return (unsigned)LEXER_PROTOTYPE(122);
        case SC_COMMA: return (unsigned)LEXER_PROTOTYPE(55);
        case SC_SEMICOLON: return (unsigned)LEXER_PROTOTYPE(127);
        case SC_TILDE: return (unsigned)LEXER_PROTOTYPE(89);
        case 41: ++frontend_bss.source_cursor; continue;
        default:
            if (frontend_bss.options.preprocess_stdout) fputc(character, frontend_bss.preprocessor_output);
            else compiler_warning(18, character);
            continue;
        }
    }
}

/* The line processor owns its complete directive/whitespace control flow.
 * Current native support reaches the genuine first-line predicate before
 * any directive, token transfer or source-only skip state is consumed. */
extern "C" ParserTokenView *__cdecl process_source_newline(void)
{
    unsigned character, dispatch, arena_kind;
    int first_line;
    ParserTokenView *token;
    CompilerArena *arena;
    CompilerPool *pool;
#ifdef RECON_NATIVE_TEST
    /* Subsequent builtin-source lines use the existing line/depth owners.
     * Output mode remains outside this case's supported state. */
    if (frontend_bss.options.preprocess_stdout)
        recon_unsupported("newline: preprocessor-output domain pending");
#endif
next_line:
    if (frontend_bss.options.preprocess_stdout) {
        fputc('\n', frontend_bss.preprocessor_output);
        if (LEXER_FLAGS & 0x400000) {
            LEXER_FLAGS &= ~0x400000U;
            if (!frontend_data.options.preprocess_no_lines) {
                begin_line_output();
                fputc('\n', frontend_bss.preprocessor_output);
            }
        }
    }
    ++frontend_bss.saved_context_stack.position;
    first_line = !frontend_data.source_include_depth &&
        frontend_bss.saved_context_stack.position == 1 && at_source_buffer_start();
    if (first_line && begin_first_line_forced_include()) {
        ++frontend_bss.source_cursor;
        goto next_line;
    }
read_character:
    character = *frontend_bss.source_cursor++;
    dispatch = frontend_data.scanner.classes[character];
    switch (dispatch) {
    case SC_HASH:
        LEXER_FLAGS |= 0x8000;
        token = read_directive_token();
        LEXER_FLAGS &= ~0x8000U;
        if (token->kind == 1) goto read_character;
        return token;
    case SC_SLASH:
        if (skip_newline_comment()) goto read_character;
        goto end_line;
    case SC_NEWLINE:
        ++frontend_bss.saved_context_stack.position;
        first_line = !frontend_data.source_include_depth &&
            frontend_bss.saved_context_stack.position == 1 && at_source_buffer_start();
        if (first_line && begin_first_line_forced_include()) {
            ++frontend_bss.source_cursor;
            goto next_line;
        }
        /* FALL THROUGH */
    case SC_SPACE:
        if (frontend_bss.options.preprocess_stdout) {
            if (character != '\r') fputc(character, frontend_bss.preprocessor_output);
        } else {
            while (frontend_data.scanner.classes[*frontend_bss.source_cursor++] == SC_SPACE)
                ;
            --frontend_bss.source_cursor;
        }
        goto read_character;
    case SC_SPECIAL:
        if (!character || character == 0x1a) {
            if (lexical_skip_mode) {
                process_special_character();
                goto read_character;
            }
            if (!read_source_chunk()) goto read_character;
            saved_parser_value.source.origin = frontend_bss.saved_context_stack.origin;
            saved_parser_value.source.position = frontend_bss.saved_context_stack.position;
            arena_kind = frontend_bss.symbol_arena_kind;
            if (arena_kind) token = (ParserTokenView *)frontend_bss.arenas[arena_kind].Allocate(48);
            else token = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
            goto finish_marker;
        }
        if (character == '?') {
            /* This caller tests the full DWORD excluding bit0x40. Lexer::Read
             * instead consumes only bit0x40 of AL. */
            if (process_trigraph() & ~0x40U) goto read_character;
        } else if (splice_input_line()) goto read_character;
        goto end_line;
    default:
        goto end_line;
    }
end_line:
    --frontend_bss.source_cursor;
    if (!frontend_data.source_include_depth && frontend_bss.forced_source_index)
        return LEXER_PROTOTYPE(4);
    if (frontend_data.compiler_mode_flags & 0x04000000) {
        saved_parser_value.source.origin = frontend_bss.saved_context_stack.origin;
        saved_parser_value.source.position = frontend_bss.saved_context_stack.position;
        arena_kind = frontend_bss.symbol_arena_kind;
        if (arena_kind) token = (ParserTokenView *)frontend_bss.arenas[arena_kind].Allocate(48);
        else {
            pool = &frontend_bss.pools[7];
            if (pool->remaining < 48) pool->Refill(48);
            token = (ParserTokenView *)pool->cursor;
            pool->cursor += 48; pool->remaining -= 48;
        }
        goto finish_marker;
    }
    if (LEXER_FLAGS & 0x200000) {
        saved_parser_value.source.origin = frontend_bss.saved_context_stack.origin;
        saved_parser_value.source.position = frontend_bss.saved_context_stack.position;
        LEXER_ALLOCATE_TOKEN();
        goto finish_marker;
    }
    if ((LEXER_FLAGS & 0x20000) && !(LEXER_FLAGS & 0x20))
        emit_context_transition(0x24);
    return NULL;
finish_marker:
    if (token) { token->kind = 0xf0; memcpy(&token->value, &LEXER_SAVED_STORAGE, 40); }
    return token;
}

/* Preprocessor directive dispatch. The 2,048 conditional-state slots are
 * bounded by push_conditional_state's actual limit. This original-only owner
 * is source coverage, not a native table/default projection. */
#ifndef RECON_NATIVE_TEST
static int conditional_states[2048]; /* ba920, DWORD rows */
#else
#define conditional_states ((int *)(recon_unsupported("directive: conditional-state storage pending"),0))
#endif

enum CompilerDirective {
    PP_IF, PP_ELIF, PP_ELSE, PP_ENDIF, PP_IFDEF, PP_IFNDEF, PP_DEFINE,
    PP_INCLUDE, PP_PRAGMA, PP_UNDEF, PP_LINE, PP_UNKNOWN, PP_ERROR,
    PP_IDENT, PP_IMPORT
};
extern "C" {
BYTE __cdecl lookup_preprocessor_directive(const char *); /*1f4cb, actual AL result*/
void __cdecl finish_directive_line(const char *); /*1fb88*/
unsigned __cdecl read_directive_character(unsigned); /*1fcd4, lowBYTE input/result*/
int __cdecl skip_conditional_group(int); /*1fc1d*/
int __cdecl push_conditional_state(void); /*245bc*/
__int64 __cdecl evaluate_preprocessor_condition(void); /*415df*/
void __cdecl begin_conditional_replay(void); /*8179a*/
int __cdecl read_conditional_replay(void); /*907d2*/
int __cdecl record_conditional_value(short); /*9072f*/
void __cdecl process_define_directive(int); /*242f7*/
ParserTokenView *__cdecl process_pragma_directive(void); /*23847*/
ParserTokenView *__cdecl process_line_directive(void); /*81acb*/
char *__cdecl parse_include_directive(int, unsigned *); /*23f81*/
ParserTokenView *__cdecl include_directive_file(const char *, unsigned); /*288d5*/
char *__cdecl parse_import_directive(char **, unsigned *); /*a8499*/
char *__cdecl resolve_import_directive(const char *, const char *, unsigned); /*a9226*/
void __cdecl process_undef_directive(void); /*45b4d*/
BYTE __cdecl advance_multibyte_source(void); /*82780; existing callers discard AL*/
}

extern "C" ParserTokenView *__cdecl read_directive_token(void)
{
    BYTE character, directive;
    unsigned dispatch, arena_kind, include_flags;
    int saved_output, skipped, conditional_state;
    __int64 condition;
    const char *name;
    char *filename, *import_name, *text;
    CompilerRecordedLexer *new_stream;
    CompilerTokenStream *recorded;
    FrontendContextFrame *frame;
    CompilerArena *arena;
    CompilerPool *pool;
    ParserTokenView *token;

    name = frontend_data.current_identifier->name;
read_character:
    character = *frontend_bss.source_cursor++;
    if (frontend_data.scanner.flags[character] & 1) {
        /* The genuine caller's upper argument bytes are unspecified. The
         * pending scanner consumes only the low byte, established at e339. */
        scan_identifier(character);
        name = frontend_data.current_identifier->name;
        goto select_arenas;
    }
    dispatch = frontend_data.scanner.classes[character];
    switch (dispatch) {
    case SC_SPACE:
        goto read_character;
    case SC_SPECIAL:
        if (process_special_character() & ~0x50U) goto read_character;
        goto select_arenas;
    case SC_SLASH:
        if ((BYTE)read_unspliced_character() == '/') {
            LEXER_FLAGS |= 0x800;
            skip_line_comment();
            LEXER_FLAGS &= ~0x800U;
            goto read_character;
        }
        --frontend_bss.source_cursor;
        if ((BYTE)read_unspliced_character() == '*') {
            LEXER_FLAGS |= 0x800;
            skip_block_comment();
            LEXER_FLAGS &= ~0x800U;
            goto read_character;
        }
        --frontend_bss.source_cursor;
        goto select_arenas;
    case SC_NEWLINE:
        --frontend_bss.source_cursor;
        return LEXER_PROTOTYPE(1);
    default:
        compiler_warning(19, character);
        discard_input_character();
        return LEXER_PROTOTYPE(1);
    }
select_arenas:
    if ((frontend_data.compiler_mode_flags & 0x32000000) ||
        ((frontend_data.compiler_mode_flags & 0x40000000) &&
         !(frontend_data.source_mode_flags & 8) && !(LEXER_FLAGS & 0x200)) ||
        (frontend_data.source_mode_flags & 0x10) ||
        ((frontend_data.compiler_mode_flags & 0x04000000) && !(LEXER_FLAGS & 0x8000)))
        arena_kind = 0;
    else arena_kind = (frontend_data.source_mode_flags & 1) ? 4 : 5;
    frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = arena_kind;
    directive = (BYTE)lookup_preprocessor_directive(name);
#ifdef RECON_NATIVE_TEST
    if (directive != PP_PRAGMA)
        recon_unsupported("directive: conditional/recorded-token domains pending");
#endif
dispatch_directive:
    switch (directive) {
    case PP_IFDEF:
    case PP_IFNDEF:
        if (!frontend_data.source_include_depth && frontend_bss.forced_source_index) {
            begin_conditional_replay();
            if (read_conditional_replay() == 1) goto done;
            if (frontend_bss.forced_source_index == 2) goto replay_defined;
        }
        character = *frontend_bss.source_cursor++;
        if (!(frontend_data.scanner.flags[character] & 0x20)) {
            while (frontend_data.scanner.classes[character] == SC_SPACE)
                character = *frontend_bss.source_cursor++;
            if (!(frontend_data.scanner.flags[character] & 0x20))
                character = (BYTE)read_directive_character(character);
        }
        if (!(frontend_data.scanner.flags[character] & 1)) report_compiler_error(16);
        scan_identifier(character);
        condition = frontend_data.current_identifier->macro != NULL;
        finish_directive_line(directive == PP_IFDEF ? "#ifdef" : "#ifndef");
        if (directive == PP_IFNDEF) condition = !condition;
        if (!frontend_data.source_include_depth && frontend_bss.forced_source_index) {
replay_defined:
            condition = record_conditional_value((short)condition);
        }
        if (!condition) {
            skipped = skip_conditional_group(5);
            if (skipped != 3) {
                if (skipped == 2) goto elif_directive;
                goto done;
            }
        }
        if (!frontend_bss.debug_module->Slot28())
            report_compiler_diagnostic(73, ".\\p0prepro.c", 0x253);
        conditional_state = condition ? 1 : 3;
        conditional_states[push_conditional_state()] = conditional_state;
        goto done;

    case PP_IF:
        if (!frontend_data.source_include_depth && frontend_bss.forced_source_index) {
            begin_conditional_replay();
            if (read_conditional_replay() == 1) goto done;
            if (frontend_bss.forced_source_index == 2) goto replay_condition;
        }
        saved_output = frontend_bss.options.preprocess_stdout;
        LEXER_FLAGS |= 0x4000;
        frontend_bss.options.preprocess_stdout = 0;
        condition = evaluate_preprocessor_condition();
        LEXER_FLAGS &= ~0x4000U;
        frontend_bss.options.preprocess_stdout = saved_output;
        finish_directive_line("#if/#elif");
        if (!frontend_data.source_include_depth && frontend_bss.forced_source_index) {
replay_condition:
            condition = record_conditional_value((short)condition);
        }
        if (!condition) {
            skipped = skip_conditional_group(5);
            if (skipped != 3) {
                if (!frontend_data.options.preprocess_no_lines) begin_line_output();
                if (skipped == 2) goto elif_directive;
                goto done;
            }
        }
        if (!frontend_bss.debug_module->Slot28())
            report_compiler_diagnostic(73, ".\\p0prepro.c", 0x2a7);
        conditional_states[push_conditional_state()] = condition ? 1 : 3;
        if (!frontend_data.options.preprocess_no_lines && !condition) begin_line_output();
        goto done;

    case PP_ELIF:
        if (frontend_data.conditional_depth-- < 0) report_compiler_error(18);
        if (!frontend_bss.debug_module->Slot2c())
            report_compiler_diagnostic(73, ".\\p0prepro.c", 0x2d7);
        skipped = skip_conditional_group(5);
        if (skipped != 4) {
            do { skipped = skip_conditional_group(5); } while (skipped != 4);
        }
        if (!frontend_data.options.preprocess_no_lines) begin_line_output();
        goto done;
    case PP_ELSE:
        if (frontend_data.conditional_depth < 0 ||
            conditional_states[frontend_data.conditional_depth--] != 1)
            report_compiler_error(19);
        if (!frontend_bss.debug_module->Slot2c())
            report_compiler_diagnostic(73, ".\\p0prepro.c", 0x2ea);
        finish_directive_line("#else");
        skip_conditional_group(6);
        if (!frontend_data.options.preprocess_no_lines) begin_line_output();
        goto done;
    case PP_ENDIF:
        if (!frontend_bss.debug_module->Slot2c())
            report_compiler_diagnostic(73, ".\\p0prepro.c", 0x2f2);
        if (frontend_data.conditional_depth-- < 0) report_compiler_error(20);
        if (!frontend_data.options.preprocess_no_lines) begin_line_output();
        if (!frontend_data.options.language_extensions) finish_directive_line("#endif");
        else discard_input_character();
        goto done;
    case PP_DEFINE:
        process_define_directive(1);
        goto done;
    case PP_INCLUDE:
        filename = parse_include_directive(1, &include_flags);
        if (!filename) goto done;
        return include_directive_file(filename, include_flags);
    case PP_PRAGMA:
        if (!frontend_data.source_include_depth && frontend_bss.forced_source_index) {
            begin_conditional_replay();
            if (read_conditional_replay() == 1) goto done;
        }
        return process_pragma_directive();
    case PP_UNDEF:
        character = *frontend_bss.source_cursor++;
        if (!(frontend_data.scanner.flags[character] & 0x20)) {
            while (frontend_data.scanner.classes[character] == SC_SPACE)
                character = *frontend_bss.source_cursor++;
            if (!(frontend_data.scanner.flags[character] & 0x20))
                character = (BYTE)read_directive_character(character);
        }
        if (frontend_data.scanner.flags[character] & 1) {
            scan_identifier(character);
            process_undef_directive();
        } else report_language_diagnostic(1, 6);
        finish_directive_line("#undef");
        goto done;
    case PP_LINE:
        if (!frontend_data.source_include_depth && frontend_bss.forced_source_index)
            return LEXER_PROTOTYPE(3);
        return process_line_directive();
    case PP_UNKNOWN:
        report_compiler_diagnostic(21, name);
        goto done;
    case PP_ERROR:
        text = floating_token_text;
        for (;;) {
            character = *frontend_bss.source_cursor++;
            dispatch = frontend_data.scanner.classes[character];
            if (dispatch == SC_NEWLINE) break;
            if (dispatch == SC_SPECIAL) {
                if (process_special_character() & ~0x50U) continue;
            } else if (dispatch == SC_LEAD) {
                *text++ = character;
                advance_multibyte_source();
            }
            *text++ = character;
        }
        --frontend_bss.source_cursor;
        *text = 0;
        report_compiler_diagnostic(189, floating_token_text);
        finish_directive_line("#error");
        goto done;
    case PP_IDENT:
        arena_kind = frontend_bss.symbol_arena_kind;
        saved_output = frontend_bss.options.preprocess_stdout;
        frontend_bss.options.preprocess_stdout = 0;
        new_stream = (CompilerRecordedLexer *)frontend_bss.arenas[arena_kind].Allocate(64);
        if (new_stream) new(new_stream) CompilerRecordedLexer(
            frontend_bss.saved_context_stack.origin, frontend_bss.saved_context_stack.position, arena_kind);
        frontend_bss.context_stack.Push((CompilerLexer *)new_stream, arena_kind, 0, 0,
                                       &frontend_bss.saved_context_stack);
        arena_kind = frontend_bss.symbol_arena_kind;
        if (arena_kind) token = (ParserTokenView *)frontend_bss.arenas[arena_kind].Allocate(48);
        else {
            pool = &frontend_bss.pools[7];
            if (pool->remaining < 48) pool->Refill(48);
            token = (ParserTokenView *)pool->cursor;
            pool->cursor += 48; pool->remaining -= 48;
        }
        if (token) { token->kind = 47; memcpy(&token->value, &LEXER_SAVED_STORAGE, 40); }
        ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)token);
        do {
            token = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
            ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)token);
        } while (token->kind != 45);
        frame = frontend_bss.context_stack.top;
        if (frame) {
            recorded = (CompilerTokenStream *)frame->object;
            frontend_bss.context_stack.top = frame->previous;
            frontend_bss.context_stack.origin = frame->saved_origin;
            frontend_bss.context_stack.position = frame->saved_position;
            if (frame->callback) frame->callback();
        } else recorded = NULL;
        LEXER_SAVED_POINTER = recorded;
        if ((frontend_data.compiler_mode_flags & 0x32000000) ||
            ((frontend_data.compiler_mode_flags & 0x40000000) &&
             !(frontend_data.source_mode_flags & 8) && !(LEXER_FLAGS & 0x200)) ||
            (frontend_data.source_mode_flags & 0x10) ||
            ((frontend_data.compiler_mode_flags & 0x04000000) && !(LEXER_FLAGS & 0x8000)))
            arena_kind = 0;
        else arena_kind = (frontend_data.source_mode_flags & 1) ? 4 : 5;
        frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = arena_kind;
        frontend_bss.options.preprocess_stdout = saved_output;
        if (saved_output) goto done;
        LEXER_ALLOCATE_TOKEN();
        if (token) { token->kind = 0xf2; memcpy(&token->value, &LEXER_SAVED_STORAGE, 40); }
        return token;
    case PP_IMPORT:
        filename = parse_import_directive(&import_name, &include_flags);
        if (!filename) goto done;
        filename = resolve_import_directive(filename, import_name, include_flags);
        if (!filename) goto done;
        return include_directive_file(filename, 0);
    }
    goto done;

elif_directive:
    name = "if";
    directive = (BYTE)lookup_preprocessor_directive("if");
    goto dispatch_directive;
done:
    if ((frontend_data.compiler_mode_flags & 0x32000000) ||
        ((frontend_data.compiler_mode_flags & 0x40000000) &&
         !(frontend_data.source_mode_flags & 8) && !(LEXER_FLAGS & 0x200)) ||
        (frontend_data.source_mode_flags & 0x10) ||
        ((frontend_data.compiler_mode_flags & 0x04000000) && !(LEXER_FLAGS & 0x8000)))
        arena_kind = 0;
    else arena_kind = (frontend_data.source_mode_flags & 1) ? 4 : 5;
    frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = arena_kind;
    return LEXER_PROTOTYPE(1);
}

/* Scanning uses the same common16-byte name header and buckets as generic
 * lookup, but computes its key while advancing the borrowed input cursor.
 * Delimiters remain in the source; escaped/refill spans have a real child. */
extern "C" int __cdecl scan_identifier_span(const BYTE *, unsigned, unsigned); /*f241*/
extern "C" int __cdecl scan_identifier(unsigned character)
{
    const BYTE *start = frontend_bss.source_cursor - 1;
    unsigned hash = 0, folded;
    int length;
    CompilerNameEntry **bucket, **link, *entry;
    CompilerPool *pool = &frontend_bss.pools[5]; /*actual c0560, not macro pool6*/
    CompilerArena *arena = &frontend_bss.arenas[0];
    character &= 0xff;
    while (frontend_data.scanner.flags[character] & 3) {
        hash = (hash >> 4) + hash * 4 + character;
        character = *frontend_bss.source_cursor++;
    }
    if (frontend_data.scanner.classes[character] == SC_SPECIAL ||
        (character == 4 && lexical_skip_mode > 0))
        return scan_identifier_span(start, hash, character);
    --frontend_bss.source_cursor;
    length = frontend_bss.source_cursor - start;
    if (length >= 2048) report_compiler_error(64);
    folded = ((hash >> 16) ^ hash) & 0xffff;
    bucket = &frontend_data.keyword_table->buckets[hash & 0x7ff];
    link = bucket;
    for (entry = *link; entry; link = &entry->next, entry = *link) {
        if ((entry->flags & 0xffff) == folded && entry->name[length] == 0 &&
            memcmp(start, entry->name, length) == 0) {
            *link = entry->next;
            entry->next = *bucket;
            *bucket = entry;
            frontend_data.current_identifier = entry;
            return 0;
        }
    }
    if (pool->remaining < 16) pool->Refill(16);
    entry = (CompilerNameEntry *)pool->cursor;
    pool->cursor += 16; pool->remaining -= 16;
    entry->next = *bucket;
    *bucket = entry;
    entry->name = (char *)arena->cursor;
    arena->cursor += (length + 4) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    memcpy(entry->name, start, length);
    entry->name[length] = 0;
    frontend_data.current_identifier = entry;
    entry->flags = (entry->flags & 0xffff0000) | folded;
    return 0;
}

/* Meaningful keyword/ID rows replace only this helper's letter-grouped
 * suffix lookup. This is not the adjacent image's compression or padding. */
static const struct PreprocessorDirectiveName {
    const char *name;
    BYTE directive;
} preprocessor_directive_names[] = {
    { "define", 6 },
    { "error", 12 },
    { "endif", 3 },
    { "else", 2 },
    { "elif", 1 },
    { "include", 7 },
    { "import", 14 },
    { "ifndef", 5 },
    { "ifdef", 4 },
    { "if", 0 },
    { "ident", 13 },
    { "line", 10 },
    { "pragma", 8 },
    { "undef", 9 },
};
extern "C" BYTE __cdecl lookup_preprocessor_directive(const char *name)
{
    const PreprocessorDirectiveName *entry;
    for (entry = preprocessor_directive_names;
         entry != preprocessor_directive_names + 14; ++entry) {
        if (name[0] == entry->name[0] && strcmp(name + 1, entry->name + 1) == 0)
            return entry->directive;
    }
    return PP_UNKNOWN;
}



extern "C" {
ParserTokenView *__cdecl expect_directive_token(BYTE); /*1fe48*/
int __cdecl lookup_pragma_name(const char *); /*23a02*/
ParserTokenView *__cdecl preprocess_pragma_directive(void); /*81433*/
ParserTokenView *__cdecl process_implementation_key_pragma(void); /*81056*/
void __cdecl process_start_map_region_pragma(void); /*80fb5*/
void __cdecl process_include_alias_pragma(void); /*811a5*/
int __cdecl consume_pragma_end(void); /*205b4*/
}

/* Pragma arguments are tokenized through the real saved input interface.
 * Most pragmas are retained as a recorded token stream for the later parser;
 * the three immediate handlers keep their own pending function identities. */
extern "C" ParserTokenView *__cdecl process_pragma_directive(void)
{
    ParserTokenView *token, *marker;
    CompilerRecordedLexer *recorded;
    FrontendContextFrame *frame;
    CompilerArena *arena;
    CompilerPool *pool;
    unsigned arena_kind;
    int code;
    if (frontend_bss.options.preprocess_stdout)
        return preprocess_pragma_directive();
    if (suppress_parser_codegen) {
        discard_input_character();
        return LEXER_PROTOTYPE(1);
    }
    token = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
    if (token->kind == 45) return LEXER_PROTOTYPE(1);
    ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)token);
    token = expect_directive_token(6);
    if (!token) { consume_pragma_end(); return LEXER_PROTOTYPE(1); }
    code = lookup_pragma_name(((CompilerNameEntry *)token->value.pointer)->name);
    if (code == PRAGMA_INCLUDE_ALIAS) {
        process_include_alias_pragma();
        consume_pragma_end();
        return LEXER_PROTOTYPE(1);
    }
    if (code == PRAGMA_START_MAP_REGION) {
        process_start_map_region_pragma();
        return LEXER_PROTOTYPE(1);
    }
    if (code == PRAGMA_IMPLEMENTATION_KEY) return process_implementation_key_pragma();
    recorded = (CompilerRecordedLexer *)frontend_bss.arenas[1].Allocate(64);
    if (recorded) recorded = recorded->Initialize(1, 0);
    frontend_bss.context_stack.Push((CompilerLexer *)recorded, 1, 0, 0,
                                  &frontend_bss.saved_context_stack);
    arena = &frontend_bss.arenas[1];
    marker = (ParserTokenView *)arena->cursor;
    arena->cursor += 48;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (marker) { marker->kind = (BYTE)code; *(BYTE *)&marker->value = (BYTE)code; }
    ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)marker);
    LEXER_FLAGS = (LEXER_FLAGS & ~0x2000U) | (code == PRAGMA_HDRSTOP ? 0x2000U : 0);
    do {
        token = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
        ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)token);
    } while (token->kind != 45);
    LEXER_FLAGS &= ~0x2000U;
    frame = frontend_bss.context_stack.top;
    if (frame) {
        recorded = (CompilerRecordedLexer *)frame->object;
        frontend_bss.context_stack.top = frame->previous;
        frontend_bss.context_stack.origin = frame->saved_origin;
        frontend_bss.context_stack.position = frame->saved_position;
        if (frame->callback) frame->callback();
    } else recorded = NULL;
    LEXER_SAVED_POINTER = recorded;
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind) {
        arena = &frontend_bss.arenas[arena_kind];
        token = (ParserTokenView *)arena->cursor;
        arena->cursor += 48;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &frontend_bss.pools[7];
        if (pool->remaining < 48) pool->Refill(48);
        token = (ParserTokenView *)pool->cursor;
        pool->cursor += 48; pool->remaining -= 48;
    }
    if (token) { token->kind = 0xf2; memcpy(&token->value, &LEXER_SAVED_STORAGE, 40); }
    return token;
}

extern "C" void __cdecl skip_to_directive_newline(ParserTokenView *); /*810a7*/
extern "C" ParserTokenView *__cdecl expect_directive_token(BYTE kind)
{
    ParserTokenView *token = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
    if (token->kind == kind) return token;
    if (token->kind == 6)
        report_language_diagnostic(1, 83, kind, token->value.pointer);
    else report_language_diagnostic(1, 81, kind, token->kind);
    skip_to_directive_newline(token);
    return NULL;
}

/* The helper's complete49 name/code rows. Source defines the meaningful
 * names, without preserving the image's first-letter/suffix compression. */
static const struct PragmaName {
    const char *name;
    int code;
} pragma_names[] = {
    { "auto_inline", PRAGMA_AUTO_INLINE },
    { "alloc_text", PRAGMA_ALLOC_TEXT },
    { "bss_seg", PRAGMA_BSS_SEG },
    { "const_seg", PRAGMA_CONST_SEG },
    { "component", PRAGMA_COMPONENT },
    { "comment", PRAGMA_COMMENT },
    { "code_seg", PRAGMA_CODE_SEG },
    { "check_stack", PRAGMA_CHECK_STACK },
    { "check_pointer", PRAGMA_CHECK_POINTER },
    { "data_seg", PRAGMA_DATA_SEG },
    { "function", PRAGMA_FUNCTION },
    { "hdrstop", PRAGMA_HDRSTOP },
    { "intrinsic", PRAGMA_INTRINSIC },
    { "inline_recursion", PRAGMA_INLINE_RECURSION },
    { "inline_depth", PRAGMA_INLINE_DEPTH },
    { "init_seg", PRAGMA_INIT_SEG },
    { "include_alias", PRAGMA_INCLUDE_ALIAS },
    { "implementation_key", PRAGMA_IMPLEMENTATION_KEY },
    { "loop_opt", PRAGMA_LOOP_OPT },
    { "linesize", PRAGMA_LINESIZE },
    { "message", PRAGMA_MESSAGE },
    { "native_caller", PRAGMA_NATIVE_CALLER },
    { "options", PRAGMA_OPTIONS },
    { "optimize", PRAGMA_OPTIMIZE },
    { "once", PRAGMA_ONCE },
    { "push", PRAGMA_PUSH },
    { "procname", PRAGMA_PROCNAME },
    { "pop", PRAGMA_POP },
    { "pointers_to_members", PRAGMA_POINTERS_TO_MEMBERS },
    { "plmn", PRAGMA_PLMN },
    { "plmf", PRAGMA_PLMF },
    { "parameter", PRAGMA_PARAMETER },
    { "pagesize", PRAGMA_PAGESIZE },
    { "page", PRAGMA_PAGE },
    { "pack", PRAGMA_PACK },
    { "switch_check", PRAGMA_SWITCH_CHECK },
    { "subtitle", PRAGMA_SUBTITLE },
    { "string_pooling", PRAGMA_STRING_POOLING },
    { "stop_map_region", PRAGMA_STOP_MAP_REGION },
    { "start_map_region", PRAGMA_START_MAP_REGION },
    { "skipping", PRAGMA_SKIPPING },
    { "skip", PRAGMA_SKIP },
    { "setlocale", PRAGMA_SETLOCALE },
    { "segment", PRAGMA_SEGMENT },
    { "search_lib", PRAGMA_SEARCH_LIB },
    { "same_seg", PRAGMA_SAME_SEG },
    { "title", PRAGMA_TITLE },
    { "vtordisp", PRAGMA_VTORDISP },
    { "warning", PRAGMA_WARNING },
};
extern "C" int __cdecl lookup_pragma_name(const char *name)
{
    const PragmaName *entry;
    for (entry = pragma_names; entry != pragma_names + 49; ++entry) {
        if (name[0] == entry->name[0] && strcmp(name + 1, entry->name + 1) == 0)
            return entry->code;
    }
    return PRAGMA_UNKNOWN;
}

/* The actual outlined64-byte recorded-input constructor. The existing
 * forced-inline source constructor materializes the genuine C++ vptr and
 * three context-buffer headers without a raw vtable or helper vertex. */
CompilerRecordedLexer *RECON_THISCALL CompilerRecordedLexer::Initialize(unsigned arena, BYTE initial_active)
{
    new(this) CompilerRecordedLexer(frontend_bss.saved_context_stack.origin,
        frontend_bss.saved_context_stack.position, arena);
    active = initial_active;
    return this;
}
/* Outlined constructor-shaped copy. The original installs the same
 * recorded-input vtable and copies only the named header and two flag bits;
 * the remaining BYTE bits and trailing storage stay untouched. */
CompilerRecordedLexer *RECON_THISCALL CompilerRecordedLexer::CopyFrom(const CompilerRecordedLexer *source)
{
    new(this) CompilerRecordedLexer(source);
    return this;
}

/* Append a token identity to the stream's linked128-byte context blocks.
 * The initial position4 and block link are written by Initialize. Only the
 * published token slots are meaningful; other block words are not cleared. */
void RECON_THISCALL CompilerRecordedLexer::Trace(unsigned token)
{
    DWORD *block = write_buffer.block, *next;
    CompilerArena *arena;
#ifdef RECON_NATIVE_TEST
    if (!block || write_buffer.position < 4 || write_buffer.position > 31)
        recon_unsupported("recorded lexer: unsupported write cursor");
#endif
    if (write_buffer.position == 31) {
        if (!block[0]) {
            if (write_buffer.arena_kind == 9) {
#ifdef RECON_NATIVE_TEST
                recon_unsupported("recorded lexer: exhausted external context block");
#endif
                write_buffer.position = 31;
                block = NULL;
                goto write_token;
            }
#ifdef RECON_NATIVE_TEST
            if (write_buffer.arena_kind >= 9)
                recon_unsupported("recorded lexer: unsupported write arena");
#endif
            arena = &frontend_bss.arenas[write_buffer.arena_kind];
            next = (DWORD *)arena->cursor;
            arena->cursor += 128;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            if (next) next[0] = 0;
            block[0] = (DWORD)next;
        }
        write_buffer.position = 4;
        block = (DWORD *)block[0];
    }
write_token:
    write_buffer.block = block;
    block[write_buffer.position + 1] = token;
    ++write_buffer.position;
}

/* A pragma marker owns a recorded stream. Reset it, push that stream on the
 * real saved-context stack, dispatch the pragma, then restore the old frame. */
extern "C" ParserTokenView *__cdecl dispatch_recorded_pragma(void); /*1fed1*/
extern "C" ParserTokenView *__cdecl process_context_token(ParserTokenView *token)
{
    CompilerTokenStream *recorded = (CompilerTokenStream *)token->value.pointer;
    ParserTokenView *result;
    FrontendContextFrame *frame;
    recorded->Reset();
    frontend_bss.saved_context_stack.Push((CompilerLexer *)recorded, 9, 0, 0,
                                        &frontend_bss.saved_context_stack);
    result = dispatch_recorded_pragma();
    consume_pragma_end();
    frame = frontend_bss.saved_context_stack.top;
    if (frame) {
        frontend_bss.saved_context_stack.top = frame->previous;
        frontend_bss.saved_context_stack.origin = frame->saved_origin;
        frontend_bss.saved_context_stack.position = frame->saved_position;
        if (frame->callback) frame->callback();
    }
    return result;
}

void RECON_THISCALL CompilerRecordedLexer::Reset(void)
{
    read_buffer = buffer;
}

/* The cursor points at the last returned slot. A full block can gain a next
 * link later; an exhausted read therefore retains its current block owner. */
unsigned RECON_THISCALL CompilerRecordedLexer::Read(void)
{
    DWORD *block = read_buffer.block;
#ifdef RECON_NATIVE_TEST
    if (read_buffer.position < 4 || read_buffer.position > 31 ||
        (read_buffer.position == 31 && !block))
        recon_unsupported("recorded lexer: unsupported read cursor");
#endif
    if (read_buffer.position == 31) {
        if (!block[0]) {
            read_buffer.position = 31;
            block = NULL;
        } else {
            read_buffer.position = 4;
            block = (DWORD *)block[0];
        }
    }
    if (!block) return 0;
    read_buffer.block = block;
    unsigned position = read_buffer.position + 1;
    unsigned token = block[position];
    read_buffer.position = position;
    return token;
}

/* Rewind the last read slot when the token kind agrees. A differing kind is
 * replayed through a genuine one-token recorded stream on the context stack. */
void RECON_THISCALL CompilerRecordedLexer::Unread(unsigned token)
{
    DWORD *block = read_buffer.block;
    CompilerRecordedLexer *recorded;
    FrontendContextFrame *frame;
    unsigned arena_kind;
    if (!block || read_buffer.position < 5 || read_buffer.position > 31) {
#ifdef __linux__
        recon_unsupported("recorded lexer: unsupported unread cursor");
#else
        __asm { int 3 }
#endif
    }
    if (!token || !block[read_buffer.position] ||
        ((ParserTokenView *)token)->kind ==
        ((ParserTokenView *)block[read_buffer.position])->kind) {
        --read_buffer.position;
        block[read_buffer.position + 1] = token;
        return;
    }
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind >= 9) {
#ifdef __linux__
        recon_unsupported("recorded lexer: unsupported allocation arena");
#else
        __asm { int 3 }
#endif
    }
    recorded = (CompilerRecordedLexer *)frontend_bss.arenas[arena_kind].Allocate(64);
    if (recorded) recorded = recorded->Initialize(arena_kind, 1);
    frontend_bss.context_stack.Push((CompilerLexer *)recorded, arena_kind, 0, 0,
                                    &frontend_bss.saved_context_stack);
    ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace(token);
    ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace(
        (unsigned)frontend_data.canonical_token_records[243]);
    frame = frontend_bss.context_stack.top;
    if (frame) {
        recorded = (CompilerRecordedLexer *)frame->object;
        frontend_bss.context_stack.top = frame->previous;
        frontend_bss.context_stack.origin = frame->saved_origin;
        frontend_bss.context_stack.position = frame->saved_position;
        if (frame->callback) frame->callback();
    } else recorded = NULL;
    frontend_bss.saved_context_stack.Push((CompilerLexer *)recorded, 9, 1, 0,
                                         &frontend_bss.saved_context_stack);
}

extern "C" int __cdecl consume_pragma_end(void)
{
    if (expect_directive_token(45)) return 1;
    ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
    return 0;
}

unsigned RECON_THISCALL CompilerLexer::Peek(void)
{
    unsigned token = Read();
    Unread(token);
    return token;
}

/* The same context setter serves storage keywords and extended modifiers.
 * Only codes8/12/17 have a fourth incoming cdecl slot; ordinary storage
 * calls supply exactly three slots. Their actual declspec callers establish
 * a segment symbol, opaque metadata pointer and integer key respectively. */
extern "C" void __cdecl apply_declaration_specifier(
    CompilerDeclarationContext *context, BYTE storage, unsigned specifier, ...)
{
    BYTE modifier = (BYTE)specifier;
    BYTE reason = 1;
    CompilerSymbol *segment;
    CompilerDeclarationMetadata *metadata;
    unsigned key;
    va_list arguments;
#ifdef __linux__
    if (modifier != 1)
        recon_unsupported("declaration specifier: wider modifier domain pending");
#endif
    if (!(context->class_flags & 7))
        context->class_flags = (context->class_flags & ~7) | (storage & 7);
    else if (storage) compiler_warning(159);
    switch (modifier) {
    case 3: case 14:
        if (context->context_flags & 1) reason = 14;
        else context->context_flags |= 1;
        break;
    case 13:
        frontend_bss.debug_module->Slota8(frontend_bss.saved_context_stack.position,
                                          (unsigned)lexical_skip_mode);
        if (context->context_flags & 4) reason = 13;
        else context->context_flags |= 4;
        break;
    case 15:
        if (context->context_flags & 2) reason = 15;
        else context->context_flags |= 2;
        break;
    case 229:
        if (context->reference_flags & 4) reason = 229;
        else context->reference_flags |= 4;
        break;
    case 230:
        if (context->reference_flags & 8) reason = 230;
        else context->reference_flags |= 8;
        break;
    case 4:
        frontend_bss.debug_module->Slotb4();
        if (context->member_flags & 1) reason = 6;
        else context->member_flags |= 1;
        break;
    case 5:
        frontend_bss.debug_module->Slotb4();
        if (context->member_flags & 2) reason = 6;
        else context->member_flags |= 2;
        break;
    case 6:
        frontend_bss.debug_module->Slotb4();
        if (context->member_flags & 0xc) reason = 6;
        else context->member_flags |= 8;
        break;
    case 7:
        frontend_bss.debug_module->Slotb4();
        if (context->member_flags & 0xc) reason = 6;
        else context->member_flags |= 4;
        break;
    case 8:
        va_start(arguments, specifier);
        segment = va_arg(arguments, CompilerSymbol *);
        va_end(arguments);
        if (context->reference_flags & 1) reason = 6;
        else if (segment) {
            context->reference_flags |= 1;
            context->reference = segment;
        } else {
            context->reference_flags &= ~1;
            context->reference = NULL;
        }
        break;
    case 9:
        frontend_bss.debug_module->Slotb4();
        if (context->reference_flags & 0x10) reason = 6;
        else context->reference_flags |= 0x10;
        break;
    case 10:
        frontend_bss.debug_module->Slotb4();
        if (context->member_flags & 0x40) reason = 6;
        else {
            context->member_flags |= 0x40;
            context->class_flags |= 0x40;
        }
        break;
    case 12:
        va_start(arguments, specifier);
        metadata = va_arg(arguments, CompilerDeclarationMetadata *);
        va_end(arguments);
        if (context->context_flags & 0x40) reason = 6;
        else if (metadata) {
            context->context_flags |= 0x40;
            context->metadata = metadata;
        } else {
            context->context_flags &= ~0x40;
            context->metadata = NULL;
        }
        break;
    case 17:
        va_start(arguments, specifier);
        key = va_arg(arguments, unsigned);
        va_end(arguments);
        if (context->implementation_key_active) reason = 6;
        else {
            context->implementation_key_active = key != 0;
            context->implementation_key = key;
        }
        break;
    case 18:
        if (context->class_context) reason = 6;
        else context->class_context = 1;
        break;
    }
    if (reason == 1) return;
    if (modifier == 8) report_language_diagnostic(1, 141, "allocate");
    else if (modifier == 12) report_language_diagnostic(1, 141, "uuid");
    else if (modifier == 17) report_language_diagnostic(1, 141, "implementation_key");
    else if (reason == 6)
        report_language_diagnostic(1, 141, frontend_data.current_identifier->name);
    else {
        /* Only these real duplicate branches select the diagnostic table. */
        const char *name;
        if (reason == 13) name = "friend";
        else if (reason == 14) name = "inline";
        else if (reason == 15) name = "virtual";
        else if (reason == 229) name = "mutable";
        else name = "explicit"; /* the remaining branch sets230 */
        report_language_diagnostic(1, 141, name);
    }
}
