#include <string.h>
#include <ctype.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <errno.h>
#include <stdlib.h>
#include "frontend.h"
#include "keywords.h"
#include "global-record.h"
#include "packing.h"
#pragma intrinsic(strcpy, memcpy, strlen)
#include <recon.h>

/* One boolean flag selects which encountered token IDs receive a shared
 * prototype. Dynamic lexer records have their own later constructors.
 * Original table RVA63d70; only the used ID0..244 domain is modeled here. */
static const BYTE token_has_prototype[245] = {
    0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0,
    0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1,
    0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1,
    0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1,
    1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 1, 1,
};

extern "C" void __cdecl initialize_predefined_tokens(void)
{
    unsigned kind;
    CompilerPool *pool = &c1_bss.pools[6];
    CompilerTokenPrototype *token;
    for (kind = 1; kind < 245; ++kind) {
        if (token_has_prototype[kind]) {
            if (pool->remaining < 32)
                pool->Refill(32);
            token = (CompilerTokenPrototype *)pool->cursor;
            pool->cursor += 32;
            pool->remaining -= 32;
            if (token) {
                token->kind = (BYTE)kind;
                token->tag = (BYTE)kind;
            }
            c1_data.predefined_tokens[kind] = token;
        } else {
            c1_data.predefined_tokens[kind] = NULL;
        }
    }
}

/* The hash consumers mask to 0x7ff and index DWORD bucket heads. The
 * initializer only reserves their storage; it does not initialize or inspect
 * any table byte. Entry layout is pending until its actual constructors. */
extern "C" void __cdecl initialize_keyword_table(void)
{
    if (!c1_data.name_buckets) {
        CompilerArena *arena = &c1_bss.arenas[0];
        BYTE *memory = arena->cursor;
        arena->cursor += 8192;
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        c1_data.name_buckets = (CompilerNameEntry **)memory;
    }
}


