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
        frontend_bss.context_stack.origin = NULL;
        frontend_bss.context_stack.position = 0;
        frontend_bss.context_stack.top = NULL;
        frontend_bss.saved_context_stack.origin = NULL;
        frontend_bss.saved_context_stack.position = 0;
        frontend_bss.saved_context_stack.top = NULL;
        for (i = 0; i < 9; ++i) {
            frontend_bss.nested_context_stacks[i].origin = NULL;
            frontend_bss.nested_context_stacks[i].position = 0;
            frontend_bss.nested_context_stacks[i].top = NULL;
        }
        frontend_bss.source_position.origin = NULL;
        frontend_bss.source_position.position = 0;
        for (i = 0; i < FRONTEND_OUTPUT_BUFFERS; ++i) {
            frontend_bss.output_buffers[i].data = NULL;
            frontend_bss.output_buffers[i].cursor = NULL;
            frontend_bss.output_buffers[i].capacity = 0;
            frontend_bss.output_buffers[i].relocation = NULL;
            frontend_bss.output_buffers[i].relocation_context = NULL;
            frontend_bss.output_buffers[i].file = NULL;
            frontend_bss.output_buffers[i].ok = 0;
            frontend_bss.output_buffers[i].descriptor = 0;
            frontend_bss.output_buffers[i].mode = 0;
        }
        frontend_bss.deferred_work = NULL;
        frontend_bss.token_value.integer = 0;
        frontend_bss.sentinel_token.kind = 0;
        frontend_bss.sentinel_token.value.integer = 0;
        frontend_bss.source_signature = 0;
        for (i = 0; i < sizeof(frontend_bss.pools) / sizeof(frontend_bss.pools[0]); ++i) {
            frontend_bss.pools[i].cursor = NULL;
            frontend_bss.pools[i].remaining = 0;
            frontend_bss.pools[i].minimum_pages = 0;
            frontend_bss.pools[i].arena = NULL;
        }
        frontend_bss.type_ranks_initialized = 0;
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

int report_compiler_diagnostic(int number, ...)
{
    __asm { int 3 }
    return 0;
}

int register_cleanup(Initializer cleanup)
{
    if (frontend_bss.cleanup_count >= 32)
        report_compiler_diagnostic(1, ".\\main.c", 1977);
    frontend_bss.cleanup_callbacks[frontend_bss.cleanup_count++] = cleanup;
    return 0;
}

void initialize_deferred_work(void)
{
    frontend_bss.deferred_work = NULL;
}

void destroy_deferred_work(void)
{
    __asm { int 3 }
}
