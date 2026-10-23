/* Driver initialization helpers. */
#include "cl.h"
#include <recon.h>

/* 004023bb..004023c4: use the CRT's complete program path. */
char *program_name(char *name)
{
    return _pgmptr;
}

/* 004022e9..00402319 */
void initialize_driver(char *name, Driver *state)
{
    memset(state, 0, sizeof(Driver));
    state->directory = program_directory(name);
    state->include_path = getenv("INCLUDE");
}

/* 00402de5..00402e6b: retain a separator on the compiler directory. */
char *program_directory(char *name)
{
    char drive_and_directory[_MAX_PATH];
    char directory[_MAX_DIR], filename[_MAX_FNAME], extension[_MAX_EXT];
    int length;
    _splitpath(name, drive_and_directory, directory, filename, extension);
    length = strlen(directory);
    if (length > 0 && directory[length - 1] != '\\' &&
        directory[length - 1] != ':' && directory[length - 1] != '/') {
        directory[length] = '\\';
        directory[length + 1] = '\0';
    }
    return duplicate(strcat(drive_and_directory, directory));
}

/* 004054c5..004054ec */
char *duplicate(char *text)
{
    char *copy;
    if (!text)
        return NULL;
    copy = _strdup(text);
    if (!copy)
        fatal(0);
    return copy;
}

/* 00405495..004054b2: all driver allocations share the fatal-error path. */
void *allocate(unsigned bytes)
{
    void *memory = malloc(bytes);
    if (!memory)
        fatal(0);
    return memory;
}

/* 004054b2..004054c5 */
void release(void *memory)
{
    if (memory)
        free(memory);
}
