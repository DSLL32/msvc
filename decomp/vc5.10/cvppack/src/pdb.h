#ifndef CV510_PDB_H
#define CV510_PDB_H
#include <recon.h>
/* Actual named MSPDB50 exports. Only the opaque pointer and 32-bit cdecl
 * result used by the target are declared; no PDB-private layout is assumed. */
#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
int __cdecl TypesClose(void *types);
#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
int __cdecl PDBClose(void *pdb);
#endif
