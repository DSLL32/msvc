/* CL's driver entry, 00401743..00401aed. */
#include "cl.h"
#include <recon.h>

Driver driver_storage;
Driver *driver = &driver_storage;
Option *unrecognized_options;
int no_logo, echo_response, help_requested;
int show_commands = 1;
int debug_exceptions, error_count, output_produced, default_c_language;
char *source_language;

/* These records select compiler passes, not executable image data. */
CompilerPass compiler_passes[5] = {
    { NULL, NULL, NULL, "D-", 1 },
    { "c1", NULL, "MSC_CMD_FLAGS=", "1-", 1 },
    { "c1xx", NULL, "MSC_CMD_FLAGS=", "P-", 1 },
    { "c2", NULL, "MSC_CMD_FLAGS=", "2-", 1 },
    { "link.exe", NULL, NULL, "C/", 1 }
};
static int c_passes[] = { 1, 3, 5 };
static int cpp_passes[] = { 2, 3, 5 };
static int link_passes[] = { 4, 5 };
InputKind input_kinds[9] = {
    { c_passes, "cpp", 0 }, { cpp_passes, "cxx", 0 },
    { cpp_passes, "obj", 1 }, { link_passes, "lib", 1 },
    { link_passes, "def", 1 }, { link_passes, "res", 1 },
    { link_passes, "exp", 1 }, { link_passes, "", 1 },
    { link_passes, "exe", 2 }
};

int main(int argc, char **argv, char **envp)
{
    char **before, **after;
    int before_count, after_count, language, *pass;
    Option *option;
    Input *input;
    char *text;
    static char *machine_defaults[] = { "-D_MSC_VER=1100", "-D_WIN32", NULL };
    static char *driver_defaults[] = {
        "-il%t", "-f%f", "-W1", "-Zep8", "-ZB", "-GBs", "-GR-", "-GX-",
        "-Otb0", "-dos", "-Fo%b.obj", "-Fe%b.%X", "-pc\\:/", "-ML",
        "-Fdvc50.pdb", "-Zn", "-EHc", "-ZM", NULL
    };

    __try {
        install_control_handler();
        text = getenv("CL");
        { char *tail = getenv("_CL_");
          before = split_arguments(text, 1);
          before_count = count_arguments(before);
          after = split_arguments(tail, 1);
        }
        after_count = count_arguments(after);
        argv[0] = program_name(argv[0]);
        initialize_driver(argv[0], driver);
        if (argc == 1 && before_count == 0)
            show_usage();
        no_logo = 0;
        if (!has_nologo(after, after_count) && !has_nologo(before, before_count))
            has_nologo(argv + 1, argc - 1);
        if (!no_logo)
            show_banner(0);
        parse_arguments(driver_defaults, count_arguments(driver_defaults));
        parse_arguments(machine_defaults, count_arguments(machine_defaults));
        for (option = driver->options; option; option = option->next)
            option->flags |= 1;
        for (option = driver->other_options; option; option = option->next)
            option->flags |= 1;
        show_commands = 0;
        if (before_count > 0)
            parse_arguments(before, before_count);
        if (argc > 1)
            parse_arguments(argv + 1, argc - 1);
        if (after_count > 0)
            parse_arguments(after, after_count);
        if (echo_response)
            write_message(2, "\n");
        finish_options();
        prepare_inputs(driver);
        prepare_environment(driver);
        if (help_requested)
            show_help();
        for (option = unrecognized_options; option; option = option->next) {
            if (option->value) {
                text = allocate(strlen(option->name) + strlen(option->value) + 2);
                append_text(append_text(text, option->name), option->value);
                warning(4002, text);
                release(text);
            } else
                warning(4002, option->name);
        }
        if (!driver->inputs)
            fatal(2003);
        if (!source_language)
            language = 8;
        else if (*source_language == 'C')
            language = 0;
        else if (*source_language == 'P')
            language = 1;
        else
            language = 3;
        for (input = driver->inputs; input; input = input->next) {
            if (language != 8 && !input->explicit_kind) {
                input->kind = language;
                input->explicit_kind = 1;
            } else if (input->kind == 8) {
                if (default_c_language)
                    input->kind = 0;
                else {
                    input->kind = 3;
                    warning(4024, input->name);
                }
            }
            for (pass = input_kinds[input->kind].passes;
                 *pass != 5 && !(compiler_passes[*pass].flags & 1); ++pass)
                ;
            if (*pass == 5)
                warning(4027, input->name);
        }
        prepare_execution();
        run_driver(driver);
        driver->current_output = NULL;
        if (!output_produced)
            warning(4021);
    } __except (!debug_exceptions && GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
        fatal(2030, argv[0]);
    }
    driver_exit(error_count ? 2 : 0);
    return 0;
}
