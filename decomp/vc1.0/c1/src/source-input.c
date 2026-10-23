/* Source input stack and overlapping buffered file windows. */
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <share.h>
#include "source-input.h"
#include "preprocessor.h"
#include "lexer.h"
#include <recon.h>
#undef _fileno

SourceFile *active_input, *root_input, *free_inputs;
int source_handle = -1; /* original 0x6a7d8 */
/* Two guard characters precede each 4097-byte window. Larger reads may
   occupy subsequent windows until an include needs that portion. */
char source_storage[3 * 4099]; /* original first guard at 0x60838 */
char *source_text_cursor; /* original 0x5b14c */
unsigned source_line_number; /* original 0x5b238 */
int conditional_depth = -1; /* original 0x6bf74 */
long browser_saved_position; /* original 0x6ddfc */

/* Original 0x1b680..0x1b966. */
int __stdcall open_source_file(char *filename, int mode)
{
    SourceFile *input, *parent;
    ArenaPool *pool;
    char *buffer, *cursor;
    long offset;
    if (mode == 2)
        reset_work_stack(compiler_stack);
    else if (!filename)
        source_handle = _fileno(stdin);
    else {
        source_handle = open_input_handle(filename);
        if (source_handle == -1) {
            if (mode == 1)
                report_fatal(83, message_text(341), filename);
            return 0;
        }
    }
    input = free_inputs;
    if (input)
        free_inputs = input->next;
    else {
        pool = &compiler_arena.pools[0];
        if (pool->remaining < (int)sizeof(SourceFile))
            refill_arena(&compiler_arena, pool);
        input = (SourceFile *)pool->next;
        pool->remaining -= sizeof(SourceFile);
        pool->next += sizeof(SourceFile);
    }
    parent = active_input;
    input->next = parent;
    active_input = input;
    ++include_depth;
    input->saved_conditional_depth = conditional_depth;
    input->saved_packing = top_packing(packing_stack)->alignment;
    if (!include_depth) {
        root_input = active_input;
        buffer = source_storage + 2;
        input->capacity = 0x3000;
    } else {
        if (include_depth == 1) {
            buffer = source_storage + 4099 + 2;
            input->capacity = 0x2000;
        } else {
            buffer = source_storage + 2 * 4099 + 2;
            input->capacity = 0x1000;
        }
        if (parent->available > 0x1000 || include_depth > 2) {
            offset = parent->offset;
            cursor = macro_expansion_active ? macro_saved_input_cursor : source_text_cursor;
            if (buffer - 2 <= cursor) {
                offset += cursor - parent->buffer;
                parent->offset = offset;
                parent->available = 0;
                if (macro_expansion_active)
                    macro_saved_input_cursor = parent->buffer;
                else
                    source_text_cursor = parent->buffer;
                parent->buffer[0] = 0;
            } else {
                offset += 0x1000;
                parent->available = 0x1000;
                parent->buffer[0x1000] = 0;
            }
            if (parent->input_kind == 2)
                seek_work_stack(compiler_stack, offset, 0);
            else
                _lseek(parent->handle, offset, 0);
        }
    }
    input->saved_cursor = source_text_cursor;
    input->saved_line = source_line_number;
    input->handle = source_handle;
    input->buffer = buffer;
    input->available = 0;
    input->offset = 0;
    input->input_kind = mode == 2;
    source_text_cursor = buffer + 1;
    fill_source_buffer();
    input->filename = filename;
    source_filename = filename;
    source_line_number = 0;
    if (!options.option_EP)
        emit_line_directive();
    if (options.option_ide)
        report_ide_source();
    input->index = 0xffff;
    source_file_index = &input->index;
    if ((scanner_flags & 0x80000) && !(scanner_flags & 0x40) && !options.option_E)
        record_line_information(2);
    if (browser_filename && mode != 2) {
        browser_pending_flags = 0;
        if (!include_depth && options.pch_auto) {
            browser_include_begin();
            browser_saved_position = ftell(browser_output);
        }
        browser_open_file();
    }
    *--source_text_cursor = '\n';
    return 1;
}

/* Original 0x43e20..0x43e79. */
int __stdcall open_input_handle(char *filename)
{
    char *name = normalize_input_name(filename);
    int attempt, handle = -1;
    for (attempt = 1; attempt < 2 && handle == -1; ++attempt) {
        handle = _sopen(name, _O_BINARY, _SH_DENYWR);
        if (handle != -1 || errno != EMFILE)
            break;
        release_input_handle();
    }
    return handle;
}

/* Original 0x43d80..0x43e11. /FAT applies DOS 8.3 component lengths. */
char *normalize_input_name(char *filename)
{
    char drive[4], directory[256], name[256], extension[256];
    char *result;
    if (!options.option_FAT)
        return filename;
    result = copy_string(filename, 2);
    _splitpath(result, drive, directory, name, extension);
    name[8] = 0;
    extension[4] = 0;
    _makepath(result, drive, directory, name, extension);
    return result;
}

int final_newline_emitted; /* original 0x6b140 */

/* Original 0x1be70..0x1bf6b. */
int fill_source_buffer(void)
{
    SourceFile *input = active_input;
    char *cursor = source_text_cursor;
    char *buffer = input->buffer;
    int bytes;
    if (cursor - buffer < input->available) {
        if (cursor[-1] == 0x1a) {
            source_text_cursor = cursor - 1;
            return 1;
        }
        return 0;
    }
    buffer[-1] = cursor[-2];
    buffer[-2] = cursor[-3];
    source_text_cursor = buffer;
    if (input->input_kind)
        bytes = read_work_stack(compiler_stack, buffer, input->capacity);
    else
        bytes = _read(source_handle, buffer, input->capacity);
    input = active_input;
    input->offset += input->available;
    input->available = bytes;
    if (bytes > 0) {
        buffer[bytes] = 0;
        return 0;
    }
    if (bytes < 0)
        report_fatal(84, message_text(include_depth ? 343 : 341), source_filename);
    if (!final_newline_emitted && (scanner_flags & 0x10000) && !(scanner_flags & 0x1000)) {
        buffer[0] = '\n';
        buffer[1] = 0;
        return 0;
    }
    buffer[0] = 0;
    return 1;
}

