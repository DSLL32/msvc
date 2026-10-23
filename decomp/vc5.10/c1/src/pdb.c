#include "compiler-state.h"
#include <recon.h>

CompilerPdbContext c1_pdb_context;

int __cdecl initialize_pdb_context_12676(void)
{
    if (!c1_reuse_state) {
        c1_pdb_context.database = NULL;
        c1_pdb_context.names = NULL;
        c1_pdb_context.types = NULL;
        c1_pdb_context.rebuild_state = NULL;
        c1_pdb_context.read_mode = 0;
        c1_pdb_context.minimal_rebuild = NULL;
    }
    return 0;
}

void __cdecl destroy_pdb_context_238dc(void)
{
    __asm { int 3 }
}

CompilerPdbContext c1_output_pdb_context;

int __cdecl initialize_pdb_context_126b6(void)
{
    if (!c1_reuse_state) {
        c1_output_pdb_context.database = NULL;
        c1_output_pdb_context.names = NULL;
        c1_output_pdb_context.types = NULL;
        c1_output_pdb_context.rebuild_state = NULL;
        c1_output_pdb_context.read_mode = 0;
        c1_output_pdb_context.minimal_rebuild = NULL;
    }
    return 0;
}

void __cdecl destroy_pdb_context_238db(void)
{
    __asm { int 3 }
}
