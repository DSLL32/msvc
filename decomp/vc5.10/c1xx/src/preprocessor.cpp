#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "frontend-init.h"
#include "preprocessor.h"
#include "parser-actions.h"
#include "lexer.h"
#include "global-output.h"
#include "packing.h"
#pragma intrinsic(strcpy, memcpy, strlen)
#include <recon.h>

/* Preserve the original direct FILE-array import on the NULL-name path. */
#if defined(_MSC_VER) && !defined(RECON_NATIVE_TEST)
#undef _iob
#undef stdin
extern "C" __declspec(dllimport) FILE _iob[];
#define stdin (&_iob[0])
#endif

extern "C" {
long __cdecl get_compilation_time(long *);
char *__cdecl format_predefined_time(long);
void __cdecl initialize_macro_buffers(void);
void __cdecl define_preprocessor_macro(const char *, unsigned, int);
void __cdecl define_command_line_macro(char *);
CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
void __cdecl write_global_header(void);
int __cdecl open_source_file(const char *, unsigned, int, int, int);
void __cdecl report_source_file_error(const char *, int);
}

extern "C" void __cdecl define_command_line_macros(CompilerVector *definitions)
{
    long stamp;
    short index;
    int file;
    frontend_data.scanner.classes[1] = SC_MARKER1;
    frontend_data.scanner.classes[2] = SC_MARKER2;
    frontend_data.scanner.classes[3] = SC_MARKER3;
    frontend_data.scanner.classes[4] = SC_MARKER4;
    if (frontend_data.options.language_extensions) {
        frontend_data.scanner.classes['$'] = SC_IDENTIFIER;
        frontend_data.scanner.flags['$'] |= 1;
    }
    if (frontend_data.options.reproducible) {
        frontend_bss.macro_date_time[0] = '1';
        frontend_bss.macro_date_time[1] = 0;
        frontend_bss.macro_date = frontend_bss.macro_time = frontend_bss.macro_date_time;
    } else {
        get_compilation_time(&stamp);
        strcpy(frontend_bss.macro_date_time, format_predefined_time(stamp));
        frontend_bss.macro_date = frontend_bss.macro_date_time + 4;
        frontend_bss.macro_date_time[15] = 0;
        frontend_bss.macro_time = frontend_bss.macro_date_time + 16;
        frontend_bss.macro_date_time[24] = 0;
    }
    initialize_macro_buffers();
    find_or_create_identifier("__LINE__");
    define_preprocessor_macro(NULL, 0, -4);
    find_or_create_identifier("__FILE__");
    define_preprocessor_macro(NULL, 0, -5);
    find_or_create_identifier("__DATE__");
    define_preprocessor_macro(NULL, 0, -6);
    find_or_create_identifier("__TIME__");
    define_preprocessor_macro(NULL, 0, -7);
    find_or_create_identifier("__TIMESTAMP__");
    define_preprocessor_macro(NULL, 0, -9);
    find_or_create_identifier("__cplusplus");
    define_preprocessor_macro(NULL, 0, -10);
    if (!frontend_data.options.no_bool) {
        find_or_create_identifier("__BOOL_DEFINED");
        define_preprocessor_macro(NULL, 0, -11);
    }
    for (index = 0; definitions && index < definitions->count; ++index)
        define_command_line_macro((char *)definitions->items[index]);
    if (!frontend_bss.options.preprocess_stdout && frontend_data.option_c9il &&
        !frontend_bss.options.prototypes_only)
        write_global_header();
    file = open_source_file(frontend_bss.options.source_file, 0, 1, 1, 0);
    if (file > -1 && frontend_bss.pch_file_failed)
        report_source_file_error(frontend_bss.options.source_file, file);
    frontend_bss.pending_forced_includes = frontend_bss.options.forced_includes;
    frontend_bss.forced_include_index = 0;
}

