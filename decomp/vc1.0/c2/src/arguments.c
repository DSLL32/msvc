/* Compiler argument handling; options supplied through MSC_CMD_FLAGS. */
#include "c2.h"
char **copy_arguments(void);
/* Original [000710f0,00071180). */
void __fastcall append_environment_arguments(char *flags)
{
    char **destination;
    char *output;
    int quoted = 0;
    int count;
    if (!flags) return;
    destination = copy_arguments();
    count = argument_count;
    for (;;) {
        argument_count = count;
        if (!*flags) break;
        while (*flags == ' ') ++flags;
        if (!*flags) continue;
        output = flags;
        *destination++ = flags;
        ++count;
        while (*flags) {
            if (*flags == ' ' && !quoted) break;
            if (*flags == '"') {
                if (flags[-1] == '\\') output[-1] = *flags;
                else quoted = !quoted;
            } else *output++ = *flags;
            ++flags;
        }
        if (*flags) ++flags;
        *output = 0;
    }
    *destination = NULL;
}

/* Original [000710b0,000710f0). */
/* Original argv storage [0007eb38,0007ec00), followed by CRT bss. */
char *argument_vector[50];
char **copy_arguments(void)
{
    char **source = arguments;
    char **destination = argument_vector;
    *destination = *source++;
    while (*destination) *++destination = *source++;
    arguments = argument_vector;
    return destination;
}

/* Original [0004c610,0004c640). Extra caller arguments are ignored. */
char *next_argument()
{
    --argument_count;
    if (argument_count > 0) return *++arguments;
    return NULL;
}
