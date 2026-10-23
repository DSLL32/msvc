/* Original option parser [004048c0,004049a6); its two switch tables are
 * separately owned .text data at [004049a8,004049d8). */
#include <ctype.h>
#include "cvpack.h"
#include <recon.h>

int minimum_pack, verbose;

void parse_options(int argc, char **argv)
{
    int remaining = argc - 1;
    char **argument = argv + 1;
    while (remaining && (**argument == '/' || **argument == '-')) {
        ++*argument;
        switch (toupper(**argument)) {
        case '?':
        case 'H':
            fatal_error(1029, NULL, NULL);
            break;
        case 'M':
            if (!_strcmpi(*argument, "min")) minimum_pack = 1;
            else fatal_error(1029, NULL, NULL);
            break;
        case 'N':
            show_logo = 0;
            break;
        case 'V':
            verbose = 1;
            break;
        default:
            warning(4007, *argument, NULL);
            break;
        }
        ++argument;
        --remaining;
    }
    if (remaining != 1) {
        if (show_logo && banner_pending) print_banner();
        exit(0);
    }
}
