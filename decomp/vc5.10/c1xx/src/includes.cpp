/* Include spelling is read through the current source-context interface.
 * Quoted and angle-bracket forms retain their separate parser identities. */
#include "parser-state.h"
#include "lexer.h"
#include "diagnostics.h"
#include "includes.h"
#include <string.h>
#include <mbctype.h>
#include <io.h>
#include "preprocessor.h"
#include "frontend-init.h"
#include <recon.h>
#pragma intrinsic(strlen, memcpy)
#if defined(_MSC_VER) && defined(_M_IX86) && defined(_DLL)
#undef _mbctype
extern "C" { _CRTIMP extern unsigned char _mbctype[]; }
#endif

extern "C" {
char *__cdecl parse_angle_include(int);
int __cdecl open_forced_source(const char *, unsigned);
ParserTokenView *__cdecl record_source_location(const void *);
unsigned __cdecl make_arena_token(unsigned);
void __cdecl discard_input_character(void);
unsigned __cdecl read_directive_character(unsigned);
}

extern "C" char *__cdecl parse_include_directive(int diagnose, unsigned *kind)
{
    ParserTokenView *token;
    char *name;
    int saved_output = frontend_bss.options.preprocess_stdout;
    frontend_data.compiler_flags |= 0x2000;
    frontend_bss.options.preprocess_stdout = 0;
    token = ((ParserFrontendContextView *)&frontend_bss.saved_context_stack)->ReadToken();
    frontend_data.compiler_flags &= ~0x2000U;
    frontend_bss.options.preprocess_stdout = saved_output;
    if (token->kind == 7) {
        if ((token->value.quoted.flags & 1) && diagnose)
            compiler_warning(2);
        if (kind) *kind = 0;
        return (char *)token->value.quoted.text;
    }
    if (token->kind == 68) {
        name = parse_angle_include(diagnose);
        if (name) {
            if (kind) *kind = 1;
            return name;
        }
    } else if (diagnose) {
        /* Only the established spelling rows may be indexed. */
        if (token->kind >= 246 || !compiler_token_spellings[token->kind]) {
            __asm { int 3 }
        }
        compiler_warning(6, compiler_token_spellings[token->kind]);
    }
    discard_input_character();
    if (diagnose) {
        unsigned mode = frontend_data.compiler_mode_flags;
        unsigned source = frontend_data.source_mode_flags;
        unsigned flags = frontend_data.compiler_flags;
        unsigned arena;
        if ((mode & 0x32000000) ||
            ((mode & 0x40000000) && !(source & 8) && !(flags & 0x200)) ||
            (source & 0x10) || ((mode & 0x04000000) && !(flags & 0x8000)))
            arena = 0;
        else arena = (source & 1) ? 4 : 5;
        frontend_data.tagged_type_arena_index = frontend_data.constant_arena_index = arena;
    }
    return NULL;
}

extern "C" char *__cdecl parse_angle_include(int) { __asm { int 3 } return NULL; }

extern "C" ParserTokenView *__cdecl include_directive_file(const char *name, unsigned kind)
{
    ParserTokenView *token;
    if (open_forced_source(name, kind) <= -1)
        return (ParserTokenView *)frontend_data.canonical_token_records[0];
    token = record_source_location(frontend_bss.saved_context_stack.origin);
    if (frontend_data.compiler_mode_flags & 0x04000000) {
        process_context_token(token);
        token = (ParserTokenView *)make_arena_token(0);
    }
    return token;
}

extern "C" {
void __cdecl finish_directive_line(const char *);
int __cdecl compare_pch_paths(const char *, const char *);
unsigned __cdecl classify_pch_include(const char *);
int __cdecl search_relative_include(const char *, int, char *);
int __cdecl search_include_directories(const char *, int, char *);
int __cdecl open_source_file(const char *, unsigned, int, int, int);
void __cdecl report_source_file_error(const char *, int);
int __cdecl report_response_error(int, ...);
}

extern "C" int __cdecl open_forced_source(const char *name, unsigned kind)
{
    int descriptor;
    unsigned pch_kind;
    const char *alias, *separators;
    if (!frontend_data.source_include_depth) {
        if (frontend_data.options.pch_create_file &&
            compare_pch_paths(frontend_data.options.pch_create_file, name))
            frontend_data.pch_creation_name_seen = 1;
        if (frontend_bss.forced_source_index) {
            pch_kind = classify_pch_include(name);
            if (pch_kind == 1) return -4;
            if (pch_kind == 2 && frontend_bss.forced_source_index != 1) {
                finish_directive_line("#include");
                return -4;
            }
        }
    }
    if (!(frontend_data.source_mode_flags & 0x01000000))
        finish_directive_line("#include");
    alias = frontend_data.options.include_aliases->Find(name, &kind);
    if (alias) name = alias;
    separators = frontend_data.options.path_separators;
    if (strchr(separators, (BYTE)name[0]) ||
        (strchr(separators, ':') && !(_mbctype[(BYTE)name[0] + 1] & 4) && name[1] == ':')) {
        descriptor = open_source_file(name, 0, 0, 1, 1);
        if (descriptor == -1) {
            report_response_error(83, 343, name);
            return descriptor;
        }
        if (descriptor > -1 && (frontend_bss.forced_source_index == 1 || frontend_bss.pch_file_failed))
            report_source_file_error(name, descriptor);
        return descriptor;
    }
    if (!kind) {
        descriptor = search_relative_include(name, 1, NULL);
        if (descriptor != -1) return descriptor;
    }
    return search_include_directories(name, 1, NULL);
}

