#include <string.h>
#include <stdlib.h>
#include "compiler-state.h"
#pragma intrinsic(memset)
#include <recon.h>

/* Named next frontiers; no pass implementation accepted yet. */
int __cdecl initialize_compiler_pass(void)
{
    unsigned index;
    int result = 0;
    CompilerInitializedState *saved;

    if (c1_data.first_call) {
        c1_bss.saved_data = (CompilerInitializedState *)allocate_compiler_memory(6548);
        *c1_bss.saved_data = c1_data;
        c1_reuse_state = 0;
    } else {
        saved = c1_bss.saved_data;
        c1_data = *saved;
        memset(&c1_bss, 0, sizeof(c1_bss));
        c1_bss.saved_data = saved;
        c1_reuse_state = !c1_pass_mode;
    }
    c1_data.first_call = 0;
    for (index = 0; index < 34; ++index) {
        if (c1_initialization_callbacks[index]) {
            __try {
                (*c1_initialization_callbacks[index])();
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                result = 1;
            }
        }
    }
    return result;
}

/* Original checked allocation, hot0xe799..0xe7b6/cold0x2e51c..0x2e52b. */
void *__cdecl allocate_compiler_memory(unsigned size)
{
    void *memory = malloc(size);
    if (!memory)
        compiler_fatal(60);
    return memory;
}

int __cdecl main(int argc, char **argv)
{
    __asm { int 3 }
    return 1;
}

int __cdecl cleanup_compiler_pass(void)
{
    __asm { int 3 }
    return 1;
}
