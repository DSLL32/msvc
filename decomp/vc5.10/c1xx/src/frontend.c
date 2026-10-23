#include "frontend.h"
#include <recon.h>

FrontendData frontend_data = { 1 };
FrontendBss frontend_bss;
int restore_mode;

int initialize_compiler_pass(void)
{
    FrontendData *saved;
    Initializer *initializer;
    int error = 0;
    unsigned i;
    if (frontend_data.first_invocation) {
        frontend_bss.saved_state = (FrontendData *)checked_malloc(FRONTEND_STATE_BYTES);
        *frontend_bss.saved_state = frontend_data;
        restore_mode = 0;
    } else {
        saved = frontend_bss.saved_state;
        frontend_data = *saved;
        frontend_bss.saved_state = NULL;
        frontend_bss.cleanup_count = 0;
        for (i = 0; i < 32; ++i)
            frontend_bss.cleanup_callbacks[i] = NULL;
        frontend_bss.context_stack.begin = NULL;
        frontend_bss.context_stack.end = NULL;
        frontend_bss.context_stack.top = NULL;
        frontend_bss.saved_context_stack.begin = NULL;
        frontend_bss.saved_context_stack.end = NULL;
        frontend_bss.saved_context_stack.top = NULL;
        frontend_bss.saved_state = saved;
        restore_mode = compiler_mode == 0;
    }
    frontend_data.first_invocation = 0;
    for (initializer = frontend_initializers;
            initializer < frontend_initializers + FRONTEND_INITIALIZERS; ++initializer) {
        if (*initializer) {
            __try {
                (*initializer)();
            } __except (1) {
                error = 1;
            }
        }
    }
    return error;
}

void *checked_malloc(unsigned bytes)
{
    void *block = malloc(bytes);
    if (block == NULL)
        report_compiler_error(60);
    return block;
}

int report_compiler_error(int number)
{
    __asm { int 3 }
    return 0;
}

int main(int argc, char **argv)
{
    __asm { int 3 }
    return 0;
}

int cleanup_compiler_pass(void)
{
    __asm { int 3 }
    return 0;
}
