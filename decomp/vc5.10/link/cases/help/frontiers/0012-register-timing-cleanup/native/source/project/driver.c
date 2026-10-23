#include <windows.h>
#include <recon.h>

int __cdecl main(int argc, char **argv, char **environment)
{
    __asm { int 3 }
    return 255;
}
