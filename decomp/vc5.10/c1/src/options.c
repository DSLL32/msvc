#include <stdlib.h>
#include "compiler-state.h"
#include <recon.h>

/* Internal switches can continue across argv or environment tokens. */
void __cdecl scan_internal_switches(char **arguments)
{
    unsigned mode = 0;
    const char *flags;
    char token[514];

    for (++arguments; *arguments; ++arguments)
        mode = parse_internal_switch(*arguments, mode);
    flags = getenv("MSC_CMD_FLAGS");
    if (flags) {
        mode = 0;
        flags = next_internal_token(flags, token, 514);
        while (*token) {
            mode = parse_internal_switch(token, mode);
            flags = next_internal_token(flags, token, 514);
        }
    }
}

unsigned __cdecl parse_internal_switch(const char *argument, unsigned mode)
{
    if (!argument)
        return 0;
    for (;;) {
        switch (mode) {
        case 0:
            if (argument[0] == '-' && argument[1] == 'B' && argument[2] == 'm') {
                argument += 3;
                mode = 1;
            } else if (argument[0] == '-' && argument[1] == 'Z' && argument[2] == 'B') {
                argument += 3;
                mode = 2;
            } else if (argument[0] == '-' && argument[1] == 'Z' && argument[2] == 'm') {
                argument += 3;
                mode = 3;
            } else {
                return 0;
            }
            break;
        case 1:
            c1_data.internal_bm_flags = atoi(argument);
            return 0;
        case 2:
            c1_data.internal_zb_value = atoi(argument);
            return 0;
        case 3:
            c1_data.memory_percent = atoi(argument);
            if (!c1_data.memory_percent)
                c1_data.memory_percent = 100;
            return 0;
        }
        if (!*argument)
            return mode;
        if (!mode)
            return 0;
    }
}

const char *__cdecl next_internal_token(const char *input, char *token, unsigned size)
{
    __asm { int 3 }
    return NULL;
}
