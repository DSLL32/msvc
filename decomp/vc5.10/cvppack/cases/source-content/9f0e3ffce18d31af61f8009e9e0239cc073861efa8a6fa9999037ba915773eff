/* Original input-name normalization and open [00404a30,00404b4c). */
#include "files.h"
#include <recon.h>

char input_name[260];

void select_input_name(char *name)
{
    char drive[4], directory[256], filename[256], extension[256];
    const char *source = name;
    char *destination = input_name;
    do { *destination++ = *source; } while (*source++);
    _splitpath(input_name, drive, directory, filename, extension);
    if (!extension[0]) {
        source = ".exe";
        destination = extension;
        do { *destination++ = *source; } while (*source++);
    }
    _makepath(input_name, drive, directory, filename, extension);
    input_file = open_cached_file(input_name, _O_RDWR | _O_BINARY, 0);
    if (input_file == -1) {
        input_file = open_cached_file(input_name, _O_RDONLY | _O_BINARY, 0);
        if (input_file == -1) fatal_error(1003, input_name, NULL);
        else fatal_error(1004, input_name, NULL);
    }
}