extern "C" long __cdecl get_compilation_time(long *stamp)
{
    return (long)time((time_t *)stamp);
}
extern "C" char *__cdecl format_predefined_time(long stamp)
{
    stamp &= 0x7fffffff;
    char *text = ctime((time_t *)&stamp);
    if (text) {
        /* ctime supplies 25 bytes including its newline. The initialized
         * trailing NUL belongs to this mutable buffer and is retained. */
        memcpy(frontend_data.predefined_time_text, text, 25);
        if (frontend_data.predefined_time_text[8] == '0')
            frontend_data.predefined_time_text[8] = ' ';
        memcpy(frontend_data.predefined_time_text + 11, text + 20, 4);
        memcpy(frontend_data.predefined_time_text + 16, text + 11, 4);
        memcpy(frontend_data.predefined_time_text + 20, text + 15, 4);
    }
    return frontend_data.predefined_time_text;
}
extern "C" void __cdecl initialize_macro_buffers(void)
{
    frontend_data.macro_reset_value = 0;
    frontend_bss.macro_token_cursor = frontend_bss.work_buffers[0].data;
    frontend_bss.macro_text_cursor = frontend_bss.work_buffers[1].data;
    frontend_bss.saved_macro_text_cursor = frontend_bss.work_buffers[1].data;
}
extern "C" void __cdecl record_macro_definition(const char *, const char *, int);
extern "C" int __cdecl compare_macro_text(const void *, const void *, unsigned);
extern "C" void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
extern "C" void __cdecl define_preprocessor_macro(const char *text, unsigned length, int kind)
{
    if (!length)
        text = NULL;
    CompilerNameEntry *identifier = frontend_data.current_identifier;
    if (identifier == frontend_data.builtin_names.defined_operator) {
        report_language_diagnostic(1, 117, identifier, "#define");
        return;
    }
    CompilerMacro *macro = identifier->macro;
    if (!frontend_data.source_include_depth && frontend_bss.forced_source_index) {
        record_macro_definition(identifier->name,
            length ? frontend_bss.current_macro_text : "", kind);
        identifier = frontend_data.current_identifier;
    }
    if (macro) {
        int previous_kind = macro->body->kind;
        if (previous_kind < -3) {
            report_language_diagnostic(1, 117, identifier, "#define");
            return;
        }
        int changed;
        if (kind != -3 && kind != previous_kind)
            changed = 1;
        else if (length)
            changed = compare_macro_text(text, macro->body->payload, length - 1);
        else
            changed = macro->body->payload[0] != 0;
        if (!changed)
            return;
        report_language_diagnostic(1, 5, identifier);
    }
    CompilerPool *pool = &frontend_bss.pools[6]; /* actual c04a8 */
    if (pool->remaining < 8)
        pool->Refill(8);
    identifier = frontend_data.current_identifier;
    macro = (CompilerMacro *)pool->cursor;
    pool->cursor += 8;
    pool->remaining -= 8;
    macro->identifier = identifier;
    CompilerArena *arena = &frontend_bss.arenas[0];
    CompilerMacroBody *body = (CompilerMacroBody *)arena->cursor;
    arena->cursor += (length + 11) & ~3U;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    macro->body = body;
    if (length)
        memcpy(body->payload, text, length);
    identifier = frontend_data.current_identifier;
    identifier->flags |= 0x80000;
    identifier->macro = macro;
    body->kind = (signed char)kind;
    frontend_data.backend->Slot28(macro);
}
extern "C" void __cdecl record_macro_definition(const char *name, const char *text, int kind)
{ __asm { int 3 } }
extern "C" int __cdecl compare_macro_text(const void *left, const void *right, unsigned bytes)
{ __asm { int 3 } return 0; }
extern "C" void __cdecl grow_work_buffer(BYTE *, BYTE *, BYTE **);
extern "C" void __cdecl define_command_line_macro(char *definition)
{
    char *source = definition;
    char *equals = NULL;
    BYTE *cursor = frontend_bss.work_buffers[1].data;
    BYTE *space_start = NULL;
    while (*source && *source != '=') {
        if (frontend_data.scanner.classes[(unsigned char)*source] == SC_LEAD)
            ++source;
        ++source;
    }
    if (*source == '=') {
        equals = source;
        *source++ = 0;
        while (*source) {
            if (cursor >= frontend_bss.work_buffers[1].limit)
                grow_work_buffer(cursor, frontend_bss.work_buffers[1].data,
                    &frontend_bss.work_buffers[1].limit);
            *cursor++ = *source;
            unsigned ch = (unsigned char)*source;
            if (frontend_data.scanner.classes[ch] == SC_LEAD) {
                *cursor++ = *++source;
                space_start = NULL;
            } else if ((isspace)(ch)) {
                if (!space_start)
                    space_start = cursor;
            } else {
                space_start = NULL;
            }
            ++source;
        }
        if (space_start)
            cursor = space_start;
    } else {
        if (cursor >= frontend_bss.work_buffers[1].limit)
            grow_work_buffer(cursor, cursor, &frontend_bss.work_buffers[1].limit);
        *cursor++ = '1';
    }
    find_or_create_identifier(definition);
    BYTE *base = frontend_bss.work_buffers[1].data;
    if (cursor == base) {
        define_preprocessor_macro(NULL, 0, 0);
    } else {
        if (cursor >= frontend_bss.work_buffers[1].limit)
            grow_work_buffer(cursor, base, &frontend_bss.work_buffers[1].limit);
        *cursor++ = 0;
        *cursor++ = 'D';
        define_preprocessor_macro((const char *)base, (unsigned)(cursor - base), 0);
    }
    if (equals)
        *equals = '=';
}
extern "C" void __cdecl grow_work_buffer(BYTE *cursor, BYTE *base, BYTE **limit)
{ __asm { int 3 } }
extern "C" void __cdecl reset_source_text_buffer(CompilerSourceTextBuffer *);
extern "C" int __cdecl open_source_descriptor(const char *);
extern "C" void __cdecl push_source_context(CompilerSourceFile *, int, int, int);
extern "C" void __cdecl report_include_file(int, const char *);
extern "C" int __cdecl report_response_error(int, ...);
extern "C" int __cdecl open_source_file(const char *name, unsigned hash,
    int kind, int flags, int track)
{
    CompilerSourceFile *file = frontend_data.translation_unit->FindSourceFile(name, hash);
    int descriptor;
    if (file->record_index & 0xff000000) return -3;
    if (kind == 2) {
        /* The builtin source writer supplies this same initialized union
         * cell; reset preserves its owned blocks before source input reads. */
        reset_source_text_buffer(frontend_bss.source_control.buffer);
        push_source_context(file, kind, frontend_data.source_descriptor, flags);
        return -2;
    }
    descriptor = name ? open_source_descriptor(name) : stdin->_file;
    if (descriptor == -1) {
        if (kind == 1) report_response_error(83, 341, name);
        return -1;
    }
    push_source_context(file, kind, descriptor, flags);
    if (name && track && frontend_bss.options.show_includes)
        report_include_file(frontend_data.source_include_depth, name);
    return descriptor;
}
extern "C" CompilerNameEntry *__cdecl intern_name_span(const char *, unsigned, unsigned, int *);
CompilerSourceFile *RECON_THISCALL CompilerTranslationUnit::FindSourceFile(const char *name, unsigned hash)
{
    unsigned length;
    int created;
    CompilerNameEntry *identifier;
    CompilerSourceFile *file;
    CompilerArena *arena = &frontend_bss.arenas[0]; /* actual c6500 */
    if (hash) length = strlen(name);
    else {
        const unsigned char *cursor = (const unsigned char *)name;
        while (*cursor) {
            hash = (hash >> 4) + hash * 4 + *cursor;
            ++cursor;
        }
        length = (unsigned)(cursor - (const unsigned char *)name);
    }
    if (length > 256) report_compiler_diagnostic(81, name, 256);
    identifier = intern_name_span(name, hash, length, &created);
    if (!created) {
        file = LookupSourceFile(identifier);
        if (file) return file;
    }
    file = (CompilerSourceFile *)arena->cursor;
    arena->cursor += 20;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (file) {
        file->name = identifier;
        file->kind = 21;
    }
    /* Untouched bytes retain their actual arena origin. In particular, no
     * invented initialization is assigned to the opaque bytes/reference state. */
    if (source_files) file->next = source_files;
    source_files = file;
    return file;
}
CompilerSourceFile *RECON_THISCALL CompilerTranslationUnit::LookupSourceFile(CompilerNameEntry *)
{ __asm { int 3 } return NULL; }
extern "C" const char *__cdecl prepare_source_path(const char *);
extern "C" int __cdecl close_old_source_descriptor(void);
extern "C" int __cdecl open_source_descriptor(const char *name)
{
    const char *path = prepare_source_path(name);
    int descriptor = -1;
    int attempt = 1;
    while (attempt < 2) {
        descriptor = _sopen(path, _O_BINARY, _SH_DENYWR);
        if (descriptor != -1 || errno != EMFILE) break;
        close_old_source_descriptor();
        ++attempt;
    }
    return descriptor;
}
extern "C" char *__cdecl duplicate_argument(const char *, int);
extern "C" const char *__cdecl prepare_source_path(const char *name)
{
    if (frontend_bss.options.fat) {
        char drive[4], directory[256], base[256], extension[256];
        char *path = duplicate_argument(name, 5);
        _splitpath(path, drive, directory, base, extension);
        base[8] = 0;
        extension[4] = 0;
        _makepath(path, drive, directory, base, extension);
        return path;
    }
    return name;
}
extern "C" int __cdecl close_old_source_descriptor(void) { __asm { int 3 } return 0; }
extern "C" CompilerBackend *__cdecl acquire_initial_backend(void);
extern "C" int __cdecl read_source_chunk(void);
extern "C" void __cdecl begin_line_output(void);
extern "C" void __cdecl push_source_context(CompilerSourceFile *file, int kind, int descriptor, int flags)
{
    CompilerArena *arena = &frontend_bss.arenas[1]; /* original c6520 */
    CompilerSourceFrame *frame = frontend_data.free_source_frames;
    if (frame) frontend_data.free_source_frames = frame->next;
    else {
        frame = (CompilerSourceFrame *)arena->cursor;
        arena->cursor += 48;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        BYTE *buffer = arena->cursor;
        arena->cursor += 0x7004;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        frame->buffer = buffer + 2;
    }
    frame->next = frontend_data.source_frames;
    frontend_data.source_frames = frame;
    if (++frontend_data.source_include_depth > 1024) {
        --frontend_data.source_include_depth;
        report_compiler_diagnostic(14, frontend_data.source_include_depth);
    }
    frontend_data.source_descriptor = descriptor;
    frame->conditional_depth = frontend_data.conditional_depth;
    frame->packing_alignment = frontend_data.options.packing_stack->top->alignment;
    if (!frontend_data.source_include_depth)
        frontend_data.root_source_frame = frontend_data.source_frames;
    frame->saved_cursor = frontend_bss.source_cursor;
    frame->saved_origin = frontend_bss.context_stack.origin;
    frame->saved_position = frontend_bss.context_stack.position;
    frame->descriptor = frontend_data.source_descriptor;
    frame->available_bytes = 0;
    frame->consumed_bytes = 0;
    frame->flags = (frame->flags & ~1U) | (kind == 2);
    frame->flags &= ~2U;
    frame->flags = (frame->flags & ~0x10U) | ((frontend_data.source_mode_flags >> 20) & 0x10);
    frontend_data.source_frames->flags &= ~8U;
    frame->flags = (frame->flags & ~4U) | ((flags & 1) << 2);
    if (!flags) {
        frontend_bss.lexer_backend->backend = frontend_data.backend;
        frontend_data.backend = acquire_initial_backend();
        frontend_bss.lexer_backend->enabled = flags;
    }
    frontend_bss.source_cursor = frame->buffer;
    frontend_bss.input->Begin(frontend_bss.text_input);
    frame->file = file;
    frontend_bss.context_stack.position = 0;
    frontend_bss.context_stack.origin = file;
    frontend_bss.source_cursor[-1] = '\n';
    ++frontend_bss.source_cursor;
    read_source_chunk();
    if (frontend_bss.debug_module) {
        char extension[256];
        int ignored_extension = 0;
        frontend_bss.text_input->Reset();
        if (!frontend_bss.debug_module->BeginSource(file->name->name, flags == 0))
            report_compiler_diagnostic(73, ".\\p0io.c", 288);
        if (frontend_data.options.incremental_rebuild || frontend_data.options.dependency_output) {
            _splitpath(frontend_bss.context_stack.FileName(), NULL, NULL, NULL, extension);
            if (*extension && strlen(extension) == 4 && !strncmp(extension, ".tl", 3))
                ignored_extension = 1;
            if (!ignored_extension) {
                frontend_data.dependency->RecordSource(frontend_bss.context_stack.FileName(), descriptor, kind != 2);
                if (frontend_data.options.incremental_rebuild) {
                    void *result = NULL;
                    if (frontend_data.dependency->QuerySource(&result) &&
                        !frontend_bss.debug_module->SetSourceDependency(result))
                        report_compiler_diagnostic(73, ".\\p0io.c", 325);
                }
            }
        }
    }
    if (!frontend_data.options.preprocess_no_lines) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("push_source_context: unmodeled line-output selector");
#else
        if (frontend_bss.preprocessor_output) begin_line_output();
#endif
    }
    --frontend_bss.source_cursor;
    *frontend_bss.source_cursor = '\n';
}
extern "C" int __cdecl read_pch_input(CompilerSourceTextBuffer *, BYTE *, unsigned);
extern "C" int __cdecl read_source_chunk(void)
{
    CompilerSourceFrame *frame = frontend_data.source_frames;
    BYTE *buffer = frame->buffer;
    int count;
    unsigned i;
    if (frontend_bss.source_cursor - buffer < frame->available_bytes) {
        if (frontend_bss.source_cursor[-1] == 0x1a) {
            --frontend_bss.source_cursor;
            return 1;
        }
        return 0;
    }
    --frontend_bss.source_cursor;
    frontend_bss.input->Flush();
    for (i = 1; i <= 2; ++i)
        buffer[-(int)i] = frontend_bss.source_cursor[-(int)i];
    frontend_bss.source_cursor = buffer;
    frontend_bss.input->Mark();
    if (frontend_data.source_frames->flags & 2) {
        *buffer = 0;
        frontend_data.source_frames->available_bytes = 0;
        if (!(frontend_data.source_frames->flags & 8)) {
            frontend_bss.input->Location((unsigned)buffer, 1);
            frontend_data.source_frames->flags |= 8;
        }
        return 1;
    }
    if (frontend_data.source_frames->flags & 1) {
        count = read_pch_input(frontend_bss.source_control.buffer, buffer, 0x7000);
    } else count = _read(frontend_data.source_descriptor, buffer, 0x7000);
    frame = frontend_data.source_frames;
    frame->consumed_bytes += frame->available_bytes;
    frontend_data.source_frames->available_bytes = count;
    if (count <= 0) {
        if (count < 0)
            report_response_error(84, frontend_data.source_include_depth ? 343 : 341,
                frontend_data.source_frames->file->name->name);
        frontend_data.source_frames->flags |= 2;
        if (frontend_data.options.language_extensions) *buffer++ = '\n';
        *buffer = 0;
        return 0;
    }
    if (count < 0x7000) {
        frontend_data.source_frames->flags |= 2;
        if (frontend_data.options.language_extensions &&
            !(frontend_data.source_frames->flags & 1) && buffer[count - 1] != '\n') {
            buffer[count++] = '\n';
            frontend_data.source_frames->available_bytes = count;
        }
    }
    buffer[count] = 0;
    return 0;
}

