/* Backend process initialization. */
#include "c2.h"
unsigned memory_limit;
/* Original [00071a80,00071a90). */
void initialize_limits(void)
{
    memory_limit = 0x7fffffff;
}

void read_frontend_header(void);
/* Original [0004c4d0,0004c4e0): a distinct initialization adapter. */
void initialize_frontend_data(void)
{
    read_frontend_header();
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
    sentinel_node = arena_calloc(5, sizeof(Node));
    sentinel_node->opcode = 0x4c;
    scratch_node = arena_calloc(5, sizeof(Node));
    null_node = arena_calloc(5, sizeof(Node));
}

#include "symbols.h"
ScopeHash global_scope;
/* Original [0004e310,0004e330). */
void initialize_symbols(void)
{
    unsigned i;
    global_scope.previous = NULL;
    for (i = 0; i != 64; ++i) global_scope.buckets[i] = NULL;
}

void *__fastcall allocate_descriptor(signed char kind, unsigned arena);
/* Original [0004e4f0,0004e500). */
void *initialize_output(void)
{
    return allocate_descriptor(1, 0);
}

#include "extended.h"
#include <recon.h>
Extended extended_zero; /* 0007bf9c */
Extended extended_one; /* 0007bf90 */
/* Original [00057be0,00057c20). */
void initialize_codegen(void)
{
    int integer = 0;
    integer_to_extended(&integer, &extended_zero);
    integer = 1;
    integer_to_extended(&integer, &extended_one);
}

extern int cpu_model;
unsigned selected_save_register_mask; /* 0007c950 */
unsigned call_clobbered_register_mask; /* 0007caa4 */
int stack_alignment = 4; /* 000806d0: code/section alignment */
/* Original [00026c50,00026c80). */
void initialize_registers(void)
{
    selected_save_register_mask = 0xc8; /* EBX, ESI, EDI */
    call_clobbered_register_mask = 7; /* EAX, ECX, EDX */
    if (cpu_model >= 4) stack_alignment = 16;
}
