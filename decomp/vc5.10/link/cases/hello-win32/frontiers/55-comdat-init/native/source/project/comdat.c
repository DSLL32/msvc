#include <windows.h>
#include "memory.h"
#include <recon.h>

/* release_comdat_tables passes this arena chain to the arena cleanup helper. */
MemoryArena *comdat_arena;
void __cdecl initialize_comdats(void)
{
    comdat_arena=NULL;
}