const char *RECON_THISCALL CompilerIncludeAliases::Find(const char *name, unsigned *kind)
{
    CompilerIncludeAlias *alias;
    for (alias = head; alias; alias = alias->next) {
        if (!_stricmp(alias->name, name) && alias->kind == *kind) {
            *kind = alias->replacement_kind;
            return alias->replacement;
        }
    }
    return NULL;
}
extern "C" unsigned __cdecl classify_pch_include(const char *)
{ __asm { int 3 } return 0; }
/* Search the active source chain, nearest includer first. The length
 * check includes the original full source name, before dropping its suffix. */
extern "C" int __cdecl search_relative_include(const char *name, int open_file, char *resolved)
{
    char path[512];
    CompilerSourceFrame *frame = frontend_data.source_frames, *next;
    const char *source, *cursor, *separator;
    char *out;
    unsigned hash;
    int descriptor;
    do {
        source = frame->file->name->name;
        next = frame->next;
        separator = NULL;
        for (cursor = source; *cursor; ++cursor) {
            if (_mbctype[(BYTE)*cursor + 1] & 4) ++cursor;
            else if (strchr(frontend_data.options.path_separators, (BYTE)*cursor))
                separator = cursor;
        }
        if (strlen(name) + strlen(source) + 2 >= sizeof(path))
            report_compiler_error(64);
        hash = 0;
        out = path;
        while (separator && source <= separator) {
            *out = *source++;
            hash = (hash >> 4) + hash * 4 + (BYTE)*out++;
        }
        source = name;
        while ((*out = *source++) != 0)
            hash = (hash >> 4) + hash * 4 + (BYTE)*out++;
        if (open_file) {
            descriptor = open_source_file(path, hash, 0, 1, 1);
            if (descriptor != -1) {
                if (descriptor > -1 &&
                    (frontend_bss.forced_source_index == 1 || frontend_bss.pch_file_failed))
                    report_source_file_error(path, descriptor);
                if (resolved) memcpy(resolved, path, strlen(path) + 1);
                return descriptor;
            }
        } else if (!_access(path, 0)) {
            if (resolved) memcpy(resolved, path, strlen(path) + 1);
            return 0;
        }
        frame = next;
    } while (frame);
    if (resolved) *resolved = 0;
    return -1;
}
extern "C" int __cdecl search_include_directories(const char *, int, char *)
{ __asm { int 3 } return -1; }

/* These spellings are also compared by address in the trailing-token rule. */
const char compiler_directive_else[] = "#else";
const char compiler_directive_endif[] = "#endif";

extern "C" void __cdecl finish_directive_line(const char *directive)
{
    BYTE character = *frontend_bss.source_cursor++;
    if (!(frontend_data.scanner.flags[character] & 0x20)) {
        while (frontend_data.scanner.classes[character] == SC_SPACE)
            character = *frontend_bss.source_cursor++;
        if (!(frontend_data.scanner.flags[character] & 0x20))
            character = (BYTE)read_directive_character(character);
    }
    if (character == '\n') {
        --frontend_bss.source_cursor;
        return;
    }
    if (!frontend_data.options.language_extensions ||
        (directive != compiler_directive_else && directive != compiler_directive_endif))
        report_language_diagnostic(1, 67, directive);
    discard_input_character();
}

/* Record a source-origin marker followed by the line terminator, then return
 * the stream in the real saved-token pointer arm. Opaque literal bytes retain
 * the authentic forty-byte transfers without interpreting their contents. */
extern "C" ParserTokenView *__cdecl record_source_location(const void *origin)
{
    unsigned arena_kind = frontend_bss.symbol_arena_kind;
    CompilerRecordedLexer *recorded;
    CompilerTokenStream *stream;
    CompilerArena *arena;
    CompilerPool *pool;
    FrontendContextFrame *frame;
    ParserTokenView *token;
    if (arena_kind >= 9) { __asm { int 3 } }
    recorded = (CompilerRecordedLexer *)frontend_bss.arenas[arena_kind].Allocate(64);
    if (recorded) recorded = recorded->Initialize(arena_kind, 0);
    frontend_bss.context_stack.Push(recorded, arena_kind, 0, NULL,
        &frontend_bss.saved_context_stack);
    frontend_bss.saved_token_value.pointer = (void *)origin;
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind >= 9) { __asm { int 3 } }
    if (arena_kind) token = (ParserTokenView *)frontend_bss.arenas[arena_kind].Allocate(48);
    else token = (ParserTokenView *)frontend_bss.pools[7].Allocate(48);
    if (token) {
        token->kind = 44;
        memcpy(&token->value, &frontend_bss.saved_token_value, 40);
    }
    ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace((unsigned)token);
    ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace(
        (unsigned)frontend_data.canonical_token_records[44]);
    frame = frontend_bss.context_stack.top;
    if (frame) {
        stream = (CompilerTokenStream *)frame->object;
        frontend_bss.context_stack.top = frame->previous;
        frontend_bss.context_stack.origin = frame->saved_origin;
        frontend_bss.context_stack.position = frame->saved_position;
        if (frame->callback) frame->callback();
    } else stream = NULL;
    frontend_bss.saved_token_value.pointer = stream;
    arena_kind = frontend_bss.symbol_arena_kind;
    if (arena_kind >= 9) { __asm { int 3 } }
    if (arena_kind) {
        arena = &frontend_bss.arenas[arena_kind];
        token = (ParserTokenView *)arena->cursor;
        arena->cursor += 48;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    } else {
        pool = &frontend_bss.pools[7];
        if (pool->remaining < 48) pool->Refill(48);
        token = (ParserTokenView *)pool->cursor;
        pool->cursor += 48;
        pool->remaining -= 48;
    }
    if (token) {
        token->kind = 0xf2;
        memcpy(&token->value, &frontend_bss.saved_token_value, 40);
    }
    return token;
}
