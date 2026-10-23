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
    __asm { int 3 }
    return 0;
}

const char *__cdecl next_internal_token(const char *input, char *token, unsigned size)
{
    __asm { int 3 }
    return NULL;
}