/* A view of the existing dependency record, not a second records owner. */
struct SourceDependencyRecordView {
    BYTE unmodeled_00_07[8];
    struct DependencyRecords *previous;
};
extern "C" int __cdecl restore_source_input(void)
{
    ParserTokenView *token;
    CompilerSourceFrame *frame;
    void *origin;
    unsigned position, consumed;
    const char *name;
    int result = 0, descriptor;
    if (!frontend_data.source_include_depth) {
        token = frontend_bss.pending_groups.ReadNextToken();
        if (token->kind != 1) {
            ((CompilerTokenStream *)frontend_bss.saved_context_stack.top->object)->Unread((unsigned)token);
            return 1000;
        }
    }
    if (frontend_data.source_include_depth == -1) return 0;
    if (frontend_data.source_include_depth > 0) {
        if (!frontend_bss.debug_module->Slot34(frontend_bss.saved_context_stack.position))
            report_compiler_diagnostic(73, ".\\p0io.c", 0x3b3);
    } else if (!frontend_bss.debug_module->Slot10())
        report_compiler_diagnostic(73, ".\\p0io.c", 0x3b7);
    frontend_bss.input->End(frontend_bss.text_input);
    frame = frontend_data.source_frames;
    if (!(frame->flags & 4)) {
        frontend_data.backend = frontend_bss.lexer_backend->backend;
        frontend_bss.lexer_backend->enabled = 1;
        result = 1000;
    }
    if (frame->flags & 1) {
        if (frontend_data.compiler_mode_flags & 0x20000)
            frontend_data.compiler_mode_flags &= ~0x20000U;
    } else {
        _close(frontend_data.source_descriptor);
        frame = frontend_data.source_frames;
    }
    if (frame->conditional_depth != frontend_data.conditional_depth) {
        report_compiler_diagnostic(70, frontend_bss.saved_context_stack.FileName());
        frame = frontend_data.source_frames;
    }
    if (result != 1000 && frontend_data.source_include_depth > 0 &&
        frame->packing_alignment != frontend_data.options.packing_stack->top->alignment)
        result = 1;
    if ((frontend_data.options.incremental_rebuild || frontend_data.options.dependency_output) &&
        ((SourceDependencyRecordView *)frontend_data.dependency->records)->previous) {
        frontend_data.dependency->RestoreSource();
        frame = frontend_data.source_frames;
    }
    frontend_bss.source_cursor = frame->saved_cursor;
    origin = frontend_bss.saved_context_stack.origin;
    position = frontend_bss.saved_context_stack.position;
    frontend_bss.saved_context_stack.origin = frame->saved_origin;
    frontend_bss.saved_context_stack.position = frame->saved_position;
    frontend_data.source_frames = frame->next;
    frame->next = frontend_data.free_source_frames;
    frontend_data.free_source_frames = frame;
    if (--frontend_data.source_include_depth < 0) {
        frontend_bss.saved_context_stack.origin = origin;
        frontend_bss.saved_context_stack.position = position;
        return 0;
    }
    frame = frontend_data.source_frames;
    frontend_data.source_descriptor = frame->descriptor;
    if (frame->descriptor == -1) {
        name = frame->file->name->name;
        consumed = frame->consumed_bytes;
        descriptor = open_source_descriptor(name);
        frontend_data.source_frames->descriptor = descriptor;
        frontend_data.source_descriptor = descriptor;
        if (descriptor == -1)
            report_response_error(83, frontend_data.source_include_depth ? 343 : 341, name);
        if (_lseek(frontend_data.source_descriptor, consumed, 0) == -1)
            report_response_error(84, frontend_data.source_include_depth ? 343 : 341, name);
    }
    if (!frontend_data.options.preprocess_no_lines) begin_line_output();
    if (result == 1000) return result;
    return (result ? -1 : 1) * (frontend_data.source_include_depth + 1);
}
/* Only the pointer fields consumed by ReadNextToken are interpreted. Neither
 * this view nor the unvisited Add method establishes the allocation extent. */
