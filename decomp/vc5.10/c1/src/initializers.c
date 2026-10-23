#include "compiler-state.h"
#include <recon.h>

void __cdecl initialize_state_12365(void)
{
    c1_bss.context.object = NULL;
    c1_bss.context.value = 0;
    c1_bss.context.saved_frame = NULL;
}

void __cdecl initialize_state_12377(void)
{
    c1_bss.current_context.object = NULL;
    c1_bss.current_context.value = 0;
    c1_bss.current_context.saved_frame = NULL;
}

void __cdecl initialize_state_12389(void)
{
    unsigned index;
    for (index = 0; index < 6; ++index) {
        c1_bss.contexts[index].object = NULL;
        c1_bss.contexts[index].value = 0;
        c1_bss.contexts[index].saved_frame = NULL;
    }
}

void __cdecl initialize_state_123a4(void)
{
}

void __cdecl initialize_state_123a5(void)
{
}

void __cdecl initialize_state_123a6(void)
{
}

void __cdecl initialize_state_123a7(void)
{
    initialize_output_buffer_123ae();
    register_compiler_cleanup(destroy_output_buffer_23940);
}

void __cdecl initialize_state_1241c(void)
{
    initialize_output_buffer_12423();
    register_compiler_cleanup(destroy_output_buffer_23939);
}

void __cdecl initialize_state_1246b(void)
{
    initialize_output_buffer_12472();
    register_compiler_cleanup(destroy_output_buffer_23932);
}

void __cdecl initialize_state_124ba(void)
{
    initialize_output_buffer_124c1();
    register_compiler_cleanup(destroy_output_buffer_2392b);
}

void __cdecl initialize_state_12509(void)
{
    initialize_output_buffer_12510();
    register_compiler_cleanup(destroy_output_buffer_23924);
}

void __cdecl initialize_state_12558(void)
{
    initialize_output_buffer_1255f();
    register_compiler_cleanup(destroy_output_buffer_2391d);
}

void __cdecl initialize_state_125a7(void)
{
    initialize_output_buffer_125ae();
    register_compiler_cleanup(destroy_output_buffer_23916);
}

void __cdecl initialize_state_125f6(void)
{
    initialize_output_buffer_125fd();
    register_compiler_cleanup(destroy_output_buffer_238dd);
}

void __cdecl initialize_state_12645(void)
{
}

void __cdecl initialize_state_12646(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_12663(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_12664(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_1266f(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_126af(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_126ef(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_12712(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_1271f(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_1272c(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_12739(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_12746(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_12753(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_12760(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_1276d(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_1277a(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_1277b(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_1279c(void)
{
    __asm { int 3 }
}

void __cdecl initialize_state_127a6(void)
{
    __asm { int 3 }
}

void __cdecl compiler_fatal(int message)
{
    __asm { int 3 }
}

int __cdecl compiler_error(int message, ...)
{
    __asm { int 3 }
    return 0;
}
