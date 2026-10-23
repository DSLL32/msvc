#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "initializers.h"
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
void __cdecl free_buffer12(void) { __asm { int 3 } }
void __cdecl initialize_buffer8(void)
{
    allocate_buffer8();
    register_buffer8_cleanup();
}
void __cdecl allocate_buffer8(void) { __asm { int 3 } }
void __cdecl register_buffer8_cleanup(void) { __asm { int 3 } }
void __cdecl free_buffer8(void) { __asm { int 3 } }
void __cdecl allocate_buffer4(void) { __asm { int 3 } }
void __cdecl register_buffer4_cleanup(void) { __asm { int 3 } }
void __cdecl free_buffer4(void) { __asm { int 3 } }
void __cdecl initialize_buffer4(void) { __asm { int 3 } }
void __cdecl initialize_timing(void) { __asm { int 3 } }