struct PendingGroupSource {
    BYTE unmodeled_00_03[4];
    const char *name;
};
struct PendingGroup {
    PendingGroupSource *source;
    BYTE unmodeled_04_0b[8];
    PendingGroup *next;
};
typedef char PendingGroupList_active_offset[offsetof(PendingGroupList, active) == 8 ? 1 : -1];
typedef char PendingGroup_next_offset[offsetof(PendingGroup, next) == 12 ? 1 : -1];
typedef char PendingGroupSource_name_offset[offsetof(PendingGroupSource, name) == 4 ? 1 : -1];
extern "C" ParserTokenView *__cdecl include_directive_file(const char *, unsigned);
ParserTokenView *RECON_THISCALL PendingGroupList::ReadNextToken(void)
{
    PendingGroup *group = head;
    ParserTokenView *token;
    if (group) head = group->next;
    active = group;
    if (!group) return (ParserTokenView *)frontend_data.canonical_token_records[0];
    frontend_data.source_mode_flags |= 0x1000000;
    token = include_directive_file(active->source->name, 0);
    frontend_data.source_mode_flags &= ~0x1000000;
    return token;
}
int RECON_THISCALL CompilerDependency::RestoreSource(void)
{ __asm { int 3 } return 0; }
void RECON_THISCALL CompilerDependency::RecordSource(const char *, int, int) { __asm { int 3 } }
int RECON_THISCALL CompilerDependency::QuerySource(void **) { __asm { int 3 } return 0; }
extern "C" void __cdecl report_include_file(int, const char *) { __asm { int 3 } }

