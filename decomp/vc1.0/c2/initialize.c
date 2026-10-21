/* Backend process initialization. */
#include "c2.h"
unsigned memory_limit;
/* Original [00071a80,00071a90). */
void initialize_limits(void)
{
    memory_limit = 0x7fffffff;
}

void read_frontend_header(void);
/* Original [0004c4d0,0004c4e0): a distinct tail-call graph vertex. */
__declspec(naked) void initialize_frontend_data(void)
{
    __asm jmp read_frontend_header
}

#include "input.h"
#include "options.h"
unsigned first_serial, next_serial;
int frontend_mode, floating_frontend;
/* Original [0004de50,0004df20). */
void read_frontend_header(void)
{
    unsigned short machine;
    unsigned build;
    input_file = global_file;
    if (input_byte() != 0x11) compiler_report(0x384, "P1", 0, "P2", 0x1301d2e);
    machine = input_word();
    if (machine != 0x2386 && machine != 0x386)
        compiler_internal_error("@(#)p2symtab.c:1.89", 0x306);
    build = input_dword();
    if (build != 0x1301d2e) compiler_report(0x384, "P1", build, "P2", 0x1301d2e);
    first_serial = next_serial = input_dword();
    frontend_mode = (signed char)input_byte();
    floating_frontend = frontend_mode == 8;
    if (!error_filename) error_filename = floating_frontend ? "f23.err" : "c23.err";
}

#include "backend.h"
void *backend_arena_handle;
Node *null_node, *scratch_node, *sentinel_node;
/* Original [0004f6e0,0004f740). */
void initialize_backend(void)
{
    backend_arena_handle = configure_arena(5, 2, 64000);
    null_node = arena_calloc(5, sizeof(Node));
    null_node->opcode = 0x4c;
    scratch_node = arena_calloc(5, sizeof(Node));
    sentinel_node = arena_calloc(5, sizeof(Node));
}

unsigned symbol_count;
void *symbol_buckets[64];
/* Original [0004e310,0004e330). */
void initialize_symbols(void)
{
    unsigned i;
    symbol_count = 0;
    for (i = 0; i != 64; ++i) symbol_buckets[i] = NULL;
}

void *__fastcall allocate_descriptor(signed char kind, unsigned arena);
/* Original [0004e4f0,0004e500). */
__declspec(naked) void *initialize_output(void)
{
    __asm {
        xor edx, edx
        mov ecx, 1
        jmp allocate_descriptor
    }
}
