/* C23232.EXE main, original [0004c100,0004c2c0). */
#include "c2.h"
int argument_count;
char **arguments;
char *intermediate_prefix = "";
int compiler_errors, debugger_enabled;
FILE *expression_file, *symbol_file, *initializer_file, *global_file;
void *output_state;
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
