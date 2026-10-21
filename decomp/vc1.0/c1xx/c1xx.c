/* Process driver, original 0x69630..0x696a8. */
#include <signal.h>
#include "c1xx.h"

int argument_count;
char **argument_vector;
char *fatal_error_label;
int unguarded_compilation;

int main(int argc, char **argv, char **environment)
{
    signal(SIGINT, interrupt_compile);
    signal(SIGTERM, interrupt_compile);
    signal(SIGBREAK, interrupt_compile);
    argument_count = argc;
    argument_vector = argv;
    fatal_error_label = message_text(315);
    parse_options(argc, argv);
    if (!unguarded_compilation)
        return guarded_compile();
    return compile_translation_unit();
}
