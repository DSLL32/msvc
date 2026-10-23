#include "c2.h"
#include <recon.h>

/* The CRT precision helper uses the import-library call thunk. */
void c2_set_default_precision(void)
{
    _controlfp(_PC_53, _MCW_PC);
}
