#include "c2.h"
#include <recon.h>

int c2_argc, c2_trace_enabled, c2_more_debug, c2_errors;
int c2_internal_error_active;
char **c2_argv;
char *c2_intermediate_base = "";
FILE *c2_input, *c2_globals_input;
void *c2_types;

int c2_main(int argc, char **argv, char **environment)
{
    int fault;
    __try {
        SetConsoleCtrlHandler(c2_console_handler, TRUE);
        c2_register_exit(c2_cleanup);
        c2_argc = argc;
        c2_argv = argv;
        c2_read_environment(getenv("MSC_CMD_FLAGS"));
        c2_initialize_options();
        _controlfp(0x8001f, 0x8001f);
        if (c2_trace_enabled)
            c2_print_banner();
        c2_input = c2_open_intermediate(c2_intermediate_base, "in", "r", "init il");
        c2_globals_input = c2_open_intermediate(c2_intermediate_base, "gl", "r", "glo il");
        c2_read_header();
        c2_reset_symbol_table();
        c2_types = c2_create_type_table();
        c2_initialize_constants();
        c2_initialize_opcodes();
        c2_initialize_machine();
        c2_initialize_trees();
        c2_initialize_codegen();
        c2_initialize_registers();
        c2_initialize_optimizer();
        c2_set_argv();
        c2_compile_unit();
        c2_finalize_output();
        if (c2_more_debug)
            c2_flush_debug();
    } __except(!c2_internal_error_active &&
               ((fault = GetExceptionCode()) == EXCEPTION_ACCESS_VIOLATION ||
                fault == EXCEPTION_STACK_OVERFLOW)) {
        if (fault == EXCEPTION_ACCESS_VIOLATION) {
            c2_report_internal("E:\\utc\\src\\\\P2\\main.c", 379);
            exit(525);
        } else {
            c2_fatal(63);
        }
    }
    exit(c2_errors);
    return 0;
}
