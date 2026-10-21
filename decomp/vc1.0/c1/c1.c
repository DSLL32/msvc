/* Frontend process setup, original main at 0x43850. */
#include <signal.h>
#include "options.h"

int driver_argc; /* original 0x5b218 */
char **driver_argv; /* original 0x5b214 */
char *fatal_error_label; /* original 0x5b220 */
int skip_fault_protection; /* original 0x6bbd8 */

int main(int argc, char **argv, char **envp)
{
    signal(2, compiler_signal);
    signal(15, compiler_signal);
    signal(21, compiler_signal);
    driver_argv = argv;
    driver_argc = argc;
    fatal_error_label = message_text(315);
    parse_options(argc, argv);
    if (!skip_fault_protection)
        return guarded_compile();
    return compile_translation_unit();
}

/* Original 0x438d0..0x4394e. Compiler execution exits through its own driver. */
int guarded_compile(void)
{
    __try {
        compile_translation_unit();
    }
    __except (1) {
        fatal_error(1, "msc1.cpp", 602);
    }
}
