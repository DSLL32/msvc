#ifndef CL510_MRE_H
#define CL510_MRE_H
#include <windows.h>
#include <recon.h>

/* External MSPDB50 objects remain opaque. Only the exported call ABI is
 * established here; no successful native MRE operation is implemented. */
typedef struct Mre Mre;
typedef struct MreDriver MreDriver;
DECLSPEC_IMPORT BOOL WINAPI MREFOpenByName(Mre **, const char *, int *, char *, BOOL, BOOL);
DECLSPEC_IMPORT BOOL WINAPI MREQueryMreDrv(Mre *, MreDriver **);
/* CL ignores this wrapper's EAX; its semantic result type is not yet known. */
DECLSPEC_IMPORT void WINAPI MREDrvOneTimeInit(MreDriver *);
DECLSPEC_IMPORT BOOL WINAPI MREDrvFFilesOutOfDate(MreDriver *, void *);
DECLSPEC_IMPORT BOOL WINAPI MREDrvFRelease(MreDriver *);
DECLSPEC_IMPORT BOOL WINAPI MREFClose(Mre *, BOOL);
#endif
