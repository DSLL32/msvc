/* C23232.EXE main, original [0004c100,0004c2c0). */
#include "c2.h"
#include "options.h"
#include <recon.h>
extern int object_generation_started;
extern FILE *object_file;
static int cleanup_started; /* 000831b8 */
static int output_cleanup_disabled; /* 00083184: zero; no writer in this build */
int argument_count;
char **arguments;
char *intermediate_prefix = "";
int compiler_errors, debugger_enabled;
FILE *expression_file, *symbol_file, *initializer_file, *global_file;
void *output_state;
/* Original [0004c4e0,0004c560). Exit cleanup is idempotent and deletes
   incomplete or failed output; successful format/listing closure clears its
   respective pending flag before this registered callback runs. */
void compiler_cleanup(void)
{
    if (cleanup_started) return;
    cleanup_started = 1;
    if (output_cleanup_disabled) return;
    if (object_generation_started || compiler_errors) {
        if (object_file) fclose(object_file);
        if (object_filename) remove(object_filename);
    }
    if (assembly_cleanup_pending) {
        fclose(stdout);
        remove(assembly_filename);
    }
}
/* Original [0004c560,0004c57e). Console events terminate compilation after
   disabling Ctrl-C handling. The original BOOL return is unreachable after
   exit, but remains part of its stdcall signature and complete body. */
BOOL WINAPI compiler_control(DWORD event)
{
    (void)event;
    SetConsoleCtrlHandler(NULL, TRUE);
    exit(0x8000);
    return TRUE;
}
int main(int argc, char **argv, char **envp)
{
    __try {
        SetConsoleCtrlHandler(compiler_control, TRUE);
        atexit(compiler_cleanup);
        argument_count = argc;
        arguments = argv;
        append_environment_arguments(getenv("MSC_CMD_FLAGS"));
        parse_options();
        _control87(0x8001f, 0x8001f);
        if (debugger_enabled) initialize_debugging();
        initialize_limits();
        expression_file = open_intermediate(intermediate_prefix, "ex", "r", "exp il");
        symbol_file = open_intermediate(intermediate_prefix, "sy", "r", "sym il");
        initializer_file = open_intermediate(intermediate_prefix, "in", "r", "init il");
        global_file = open_intermediate(intermediate_prefix, "gl", "r", "glo il");
        initialize_frontend_data();
        initialize_backend();
        initialize_symbols();
        output_state = initialize_output();
        initialize_codegen();
        initialize_registers();
        initialize_segments();
        initialize_globals();
        compile_functions();
        finish_output();
        finish_debugging();
        finish_globals();
    } __except ((unsigned)(GetExceptionCode() + 0x3ffffffb) <= 1) {
        compiler_internal_error("@(#)main.c:1.82", 0x121);
        exit(0x20d);
    }
    exit(compiler_errors);
    return 0;
}
