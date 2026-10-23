#include "c2.h"
#include <recon.h>

FILE *__fastcall c2_open_intermediate(char *base, char *suffix, char *mode,
                                     char *description)
{
    char filename[260];
    FILE *file;

    strcpy(filename, base);
    strcat(filename, suffix);
    if (strlen(filename) > sizeof(filename))
        c2_report_internal("E:\\utc\\src\\\\Common\\ioin.c", 326);
    file = fopen(filename, mode);
    if (!file)
        c2_system_fatal(83, c2_get_diagnostic_string(311), filename);
    return file;
}
