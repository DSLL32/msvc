#include "c2.h"
#include <recon.h>

/* The driver's environment flags are split in place and appended to argv. */
void __fastcall c2_read_environment(char *flags)
{
    char **argument;
    char *input, *output;
    int quoted = 0;
    int count;

    if (!flags)
        return;
    argument = c2_copy_arguments();
    input = flags;
    if (*input) {
        count = c2_argc;
        while (*input) {
            while (*input == ' ')
                ++input;
            if (!*input)
                break;
            *argument++ = input;
            output = input;
            ++count;
            do {
                if (*input == '"') {
                    if (input[-1] == '\\')
                        output[-1] = '"';
                    else
                        quoted = !quoted;
                } else {
                    *output++ = *input;
                }
                ++input;
            } while (*input && (*input != ' ' || quoted));
            if (*input)
                ++input;
            *output = '\0';
        }
        c2_argc = count;
    }
    *argument = NULL;
}
