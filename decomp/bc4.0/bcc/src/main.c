#include <io.h>
#include "bcc.h"
#include <recon.h>

/* Called only on an exception; its application body remains unreconstructed. */
ULONG WINAPI handle_exception(EXCEPTION_RECORD *record, void *registration,
                              CONTEXT *context, void *dispatcher)
{
    (void)record;
    (void)registration;
    (void)context;
    (void)dispatcher;
    BCC_TRAP();
    return 0;
}

void initialize_exception_handler(void)
{
    _SetUserHandler(handle_exception);
}

struct date build_date;
struct time build_time;
int error_count;
unsigned char option_82, option_a6, option_a7;
char *output_path;

int main(int argc, char **argv, char **envp)
{
    (void)envp;
    signal(SIGINT, interrupt_handler);
    print_text(lookup_message(25));
    print_text("4.00");
    print_text(lookup_message(26));
    if (argc == 1)
        print_usage();

    getdate(&build_date);
    gettime(&build_time);
    initialize_memory();
    expand_response_files(&argv, &argc);
    initialize_options(argv, argc);
    finish_options();
    if (output_path != NULL) {
        const char *extension = !option_a7 && option_a6 ? ".asm" : ".obj";
        output_path = add_extension(extension, output_path);
    }
    reverse_lists();

    --argc;
    ++argv;
    while (argc > 0 && (**argv == '-' || **argv == '/' || **argv == '+')) {
        ++argv;
        --argc;
    }
    if (argc == 0)
        report_no_input(0xe1);
    {
        int input_count = argc;
        char **inputs = argv;
        unsigned char saved_option = option_82;
        while (argc > 0) {
            if (**argv != '-' && **argv != '/')
                inspect_input(*argv);
            else if ((*argv)[1] == 'v')
                inspect_v_option(0, *argv + 1);
            ++argv;
            --argc;
        }
        option_82 = saved_option;
        finish_input_scan();
        if (compile_inputs(inputs, input_count)) {
            int result;
            release_memory();
            result = run_linker(" @turboc.$ln", "tlink32.exe");
            unlink("turboc.$ln");
            if (result >= 1)
                error_count = 1;
        } else {
            release_memory();
        }
    }
    terminate_program(error_count != 0);
    return 0;
}
