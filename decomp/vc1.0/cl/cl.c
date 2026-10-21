/* CL.EXE 8.00, original main: [000186cb, 00018a85). */
#include "cl.h"
Driver driver_storage, *driver = &driver_storage;
int show_commands = 1, quiet = 1, errors, help_mode, preprocess;
int echo_response, output_produced;
Item *pending;
int main(int argc, char **argv, char **envp)
{
    char **ide_args, **before_args, **after_args;
    int ide_count, before_count, after_count, count;
    Item *item;
    char *joined;
    __try {
        setbuf(stdout, NULL);
        setbuf(stderr, NULL);
        install_control_handler();
        ide_args = split_arguments(getenv("_MSC_IDE_FLAGS"));
        ide_count = count_arguments(ide_args);
        before_args = split_arguments(getenv("CL"));
        before_count = count_arguments(before_args);
        after_args = split_arguments(getenv("_CL_"));
        after_count = count_arguments(after_args);
        if (argc == 1 && !before_count) usage();
        count = argc - 1;
        if (!has_nologo(argv + 1, count) &&
            !has_nologo(before_args, before_count) &&
            !has_nologo(after_args, after_count)) {
            quiet = 0;
            banner(0);
        }
        argv[0] = program_name(argv[0]);
        initialize_driver(argv[0], driver);
        parse_arguments(frontend_defaults, count_arguments(frontend_defaults));
        parse_arguments(preprocessor_defaults, count_arguments(preprocessor_defaults));
        for (item = driver->options; item; item = item->next) item->flags |= 1;
        for (item = driver->other_options; item; item = item->next) item->flags |= 1;
        show_commands = 0;
        if (ide_count > 0) parse_arguments(ide_args, ide_count);
        if (before_count > 0) parse_arguments(before_args, before_count);
        if (argc > 1) parse_arguments(argv + 1, count);
        if (after_count > 0) parse_arguments(after_args, after_count);
        if (echo_response) write_message(2, "\n");
        finish_options(driver);
        if (help_mode) load_messages(join_path(driver->directory, "cl32.msg"));
        for (item = pending; item; item = item->next) {
            if (item->extra) {
                joined = allocate(strlen(item->name) + strlen(item->extra) + 2);
                append_string(append_string(joined, item->name), item->extra);
                warning(4002, joined);
                release(joined);
            } else warning(4002, item->name);
        }
        prepare_inputs();
        if (!driver->inputs) fatal(2003);
        for (item = driver->inputs; item; item = item->next) {
            if (item->value.kind == 7) {
                if (preprocess) item->value.kind = 0;
                else { item->value.kind = 3; warning(4024, item->flags); }
            }
        }
        prepare_execution();
        run_driver(driver);
        if (!output_produced) warning(4021);
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
        fatal(2030, argv[0]);
    }
    driver_exit(errors ? 2 : 0);
    return 0;
}
