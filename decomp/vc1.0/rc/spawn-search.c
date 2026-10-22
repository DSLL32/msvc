/* Older CRT search fallback, reached when RC has no sibling RCPP.EXE.
 * Keep the public wrapper, search walker and component parser separate. */
#include "rc.h"
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#pragma intrinsic(strcpy, strcat, strlen)

char * __cdecl _getpath(char *, char *, unsigned int);

/* Original 0x0100ed20..0x0100ed3a. */
int __cdecl _spawnvp(int mode, const char *program, const char *const *arguments)
{
    return _spawnvpe(mode, program, arguments, 0);
}

/* Original 0x0100ed50..0x0100eedb. The initial direct attempt is part of
 * this function, even though RC has already tried its sibling path. */
int __cdecl _spawnvpe(int mode, const char *program, const char *const *arguments,
                      const char *const *environment)
{
    int result;
    char *path, *candidate;
    unsigned int length;

    result = _spawnve(mode, program, arguments, environment);
    if (result != -1 || errno != ENOENT || strchr(program, '/'))
        return result;
    path = getenv("PATH");
    if (!path)
        return result;
    candidate = malloc(MAX_PATH);
    if (!candidate)
        return result;
    while ((path = _getpath(path, candidate, MAX_PATH - 1)) && *candidate) {
        length = strlen(candidate);
        if (candidate[length - 1] != '\\' && candidate[length - 1] != '/')
            strcat(candidate, "\\");
        if (strlen(program) + strlen(candidate) >= MAX_PATH)
            break;
        strcat(candidate, program);
        result = _spawnve(mode, candidate, arguments, environment);
        if (result != -1)
            break;
        if (errno == ENOENT)
            continue;
        /* Native search also continues past other failures on UNC roots. */
        if ((candidate[0] == '\\' || candidate[0] == '/') &&
            (candidate[1] == '\\' || candidate[1] == '/'))
            continue;
        break;
    }
    free(candidate);
    return result;
}

/* Original 0x0100eef0..0x0100ef71. Empty separators are skipped; quoted
 * spans lose their quotes and may contain semicolons. Filling the limit ends
 * the search with NULL, while preserving the copied, NUL-terminated component.
 */
char * __cdecl _getpath(char *path, char *destination, unsigned int count)
{
    char *start, *output = destination;
    unsigned int remaining = count - 1;
    while (*path == ';')
        ++path;
    start = path;
    if (!remaining)
        goto finished;
    while (*path && *path != ';') {
        if (*path == '"') {
            ++path;
            while (*path && *path != '"') {
                *output++ = *path++;
                if (!--remaining) {
                    start = path;
                    goto finished;
                }
            }
            if (!*path)
                break;
            ++path;
        } else {
            *output++ = *path++;
            if (!--remaining) {
                start = path;
                goto finished;
            }
        }
    }
    while (*path == ';')
        ++path;
finished:
    *output = 0;
    return path == start ? 0 : path;
}
