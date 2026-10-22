/* Resource filename lookup uses the driver's assembled INCLUDE path list. */
#include "resource-data.h"
#include <stdlib.h>
#include <string.h>
#include <recon.h>
#pragma intrinsic(strcmp, strcpy)
void __stdcall resolve_resource_path(char *, char *, char *);

/* Original 0x01009730..0x010097aa. */
unsigned int __cdecl read_resource_file(void)
{
    char filename[MAX_PATH];
    FILE *file;
    long size;
    resolve_resource_path(current_token.spelling, "INCLUDE", filename);
    if (*filename) {
        file = fopen(filename, "rb");
        if (file) {
            size = seek_file(file, 0, SEEK_END);
            seek_file(file, 0, SEEK_SET);
            resource_stream(file);
            return size;
        }
    }
    parser_error("File not found: %s", current_token.spelling);
    return 0;
}

/* Original 0x0100f420..0x0100f52d. Search the current directory first, then
 * semicolon-separated directories. Empty path components end the search. */
void __stdcall resolve_resource_path(char *filename, char *environment, char *destination)
{
    char *paths = !strcmp(environment, "INCLUDE") ? include_paths : getenv(environment);
    char *end, *input;
    unsigned int length;
    FILE *file;
    if (*filename == '\\' || *filename == '/' || filename[1] == ':') {
        strcpy(destination, filename);
        return;
    }
    strcpy(destination, ".\\");
    while (*destination) {
        end = destination;
        while (*end)
            ++end;
        if (end[-1] != '\\' && end[-1] != '/')
            *end++ = '\\';
        input = filename;
        while ((*end++ = *input++) != 0)
            ;
        file = fopen(destination, "r");
        if (file) {
            fclose(file);
            return;
        }
        for (length = 0; ; ++length) {
            destination[length] = paths[length];
            if (!destination[length] || paths[length] == ';')
                break;
        }
        paths += length;
        destination[length] = 0;
        if (*paths)
            ++paths;
    }
}
