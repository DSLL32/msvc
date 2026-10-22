/* Command-line setup in the older RC runtime. */
#include "rc.h"
#include <stdlib.h>

extern char *_acmdln;
extern char *_pgmptr;
extern int __argc;
extern char **__argv;
void __cdecl _amsg_exit(int);
void __cdecl parse_cmdline(char *, char **, char *, int *, int *);
static char program_name[MAX_PATH];

/* Original 0x01011d40..0x01011de2. */
void __cdecl _setargv(void)
{
    int arguments, characters;
    char *command;
    char **vector;

    GetModuleFileNameA(0, program_name, sizeof(program_name));
    _pgmptr = program_name;
    command = *_acmdln ? _acmdln : program_name;
    parse_cmdline(command, 0, 0, &arguments, &characters);
    vector = malloc(arguments * sizeof(char *) + characters);
    if (!vector)
        _amsg_exit(8);
    parse_cmdline(command, vector, (char *)(vector + arguments),
                  &arguments, &characters);
    __argc = arguments - 1;
    __argv = vector;
}

/* Original 0x01011df0..0x01011f5c. */
void __cdecl parse_cmdline(char *command, char **vector, char *text,
                          int *arguments, int *characters)
{
    unsigned char *next = (unsigned char *)command;
    unsigned char character;
    unsigned int slashes;
    int quoted = 0, copy;

    *characters = 0;
    *arguments = 1;
    if (vector)
        *vector++ = text;
    if (*next == '"') {
        ++next;
        while (*next && *next != '"') {
            ++*characters;
            if (text)
                *text++ = *next;
            ++next;
        }
        ++*characters;
        if (text)
            *text++ = 0;
        if (*next == '"')
            ++next;
    } else {
        do {
            ++*characters;
            if (text)
                *text++ = *next;
            character = *next++;
        } while (character > ' ');
        if (!character)
            --next;
        else if (text)
            text[-1] = 0;
    }
    while (*next) {
        while (*next == ' ' || *next == '\t')
            ++next;
        if (!*next)
            break;
        if (vector)
            *vector++ = text;
        ++*arguments;
        for (;;) {
            copy = 1;
            slashes = 0;
            while (*next == '\\') {
                ++next;
                ++slashes;
            }
            if (*next == '"') {
                if (!(slashes & 1)) {
                    if (quoted && next[1] == '"')
                        ++next;
                    else
                        copy = 0;
                    quoted = !quoted;
                }
                slashes >>= 1;
            }
            while (slashes--) {
                if (text)
                    *text++ = '\\';
                ++*characters;
            }
            character = *next;
            if (!character || (!quoted && (character == ' ' || character == '\t')))
                break;
            if (copy) {
                if (text)
                    *text++ = character;
                ++*characters;
            }
            ++next;
        }
        if (text)
            *text++ = 0;
        ++*characters;
    }
    if (vector)
        *vector = 0;
    ++*arguments;
}
