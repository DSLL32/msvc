#include <stdio.h>
#include "frontend.h"
#include <recon.h>

extern "C" void __cdecl write_global_ir_record(unsigned, struct CompilerSymbol *);

extern "C" void __cdecl write_global_header(void)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[2]; /* original bf358 */
    buffer->Flush();
    fflush(buffer->file);
    rewind(buffer->file);
    frontend_bss.global_output_active = 0;
    write_global_ir_record(17, NULL);
}

extern "C" void __cdecl write_global_ir_record(unsigned kind, struct CompilerSymbol *symbol)
{ __asm { int 3 } }
