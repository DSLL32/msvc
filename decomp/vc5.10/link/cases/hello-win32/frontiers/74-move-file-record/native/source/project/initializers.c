#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "initializers.h"
#include "crt-imports.h"
#include <recon.h>

BufferBlock buffer12, buffer8, buffer4;
TimingState timing;

#if defined(RECON_NATIVE_TEST)
Initializer application_initializers[] = {
    NULL,
    initialize_buffer12, initialize_buffer8, initialize_buffer4, initialize_timing
};
#else
#pragma data_seg(".CRT$XCU")
Initializer application_initializer1 = initialize_buffer12;
Initializer application_initializer2 = initialize_buffer8;
Initializer application_initializer3 = initialize_buffer4;
Initializer application_initializer4 = initialize_timing;
#pragma data_seg()
#endif

/* Independent original application constructor frontiers. */
void __cdecl initialize_buffer12(void)
{
    allocate_buffer12();
    register_buffer12_cleanup();
}
void __cdecl allocate_buffer12(void)
{
    buffer12.base = crt_operator_new(0x3000);
    buffer12.remaining = buffer12.capacity = buffer12.base ? 1024 : 0;
}
void __cdecl register_buffer12_cleanup(void)
{
    atexit(free_buffer12);
}
void __cdecl free_buffer12(void)
{
    if (buffer12.base) crt_operator_delete(buffer12.base);
}
void __cdecl initialize_buffer8(void)
{
    allocate_buffer8();
    register_buffer8_cleanup();
}
void __cdecl allocate_buffer8(void)
{
    buffer8.base = crt_operator_new(4000);
    buffer8.remaining = buffer8.capacity = buffer8.base ? 500 : 0;
}
void __cdecl register_buffer8_cleanup(void)
{
    atexit(free_buffer8);
}
void __cdecl free_buffer8(void)
{
    if (buffer8.base) crt_operator_delete(buffer8.base);
}
void __cdecl allocate_buffer4(void)
{
    buffer4.base = crt_operator_new(4096);
    buffer4.remaining = buffer4.capacity = buffer4.base ? 1024 : 0;
}
void __cdecl register_buffer4_cleanup(void)
{
    atexit(free_buffer4);
}
void __cdecl free_buffer4(void)
{
    if (buffer4.base) crt_operator_delete(buffer4.base);
}
void __cdecl initialize_buffer4(void)
{
    allocate_buffer4();
    register_buffer4_cleanup();
}
void __cdecl initialize_timing(void)
{
    setup_timing();
    register_timing_cleanup();
}
void __cdecl setup_timing(void)
{
    timing.output = stdout;
    timing.display = 0;
    timing.options = 0;
    timing.start_tick = GetTickCount();
    timing.end_tick = timing.start_tick;
}
void __cdecl register_timing_cleanup(void)
{
    atexit(close_timing);
}
void __cdecl close_timing(void)
{
    DWORD elapsed;
    timing.end_tick=GetTickCount();
    elapsed=timing.end_tick-timing.start_tick;
    if (timing.display) {
        fprintf(timing.output,"Final: Total time = %d.%03ds\n",elapsed/1000,elapsed%1000);
        fflush(timing.output);
    }
    timing.display=0;
}
