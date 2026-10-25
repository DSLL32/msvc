#include <stdio.h>
#include <new.h>
#include "frontend.h"
#include <string.h>
#pragma intrinsic(strcmp, strcpy, strcat, memcpy, memset)
#include <recon.h>

extern "C" {
void __cdecl prepare_pch_expression_input(const char *);
CompilerIntegerConstant *__cdecl make_integer_constant(int);
void __cdecl write_ir_record(int, const void *);
void __cdecl report_response_error(int, ...);
}

extern "C" void __cdecl open_expression_output(void)
{
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    int ready;
    if (c1_data.options.pch_auto &&
        (c1_data.options.pch_use_file || c1_data.options.pch_file) &&
        !c1_data.options.debug_info) {
        prepare_pch_expression_input(c1_data.options.pch_use_file);
        buffer->Open(c1_data.options.pch_file, "", "wb");
        c1_data.options.pch_no_preprocess = 1;
    } else {
        buffer->Open(c1_data.options.il_prefix, "ex", "w+b");
    }
    if (!c1_data.options.debug_info) {
        write_ir_record(91, make_integer_constant(1132));
        buffer->Flush();
        fflush(buffer->stream);
        c1_bss.expression_seek_status = fseek(buffer->stream, 1131, SEEK_SET);
        if (c1_bss.expression_seek_status)
            report_response_error(86, 339, NULL);
        ready = 1;
        if (buffer->cursor + 2 > buffer->data + buffer->capacity)
            ready = buffer->Grow(2);
        buffer->good &= ready;
        if (buffer->good)
            *buffer->cursor++ = 0;
        buffer->Flush();
    }
}

extern "C" void __cdecl prepare_pch_expression_input(const char *filename) { __asm { int 3 } }
void RECON_THISCALL CompilerOutputBuffer::Flush(void)
{
    unsigned used = (unsigned)(cursor - data);
    if (used) {
        if (c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1) == 0 &&
            !c1_bss.options.preprocess_stdout && !c1_bss.options.prototypes_only) {
            if (!good) {
                report_response_error(88, 339, filename);
            } else {
                unsigned written = fwrite(data, 1, used, stream);
                if (written != (unsigned)(cursor - data))
                    report_response_error(88, 339, filename);
            }
        }
        cursor = data;
    }
}

int RECON_THISCALL CompilerOutputBuffer::Open(const char *prefix,
                                            const char *suffix,
                                            const char *mode)
{
    if (!strcmp(prefix, "nul"))
        suffix = NULL;
    strcpy(filename, prefix);
    if (suffix)
        strcat(filename, suffix);
    stream = fopen(filename, mode);
    if (!stream)
        report_response_error(83, 339, filename);
    return setvbuf(stream, NULL, _IOFBF, 4096);
}

extern "C" CompilerIntegerConstant *__cdecl create_integer_constant(int value)
{
    CompilerArena *arena = &c1_bss.arenas[c1_data.expression_arena_kind];
    CompilerIntegerConstant *node = (CompilerIntegerConstant *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    node->flags = 3;
    node->kind = 101;
    node->type = c1_data.builtin_types.type_0063;
    node->integer = value;
    return node;
}
extern "C" CompilerIntegerConstant *__cdecl make_integer_constant(int value)
{
    CompilerIntegerConstant *node = create_integer_constant(value);
    if (!value)
        node->flags |= 0x80;
    return node;
}

int RECON_THISCALL CompilerOutputBuffer::Grow(int bytes)
{
    int increment = capacity / 2;
    unsigned new_capacity;
    unsigned used;
    BYTE *replacement, *new_cursor, *old_data;
    if (bytes + 4096 > increment)
        increment = bytes + 4096;
    new_capacity = (capacity + increment + 4095) & ~4095U;
    replacement = (BYTE *)::operator new(new_capacity);
    if (!replacement) return 0;
    capacity = new_capacity;
    used = (unsigned)(cursor - data);
    memcpy(replacement, data, used);
    new_cursor = replacement + used;
    memset(new_cursor, 0, capacity - used);
    ::operator delete(data);
    old_data = data;
    data = replacement;
    cursor = new_cursor;
    if (old_data != replacement && relocated)
        relocated(relocation_context, old_data, replacement);
    return 1;
}

extern "C" void __cdecl write_global_record(unsigned, CompilerIrSymbol *);
extern "C" void __cdecl write_global_header(void)
{
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[2];
    buffer->Flush();
    fflush(buffer->stream);
    rewind(buffer->stream);
    c1_bss.declaration_scope_active = 0;
    write_global_record(17, NULL);
}
