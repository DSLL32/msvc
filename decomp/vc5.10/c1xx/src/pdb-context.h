#ifndef C1XX_PDB_CONTEXT_H
#define C1XX_PDB_CONTEXT_H
#include <stddef.h>
#include "c1xx.h"
#include <recon.h>

/* Offset +0x10 remains uninterpreted storage; later fields retain their
 * original addresses rather than sliding across that gap. */
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
    BYTE unmodeled_10[4];
    PdbStreamState *stream_state;
    int read_mode;
#ifdef __cplusplus
    int RECON_THISCALL Open(const char *, unsigned *, unsigned *, unsigned, const char *);
    void RECON_THISCALL Finish(void);
#endif
} CompilerPdbContext;
typedef char CompilerPdbContext_layout[(sizeof(CompilerPdbContext) == 28 &&
    offsetof(CompilerPdbContext, stream_state) == 0x14 &&
    offsetof(CompilerPdbContext, read_mode) == 0x18) ? 1 : -1];
#ifdef __cplusplus
struct NameMap {
    static __declspec(dllimport) int __cdecl open(PDB *, int, NameMap **)
        RECON_MSPDB_NAMEMAP_OPEN;
    virtual int RECON_THISCALL Close(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot04(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot08(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot0c(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot10(void) = 0;
    virtual void RECON_THISCALL UnrecoveredSlot14(void) = 0;
    virtual int RECON_THISCALL Commit(void) = 0;
};
struct PdbStreamState {
    int RECON_THISCALL Close(void); /* a1683, actual internal owner */
};
#endif
#ifdef __cplusplus
extern "C" {
#endif
extern CompilerPdbContext input_pdb_context;
extern CompilerPdbContext output_pdb_context;
extern CompilerPdbContext *current_pdb_context;
int initialize_pdb_context_01(void);
void destroy_pdb_context_01(void);
int initialize_pdb_context_02(void);
void destroy_pdb_context_02(void);
#ifdef __cplusplus
}
#endif
#endif
