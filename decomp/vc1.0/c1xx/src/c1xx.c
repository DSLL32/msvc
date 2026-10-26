/* Process driver, original 0x69630..0x696a8. */
#include <signal.h>
#include <stdlib.h>
#include "options.h"
#include <recon.h>

int argument_count;
char **argument_vector;
char *fatal_error_label;

int main(int argc, char **argv, char **environment)
{
    signal(SIGINT, interrupt_compile);
    signal(SIGTERM, interrupt_compile);
    signal(SIGBREAK, interrupt_compile);
    argument_count = argc;
    argument_vector = argv;
    fatal_error_label = message_text(315);
    parse_options(argc, argv);
    if (!options.configuration.unguarded_compilation)
        return guarded_compile();
    return compile_translation_unit();
}

/* Original 0x696b0..0x6972e, including the exception filter and handler. */
int guarded_compile(void)
{
    __try {
        compile_translation_unit();
    }
    __except (1) {
        fatal_error(1, "msc1.cpp", 602);
    }
}

void cleanup_fatal_outputs(void);

/* Original0x69c80..0x69c93, cdecl signal callback. The signal number
   is unused; cleanup precedes the original complete0x8000 exit status. */
void interrupt_compile(int signal_number)
{
    cleanup_fatal_outputs();
    exit(0x8000);
}
