#ifndef C1XX_PDB_CONTEXT_H
#define C1XX_PDB_CONTEXT_H
#include "c1xx.h"
#include <recon.h>

/* Only encountered members are projected. Original offset +0x10 is not modeled. */
typedef struct PDB PDB;
typedef struct NameMap NameMap;
typedef struct TPI TPI;
typedef struct MREngine MREngine;
typedef struct PdbStreamState PdbStreamState;
typedef struct CompilerPdbContext {
    PDB *database;
    NameMap *names;
    TPI *types;
    MREngine *minimal_rebuild;
    PdbStreamState *stream_state;
    int read_mode;
} CompilerPdbContext;
extern CompilerPdbContext input_pdb_context;
int initialize_pdb_context_01(void);
void destroy_pdb_context_01(void);
#endif
