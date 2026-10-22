/* Command-line driver, original 0x01007210..0x01007c62.
 * The adjacent switch table ends at 0x01007ccc. */
#include "driver.h"
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <errno.h>
#include <excpt.h>
#pragma intrinsic(strcpy, strcat, strlen, strcmp)

FILE *diagnostics, *resource_output, *preprocessor_input;
char resource_filename[MAX_PATH], input_filename[MAX_PATH];
char preprocessor_options[1032], include_paths[520];
char *temporary_input, *temporary_output;
unsigned int default_language = 0x409;
int error_count, kanji_mode, verbose;
static char *preprocessor_arguments[] = { "RCPP.EXE", preprocessor_options, 0 };
extern char *_pgmptr;

int __cdecl resource_main(int argc, char **argv, char **environment)
{
    char drive[4], directory[256], filename[256], extension[256];
    char preprocessor[MAX_PATH];
    char *definitions[101];
    char *include_end = include_paths, *entry, *next;
    char **option;
    int use_environment = 1, definition_count = 0, index, result;
    unsigned long padding = 0;
    long position;

    error_count = kanji_mode = verbose = 0;
    include_paths[0] = input_filename[0] = resource_filename[0] = 0;
    diagnostics = stdout;
    while (argc > 1) {
        option = argv + 1;
        if (!switch_character((*option)[0]))
            break;
        switch (rc_toupper((unsigned char)(*option)[1])) {
        case '?':
        case 'H':
            fprintf(diagnostics, "\n");
            fprintf(diagnostics, "Microsoft (R) Windows 32 Resource Compiler, Version %s\n", "3.10");
            fprintf(diagnostics, "Copyright (C) Microsoft Corp. 1985-1993.  All rights reserved.\n\n");
            fprintf(diagnostics, "Usage:\trc [switches] .RC input file \n");
            fprintf(diagnostics, "Switches:\n");
            fprintf(diagnostics, "   -r    Emit .RES file\n");
            fprintf(diagnostics, "   -v    Verbose (print progress messages)\n");
            fprintf(diagnostics, "   -d    Define a symbol\n");
            fprintf(diagnostics, "   -fo   Rename .RES file\n");
            fprintf(diagnostics, "   -l    Default language ID in hex\n");
            fprintf(diagnostics, "   -i    Add a path for INCLUDE searches\n");
            fprintf(diagnostics, "   -x    Ignore INCLUDE environment variable\n");
            fprintf(diagnostics, "\n");
            resource_finish(0);
            break;
        case 'X':
            use_environment = 0;
            goto continued_switches;
        case 'R':
            goto continued_switches;
        case 'V':
            verbose = 1;
continued_switches:
            if ((*option)[2]) {
                (*option)[1] = '-';
                ++*option;
                continue;
            }
            break;
        case 'D':
            if ((*option)[2])
                *option += 2;
            else {
                --argc;
                argv = option;
            }
            definitions[definition_count++] = argv[1];
            if (definition_count > 100)
                resource_finish("RC : fatal error RW1005: Invalid switch:  too many -D switches", argv[1]);
            break;
        case 'F':
            if (rc_toupper((unsigned char)(*option)[2]) != 'O')
                resource_finish("RC : fatal error RW1003: Invalid switch, %s", *option);
            else {
                if ((*option)[3])
                    *option += 3;
                else {
                    --argc;
                    argv = option;
                }
                if (argc > 1)
                    strcpy(resource_filename, argv[1]);
                else
                    resource_finish("RC : fatal error RW1001: No resource binary filename specified.");
            }
            break;
        case 'I':
            if ((*option)[2])
                *option += 2;
            else {
                --argc;
                argv = option;
            }
            if (!argv[1])
                resource_finish("RC : fatal error RW1003: Invalid switch - missing include path after -I");
            if (include_end != include_paths)
                include_end[-1] = ';';
            do {
                *include_end++ = *argv[1]++;
            } while (include_end[-1]);
            break;
        case 'L':
            if ((*option)[2])
                *option += 2;
            else {
                --argc;
                argv = option;
            }
            if (sscanf(argv[1], "%x", &default_language) != 1)
                resource_finish("RC : fatal error RW1004: Invalid hexidecimal default language Id specified.");
            while (*argv[1]++)
                ;
            break;
        default:
            resource_finish("RC : fatal error RW1006: Invalid switch: %s.", *option);
            break;
        }
        --argc;
        ++argv;
    }
    if (argc != 2)
        resource_finish("RC : fatal error RW1007: Invalid usage.  Use rc -? for Help");
    if (verbose) {
        fprintf(diagnostics, "Microsoft (R) Windows 32 Resource Compiler, Version %s\n", "3.10");
        fprintf(diagnostics, "Copyright (C) Microsoft Corp. 1985-1993.  All rights reserved.\n\n");
    }
    _strupr(argv[1]);
    _splitpath(argv[1], drive, directory, filename, extension);
    if (!*extension)
        strcpy(extension, ".RC");
    else if (!strcmp(extension, ".RES"))
        resource_finish("RC : fatal error RW1018: Input file has .RES suffix");
    _makepath(input_filename, drive, directory, filename, extension);
    if (!*resource_filename)
        _makepath(resource_filename, drive, directory, filename, ".RES");
    temporary_input = resource_alloc(MAX_PATH);
    _makepath(temporary_input, drive, directory, "RCXXXXXX", "");
    _mktemp(temporary_input);
    temporary_output = resource_alloc(MAX_PATH);
    _makepath(temporary_output, drive, directory, "RDXXXXXX", "");
    _mktemp(temporary_output);
    __try {
        SetConsoleCtrlHandler(interrupt_resource, TRUE);
        resource_output = fopen(resource_filename, "w+b");
        if (!resource_output)
            resource_finish("RC : fatal error RW1009: Error Creating %s", resource_filename);
        else {
            if (verbose)
                fprintf(diagnostics, "Creating %s", resource_filename);
            strcat(preprocessor_options, " -f ");
            strcat(preprocessor_options, temporary_input);
            strcat(preprocessor_options, " -g ");
            strcat(preprocessor_options, temporary_output);
            strcat(preprocessor_options, " -D RC_INVOKED -pc \\:/ -E ");
            if (use_environment) {
                if (include_end != include_paths)
                    include_end[-1] = ';';
                entry = getenv("INCLUDE");
                if (entry)
                    strcpy(include_end, entry);
                else
                    *include_end = 0;
            }
            next = include_paths;
            while (*next) {
                entry = next;
                while (*next && *next != ';')
                    ++next;
                if (*next) {
                    *next = 0;
                    --next;
                }
                strcat(preprocessor_options, "  -I");
                strcat(preprocessor_options, entry);
                if (*next) {
                    ++next;
                    *next++ = ';';
                }
            }
            for (index = 0; index < definition_count; ++index) {
                strcat(preprocessor_options, "  -D");
                strcat(preprocessor_options, definitions[index]);
            }
            if (strlen(preprocessor_options) > 1024)
                resource_finish("RC : fatal error RW1002: Internal error %s", "too many arguments to rcpp");
            if (verbose)
                fprintf(diagnostics, "\nRC: RCPP %s\n", preprocessor_options);
            if (!prepare_input(input_filename))
                resource_finish("RC : fatal error RW1010: Could not open %s", input_filename);
            _splitpath(_pgmptr, drive, directory, 0, 0);
            _makepath(preprocessor, drive, directory, "rcpp", ".exe");
            result = _spawnv(_P_WAIT, preprocessor, preprocessor_arguments);
            if (result == -1 && errno == ENOENT)
                result = _spawnvp(_P_WAIT, "rcpp.exe", preprocessor_arguments);
            if (result) {
                if (result == -1) {
                    switch (errno) {
                    case E2BIG:
                        resource_finish("RC : fatal error RW1011: RCPP.EXE command line greater than 128 bytes");
                    case ENOENT:
                        resource_finish("RC : fatal error RW1012: Could not find RCPP.EXE");
                    case ENOEXEC:
                        resource_finish("RC : fatal error RW1013: RCPP.EXE is not a valid executable");
                    case ENOMEM:
                        resource_finish("RC : fatal error RW1014: Insufficient memory to spawn RCPP.EXE");
                    default:
                        resource_finish("RC : fatal error RW1015: Unknown error spawning RCPP.EXE");
                    }
                } else
                    resource_finish("RC : fatal error RW1016: RC terminating after preprocessor errors");
            }
        }
        if (verbose) {
            fprintf(diagnostics, "\n%s", input_filename);
            fflush(diagnostics);
        }
        preprocessor_input = fopen(temporary_output, "rb");
        if (!preprocessor_input)
            resource_finish("RC : fatal error RW1010: Could not open temp file %s", temporary_output);
        initialize_lexer(preprocessor_input);
        compile_resources();
        position = seek_file(resource_output, 0, SEEK_CUR);
        if (position & 3)
            write_file(resource_output, &padding, 4 - (position & 3));
        SetConsoleCtrlHandler(interrupt_resource, FALSE);
    }
    __except ((GetExceptionCode() - 0xc000013aUL) < 1) {
        interrupt_resource(1);
    }
    resource_finish(0);
    return 0;
}
