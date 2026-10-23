#include "cvtres.h"
#include <stdlib.h>
#include <recon.h>

void __stdcall usage(int status)
{
    printf(load_message(8003), "5.00.1668.1");
    puts(load_message(8004));
    puts("usage: CVTRES [options] ResFile\n\n"
         "   options:\n\n"
         "      /MACHINE:{IX86|ALPHA|ARM|MIPS|MIPSR41XX|PPC|SH3|SH4}\n"
         "      /NOLOGO\n"
         "      /OUT:filename\n"
         "      /READONLY\n"
         "      /VERBOSE\n"
         "      /WINDOWSCE[:{EMULATION|NOCONVERT}]\n");
    exit(status);
}
