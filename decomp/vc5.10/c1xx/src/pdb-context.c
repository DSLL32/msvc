#include "pdb-context.h"
#include "frontend.h"
#include <recon.h>

CompilerPdbContext input_pdb_context;

int initialize_pdb_context_01(void)
{
    if (!restore_mode) {
        input_pdb_context.database = NULL;
        input_pdb_context.names = NULL;
        input_pdb_context.types = NULL;
        input_pdb_context.stream_state = NULL;
        input_pdb_context.read_mode = 0;
        input_pdb_context.minimal_rebuild = NULL;
    }
    return 0;
}

void destroy_pdb_context_01(void)
{
    __asm { int 3 }
}
