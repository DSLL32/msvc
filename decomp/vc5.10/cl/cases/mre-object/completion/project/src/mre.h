#ifndef CL510_MRE_H
#define CL510_MRE_H
#include <windows.h>
#include <recon.h>

/* External MSPDB50 objects remain opaque. Only the exported call ABI is
 * established here; case-bound native responses live in the external host. */
typedef struct Mre Mre;
typedef struct MreDriver MreDriver;
DECLSPEC_IMPORT BOOL WINAPI MREFOpenByName(Mre **, const char *, int *, char *, BOOL, BOOL);
/* EAX is ignored by CL; DWORD records only the observed ABI width. */
DECLSPEC_IMPORT DWORD WINAPI MREQueryMreDrv(Mre *, MreDriver **);
/* CL ignores this wrapper's EAX; its semantic result type is not yet known. */
DECLSPEC_IMPORT void WINAPI MREDrvOneTimeInit(MreDriver *);
DECLSPEC_IMPORT BOOL WINAPI MREDrvFFilesOutOfDate(MreDriver *, void *);
DECLSPEC_IMPORT BOOL WINAPI MREDrvFRelease(MreDriver *);
DECLSPEC_IMPORT BOOL WINAPI MREFClose(Mre *, BOOL);
/* The caller ignores SuccessfulCompile's EAX; only its ABI width is known. */
DECLSPEC_IMPORT DWORD WINAPI MREDrvFSuccessfulCompile(MreDriver *, BOOL, const char *, const char *);
DECLSPEC_IMPORT BOOL WINAPI MREDrvFUpdateTargetFile(MreDriver *, const char *, BOOL);
#endif
