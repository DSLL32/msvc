/* RC command-line option prefix, original 0x0100f400..0x0100f419. */
#include "driver.h"
#include <recon.h>

int __stdcall switch_character(char character)
{
    return character == '/' || character == '-';
}
