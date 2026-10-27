#include <new.h>
#include <string.h>
#include "frontend.h"
#pragma intrinsic(memcpy, memset)
#include <recon.h>

extern "C" int __cdecl report_response_error(int, ...);

int RECON_THISCALL OutputBuffer::Grow(int bytes)
{
    int increment = (int)capacity / 2;
    unsigned new_capacity, used;
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
    if (old_data != replacement && relocation)
        relocation(relocation_context, old_data, replacement);
    return 1;
}

void RECON_THISCALL OutputBuffer::Flush(void)
{
    unsigned bytes = (unsigned)(cursor - data);
    size_t written;
    if (bytes) {
        if (frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1) == 0 &&
            !frontend_bss.options.preprocess_stdout && !frontend_bss.options.prototypes_only) {
            if (!ok)
                report_response_error(88, 339, filename);
            else {
                written = fwrite(data, 1, bytes, file);
                if ((unsigned)(cursor - data) != written)
                    report_response_error(88, 339, filename);
            }
        }
        cursor = data;
    }
}

extern "C" void __cdecl free_argument_allocation(void *);
void RECON_THISCALL OutputBuffer::Destroy(void)
{
    if (file) fclose(file);
    if (auxiliary_memory) free_argument_allocation(auxiliary_memory);
    if (data) {
        ::operator delete(data);
        capacity = 0;
    }
}

extern "C" void __cdecl destroy_output_buffer_01(void)
{
    frontend_bss.output_buffers[0].Destroy();
}

extern "C" void __cdecl destroy_output_buffer_02(void)
{
    frontend_bss.output_buffers[1].Destroy();
}

extern "C" void __cdecl destroy_output_buffer_03(void)
{
    frontend_bss.output_buffers[2].Destroy();
}

extern "C" void __cdecl destroy_output_buffer_04(void)
{
    frontend_bss.output_buffers[3].Destroy();
}

extern "C" void __cdecl destroy_output_buffer_05(void)
{
    frontend_bss.output_buffers[4].Destroy();
}

extern "C" void __cdecl destroy_output_buffer_06(void)
{
    frontend_bss.output_buffers[5].Destroy();
}

extern "C" void __cdecl destroy_output_buffer_07(void)
{
    frontend_bss.output_buffers[6].Destroy();
}

extern "C" void __cdecl destroy_output_buffer_08(void)
{
    frontend_bss.output_buffers[7].Destroy();
}