extern "C" void __cdecl report_source_file_error(const char *name, int file) { __asm { int 3 } }

/* The lexer asks this only after advancing past its synthetic first newline. */
extern "C" int __cdecl at_source_buffer_start(void)
{
    return frontend_bss.source_cursor == frontend_data.source_frames->buffer;
}

extern "C" void __cdecl initialize_source_preprocessing(void); /*4933e*/
extern "C" int __cdecl process_forced_source_input(void); /*4952b*/
/* Rewind the synthetic newline while preparing the first source. The flag
 * belongs to initialized state and is restored with that existing snapshot. */
extern "C" int __cdecl begin_first_line_forced_include(void)
{
    --frontend_bss.source_cursor;
    --frontend_bss.saved_context_stack.position;
    if (!frontend_bss.options.preprocess_stdout && !frontend_data.initial_source_setup_done) {
        initialize_source_preprocessing();
        frontend_data.initial_source_setup_done = 1;
        return 1;
    }
    if (process_forced_source_input()) return 1;
    ++frontend_bss.saved_context_stack.position;
    ++frontend_bss.source_cursor;
    return 0;
}
extern "C" int __cdecl open_forced_source(const char *, unsigned); /*28915*/
extern "C" ParserTokenView *__cdecl record_source_location(const void *); /*28a24*/
extern "C" int __cdecl process_forced_source_input(void)
{
    const char *name;
    CompilerArena *arena;
    CompilerSymbolArenaFrame *saved;
    CompilerRecordedLexer *lexer;
    CompilerTokenStream *stream;
    FrontendContextFrame *frame;
    FrontendContextPopCallback callback;
    while (frontend_bss.pending_forced_includes &&
        (short)frontend_bss.forced_include_index < frontend_bss.pending_forced_includes->count) {
        name = (const char *)frontend_bss.pending_forced_includes->items[
            (short)frontend_bss.forced_include_index];
        ++frontend_bss.forced_include_index;
        frontend_bss.default_source_name = name;
        if (open_forced_source(name, 0) <= -1) continue;
        if (frontend_bss.options.preprocess_stdout) return 1;
        arena = &frontend_bss.arenas[4]; /* original C6580 */
        saved = (CompilerSymbolArenaFrame *)arena->cursor;
        arena->cursor += 8;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        if (saved) {
            saved->kind = frontend_bss.symbol_arena_kind;
            saved->previous = frontend_bss.saved_symbol_arenas;
        }
        frontend_bss.saved_symbol_arenas = saved;
        frontend_bss.symbol_arena_kind = 4;
        lexer = (CompilerRecordedLexer *)arena->Allocate(64);
        if (lexer) lexer = lexer->Initialize(4, 1);
        frontend_bss.context_stack.Push(lexer, 4, 0, NULL, &frontend_bss.saved_context_stack);
        ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace(
            (unsigned)record_source_location(0));
        ((CompilerTokenStream *)frontend_bss.context_stack.top->object)->Trace(
            (unsigned)frontend_data.canonical_token_records[243]);
        frame = frontend_bss.context_stack.top;
        if (frame) {
            stream = (CompilerTokenStream *)frame->object;
            frontend_bss.context_stack.top = frame->previous;
            frontend_bss.context_stack.origin = frame->saved_origin;
            frontend_bss.context_stack.position = frame->saved_position;
            callback = frame->callback;
            if (callback) callback();
        } else stream = NULL;
        frontend_bss.saved_context_stack.Push(stream, 4, 1, NULL, &frontend_bss.saved_context_stack);
        saved = frontend_bss.saved_symbol_arenas;
        frontend_bss.symbol_arena_kind = saved->kind;
        frontend_bss.saved_symbol_arenas = saved->previous;
        return 1;
    }
    return 0;
}

extern "C" {
CompilerBackend *__cdecl acquire_initial_backend(void);
void __cdecl prepare_expression_input(const char *);
void __cdecl finish_precompiled_input(void); /*4eb64*/
int __cdecl finish_forced_include(void);
}
extern "C" void __cdecl finish_token_input(void)
{
    if (frontend_data.options.pch_create && !frontend_data.pch_reference_output_mode) {
        if (frontend_data.options.pch_create_file)
            compiler_warning(0x359, frontend_data.options.pch_create_file);
        else {
            prepare_expression_input(frontend_bss.options.source_file);
            finish_precompiled_input();
        }
    } else if (frontend_bss.forced_source_index) finish_forced_include();
    if (frontend_bss.browser_filename) frontend_data.backend->Slot14(0);
    frontend_bss.processing_pending_parser_input = 1;
    frontend_data.backend = acquire_initial_backend();
    frontend_data.compiler_mode_flags |= 0x80;
}
extern "C" void __cdecl finish_precompiled_input(void) { __asm { int 3 } }
