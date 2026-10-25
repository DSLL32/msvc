/* Unobserved external methods stay unavailable in native fixtures. */
#define RECON_SHIM_IMPL
#include <recon.h>
int __cdecl TypesClose(void *types)
{
    (void)types;
    recon_unsupported("CVPACK unobserved MSPDB TypesClose");
    return 0;
}
int __cdecl PDBClose(void *pdb)
{
    (void)pdb;
    recon_unsupported("CVPACK unobserved MSPDB PDBClose");
    return 0;
}
