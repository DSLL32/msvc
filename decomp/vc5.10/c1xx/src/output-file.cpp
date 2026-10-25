#include <stdio.h>
#include <string.h>
#include "output.h"
#pragma intrinsic(strcmp, strcpy, strcat)
#include <recon.h>

extern "C" int __cdecl report_response_error(int, ...);

int RECON_THISCALL OutputBuffer::Open(const char *prefix, const char *suffix,
                                    const char *mode)
{
    if (strcmp(prefix, "nul") == 0)
        suffix = NULL;
    strcpy(filename, prefix);
    if (suffix)
        strcat(filename, suffix);
    file = fopen(filename, mode);
    if (!file)
        report_response_error(83, 339, filename);
    return setvbuf(file, NULL, _IOFBF, 4096);
}
