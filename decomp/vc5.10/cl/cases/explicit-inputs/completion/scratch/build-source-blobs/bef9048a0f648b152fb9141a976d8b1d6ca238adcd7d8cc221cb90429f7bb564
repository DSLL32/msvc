/* Uninstrumented external dependency boundary; no MSPDB response is supplied. */
#include "../src/mre.h"
#include <recon.h>

unsigned mre_call_count;
static void unavailable(void)
{
    ++mre_call_count;
    recon_unsupported("unrecorded MSPDB50 minimal-rebuild operation");
}
BOOL WINAPI MREFOpenByName(Mre **owner,const char *path,int *error,char *message,BOOL reproducible,BOOL write)
{
    (void)owner;(void)path;(void)error;(void)message;(void)reproducible;(void)write;
    unavailable();return FALSE;
}
BOOL WINAPI MREQueryMreDrv(Mre *owner,MreDriver **driver)
{
    (void)owner;(void)driver;unavailable();return FALSE;
}
void WINAPI MREDrvOneTimeInit(MreDriver *driver)
{
    (void)driver;unavailable();
}
BOOL WINAPI MREDrvFFilesOutOfDate(MreDriver *driver,void *work)
{
    (void)driver;(void)work;unavailable();return FALSE;
}
BOOL WINAPI MREDrvFRelease(MreDriver *driver)
{
    (void)driver;unavailable();return FALSE;
}
BOOL WINAPI MREFClose(Mre *owner,BOOL commit)
{
    (void)owner;(void)commit;unavailable();return FALSE;
}
DWORD WINAPI MREDrvFSuccessfulCompile(MreDriver *driver,BOOL success,const char *source,const char *object)
{
    (void)driver;(void)success;(void)source;(void)object;unavailable();return 0;
}
BOOL WINAPI MREDrvFUpdateTargetFile(MreDriver *driver,const char *target,BOOL browser)
{
    (void)driver;(void)target;(void)browser;unavailable();return FALSE;
}
