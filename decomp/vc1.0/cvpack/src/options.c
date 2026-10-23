/* Command switches [000174c0,0001765b); the original table is text data. */
#include <ctype.h>
#include "cvpack.h"
#include <recon.h>

int verbose, strip_symbols, minimum_types;
char *debug_filename;

void parse_options(int argc, char **argv)
{
    int remaining = argc-1;
    char **argument = argv+1;
    int child;
    while (remaining) {
        if (**argument != '/' && **argument != '-') break;
        ++*argument;
        switch (toupper((signed char)**argument)) {
        case '?':
            fatal_error(39, NULL, NULL);
            break;
        case 'C':
            if (!_stricmp(*argument, "cvold")) cvnew = 0;
            break;
        case 'H':
            child = _spawnlp(_P_WAIT, "qh.exe", "qh", "-u", "cvpack.exe", NULL);
            if (child == 3 || child < 0) fatal_error(39, NULL, NULL);
            break;
        case 'M':
            if (!_stricmp(*argument, "mapio")) mapped_io = 1;
            else if (!_stricmp(*argument, "min")) minimum_types = 1;
            else fatal_error(39, NULL, NULL);
            break;
        case 'N':
            show_logo = 0;
            break;
        case 'P':
            pcode = 1;
            break;
        case 'S':
            strip_symbols = 1;
            if (remaining > 2) {
                ++argument;
                if (**argument != '/' && **argument != '-') {
                    --remaining;
                    debug_filename = *argument;
                } else --argument;
            }
            break;
        case 'V':
            verbose = 1;
            break;
        default:
            warning(7, *argument, NULL);
            break;
        }
        ++argument;
        --remaining;
    }
    if (remaining != 1) {
        if (show_logo == 1 && banner_pending == 1) print_banner();
        exit(0);
    }
}
