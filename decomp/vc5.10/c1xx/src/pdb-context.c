#include "pdb-context.h"
#include "frontend.h"
#include <recon.h>

CompilerPdbContext *current_pdb_context;

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

void destroy_pdb_context_01(void) {}

CompilerPdbContext output_pdb_context;

int initialize_pdb_context_02(void)
{
    if (!restore_mode) {
        output_pdb_context.database = NULL;
        output_pdb_context.names = NULL;
        output_pdb_context.types = NULL;
        output_pdb_context.stream_state = NULL;
        output_pdb_context.read_mode = 0;
        output_pdb_context.minimal_rebuild = NULL;
    }
    return 0;
}

void destroy_pdb_context_02(void) {}