extern "C" {
long __cdecl get_compilation_time(long *);
char *__cdecl format_predefined_time(long);
void __cdecl initialize_macro_buffers(void);
void __cdecl define_command_line_macro(char *);
void __cdecl define_preprocessor_macro(const char *, unsigned, int);
struct CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
void __cdecl write_global_header(void);
int __cdecl open_source_file(const char *, unsigned, int, int, int);
void __cdecl report_source_file_error(const char *, int);
}
extern "C" void __cdecl define_command_line_macros(CompilerVector *definitions)
{
    long stamp;
    short index;
    int file;
    /* These dispatch classes are used for the four internal marker bytes. */
    c1_data.scanner.classes[1] = 38;
    c1_data.scanner.classes[2] = 39;
    c1_data.scanner.classes[3] = 40;
    c1_data.scanner.classes[4] = 41;
    if (c1_data.options.language_extensions) {
        c1_data.scanner.classes['$'] = SC_IDENTIFIER;
        c1_data.scanner.flags['$'] |= 1;
    }
    if (c1_data.options.reproducible) {
        c1_bss.macro_date_time[0] = '1';
        c1_bss.macro_date_time[1] = 0;
        c1_bss.macro_date = c1_bss.macro_time = c1_bss.macro_date_time;
    } else {
        get_compilation_time(&stamp);
        strcpy(c1_bss.macro_date_time, format_predefined_time(stamp));
        c1_bss.macro_date = c1_bss.macro_date_time + 4;
        c1_bss.macro_date_time[15] = 0;
        c1_bss.macro_time = c1_bss.macro_date_time + 16;
        c1_bss.macro_date_time[24] = 0;
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
    if (!c1_data.options.language_extensions) {
        find_or_create_identifier("__STDC__");
        define_preprocessor_macro(NULL, 0, -8);
    }
    for (index = 0; definitions && index < definitions->count; ++index)
        define_command_line_macro((char *)definitions->items[index]);
    if (!c1_bss.options.preprocess_stdout && c1_data.c9_il &&
        !c1_bss.options.prototypes_only)
        write_global_header();
    file = open_source_file(c1_bss.options.source_file, 0, 1, 1, 0);
    if (file > -1 && c1_bss.pch_file_failed)
        report_source_file_error(c1_bss.options.source_file, file);
    c1_bss.forced_include_index = 0;
    c1_bss.pending_forced_includes = c1_bss.options.forced_includes;
}
extern "C" long __cdecl get_compilation_time(long *result)
{ return time(result); }
extern "C" char *__cdecl format_predefined_time(long stamp)
{
    stamp &= 0x7fffffff;
    char *text = ctime(&stamp);
    if (text) {
        /* The first25 bytes retain the original newline. The separate
         * initialized trailing NUL is not overwritten by this copy. */
        memcpy(c1_data.predefined_time_text, text, 25);
        if (c1_data.predefined_time_text[8] == '0')
            c1_data.predefined_time_text[8] = ' ';
        memcpy(c1_data.predefined_time_text + 11, text + 20, 4);
        memcpy(c1_data.predefined_time_text + 16, text + 11, 4);
        memcpy(c1_data.predefined_time_text + 20, text + 15, 4);
    }
    return c1_data.predefined_time_text;
}
extern "C" void __cdecl initialize_macro_buffers(void)
{
    c1_data.macro_reset_value = 0;
    c1_bss.macro_token_cursor = c1_bss.work_buffers[0].data;
    c1_bss.macro_text_cursor = c1_bss.work_buffers[1].data;
    c1_bss.saved_macro_text_cursor = c1_bss.work_buffers[1].data;
}
extern "C" BYTE *__cdecl grow_macro_text(BYTE *, BYTE *, BYTE **);
extern "C" void __cdecl define_command_line_macro(char *definition)
{
    char *scan = definition;
    char *equals = NULL;
    BYTE *out = c1_bss.work_buffers[1].data;
    BYTE *trailing_space = NULL;
    while (*scan) {
        if (c1_data.scanner.classes[(BYTE)*scan] == SC_LEAD) ++scan;
        else if (*scan == '=') break;
        ++scan;
    }
    if (*scan == '=') {
        equals = scan;
        *scan++ = 0;
        while (*scan) {
            if (out >= c1_bss.work_buffers[1].limit)
                grow_macro_text(out, c1_bss.work_buffers[1].data, &c1_bss.work_buffers[1].limit);
            *out++ = (BYTE)*scan;
            if (c1_data.scanner.classes[(BYTE)*scan] == SC_LEAD) {
                *out++ = (BYTE)*++scan;
                trailing_space = NULL;
            } else if ((isspace)((BYTE)*scan)) {
                if (!trailing_space) trailing_space = out;
            } else trailing_space = NULL;
            ++scan;
        }
        if (trailing_space) out = trailing_space;
    } else {
        if (out >= c1_bss.work_buffers[1].limit)
            grow_macro_text(out, out, &c1_bss.work_buffers[1].limit);
        *out++ = '1';
    }
    find_or_create_identifier(definition);
    if (out != c1_bss.work_buffers[1].data) {
        if (out >= c1_bss.work_buffers[1].limit)
            grow_macro_text(out, c1_bss.work_buffers[1].data, &c1_bss.work_buffers[1].limit);
        *out++ = 0;
        *out++ = 0x44; /* Encoded end of replacement text. */
        define_preprocessor_macro((const char *)c1_bss.work_buffers[1].data,
            (unsigned)(out - c1_bss.work_buffers[1].data), 0);
    } else define_preprocessor_macro(NULL, 0, 0);
    if (equals) *equals = '=';
}
extern "C" void __cdecl record_macro_definition(const char *, const char *, int);
extern "C" int __cdecl compare_macro_text(const void *, const void *, unsigned);
extern "C" void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
extern "C" void __cdecl define_preprocessor_macro(const char *text, unsigned length, int kind)
{
    CompilerNameEntry *identifier = c1_data.current_identifier;
    if (identifier == c1_data.builtin_names.defined_operator) {
        report_language_diagnostic(1, 117, identifier, "#define");
        return;
    }
    CompilerMacro *macro = identifier->macro;
    if (!c1_data.source_include_depth && c1_bss.forced_source_index) {
        record_macro_definition(identifier->name,
            length ? c1_bss.current_macro_text : "", kind);
        identifier = c1_data.current_identifier;
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
    CompilerPool *pool = &c1_bss.pools[5];
    if (pool->remaining < 8)
        pool->Refill(8);
    identifier = c1_data.current_identifier;
    macro = (CompilerMacro *)pool->cursor;
    pool->cursor += 8;
    pool->remaining -= 8;
    macro->identifier = identifier;
    CompilerArena *arena = &c1_bss.arenas[0];
    CompilerMacroBody *body = (CompilerMacroBody *)arena->cursor;
    arena->cursor += (length + 11) & ~3U;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    macro->body = body;
    if (length)
        memcpy(body->payload, text, length);
    identifier = c1_data.current_identifier;
    identifier->flags |= 0x80000;
    identifier->macro = macro;
    body->kind = (signed char)kind;
    c1_data.backend->Slot28(macro);
}
extern "C" void __cdecl record_macro_definition(const char *name, const char *text, int kind)
{ __asm { int 3 } }
extern "C" int __cdecl compare_macro_text(const void *first, const void *second, unsigned size)
{ __asm { int 3 } return 0; }
/* These alternate PCH inputs remain unmodeled until their actual producers
 * and consumers are reached. The ordinary source-file path does not read them. */
static void *unmodeled_pch_input_cursor; /* BSS RVA55924 */
extern "C" void __cdecl select_pch_input(void *);
extern "C" int __cdecl open_source_descriptor(const char *);
extern "C" void __cdecl push_source_context(CompilerSourceFile *, int, int, int);
extern "C" void __cdecl report_include_file(int depth, const char *name);
extern "C" void __cdecl report_response_error(int, ...);
extern "C" int __cdecl open_source_file(const char *name, unsigned hash,
                                        int kind, int flags, int track)
{
    CompilerSourceFile *file = c1_data.translation_unit->FindSourceFile(name, hash);
    int descriptor;
    if (file->record_id & 0xff000000) return -3;
    if (kind == 2) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("open_source_file: unmodeled PCH source context");
#endif
        select_pch_input(unmodeled_pch_input_cursor);
        push_source_context(file, kind, c1_data.source_descriptor, flags);
        return -2;
    }
    descriptor = name ? open_source_descriptor(name) : stdin->_file;
    if (descriptor == -1) {
        if (kind == 1) report_response_error(83, 341, name);
        return -1;
    }
    push_source_context(file, kind, descriptor, flags);
    if (name && track && c1_bss.options.show_includes)
        report_include_file(c1_data.source_include_depth, name);
    return descriptor;
}
extern "C" CompilerNameEntry *__cdecl intern_name_span(const char *, unsigned, unsigned, int *);
CompilerSourceFile *RECON_THISCALL CompilerTranslationUnit::LookupSourceFile(CompilerNameEntry *name)
{
    CompilerSourceFile *file;
    for (file = source_files; file; file = file->next)
        if (file->name == name) return file;
    return NULL;
}
CompilerSourceFile *RECON_THISCALL CompilerTranslationUnit::FindSourceFile(
    const char *name, unsigned hash)
{
    unsigned length;
    int created;
    CompilerNameEntry *identifier;
    CompilerSourceFile *file;
    CompilerArena *arena = &c1_bss.arenas[0];
    if (hash) length = strlen(name);
    else {
        const unsigned char *text = (const unsigned char *)name;
        while (*text) {
            hash = (hash >> 4) + hash * 4 + *text;
            ++text;
        }
        length = (unsigned)(text - (const unsigned char *)name);
    }
    if (length > 256) compiler_error(81, name, 256);
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
    if (source_files) file->next = source_files;
    source_files = file;
    return file;
}
extern "C" void __cdecl select_pch_input(void *cursor) { __asm { int 3 } }
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
    if (c1_bss.options.fat) {
        char drive[4], directory[256], base[256], extension[256];
        char *path = duplicate_argument(name, 3);
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
/* The optional line-output stream shares the existing RVA54dbc owner. */
extern "C" void __cdecl push_source_context(CompilerSourceFile *file, int kind, int descriptor, int flags)
{
    CompilerArena *arena = &c1_bss.arenas[1];
    CompilerSourceFrame *frame = c1_data.free_source_frames;
    if (frame) c1_data.free_source_frames = frame->next;
    else {
        frame = (CompilerSourceFrame *)arena->cursor;
        arena->cursor += 48;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        BYTE *buffer = arena->cursor;
        arena->cursor += 0x7004;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        frame->buffer = buffer + 2;
    }
    frame->next = c1_data.source_frames;
    c1_data.source_frames = frame;
    if (++c1_data.source_include_depth > 1024) {
        --c1_data.source_include_depth;
        compiler_error(14, c1_data.source_include_depth);
    }
    c1_data.source_descriptor = descriptor;
    frame->conditional_depth = c1_data.conditional_depth;
    frame->packing_alignment = c1_data.options.packing_stack->active->alignment;
    if (c1_data.source_include_depth == 0)
        c1_data.root_source_frame = c1_data.source_frames;
    frame->saved_cursor = c1_bss.source_cursor;
    frame->saved_context = c1_bss.current_context.object;
    frame->saved_context_value = c1_bss.current_context.value;
    frame->descriptor = c1_data.source_descriptor;
    frame->available_bytes = frame->consumed_bytes = 0;
    frame->flags = (frame->flags & ~1U) | (kind == 2);
    frame->flags &= ~2U;
    c1_data.source_frames->flags &= ~8U;
    frame->flags = (frame->flags & ~4U) | ((flags & 1) << 2);
    if (!flags) {
        c1_bss.lexer_backend->backend = c1_data.backend;
        c1_data.backend = acquire_initial_backend();
        c1_bss.lexer_backend->enabled = flags;
    }
    c1_bss.source_cursor = frame->buffer;
    c1_bss.input->Begin(c1_bss.text_input);
    frame->file = file;
    c1_bss.current_context.value = 0;
    c1_bss.current_context.object = file;
    c1_bss.source_cursor[-1] = '\n';
    ++c1_bss.source_cursor;
    read_source_chunk();
    if (c1_bss.debug_module) {
        c1_bss.text_input->Reset();
        if (!c1_bss.debug_module->Slot30(file->name->name, flags == 0))
            compiler_error(73, ".\\p0io.c", 288);
        if (c1_data.dependency_local || c1_data.options.dependency_output) {
            c1_data.dependency->RecordSource(c1_bss.current_context.FileName(), descriptor, kind != 2);
            if (c1_data.dependency_local) {
                void *result = NULL;
                if (c1_data.dependency->QuerySource(&result) &&
                    !c1_bss.debug_module->Slot88(result))
                    compiler_error(73, ".\\p0io.c", 342);
            }
        }
    }
    if (!c1_data.options.preprocess_no_lines) {
        if (c1_bss.preprocessor_output) begin_line_output();
    }
    --c1_bss.source_cursor;
    *c1_bss.source_cursor = '\n';
}
extern "C" int __cdecl read_pch_input(void *, BYTE *, unsigned);
extern "C" int __cdecl read_source_chunk(void)
{
    CompilerSourceFrame *frame = c1_data.source_frames;
    BYTE *buffer = frame->buffer;
    int count;
    unsigned i;
    if (c1_bss.source_cursor - buffer < frame->available_bytes) {
        if (c1_bss.source_cursor[-1] == 0x1a) {
            --c1_bss.source_cursor;
            return 1;
        }
        return 0;
    }
    --c1_bss.source_cursor;
    c1_bss.input->Flush();
    for (i = 1; i <= 2; ++i)
        buffer[-(int)i] = c1_bss.source_cursor[-(int)i];
    c1_bss.source_cursor = buffer;
    c1_bss.input->Mark();
    if (c1_data.source_frames->flags & 2) {
        *buffer = 0;
        c1_data.source_frames->available_bytes = 0;
        if (!(c1_data.source_frames->flags & 8)) {
            c1_bss.input->Location((unsigned)buffer, 1);
            c1_data.source_frames->flags |= 8;
        }
        return 1;
    }
    if (c1_data.source_frames->flags & 1) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("read_source_chunk: unmodeled PCH input cursor");
#endif
        count = read_pch_input(unmodeled_pch_input_cursor, buffer, 0x7000);
    } else count = _read(c1_data.source_descriptor, buffer, 0x7000);
    frame = c1_data.source_frames;
    frame->consumed_bytes += frame->available_bytes;
    frame->available_bytes = count;
    if (count <= 0) {
        if (count < 0)
            report_response_error(84, c1_data.source_include_depth ? 343 : 341,
                frame->file->name->name);
        c1_data.source_frames->flags |= 2;
        if (c1_data.options.language_extensions) *buffer++ = '\n';
        *buffer = 0;
        return 0;
    }
    if ((unsigned)count < 0x7000) {
        frame->flags |= 2;
        if (c1_data.options.language_extensions && !(frame->flags & 1) && buffer[count - 1] != '\n') {
            buffer[count++] = '\n';
            c1_data.source_frames->available_bytes = count;
        }
    }
    buffer[count] = 0;
    return 0;
}
extern "C" int __cdecl read_pch_input(void *cursor, BYTE *buffer, unsigned count)
{ __asm { int 3 } return 0; }
void RECON_THISCALL CompilerDependency::RecordSource(const char *name, int descriptor, int ordinary)
{ __asm { int 3 } }
int RECON_THISCALL CompilerDependency::QuerySource(void **result) { __asm { int 3 } return 0; }
extern "C" void __cdecl report_include_file(int depth, const char *name) { __asm { int 3 } }
extern "C" void __cdecl report_source_file_error(const char *name, int file)
{ __asm { int 3 } }

extern "C" int __cdecl restore_forced_source_input(void)
{
    CompilerSourceFrame *frame = c1_data.source_frames;
    CompilerSourceFrame *previous;
    void *saved_object;
    unsigned saved_value;
    const char *name;
    int status = 0;
    if (c1_data.source_include_depth == -1) return 0;
    if ((unsigned)c1_data.source_include_depth > 2 || !frame || (frame->flags & 1) ||
        !(frame->flags & 4) || c1_data.dependency_local || c1_data.options.dependency_output) {
#ifdef __linux__
        recon_unsupported("deeper, PCH or dependency source restoration is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    if (c1_data.source_include_depth > 0) {
        if (!c1_bss.debug_module->Slot34(c1_bss.current_context.value))
            compiler_error(73, ".\\p0io.c", 947);
    } else if (!c1_bss.debug_module->Slot10()) {
        compiler_error(73, ".\\p0io.c", 951);
    }
    c1_bss.input->End(c1_bss.text_input);
    frame = c1_data.source_frames;
    if (!(frame->flags & 4)) {
        status = 1000;
        c1_data.backend = c1_bss.lexer_backend->backend;
        c1_bss.lexer_backend->enabled = 1;
    }
    if (!(frame->flags & 1)) {
        _close(c1_data.source_descriptor);
        frame = c1_data.source_frames;
    }
    if (frame->conditional_depth != c1_data.conditional_depth) {
        compiler_error(70, c1_bss.current_context.FileName());
        frame = c1_data.source_frames;
    }
    if (status != 1000 && c1_data.source_include_depth > 0 &&
        frame->packing_alignment != c1_data.options.packing_stack->active->alignment)
        status = 1;
    if ((c1_data.dependency_local || c1_data.options.dependency_output) &&
        c1_data.dependency->callbacks->source_ready) {
        c1_data.dependency->NotifySource();
        frame = c1_data.source_frames;
    }
    c1_bss.source_cursor = frame->saved_cursor;
    saved_object = c1_bss.current_context.object;
    saved_value = c1_bss.current_context.value;
    c1_bss.current_context.object = frame->saved_context;
    c1_bss.current_context.value = frame->saved_context_value;
    previous = frame->next;
    c1_data.source_frames = previous;
    frame->next = c1_data.free_source_frames;
    c1_data.free_source_frames = frame;
    if (--c1_data.source_include_depth < 0) {
        c1_bss.current_context.object = saved_object;
        c1_bss.current_context.value = saved_value;
        return 0;
    }
    c1_data.source_descriptor = previous->descriptor;
    if (c1_data.source_descriptor == -1) {
        name = previous->file->name->name;
        int position = previous->consumed_bytes;
        previous->descriptor = open_source_descriptor(name);
        c1_data.source_descriptor = previous->descriptor;
        if (c1_data.source_descriptor == -1)
            report_response_error(83, c1_data.source_include_depth ? 343 : 341, name);
        if (_lseek(previous->descriptor, position, SEEK_SET) == -1)
            report_response_error(84, c1_data.source_include_depth ? 343 : 341, name);
    }
    if (!c1_data.options.preprocess_no_lines) begin_line_output();
    if (status == 1000) return status;
    return (status ? -1 : 1) * (c1_data.source_include_depth + 1);
}
void RECON_THISCALL CompilerDependency::NotifySource(void) { __asm { int 3 } }

/* Ordinary source refill shares the established source-stack reader. The
 * alternate PCH branch passes its existing cursor first to a genuine pending
 * initializer; no initialized cursor value or PCH semantics are inferred. */
extern "C" void __cdecl report_macro_input_end(int);
extern "C" void __cdecl initialize_pch_text(void **) { __asm { int 3 } }
extern "C" void __cdecl append_pch_input_text(void *, const char *, ...) { __asm { int 3 } }
extern "C" void __cdecl parse_pch_directive_text(int) { __asm { int 3 } }
extern "C" void __cdecl refill_lexer_source(void)
{
    if (!read_source_chunk()) return;
    if (c1_data.lexer_flags & 0x800) compiler_fatal(71);
    else if (c1_data.lexer_flags & 0x1000) report_macro_input_end(57);
    else if (!(c1_data.parser_flags & 0x40000)) compiler_fatal(4);
    else if (c1_data.function_record_flags & 0x10000) {
        initialize_pch_text(&unmodeled_pch_input_cursor);
        append_pch_input_text(unmodeled_pch_input_cursor, "#pragma hdrstop\n");
        parse_pch_directive_text(0);
    } else compiler_fatal(10);
}
