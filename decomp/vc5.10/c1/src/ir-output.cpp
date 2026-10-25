#include <stdio.h>
#include "frontend.h"
#include <string.h>
#pragma intrinsic(strcmp, strcpy, strcat)
#include <recon.h>

extern "C" {
void __cdecl prepare_pch_expression_input(const char *);
void *__cdecl make_integer_constant(int);
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
void RECON_THISCALL CompilerOutputBuffer::Flush(void) { __asm { int 3 } }

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