/* Original 0x1b9b0..0x1bc6b. Pop an input file, restore its parent's
   buffered position, and reopen an evicted parent handle when necessary. */
unsigned char close_source_file(void)
{
    SourceFile *input, *parent;
    PackingEntry *packing;
    char *packing_warning = 0;
    unsigned final_line;
    long offset;
    int handle;
    if (include_depth == -1) return 0;
    input = active_input;
    if (!input->input_kind) {
        _close(source_handle);
        input = active_input;
    }
    if (input->saved_conditional_depth != conditional_depth) {
        fatal_error(70, source_filename);
        input = active_input;
    }
    if (include_depth) {
        packing = top_packing(packing_stack);
        input = active_input;
        if (packing->alignment != input->saved_packing) packing_warning = source_filename;
    }
    if (browser_filename && !input->input_kind) {
        browser_close_file();
        input = active_input;
    }
    if (options.option_ide) {
        report_ide_file_end();
        input = active_input;
    }
    final_line = source_line_number;
    source_text_cursor = input->saved_cursor;
    source_buffer_cursor = input->filename;
    source_line_number = input->saved_line;
    parent = input->next;
    active_input = parent;
    input->next = free_inputs;
    free_inputs = input;
    if (--include_depth < 0) {
        source_line_number = final_line;
        source_filename = root_input->filename;
        source_file_index = &root_input->index;
        return 0;
    }
    source_filename = parent->filename;
    source_file_index = &parent->index;
    handle = parent->handle;
    if (!include_depth && pch_include_state == 1) {
        active_input = parent;
        source_handle = handle;
        if (!pch_output_written) {
            prepare_pch_output(options.pch_create_file);
            finish_pch_output();
            parent = active_input;
            handle = source_handle;
        }
    }
    active_input = parent;
    source_handle = handle;
    if (handle == -1) {
        offset = parent->offset;
        parent->handle = open_input_handle(source_filename);
        source_handle = active_input->handle;
        if (source_handle == -1)
            report_fatal(83, message_text(include_depth ? 343 : 341), source_filename);
        if (_lseek(active_input->handle, offset, 0) == -1)
            report_fatal(84, message_text(include_depth ? 343 : 341), source_filename);
    }
    if (packing_warning) compiler_warning(1, 103, packing_warning);
    if (include_depth >= 2) fill_source_buffer();
    if (!options.option_EP) emit_line_directive();
    if (options.option_ide) report_ide_source();
    if ((scanner_flags & 0x80000) && !(scanner_flags & 0x40) && !options.option_E)
        record_line_information(2);
    if (!include_depth && forced_source_inputs->count > forced_source_index) {
        current_forced_source = forced_source_inputs->items[forced_source_index++];
        open_include_file(current_forced_source, 0);
    }
    return 1;
}

/* Original 0x1e120..0x1e170. The token continuation paths require more
   physical input; report the active lexical context if the file has ended. */
void refill_source_boundary(void)
{
    if (!fill_source_buffer()) return;
    if (scanner_flags & 0x1000) fatal_error(71);
    else if (scanner_flags & 0x2000) macro_scan_error(57);
    else if (final_newline_emitted) fatal_error(10);
    else fatal_error(4);
}

/* Original 0x1bc70..0x1bd92. Quoted includes try the active source's
   directory, then each enclosing source directory, before /I search begins. */
int __stdcall open_include_in_source_directory(char *filename)
{
    SourceFile *input = active_input;
    char *directory, *scan, *separator, *start, *text;
    unsigned bytes;
    do {
        directory = input->filename;
        input = input->next;
        separator = 0;
        for (scan = directory; *scan; ++scan)
            if (strchr(options.path_characters, (unsigned char)*scan)) separator = scan;
        bytes = 1;
        for (text = filename; *text; ++text) ++bytes;
        for (text = directory; *text; ++text) ++bytes;
        if (source_buffer_cursor + bytes > token_buffer_end) fatal_error(14, include_depth + 1);
        start = source_buffer_cursor;
        if (separator && separator >= directory) {
            scan = directory;
            do { *source_buffer_cursor++ = *scan++; } while (scan <= separator);
        }
        text = filename;
        do { *source_buffer_cursor++ = *text++; } while (text[-1]);
        if (open_source_file(start, 0)) {
            if (pch_mode == 1) pch_record_include(start);
            return 1;
        }
        source_buffer_cursor = start;
    } while (input);
    return 0;
}

/* Original 0x27190..0x27191. This build retains an empty browser hook. */
void browser_include_begin(void)
{
}

/* Original0x1bdf0..0x1be67. Generated preprocessing line markers combine
   the next line number and an escaped quoted file name in two writes. */
void emit_line_directive(void)
{
    char line[16], *end, *cursor;
    unsigned length;
    format_text(line,"#line %d ",source_line_number + 1);
    end = quote_macro_filename(token_text,source_filename);
    cursor = line;
    length = 0;
    while (*cursor++) ++length;
    fwrite(line,length,1,preprocessor_stream);
    fwrite(token_text,end - token_text,1,preprocessor_stream);
}
