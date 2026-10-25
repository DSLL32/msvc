#include <stdio.h>
#include "frontend.h"
#include <recon.h>

extern "C" {
void *__cdecl make_integer_constant(int);
void __cdecl write_ir_record(int, const void *);
void __cdecl prepare_expression_input(const char *);
int __cdecl report_response_error(int, ...);
}

extern "C" void __cdecl open_expression_output(void)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[3]; /* c0360 */
    if (!frontend_data.options.pch_create) {
        buffer->Open(frontend_data.options.il_prefix, "ex", "w+b");
    } else if (!(frontend_data.options.pch_create_file || frontend_data.options.pch_file) ||
               frontend_data.options.pch_use) {
        buffer->Open(frontend_data.options.il_prefix, "ex", "w+b");
    } else {
        prepare_expression_input(frontend_data.options.pch_create_file);
        buffer->Open(frontend_data.options.pch_file, "", "wb");
        frontend_data.pch_expression_file = 1;
    }
    if (frontend_data.options.pch_use)
        return;
    write_ir_record(0x5b, make_integer_constant(0x484));
    buffer->Flush();
    fflush(buffer->file);
    frontend_bss.expression_seek_result = fseek(buffer->file, 0x483, SEEK_SET);
    if (frontend_bss.expression_seek_result)
        report_response_error(86, 339, NULL);
    buffer->ok &= buffer->cursor + 2 > buffer->data + buffer->capacity ?
        buffer->Grow(2) : 1;
    if (buffer->ok) {
        *buffer->cursor = 0;
        ++buffer->cursor;
    }
    buffer->Flush();
}

extern "C" void __cdecl prepare_expression_input(const char *name) { __asm { int 3 } }
