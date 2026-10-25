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

#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
int __cdecl PDBExportValidateInterface(unsigned version);
#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
int __cdecl PDBOpen(const char *path, const char *mode, unsigned signature,
                    int *error, char *error_text, void **pdb);
#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
unsigned __cdecl PDBQuerySignature(void *pdb);
#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
unsigned __cdecl PDBQueryAge(void *pdb);
#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
int __cdecl PDBOpenTpi(void *pdb, const char *mode, void **types);

#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
unsigned __cdecl TypesQueryTiMinEx(void *types);
#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
unsigned __cdecl TypesQueryTiMacEx(void *types);
#ifndef RECON_NATIVE_TEST
__declspec(dllimport)
#endif
int __cdecl TypesQueryPbCVRecordForTiEx(void *types, unsigned index, unsigned char **record);
#endif
